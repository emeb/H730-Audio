/*
 * led.c - H730 Audio LED setup
 * 12-04-20 E. Brombaugh
 */

#include "led.h"

#define LD0_Pin GPIO_PIN_9
#define LD0_GPIO_Port GPIOD

/*
 * Initialize the breakout board LED
 */
void LEDInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable GPIO A Clock */
	__HAL_RCC_GPIOD_CLK_ENABLE();
	
	/* Enable LD0 for output */
	GPIO_InitStructure.Pin =  LD0_Pin;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;
	GPIO_InitStructure.Pull = GPIO_NOPULL ;
	HAL_GPIO_Init(LD0_GPIO_Port, &GPIO_InitStructure);
}

/*
 * Turn on LED
 */
void LEDOn(uint8_t led)
{
	LD0_GPIO_Port->BSRR = LD0_Pin;
}

/*
 * Turn off LED
 */
void LEDOff(uint8_t led)
{
	LD0_GPIO_Port->BSRR = LD0_Pin<<16;
}

/*
 * Toggle LED
 */
void LEDToggle(uint8_t led)
{
	LD0_GPIO_Port->ODR ^= LD0_Pin;
}

