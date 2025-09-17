/*
 * led.h - H750 Audio LED setup
 * 03-02-20 E. Brombaugh
 */

#ifndef __led__
#define __led__

#include "stm32h7xx_hal.h"

/* array indexes for LEDs */
enum leds
{
	LED_0,
	LED_1,
};

void LEDInit(void);
void LEDOn(uint8_t led);
void LEDOff(uint8_t led);
void LEDToggle(uint8_t led);

#endif
