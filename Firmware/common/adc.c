/*
 * adc.h - H730 Audio prototype ADC setup
 * 12-06-20 E. Brombaugh
 *
 * NOTE - CV sampling rate is ~ 3.1kHz, approx 2x the faster buffer rate
 */

#include <stdlib.h>
#include "adc.h"
#include "printf.h"

/* Diagnostic flag */
//#define ENABLE_ADCDIAG
#ifdef ENABLE_ADCDIAG
#define FLAG_0  GPIOB->BSRR=(1<<(7+16))
#define FLAG_1  GPIOB->BSRR=(1<<7)
#else
#define FLAG_0
#define FLAG_1
#endif

/* statistics on ADC values? */
//#define ENABLE_STATS
#ifdef ENABLE_STATS
#define STATS_SAMPS 1024
uint16_t stats_buf[STATS_SAMPS], stats_cnt, stats_rdy, stats_chl;
float32_t stats[ADC_NUMCHLS*2];
#endif

ADC_HandleTypeDef hadc1, hadc2, hadc3;
DMA_HandleTypeDef hdma_adc1;

/* Vsense alarm callback */
void (*adc_vsense_alarm)(void) = NULL;

/* pins for ADC */
uint32_t adc_pins;

/* ADC buffers and routing */
uint16_t adc_buffer[ADC_NUMCHLS] __attribute__ ((section (".ram_d3_data"))),
		adc_raw[ADC_NUMCHLS],
		adc_hyst[ADC_NUMCHLS];
uint8_t adc_chlstat[ADC_NUMCHLS];

/* Filtering and hysteresis */
#define HYST_THRESH 16
#define OS_RATIO 32
#define OS_SHIFT 9
int32_t adc_acc[ADC_NUMCHLS];
uint16_t adc_dly[ADC_NUMCHLS][OS_RATIO];
uint16_t adc_dly_ptr;

/*
 * Initialize the ADC
 */
void ADC_Init(uint32_t Pin)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	ADC_MultiModeTypeDef multimode = {0};
	ADC_ChannelConfTypeDef sConfig = {0};
	DMA_Stream_TypeDef *stream_adc1;

	adc_pins = Pin;

	/* init filter, hysteresis and routing */
	uint16_t i,j;

	for(i=0;i<ADC_NUMCHLS;i++)
	{
		adc_raw[i] = 0;
		adc_hyst[i] = 0;
		adc_chlstat[i] = 0;
		adc_acc[i] = 0;
		for(j=0;j<OS_RATIO;j++)
			adc_dly[i][j] = 0;
	}
	adc_dly_ptr = 0;

#ifdef ENABLE_STATS
	/* start statistics */
	stats_cnt = 0;
	stats_rdy = 0;
	stats_chl = 0;
#endif

    /*
		ADC1 GPIO Configuration
		CV1 = PA0 = ADC1_INP16
		CV2 = PA1 = ADC1_INP17
		CV3 = PA2 = ADC1_INP14
		CV4 = PA3 = ADC1_INP15
		CV5 = PA4 = ADC1_INP18
		CV6 = PA5 = ADC1_INP19
		CV7 = PA6 = ADC1_INP3
        CV8 = PA7 = ADC1_INP7
        VSENSE = PC5 = ADC1_INP8
    */

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitStruct.Pin = adc_pins;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_5;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

#ifdef ENABLE_ADCDIAG
    __HAL_RCC_GPIOB_CLK_ENABLE();

	/* Enable PB7 for diagnostic output */
	GPIO_InitStruct.Pin =  GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
	GPIO_InitStruct.Pull = GPIO_NOPULL ;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

#if 0
    /* test diag bit */
    while(1)
    {
        FLAG_1;
        HAL_Delay(10);
        FLAG_0;
        HAL_Delay(10);
    }
