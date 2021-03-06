/** @file */

#ifndef _HUBSAN_AYA_H_
#define _HUBSAN_AYA_H_

#include "IProtocol.h"

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
  Hubsan(uint16_t id = 0x35000001, bool forceBind = false, uint16_t vtxFreq = 5800);
  virtual ~Hubsan(){};

  bool setup();
  bool bind();
  bool setCommand(ProtocolCommand command, uint16_t value);
  uint16_t tx();

private:
  bool initRadio();
  void setBindState(uint32_t ms);
  void buildPacket();
  void buildBindPacket(uint8_t state);
  void updateTelemetry();

  uint16_t m_id;
  int16_t m_state;
  uint16_t m_vtxFreq;
  uint8_t m_channel;
  uint32_t m_sessionID;
  uint8_t m_packetCount;
  uint32_t m_bindTime;
  uint8_t m_rssiChannel;
  bool m_enableFlip;
  bool m_enableLED;
  bool m_recordVideo;
  uint8_t m_sticks[4];
  bool m_forceBind;
};

#endif
