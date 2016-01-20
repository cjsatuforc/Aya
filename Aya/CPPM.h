/** @file */

#ifndef _CPPM_AYA_H_
#define _CPPM_AYA_H_

/**
 * @def CPPM_NUM_CHANNELS
 * @brief Number of channels to be read by CPPM
 */
#define CPPM_NUM_CHANNELS 8

#include <Arduino.h>

/**
 * @var cppm_fresh
 * @brief Flag to indicate if there is new data to be retrieved.
 *
 * Automatically reset on every call to cppm_read().
 */
extern bool cppm_fresh;

/**
 * @var cppm_channels
 * @brief Array of timing values read form CPPM.
 *
 * Values are in microseconds and should range from around 1000 - 2000.
 */
extern int cppm_channels[CPPM_NUM_CHANNELS];

bool cppm_init(int interrupt, int logic_direction = FALLING);

void cppm_read();

#endif