#endif
#endif

	/* Enable ADC clock */
	__HAL_RCC_ADC12_CLK_ENABLE();

	/* init main ADC */
	hadc1.Instance = ADC1;
	hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
	hadc1.Init.Resolution = ADC_RESOLUTION_16B;
	hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
	hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
	hadc1.Init.LowPowerAutoWait = DISABLE;
	hadc1.Init.ContinuousConvMode = ENABLE;
	hadc1.Init.NbrOfConversion = ADC_NUMCHLS;
	hadc1.Init.DiscontinuousConvMode = DISABLE;
	hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc1.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR;
	hadc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
	hadc1.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
	hadc1.Init.OversamplingMode = ENABLE;
	hadc1.Init.Oversampling.Ratio = 32;
	hadc1.Init.Oversampling.RightBitShift = 5;
	hadc1.Init.Oversampling.TriggeredMode = ADC_TRIGGEREDMODE_SINGLE_TRIGGER;
	hadc1.Init.Oversampling.OversamplingStopReset = ADC_REGOVERSAMPLING_RESUMED_MODE;
	HAL_ADC_Init(&hadc1);

	/* manually force the OVSS bits since HAL doesn't */
	ADC1->CFGR2 = (ADC1->CFGR2 & ~ADC_CFGR2_OVSS) | (hadc1.Init.Oversampling.RightBitShift << ADC_CFGR2_OVSS_Pos);

	/* calibrate */
	HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);

	/* set up multi-chl conversions */
	multimode.Mode = ADC_MODE_INDEPENDENT;
	HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode);

	sConfig.Channel = ADC_CHANNEL_16;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_16CYCLES_5;
	sConfig.SingleDiff = ADC_SINGLE_ENDED;
	sConfig.OffsetNumber = ADC_OFFSET_NONE;
	sConfig.Offset = 0;
	HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    sConfig.Channel = ADC_CHANNEL_17;
	sConfig.Rank = ADC_REGULAR_RANK_2;
	HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    sConfig.Channel = ADC_CHANNEL_14;
	sConfig.Rank = ADC_REGULAR_RANK_3;
	HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    sConfig.Channel = ADC_CHANNEL_15;
	sConfig.Rank = ADC_REGULAR_RANK_4;
	HAL_ADC_ConfigChannel(&hadc1, &sConfig);

	sConfig.Channel = ADC_CHANNEL_18;
	sConfig.Rank = ADC_REGULAR_RANK_5;
	HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    sConfig.Channel = ADC_CHANNEL_19;
	sConfig.Rank = ADC_REGULAR_RANK_6;
	HAL_ADC_ConfigChannel(&hadc1, &sConfig);

	sConfig.Channel = ADC_CHANNEL_3;
	sConfig.Rank = ADC_REGULAR_RANK_7;
	HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    sConfig.Channel = ADC_CHANNEL_7;
	sConfig.Rank = ADC_REGULAR_RANK_8;
	HAL_ADC_ConfigChannel(&hadc1, &sConfig);

	/* Enable the DMA clock */
	__HAL_RCC_DMA2_CLK_ENABLE();

    /* DMA Init */
    hdma_adc1.Instance = DMA2_Stream0;
    hdma_adc1.Init.Request = DMA_REQUEST_ADC1;
    hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode = DMA_CIRCULAR;
    hdma_adc1.Init.Priority = DMA_PRIORITY_LOW;
    hdma_adc1.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_adc1);

    __HAL_LINKDMA(&hadc1,DMA_Handle,hdma_adc1);

	/* set up DMA details */
	stream_adc1 = hdma_adc1.Instance;
	stream_adc1->CR &= (uint32_t)(~DMA_SxCR_DBM);
	stream_adc1->NDTR = (uint32_t)ADC_NUMCHLS;
	stream_adc1->PAR = (uint32_t)&hadc1.Instance->DR;
	stream_adc1->M0AR = (uint32_t)&adc_buffer;

	/* Enable the DMA transfer complete interrupt */
	__HAL_DMA_ENABLE_IT(&hdma_adc1, DMA_IT_TC);

	/* DMA2_Stream0_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 6, 0);
	HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

	/* Enable DMA */
	__HAL_DMA_ENABLE(&hdma_adc1);

	/* enable ADC */
    ADC_Enable(&hadc1);

	/* start conversions */
	LL_ADC_REG_StartConversion(hadc1.Instance);
}

