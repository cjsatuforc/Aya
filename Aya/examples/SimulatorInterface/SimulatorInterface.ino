/** @file */

#include "CPPM.h"

/**
 * @brief Setup routine.
 */
void setup()
{
  Serial.begin(9600);

  bool initResult = cppm_init(1); // Interrupt 1, pin 3
  Serial.println(initResult);
}

/**
 * @brief Main routine.
 */
void loop()
{
  if (cppm_fresh)
  {
    cppm_read();

    Joystick.X(map(cppm_channels[0], 1000, 2000, 0, 1023));
    Joystick.Y(map(cppm_channels[1], 1000, 2000, 0, 1023));
    Joystick.Z(map(cppm_channels[2], 1000, 2000, 0, 1023));
    Joystick.Zrotate(map(cppm_channels[3], 1000, 2000, 0, 1023));

    Joystick.sliderLeft(map(cppm_channels[4], 1000, 2000, 0, 1023));
    Joystick.sliderRight(map(cppm_channels[5], 1000, 2000, 0, 1023));
  }
}
