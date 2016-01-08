#ifndef _CPPM_AYA_H_
#define _CPPM_AYA_H_

#define CPPM_NUM_CHANNELS 8

#include <Arduino.h>

extern bool cppm_fresh;
extern int cppm_channels[CPPM_NUM_CHANNELS];

void cppm_init(int interrupt);
void cppm_read();

#endif