/*
 * center dead zone converter for feedback
 * output ranges -2048 -> 2048 w/ 255 wide zero deadzone in center
 */
int16_t ADC_ctr_dz(uint16_t in)
{
	int16_t result = in-2048;
	int16_t sgn = result<0 ? -1 : 1;
	int32_t scl = abs((int32_t)result) - ((sgn < 0) ? 128 : 127);

	scl = (scl<0) ? 0 : scl;
	scl = (scl * 2185)>>11;
	return sgn * scl;
}

/*
 * get hyst status & data
 */
uint8_t ADC_gethyst(uint8_t chl, uint16_t *data)
{
	*data = adc_hyst[chl];

	if(adc_chlstat[chl]&ACS_NEW)
	{
		adc_chlstat[chl] &= ~ACS_NEW;
		return 1;
	}
	else
		return 0;
}

/*
 * set ADC GPIO pulls
 */
void ADC_Pull(uint8_t state)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = adc_pins;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;

	switch(state)
	{
		default:
		case 0:
    		GPIO_InitStruct.Pull = GPIO_NOPULL;
			break;

		case 1:
    		GPIO_InitStruct.Pull = GPIO_PULLDOWN;
			break;

		case 2:
    		GPIO_InitStruct.Pull = GPIO_PULLUP;
			break;
	}
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/*
 * handle ADC DMA IRQs
 */
void DMA2_Stream0_IRQHandler(void)
{
	uint8_t i;
	uint32_t * inval_addr;

	FLAG_1;

	/* Transfer complete interrupt */
	if (__HAL_DMA_GET_FLAG(&hdma_adc1, DMA_FLAG_TCIF0_4) != RESET)
	{
		/* Clear the Interrupt flag */
		__HAL_DMA_CLEAR_FLAG(&hdma_adc1, DMA_FLAG_TCIF0_4);

		/* invalidate Dcache of ADC buffer */
		inval_addr = (uint32_t *)((uint32_t)adc_buffer & ~0x1f);
		SCB_InvalidateDCache_by_Addr(inval_addr, ADC_NUMCHLS+32);

		/* filter raw buffer to holding buffer */
		for(i=0;i<ADC_NUMCHLS;i++)
		{
			/* add new, subtract old to acc */
			adc_acc[i] -= adc_dly[i][adc_dly_ptr];
			adc_acc[i] += adc_buffer[i];

			/* store new in buffer */
			adc_dly[i][adc_dly_ptr] = adc_buffer[i];

			/* normalize acc to output */
			adc_raw[i] = adc_acc[i]>>OS_SHIFT;

			/* process hysteresis */
			if((abs(adc_raw[i]-adc_hyst[i])>HYST_THRESH) ||
				(adc_raw[i] == 0) || (adc_raw[i] == 4095))
			{
				/* update hyst value and flag as new */
				if(adc_hyst[i] != adc_raw[i])
				{
					/*
					 * don't flag new if prev value was the same
					 * this happens at ends (0 or 4095)
					 */
					adc_chlstat[i] |= ACS_NEW;
					adc_hyst[i] = adc_raw[i];
				}
			}
		}
		adc_dly_ptr = (adc_dly_ptr+1)%OS_RATIO;

#ifdef ENABLE_STATS
		if(stats_rdy == 0)
		{
			//stats_buf[stats_cnt++] = adc_buffer[stats_chl];
			stats_buf[stats_cnt++] = adc_raw[stats_chl];
			if(stats_cnt==STATS_SAMPS)
			{
				stats_cnt = 0;
				stats_rdy = 1;
			}
		}
#endif

	}

	FLAG_0;
}

