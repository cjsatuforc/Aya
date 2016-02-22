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

#define MIN_THROTTLE_US 1100

#define USE_HUBSAN_EXTENDED

#define WAIT_WRITE 0x80

#define READY 0x01
#define BINDING 0x02
#define BINDDLG 0x04

#define FLAG_VIDEO 0x01
#define FLAG_FLIP 0x08
#define FLAG_LED 0x04

const uint8_t hubsanAllowedChannels[] = {0x14, 0x1e, 0x28, 0x32, 0x3c, 0x46,
                                         0x50, 0x5a, 0x64, 0x6e, 0x78, 0x82};

/**
 * @brief Creates a new instance of the Hubsan protocol.
 * @param id ID of this transmitter
 * @param forceBind Ignore the last bind packet (fixes failing to bind)
 * @param vtxFreq Video transmission frequency in kHz (for FPV model
 */
Hubsan::Hubsan(uint16_t id, bool forceBind, uint16_t vtxFreq)
    : IProtocol()
    , m_id(id)
    , m_state(BIND_1)
    , m_rssiChannel(0)
    , m_enableFlip(true)
    , m_enableLED(true)
    , m_recordVideo(false)
    , m_forceBind(forceBind)
    , m_vtxFreq(vtxFreq)
{
}

/**
 * @copydoc IProtocol::setup
 */
bool Hubsan::setup()
{
  bool retVal = false;

  a7105SetupSPI();

  for (uint8_t i = 0; i < 100; i++)
  {
    if (this->initRadio())
    {
      retVal = true;
      break;
    }
  }

  return retVal;
}

/**
 * @copydoc IProtocol::bind()
 */
bool Hubsan::bind()
{
  m_sessionID = random();
  m_channel = hubsanAllowedChannels[random() % sizeof(hubsanAllowedChannels)];
  setBindState(0xffffffff);
  m_state = BIND_1;
  m_packetCount = 0;

  return true;
}

/**
 * @copydoc IProtocol::setCommand
 */
bool Hubsan::setCommand(ProtocolCommand command, uint16_t value)
{
  switch (command)
  {
  case COMMAND_THROTTLE:
    if (value < MIN_THROTTLE_US)
      value = MIN_THROTTLE_US;
    m_sticks[COMMAND_THROTTLE] = map(value, MIN_THROTTLE_US, 2000, 0, 255);
    break;
  case COMMAND_YAW:
    m_sticks[command] = map(value, 1000, 2000, 0, 255);
    break;
  case COMMAND_PITCH:
  case COMMAND_ROLL:
    m_sticks[command] = map(value, 1000, 2000, 255, 0);
    break;
  case COMMAND_FLIPS:
    m_enableFlip = value > 1800;
    break;
  case COMMAND_LIGHTS:
    m_enableLED = value <= 1800;
    break;
  case COMMAND_VIDEO:
    m_recordVideo = value > 1800;
    break;
  default:
    return false;
  }

  return true;
}

/**
 * @copydoc IProtocol::tx
 */
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

  Serial.println(m_state);

  switch (m_state)
  {
  case BIND_1:
  case BIND_3:
  case BIND_5:
  case BIND_7:
    buildBindPacket(
        m_state == BIND_7 ? 9 : (m_state == BIND_5 ? 1 : m_state + 1 - BIND_1));
    a7105Strobe(A7105_STANDBY);
    a7105WriteData(a7105_packet, 16, m_channel);
    m_state |= WAIT_WRITE;
    d = 3000;
    break;
  case BIND_1 | WAIT_WRITE:
  case BIND_3 | WAIT_WRITE:
  case BIND_5 | WAIT_WRITE:
  case BIND_7 | WAIT_WRITE:
    a7105Strobe(A7105_RX);
    m_state &= ~WAIT_WRITE;
    m_state++;
    d = 4500; // 7.5mS elapsed since last write
    break;
  case BIND_2:
  case BIND_4:
  case BIND_6:
    if (a7105Busy())
    {
      m_state = BIND_1;
      d = 4500; // No signal, restart binding procedure.  12mS elapsed since
                // last write
    }
    else
    {
      a7105ReadData(a7105_packet, 16);
      m_state++;
      if (m_state == BIND_5)
        a7105WriteID((a7105_packet[2] << 24) | (a7105_packet[3] << 16) |
                     (a7105_packet[4] << 8) | a7105_packet[5]);
      d = 500; // 8mS elapsed time since last write
    }
    break;
  case BIND_8:
    if (a7105Busy() && !m_forceBind)
    {
      m_state = BIND_7;
      d = 15000; // 22.5mS elapsed since last write
    }
    else
    {
      a7105ReadData(a7105_packet, 16);
      if (a7105_packet[1] == 9 || m_forceBind)
      {
        m_state = DATA_1;
        a7105WriteReg(A7105_1F_CODE_I, 0x0F);
        setBindState(0);
        d = 28000; // 35.5mS elapsed since last write
      }
      else
      {
        m_state = BIND_7;
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
                     m_state == DATA_5 ? m_channel + 0x23 : m_channel);
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
        m_rssiChannel = a7105ReadReg(A7105_1D_RSSI_THOLD);
        updateTelemetry();
        a7105Strobe(A7105_RX);
        d = 1000;
      }

      if (++polls >= 7)
      { // 3ms + 3mS + 4*1ms
        if (m_state == DATA_5)
          m_state = DATA_1;
        else
          m_state++;
        telemetryState = doTx;
      }
      break;
    } // switch (telemetryState)
    break;
  } // switch (m_state)

  return d;
}

