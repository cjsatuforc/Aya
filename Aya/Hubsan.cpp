/** @file */

/*
 https://bitbucket.org/PhracturedBlue/deviation

 The adapter requires an a7105 transceiver and an Arduino Pro Mini or similar
 preferably at 3.3V.  It is possible to modify the original Turnigy 9X
 removable Tx module using its a7105 transciever.

 Pinouts are defined in config.h and the defaults are those adopted by
 Midelic.

 http://www.rcgroups.com/forums/showthread.php?t=1954078&highlight=hubsan+adapt

 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Deviation is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 if not, see <http://www.gnu.org/licenses/>.
 */

#include "Hubsan.h"
#include "A7105.h"

//#define USE_HUBSAN_EXTENDED // H107D LED/Flip etc control on AUX channels
#define DEFAULT_VTX_FREQ 5885 // x0.001GHz 5725:5995 steps of 5
#define HUBSAN_LEDS_BIT 2
#define HUBSAN_FLIP_BIT 3

#define WAIT_WRITE 0x80

#define READY 0x01
#define BINDING 0x02
#define BINDDLG 0x04

const uint8_t hubsanAllowedChannels[] = {0x14, 0x1e, 0x28, 0x32, 0x3c, 0x46,
                                         0x50, 0x5a, 0x64, 0x6e, 0x78, 0x82};

Hubsan::Hubsan()
    : IProtocol()
    , hubsanState(BIND_1)
    , rssiBackChannel(0)
    , enableFlip(true)
    , enableLEDs(true)
{
  a7105SetupSPI();
}

bool Hubsan::setup()
{
  bool retVal = false;

  for (uint8_t i = 0; i < 100; i++)
  {
    if (this->init())
    {
      retVal = true;
      break;
    }
  }

  if (retVal)
  {
    sessionID = random();
    chan = hubsanAllowedChannels[random() % sizeof(hubsanAllowedChannels)];
    setBindState(0xffffffff);
    hubsanState = BIND_1;
    hubsanPacketCount = 0;
    hubsanVTXFreq = DEFAULT_VTX_FREQ;
  }

  return retVal;
}

bool Hubsan::init()
{
  uint8_t vco_current;
  uint32_t timeoutuS;

  a7105Reset();

  a7105WriteID(0x55201041);
  a7105WriteReg(A7105_01_MODE_CONTROL, 0x63);
  a7105WriteReg(A7105_03_FIFOI, 0x0f);
  a7105WriteReg(A7105_0D_CLOCK, 0x05);
  a7105WriteReg(A7105_0E_DATA_RATE, 0x04);
  a7105WriteReg(A7105_15_TX_II, 0x2b);
  a7105WriteReg(A7105_18_RX, 0x62);
  a7105WriteReg(A7105_19_RX_GAIN_I, 0x80);
  a7105WriteReg(A7105_1C_RX_GAIN_IV, 0x0A);
  a7105WriteReg(A7105_1F_CODE_I, 0x07);
  a7105WriteReg(A7105_20_CODE_II, 0x17);
  a7105WriteReg(A7105_29_RX_DEM_TEST_I, 0x47);

  a7105Strobe(A7105_STANDBY);

  a7105WriteReg(A7105_02_CALC, 1);
  vco_current = a7105ReadReg(A7105_02_CALC);

  timeoutuS = a7105SetTimeout();
  while (a7105Busy())
    if (micros() > timeoutuS)
      return false;

  if (a7105ReadReg(A7105_22_IF_CALIB_I) & A7105_MASK_FBCF)
    return false;

  a7105ReadReg(A7105_24_VCO_CURCAL);
  // a7105WriteReg(0x24, 0x13); // VCO cal. from A7105 Datasheet
  // a7105WriteReg(0x26, 0x3b); // VCO bank cal. limits from A7105 Datasheet

  a7105WriteReg(A7105_0F_CHANNEL, 0); // set channel
  a7105WriteReg(A7105_02_CALC, 2);    // VCO cal.

  timeoutuS = a7105SetTimeout();
  while (a7105Busy())
    if (micros() > timeoutuS)
      return false;

  if (a7105ReadReg(A7105_25_VCO_SBCAL_I) & A7105_MASK_VBCF)
    return false;

  a7105WriteReg(A7105_0F_CHANNEL, 0xa0); // set channel
  a7105WriteReg(A7105_02_CALC, 2);       // VCO cal.

  timeoutuS = a7105SetTimeout();
  while (a7105Busy())
    if (micros() > timeoutuS)
      return false;

  if (a7105ReadReg(A7105_25_VCO_SBCAL_I) & A7105_MASK_VBCF)
    return false;

  // a7105WriteReg(0x25, 0x08); // reset VCO band cal.

  a7105SetPower(TXPOWER_150mW);
  a7105Strobe(A7105_STANDBY);

  return true;
}