#ifdef ENABLE_STATS
/*
 * compute stats from buffered samples
 */
float32_t *adc_get_stats(void)
{
	uint16_t i;
	double sum, temp;

	/* ready to compute? */
	if(stats_rdy == 0)
	{
		/* no results yet */
		return NULL;
	}

	/* compute mean */
	sum = 0.0;
	for(i=0;i<STATS_SAMPS;i++)
	{
		sum += stats_buf[i];
	}
	stats[2*stats_chl] = (float32_t)(sum / STATS_SAMPS);

	/* compute std dev */
	sum = 0.0;
	for(i=0;i<STATS_SAMPS;i++)
	{
		temp = stats_buf[i];
		temp -= stats[stats_chl*2];
		temp *= temp;
		sum += temp;
	}
	stats[stats_chl*2+1] = (float32_t)(sum / STATS_SAMPS);
	stats[stats_chl*2+1] = sqrtf(stats[stats_chl*2+1]);

	/* next channel */
	stats_chl++;

	/* done? */
	if(stats_chl == ADC_NUMCHLS)
	{
		/* start next */
		stats_chl = 0;
		stats_rdy = 0;

		/* result ready */
		return stats;
	}

	/* no results yet */
	stats_rdy = 0;
	return NULL;
}
#endif

/*
 * Init ADC2 for Vsense Watchdog
 */
void ADC2_Init(void (*callback)(void))
{
	ADC_ChannelConfTypeDef sConfig = {0};

	__HAL_RCC_ADC12_CLK_ENABLE();

	hadc2.Instance = ADC2;
	hadc2.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
	hadc2.Init.Resolution = ADC_RESOLUTION_16B;
	hadc2.Init.ScanConvMode = ADC_SCAN_DISABLE;
	hadc2.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	hadc2.Init.LowPowerAutoWait = DISABLE;
	hadc2.Init.ContinuousConvMode = ENABLE;
	hadc2.Init.NbrOfConversion = 1;
	hadc2.Init.DiscontinuousConvMode = DISABLE;
	hadc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc2.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc2.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;
	hadc2.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
	hadc2.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
	hadc2.Init.OversamplingMode = DISABLE;
	HAL_ADC_Init(&hadc2);

    ADC_Enable(&hadc2);

	//#define ADC_CHANNEL_VSENSE ADC_CHANNEL_19	// PA5
	#define ADC_CHANNEL_VSENSE ADC_CHANNEL_8	// Vsense

	sConfig.Channel = ADC_CHANNEL_VSENSE;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_810CYCLES_5;
	sConfig.SingleDiff = ADC_SINGLE_ENDED;
	sConfig.OffsetNumber = ADC_OFFSET_NONE;
	sConfig.Offset = 0;
	HAL_ADC_ConfigChannel(&hadc2, &sConfig);

	adc_vsense_alarm = callback;

	/* set up Vsense alarm */
	if(adc_vsense_alarm)
	{
		ADC_AnalogWDGConfTypeDef AnalogWDGConfig;

		/* Analog watchdog 1 configuration */
		AnalogWDGConfig.WatchdogNumber = ADC_ANALOGWATCHDOG_1;
		AnalogWDGConfig.WatchdogMode = ADC_ANALOGWATCHDOG_SINGLE_REG;
		AnalogWDGConfig.Channel = ADC_CHANNEL_VSENSE;
		AnalogWDGConfig.ITMode = ENABLE;
		AnalogWDGConfig.HighThreshold = 0x03ffffff;
		AnalogWDGConfig.LowThreshold = 45056;	// 0xb000 : Vbus = ~4.53V
		HAL_ADC_AnalogWDGConfig(&hadc2, &AnalogWDGConfig);

		/* enable common ADC IRQ */
		HAL_NVIC_SetPriority(ADC_IRQn, 0, 0);
		HAL_NVIC_EnableIRQ(ADC_IRQn);

		printf("ADC2_Init: watchdog enabled LTR1 = 0x%08X\n\r", ADC2->LTR1_TR1);
	}

	/* Run the ADC calibration in single-ended mode */
    HAL_ADCEx_Calibration_Start(&hadc2, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);

    /* start conversion */
    HAL_ADC_Start(&hadc2);

}

