#include "CPPM.h"

bool cppm_fresh;
int cppm_channels[CPPM_NUM_CHANNELS];

int cppm_raw[CPPM_NUM_CHANNELS];
bool cppm_frame_good;

const int interrupt_to_pin[] = {2, 3};

void cppm_isr()
{
  uint32_t time_us;
  int16_t pulse_width_us;
  static uint32_t last_time_us = 0;
  static uint8_t channel = 0;

  time_us = micros();

  interrupts();

  pulse_width_us = time_us - last_time_us;
  last_time_us = time_us;

  if (pulse_width_us > 3000)
  {
    cppm_fresh = cppm_frame_good;
    cppm_frame_good = true;
    channel = 0;
  }
  else if (channel < CPPM_NUM_CHANNELS)
  {
    if (pulse_width_us >= 500 && pulse_width_us <= 2500)
      cppm_raw[channel] = pulse_width_us;
    else
      cppm_frame_good = false;
    channel++;
  }
}

void cppm_init(int interrupt)
{
  cppm_frame_good = false;

  for (size_t i = 0; i < CPPM_NUM_CHANNELS; i++)
    cppm_channels[i] = 1500;

  cppm_read();

  pinMode(interrupt_to_pin[interrupt], INPUT);
  attachInterrupt(interrupt, cppm_isr, CPPM_LOGIC_DIRECTION);
}

void cppm_read()
{
  noInterrupts();

  for (size_t i = 0; i < CPPM_NUM_CHANNELS; i++)
    cppm_channels[i] = cppm_raw[i];

  cppm_fresh = false;

  interrupts();
}