/**
 * @brief Initializes the A7105 radio.
 * @return True if the radio was successfully initialised
 *
 * If initialization fails, multiple attempts can be made.
 */
bool Hubsan::initRadio()
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

void Hubsan::setBindState(uint32_t ms)
{
  if (ms)
  {
    if (ms == 0xFFFFFFFF)
      m_bindTime = ms;
    else
      m_bindTime = millis() + ms;
    m_state |= BINDING;
  }
  else
    m_state &= ~BINDING;
}

/**
 * @brief Builds a standard data packet.
 */
void Hubsan::buildPacket()
{
  memset(a7105_packet, 0, 16);

#if defined(USE_HUBSAN_EXTENDED)
  if (m_packetCount == 100)
  { // set vTX frequency (H107D)
    a7105_packet[0] = 0x40;
    a7105_packet[1] = (m_vtxFreq >> 8) & 0xff;
    a7105_packet[2] = m_vtxFreq & 0xff;
    a7105_packet[3] = 0x82;
    m_packetCount++;
  }
  else
#endif
  { // 20 00 00 00 80 00 7d 00 84 02 64 db 04 26 79 7b
    a7105_packet[0] = 0x20;
    a7105_packet[2] = m_sticks[COMMAND_THROTTLE];
  }
  a7105_packet[4] = m_sticks[COMMAND_YAW];
  a7105_packet[6] = m_sticks[COMMAND_PITCH];
  a7105_packet[8] = m_sticks[COMMAND_ROLL];

  if (m_packetCount > 100)
  {
    a7105_packet[9] = 0x20;

    if (m_enableLED)
      a7105_packet[9] |= FLAG_LED;

    if (m_enableFlip)
      a7105_packet[9] |= FLAG_FLIP;

    if (m_recordVideo)
      a7105_packet[9] |= FLAG_VIDEO;
  }
  else
  {
    // Sends default value for the 100 first packets
    a7105_packet[9] = 0x20 | FLAG_LED | FLAG_FLIP;
    m_packetCount++;
  }

  a7105_packet[10] = 0x64;
  a7105_packet[11] = (m_id >> 24) & 0xff;
  a7105_packet[12] = (m_id >> 16) & 0xff;
  a7105_packet[13] = (m_id >> 8) & 0xff;
  a7105_packet[14] = (m_id >> 0) & 0xff;

  a7105CRCUpdate(16);
}

/**
 * @brief Builds a binding packet.
 */
void Hubsan::buildBindPacket(uint8_t state)
{
  a7105_packet[0] = state;
  a7105_packet[1] = m_channel;
  a7105_packet[2] = (m_sessionID >> 24) & 0xff;
  a7105_packet[3] = (m_sessionID >> 16) & 0xff;
  a7105_packet[4] = (m_sessionID >> 8) & 0xff;
  a7105_packet[5] = (m_sessionID >> 0) & 0xff;
  a7105_packet[6] = 0x08;
  a7105_packet[7] = 0xe4; //???
  a7105_packet[8] = 0xea;
  a7105_packet[9] = 0x9e;
  a7105_packet[10] = 0x50;
  a7105_packet[11] = (m_id >> 24) & 0xff;
  a7105_packet[12] = (m_id >> 16) & 0xff;
  a7105_packet[13] = (m_id >> 8) & 0xff;
  a7105_packet[14] = (m_id >> 0) & 0xff;

  a7105CRCUpdate(16);
}

/**
 * @brief Parses telemetry data.
 */
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
