/** @file */

#ifndef _HUBSAN_AYA_H_
#define _HUBSAN_AYA_H_

#include "IProtocol.h"

#define hubsanID 0x35000001

enum
{
  BIND_1,
  BIND_2,
  BIND_3,
  BIND_4,
  BIND_5,
  BIND_6,
  BIND_7,
  BIND_8,
  DATA_1,
  DATA_2,
  DATA_3,
  DATA_4,
  DATA_5,
};

/**
 * @class Hubsan
 * @brief HUbsan RF protocol
 */
class Hubsan : public IProtocol
{
public:
  Hubsan();
  virtual ~Hubsan() {};

  bool setup();
  bool bind();
  bool setCommand(ProtocolCommand command, uint16_t value);
  uint16_t tx();

private:
  bool init();
  void setBindState(uint32_t ms);
  void updateTelemetry();
  void buildPacket();
  void buildBindPacket(uint8_t hubsanState);

  int16_t hubsanState;
  uint16_t hubsanVTXFreq;
  uint8_t chan;
  uint32_t sessionID;
  uint8_t hubsanPacketCount;
  uint8_t hubsanTxState, hubsanRFMode;
  uint32_t bind_time;
  uint8_t rssiBackChannel;
  bool enableFlip;
  bool enableLEDs;
  uint8_t sticks[4];
};

#endif
