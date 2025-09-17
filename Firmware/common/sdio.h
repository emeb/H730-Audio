/*
 * sdio.h - H7 Audio SD card setup
 * 07-20-19 E. Brombaugh
 */

#ifndef __sdio__
#define __sdio__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include "ff.h"
#include "ff_gen_drv.h"
#include "sd_diskio.h" /* defines SD_Driver as external */

void sdio_init(void);
void sdio_power(uint8_t state);
uint8_t sdio_detect(void);
HAL_StatusTypeDef sdio_abort(void);
uint8_t sdio_test(void);

#ifdef __cplusplus
}
#endif

#endif
