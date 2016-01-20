#include "CPPM.h"

void setup()
{
  Serial.begin(9600);

  cppm_init(1);
  Serial.println("Hi");
}

void loop()
{
  if (cppm_fresh)
  {
    cppm_read();
    print_cppm_values();
    delay(50);
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
