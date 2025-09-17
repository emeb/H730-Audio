/*
 * codec.h - codec I2C control port driver for WM8731
 * 12-06-20 E. Brombaugh
 */

#ifndef __codec__
#define __codec__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

extern I2C_HandleTypeDef i2c_handler;
void Codec_Init(void);
int32_t Codec_Reset(void);
void Codec_Mute(uint8_t enable);
void Codec_HPVol(uint8_t vol);
void Codec_InSrc(uint8_t src);
void Codec_InVol(uint8_t vol);
void Codec_MicBoost(uint8_t boost);

#ifdef __cplusplus
}
#endif

#endif
