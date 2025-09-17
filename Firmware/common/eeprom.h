/*
 * eeprom.h - generic I2C eeprom driver
 * 07-12-19 E. Brombaugh
 */

#ifndef __eeprom__
#define __eeprom__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

HAL_StatusTypeDef eeprom_init(void);

#ifdef __cplusplus
}
#endif

#endif
