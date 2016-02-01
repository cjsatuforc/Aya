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

  /**
   * @brief Performs protocol setup and radio initialization.
   * @return True if setup was successful
   */
  virtual bool setup() = 0;

  /**
   * @brief Binds the protocol to a model.
   * @return True if binding was successful
   */
  virtual bool bind() = 0;

  /**
   * @brief Sends a command to the model.
   * @param command Command type to send
   * @param value Value of command (within the standard 1000-2000 uS range)
   * @return True if the command was accepted
   * @see tx()
   *
   * Command may not take effect until tx() is called.
   */
  virtual bool setCommand(ProtocolCommand command, uint16_t value) = 0;

  /**
   * @brief Transmits control state to model.
   * @return If transmission was successful
   */
  virtual uint16_t tx() = 0;
};

#endif