bool Hubsan::bind()
{
  return true;
}

bool Hubsan::setCommand(ProtocolCommand command, uint16_t value)
{
  switch (command)
  {
  case COMMAND_THROTTLE:
  case COMMAND_YAW:
  case COMMAND_PITCH:
  case COMMAND_ROLL:
    sticks[command] = constrain((value >> 2) - 247, 0, 255);
    break;
  case COMMAND_FLIPS:
    enableFlip = value > 1800;
    break;
  case COMMAND_LIGHTS:
    enableLEDs = value > 1800;
    break;
  default:
    return false;
  }

  return true;
}

void Hubsan::setBindState(uint32_t ms)
{
  if (ms)
  {
    if (ms == 0xFFFFFFFF)
      bind_time = ms;
    else
      bind_time = millis() + ms;
    hubsanState |= BINDING;
  }
  else
    hubsanState &= ~BINDING;
}

void Hubsan::updateTelemetry()
{
  enum Tag0xe0
  {
    TAG,
    ROC_MSB, // ?
    ROC_LSB,
    unknown_e0_3,
    unknown_e0_4,
    unknown_e0_5,
    unknown_e0_6,
    unknown_e0_7,
    unknown_e0_8,
    Z_ACC_MSB,
    Z_ACC_LSB,
    YAW_GYRO_MSB,
    YAW_GYRO_LSB,
    VBAT,
    CRC0,
    CRC1
  };

  enum Tag0xe1
  {
    TAG_e1,
    PITCH_ACC_MSB,
    PITCH_ACC_LSB,
    ROLL_ACC_MSB,
    ROLL_ACC_LSB,
    unknown_e1_5,
    unknown_e1_6,
    PITCH_GYRO_MSB,
    PITCH_GYRO_LSB,
    ROLL_GYRO_MSB,
    ROLL_GYRO_LSB,
    unknown_e1_11,
    unknown_e1_12,
    VBAT_e1,
    CRC0_e1,
    CRC1_e1
  };

  uint8_t i;
  static uint32_t lastUpdatemS = millis();
  uint16_t intervalmS;

  if (a7105CRCCheck(16))
  {
    intervalmS = millis() - lastUpdatemS;
    lastUpdatemS = millis();

    // TODO
  }
}

void Hubsan::buildBindPacket(uint8_t hubsanState)
{
  a7105_packet[0] = hubsanState;
  a7105_packet[1] = chan;
  a7105_packet[2] = (sessionID >> 24) & 0xff;
  a7105_packet[3] = (sessionID >> 16) & 0xff;
  a7105_packet[4] = (sessionID >> 8) & 0xff;
  a7105_packet[5] = (sessionID >> 0) & 0xff;
  a7105_packet[6] = 0x08;
  a7105_packet[7] = 0xe4; //???
  a7105_packet[8] = 0xea;
  a7105_packet[9] = 0x9e;
  a7105_packet[10] = 0x50;
  a7105_packet[11] = (hubsanID >> 24) & 0xff;
  a7105_packet[12] = (hubsanID >> 16) & 0xff;
  a7105_packet[13] = (hubsanID >> 8) & 0xff;
  a7105_packet[14] = (hubsanID >> 0) & 0xff;

  a7105CRCUpdate(16);
}

void Hubsan::buildPacket()
{
  memset(a7105_packet, 0, 16);

#define VTX_STEP_SIZE 5

#if defined(USE_HUBSAN_EXTENDED)
  if (hubsanPacketCount == 100)
  { // set vTX frequency (H107D)
    a7105_packet[0] = 0x40;
    a7105_packet[1] = (hubsanVTXFreq >> 8) & 0xff;
    a7105_packet[2] = hubsanVTXFreq & 0xff;
    a7105_packet[3] = 0x82;
    hubsanPacketCount++;
  }
  else
#endif
  { // 20 00 00 00 80 00 7d 00 84 02 64 db 04 26 79 7b
    a7105_packet[0] = 0x20;
    a7105_packet[2] = sticks[COMMAND_THROTTLE];
  }
  a7105_packet[4] = 0xff - sticks[COMMAND_YAW];
  a7105_packet[6] = 0xff - sticks[COMMAND_PITCH];
  a7105_packet[8] = sticks[COMMAND_ROLL];

  a7105_packet[9] = 0x20;

  // sends default value for the 100 first packets
  if (hubsanPacketCount > 100)
  {
    if (enableLEDs)
      bitSet(a7105_packet[9], HUBSAN_LEDS_BIT);
    if (enableFlip)
      bitSet(a7105_packet[9], HUBSAN_FLIP_BIT);
  }
  else
  {
    hubsanPacketCount++;
  }

  a7105_packet[10] = 0x64;
  // could be optimised out
  a7105_packet[11] = (hubsanID >> 24) & 0xff;
  a7105_packet[12] = (hubsanID >> 16) & 0xff;
  a7105_packet[13] = (hubsanID >> 8) & 0xff;
  a7105_packet[14] = (hubsanID >> 0) & 0xff;

  a7105CRCUpdate(16);
}

