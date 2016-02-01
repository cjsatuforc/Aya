/** @file */

#include <TimerOne.h>

#include <CPPM.h>
#include <Hubsan.h>

#define LED_PIN 13

#define THROTTLE_CHANNEL 2
#define MIN_THROTTLE 1050

Hubsan hubsan(true);
uint32_t next_update_us = 0;

/**
 * @brief Setup routine.
 */
void setup()
{
  Serial.begin(9600);

  Timer1.attachInterrupt(toggle_led);
  Timer1.initialize(50000); // 0.05s

  cppm_init(1); // Interrupt 1, pin 3

  /* Wait for PPM signal */
  while (!cppm_fresh)
    delay(10);
  cppm_read();

  /* Wait for zero throttle */
  while (cppm_channels[THROTTLE_CHANNEL] > MIN_THROTTLE)
  {
    if (cppm_fresh)
      cppm_read();

    delay(10);
  }

  delay(1000);

  hubsan.setup();
  hubsan.bind();

  Timer1.setPeriod(1000000); // 1s
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
    hubsan.setCommand(COMMAND_THROTTLE, cppm_channels[THROTTLE_CHANNEL]);
    hubsan.setCommand(COMMAND_YAW, cppm_channels[3]);
    hubsan.setCommand(COMMAND_LIGHTS, cppm_channels[4]);
    hubsan.setCommand(COMMAND_FLIPS, cppm_channels[5]);
  }

  unsigned long now_us = micros();
  if (now_us >= next_update_us)
    next_update_us = (now_us + hubsan.tx());
}

/**
 * @brief Toggles status indicator LED.
 */
void toggle_led()
{
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
}
