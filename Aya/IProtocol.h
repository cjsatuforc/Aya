/** @file */

#ifndef _IPROTOCOL_AYA_H_
#define _IPROTOCOL_AYA_H_

#include <Arduino.h>

/**
 * @enum ProtocolCommand
 * @brief Command types that can be sent to a model.
 */
enum ProtocolCommand
{
  COMMAND_THROTTLE = 0,
  COMMAND_YAW = 1,
  COMMAND_PITCH = 2,
  COMMAND_ROLL = 3,
  COMMAND_VIDEO,
  COMMAND_FLIPS,
  COMMAND_LIGHTS
};

/**
 * @class IProtocol
 * @brief Interface for a RF protocol
 */
class IProtocol
{
public:
  IProtocol(){};
  virtual ~IProtocol(){};

  virtual bool setup() = 0;
  virtual bool bind() = 0;
  virtual bool setCommand(ProtocolCommand command, uint16_t value) = 0;
  virtual uint16_t tx() = 0;
};

#endif
