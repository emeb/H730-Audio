/*
 * cordic.h - various array operations with CORDIC periph
 * 11-11-20 E. Brombaugh
 */

#ifndef __cordic__
#define __cordic__

#include "stm32h7xx_hal.h"
#include "arm_math.h"

void cordic_init(void);
void cordic_r2p_int32(int32_t *data, int32_t sz);
void cordic_p2r_int32(int32_t *data, int32_t sz);
void cordic_r2p_float(float32_t *data, int32_t sz);
void cordic_p2r_float(float32_t *data, int32_t sz);

#endif
