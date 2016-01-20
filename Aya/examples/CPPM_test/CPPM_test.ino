#include "CPPM.h"

void setup()
{
  Serial.begin(9600);

  bool initResult = cppm_init(1); // Interrupt 1, pin 3
  Serial.println(initResult);
}

void loop()
{
  if (cppm_fresh)
  {
    cppm_read();
    print_cppm_values();
  }
}

void print_cppm_values()
{
  for (size_t i = 0; i < CPPM_NUM_CHANNELS; i++)
  {
    Serial.print(cppm_channels[i]);
    Serial.print("\t");
  }

  Serial.println();
}
