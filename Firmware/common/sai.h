/*
 * sai.h - H7 SAI setup
 * 06-19-19 E. Brombaugh
 */

#ifndef __sai__
#define __sai__

#include "stm32h7xx_hal.h"

#define MAX_DMA_BUFFSZ 512
#define MAX_STEREO_BUFSZ (MAX_DMA_BUFFSZ/2)
#define MAX_MONO_BUFSZ (MAX_STEREO_BUFSZ/2)
#define FAST_DMA_BUFFSZ 128
#define SLOW_DMA_BUFFSZ 512
#define LONG_DMA_BUFFSZ MAX_DMA_BUFFSZ
#define LONG_STEREO_BUFSZ (LONG_DMA_BUFFSZ/2)
#define FSAMPLE 48000

void sai_init(uint16_t bufsz);
void sai_start(void);
void sai_stop(void);
uint32_t sai_get_samplerate(void);
uint16_t sai_get_bufsz(void);
uint8_t sai_set_bufsz(uint16_t bufsz);

#endif
