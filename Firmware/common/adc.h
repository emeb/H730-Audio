/*
 * adc.h - H730 Audio prototype ADC setup
 * 12-06-20 E. Brombaugh
 */

#ifndef __adc__
#define __adc__

#include "stm32h7xx_hal.h"
#include "arm_math.h"

#define ADC_NUMCHLS 8

enum ADC_CHL_NAMES
{
	ADC_CV1,
	ADC_CV2,
	ADC_CV3,
	ADC_CV4,
	ADC_CV5,
	ADC_CV6,
	ADC_CV7,
	ADC_CV8,
};

enum ADC_CHL_STAT
{
	ACS_QUIET = 0,
	ACS_NEW = 1
};


//extern ADC_HandleTypeDef hadc1, hadc2, hadc3;

extern uint16_t adc_buffer[ADC_NUMCHLS];		// unfiltered, unrouted
extern uint16_t adc_raw[ADC_NUMCHLS];			// filtered, unrouted

#define ADC_getraw(x) (adc_raw[(x)])

void ADC_Init(uint32_t Pin);
int16_t ADC_ctr_dz(uint16_t in);
uint8_t ADC_gethyst(uint8_t chl, uint16_t *data);
void ADC_Pull(uint8_t state);
float32_t *adc_get_stats(void);
void ADC2_Init(void (*callback)(void));
float32_t ADC2_GetVbus(void);
void ADC3_Init(void);
uint16_t ADC3_GetChl(uint8_t chl);
float32_t ADC3_GetVrefp(void);
float32_t ADC3_GetTemp(void);

#endif
