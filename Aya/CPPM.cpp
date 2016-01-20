/** @file */

#include "CPPM.h"

/**
 * @def CPPM_US_NEW_FRAME
 * @brief Minimum time between pulses indicating a new CPPM frame.
 */
#define CPPM_US_NEW_FRAME 2500

/**
 * @def CPPM_US_PULSE_MIN
 * @brief Minimum valid pulse time for a channel value.
 */
#define CPPM_US_PULSE_MIN 800

/**
 * @def CPPM_US_PULSE_MAX
 * @brief Maximum valid pulse time for a channel value.
 */
#define CPPM_US_PULSE_MAX 2200

bool cppm_fresh;
int cppm_channels[CPPM_NUM_CHANNELS];

/**
 * @var cppm_raw
 * @brief Raw pulse widths read by ISR.
 */
int cppm_raw[CPPM_NUM_CHANNELS];

/**
 * @var cppm_frame_good
 * @param Flag to indicate if the current frame being counted is valid.
 */
bool cppm_frame_good;

/**
 * @var interrupt_to_pin
 * @brief Look up table of interrupt number to pin number.
 */
const int interrupt_to_pin[] = {2, 3};

/**
 * @brief Called when pin interrupt is fired, does CPPM counting
 */
void cppm_isr()
{
  uint32_t time_us;
  int16_t pulse_width_us;
  static uint32_t last_time_us = 0;
  static uint8_t channel = 0;

  time_us = micros();

  pulse_width_us = time_us - last_time_us;
  last_time_us = time_us;

  // Start of new frame
  if (pulse_width_us > CPPM_US_NEW_FRAME)
  {
    cppm_fresh = cppm_frame_good;
    cppm_frame_good = true;
    channel = 0;
  }
  // End of channel pulse
  else if (channel < CPPM_NUM_CHANNELS)
  {
    if (pulse_width_us >= CPPM_US_PULSE_MIN && pulse_width_us <= CPPM_US_PULSE_MAX)
      cppm_raw[channel] = pulse_width_us;
    else
      cppm_frame_good = false;
    channel++;
  }
}

/**
 * @brief Initialises CPPM decoder.
 * @param interrupt Interrupt number to use
 * @param logic_direction Logic direction for interrupt, must match TX
 * @return True on successful initialisation
 */
bool cppm_init(int interrupt, int logic_direction)
{
  if (logic_direction != FALLING && logic_direction != RISING)
    return false;

  cppm_frame_good = false;

  for (size_t i = 0; i < CPPM_NUM_CHANNELS; i++)
    cppm_channels[i] = 1500;

  cppm_read();

  pinMode(interrupt_to_pin[interrupt], INPUT);
  attachInterrupt(interrupt, cppm_isr, logic_direction);

  return true;
}

/**
 * @brief Reads new values from CPPM decoder.
 *
 * Should be called before reading from cppm_channels array if cppm_fresh is
 * true.
 */
void cppm_read()
{
  noInterrupts();

  for (size_t i = 0; i < CPPM_NUM_CHANNELS; i++)
    cppm_channels[i] = cppm_raw[i];

  cppm_fresh = false;

  interrupts();
}