/*
 * get the Vbus result
 */
float32_t ADC2_GetVbus(void)
{
	float32_t vrefp = ADC3_GetVrefp();

	return 2.0F*vrefp*(float32_t)HAL_ADC_GetValue(&hadc2)/65536.0F;
}

/*
 * ADC IRQ Handler
 */
void ADC_IRQHandler(void)
{
	if(__HAL_ADC_GET_FLAG(&hadc2, ADC_FLAG_AWD1) == SET)
	{
		__HAL_ADC_CLEAR_FLAG(&hadc2, ADC_FLAG_AWD1);

		if(adc_vsense_alarm)
			(*adc_vsense_alarm)();
	}
}

/*
 * Init ADC3 for volt & temp sense
 */
void ADC3_Init(void)
{
	__HAL_RCC_ADC3_CLK_ENABLE();

	hadc3.Instance = ADC3;
	hadc3.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
	hadc3.Init.Resolution = ADC_RESOLUTION_12B;
	hadc3.Init.ScanConvMode = ADC_SCAN_DISABLE;
	hadc3.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	hadc3.Init.LowPowerAutoWait = DISABLE;
	hadc3.Init.ContinuousConvMode = ENABLE;
	hadc3.Init.NbrOfConversion = 1;
	hadc3.Init.DiscontinuousConvMode = DISABLE;
	hadc3.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc3.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc3.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;
	hadc3.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
	hadc3.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
	hadc3.Init.OversamplingMode = DISABLE;
	HAL_ADC_Init(&hadc3);

    ADC_Enable(&hadc3);
}

/*
 * get vref or temp reading
 */
uint16_t ADC3_GetChl(uint8_t chl)
{
	ADC_ChannelConfTypeDef sConfig = {0};
	uint16_t result;

	if(!chl)
	{
		sConfig.Channel = ADC_CHANNEL_VREFINT;
	}
	else
	{
		sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
	}

	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_810CYCLES_5;
	sConfig.SingleDiff = ADC_SINGLE_ENDED;
	sConfig.OffsetNumber = ADC_OFFSET_NONE;
	sConfig.Offset = 0;
	HAL_ADC_ConfigChannel(&hadc3, &sConfig);

    /* Run the ADC calibration in single-ended mode */
    HAL_ADCEx_Calibration_Start(&hadc3, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);

    /* start conversion */
    HAL_ADC_Start(&hadc3);

    /* wait for end of conversion */
    HAL_ADC_PollForConversion(&hadc3, 10);

    /* get result */
    result = HAL_ADC_GetValue(&hadc3);

    /* shut off ADC */
    HAL_ADC_Stop(&hadc3);

	return result;
}

/*
 * Compute actual Vref+ using Vint
 */
float32_t ADC3_GetVrefp(void)
{
    float32_t vref, vrefcal, vrefp;

    vref = ADC3_GetChl(0);
    vrefcal = *VREFINT_CAL_ADDR;
	vrefp = 3.3F * vrefcal / vref;

	return vrefp;
}

/*
 * Compute actual temp in C
 */
float32_t ADC3_GetTemp(void)
{
    float32_t vdda, vtmp, cal1, cal2, temp;

	vdda = ADC3_GetVrefp();
    vtmp = ADC3_GetChl(1);
    vtmp = vtmp * (vdda/3.3F);   // compensate VDDA variation
	cal1 = *TEMPSENSOR_CAL1_ADDR;
	cal2 = *TEMPSENSOR_CAL2_ADDR;

    temp = ((110.0F - 30.0F)/(cal2 - cal1))*(vtmp - cal1) + 30.0F;
	return temp;
}