uint16_t Hubsan::tx()
{
  enum
  {
    doTx,
    waitTx,
    pollRx
  };
  uint16_t d;
  static uint8_t telemetryState = doTx;
  static uint8_t polls, i;

  switch (hubsanState)
  {
  case BIND_1:
  case BIND_3:
  case BIND_5:
  case BIND_7:
    buildBindPacket(
        hubsanState == BIND_7
            ? 9
            : (hubsanState == BIND_5 ? 1 : hubsanState + 1 - BIND_1));
    a7105Strobe(A7105_STANDBY);
    a7105WriteData(a7105_packet, 16, chan);
    hubsanState |= WAIT_WRITE;
    d = 3000;
    break;
  case BIND_1 | WAIT_WRITE:
  case BIND_3 | WAIT_WRITE:
  case BIND_5 | WAIT_WRITE:
  case BIND_7 | WAIT_WRITE:
    if (a7105Busy()) // check for completion
      Serial.println("pwf");
    a7105Strobe(A7105_RX);
    hubsanState &= ~WAIT_WRITE;
    hubsanState++;
    d = 4500; // 7.5mS elapsed since last write
    break;
  case BIND_2:
  case BIND_4:
  case BIND_6:
    if (a7105Busy())
    {
      Serial.println("ns");
      hubsanState = BIND_1;
      d = 4500; // No signal, restart binding procedure.  12mS elapsed since
                // last write
    }
    else
    {
      a7105ReadData(a7105_packet, 16);
      hubsanState++;
      if (hubsanState == BIND_5)
        a7105WriteID((a7105_packet[2] << 24) | (a7105_packet[3] << 16) |
                     (a7105_packet[4] << 8) | a7105_packet[5]);
      d = 500; // 8mS elapsed time since last write
    }
    break;
  case BIND_8:
    if (a7105Busy())
    {
      Serial.println("nr");
      hubsanState = BIND_7;
      d = 15000; // 22.5mS elapsed since last write
    }
    else
    {
      Serial.println("r");
      a7105ReadData(a7105_packet, 16);
      if (a7105_packet[1] == 9)
      {
        hubsanState = DATA_1;
        a7105WriteReg(A7105_1F_CODE_I, 0x0F);
        setBindState(0);
        d = 28000; // 35.5mS elapsed since last write
      }
      else
      {
        hubsanState = BIND_7;
        d = 15000; // 22.5 mS elapsed since last write
      }
    }
    break;
  case DATA_1:
    a7105SetPower(TXPOWER_150mW); // keep transmit power in sync
  case DATA_2:
  case DATA_3:
  case DATA_4:
  case DATA_5:
    switch (telemetryState)
    { // Goebish - telemetry is every ~0.1S r 10 Tx packets
    case doTx:
      buildPacket();
      a7105Strobe(A7105_STANDBY);
      a7105WriteData(a7105_packet, 16,
                     hubsanState == DATA_5 ? chan + 0x23 : chan);
      d = 3000; // nominal tx time
      telemetryState = waitTx;
      break;
    case waitTx:
      if (a7105Busy())
        d = 0;
      else
      { // wait for tx completion
        a7105Strobe(A7105_RX);
        polls = 0;
        telemetryState = pollRx;
        d = 3000; // nominal rx time
      }
      break;
    case pollRx: // check for telemetry
      if (a7105Busy())
        d = 1000;
      else
      {
        a7105ReadData(a7105_packet, 16);
        rssiBackChannel = a7105ReadReg(A7105_1D_RSSI_THOLD);
        updateTelemetry();
        a7105Strobe(A7105_RX);
        d = 1000;
      }

      if (++polls >= 7)
      { // 3ms + 3mS + 4*1ms
        if (hubsanState == DATA_5)
          hubsanState = DATA_1;
        else
          hubsanState++;
        telemetryState = doTx;
      }
      break;
    } // switch
    break;
  } // switch

  return d;
}
