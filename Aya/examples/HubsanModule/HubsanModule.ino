/** @file */

#include <CPPM.h>
#include <Hubsan.h>

Hubsan hubsan;

uint32_t next_update_us = 0;

/**
 * @brief Setup routine.
 */
void setup()
{
  Serial.begin(9600);

  Serial.print("PPM init: ");
  Serial.println(cppm_init(1)); // Interrupt 1, pin 3

  Serial.print("Hubsan init: ");
  Serial.println(hubsan.setup());

  delay(500);

  Serial.print("Hubsan bind: ");
  Serial.println(hubsan.bind());
}

/**
 * @brief Main routine.
 */
void loop()
{
  if (cppm_fresh)
  {
    cppm_read();

    hubsan.setCommand(COMMAND_ROLL, cppm_channels[0]);
    hubsan.setCommand(COMMAND_PITCH, cppm_channels[1]);
    hubsan.setCommand(COMMAND_THROTTLE, cppm_channels[2]);
    hubsan.setCommand(COMMAND_YAW, cppm_channels[3]);
    hubsan.setCommand(COMMAND_LIGHTS, cppm_channels[4]);
    hubsan.setCommand(COMMAND_FLIPS, cppm_channels[5]);
  }

  unsigned long now_us = micros();
  if (now_us >= next_update_us)
    next_update_us = (now_us + hubsan.tx());
}
