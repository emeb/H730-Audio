/*
 * main.h - top-level include for build control
 * 09-26-2019 E. Brombaugh
 */

#ifndef __main__
#define __main__

#include "stm32h7xx_hal.h"
#include "arm_math.h"

/* uncomment this to shorten programming */
//#define DEV_BUILD

extern const char *fwVersionStr;
extern const char *bdate;
extern const char *btime;

#endif
