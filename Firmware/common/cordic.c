/*
 * cordic.c - various array operations with CORDIC periph
 * 11-11-20 E. Brombaugh
 */

#include "cordic.h"

#define INV_PI (1.0F/PI)
#define NORM (2147483648.0F)
#define INV_NORM (1.0F/NORM)

CORDIC_HandleTypeDef hcordic;
CORDIC_ConfigTypeDef hconfig;

void cordic_init(void)
{
	__HAL_RCC_CORDIC_CLK_ENABLE();
	hcordic.Instance = CORDIC;
	HAL_CORDIC_Init(&hcordic);

	/* set up common stuff */
	hconfig.Function = CORDIC_FUNCTION_COSINE;
	hconfig.Scale = CORDIC_SCALE_0;        /*!< Scaling factor */
	hconfig.InSize = CORDIC_INSIZE_32BITS;       /*!< Width of input data */
	hconfig.OutSize = CORDIC_OUTSIZE_32BITS;      /*!< Width of output data */
	hconfig.NbWrite = CORDIC_NBWRITE_2;      /*!< Number of 32-bit write expected for one calculation */
	hconfig.NbRead = CORDIC_NBREAD_2;       /*!< Number of 32-bit read expected after one calculation */
	hconfig.Precision = CORDIC_PRECISION_6CYCLES;    /*!< Number of cycles for calculation */

}

/*
 * convert integer rectangular to polar. input data is interleaved x,y
 * output data is phs,mag interleaved
 */
void cordic_r2p_int32(int32_t *data, int32_t sz)
{
	int32_t *src = data, *dst = data;

	hconfig.Function = CORDIC_FUNCTION_PHASE;
	HAL_CORDIC_Configure(&hcordic, &hconfig);

	/* start first calc */
	CORDIC->WDATA = *src++;
	CORDIC->WDATA = *src++;
	sz--;

	/* do all intermediate calcs */
	while(sz--)
	{
		CORDIC->WDATA = *src++;
		CORDIC->WDATA = *src++;
		*dst++ = CORDIC->RDATA;
		*dst++ = CORDIC->RDATA;
	}

	/* get last result */
	*dst++ = CORDIC->RDATA;
	*dst++ = CORDIC->RDATA;
}

/*
 * convert integer polar to rectangular. input data is interleaved phs,mag
 * output data is interleaved x,y
 */
void cordic_p2r_int32(int32_t *data, int32_t sz)
{
	int32_t *src = data, *dst = data;

	/* don't try to process zero-length data */
	if(!sz)
		return;

	hconfig.Function = CORDIC_FUNCTION_COSINE;
	HAL_CORDIC_Configure(&hcordic, &hconfig);

	/* start first calc */
	CORDIC->WDATA = *src++;
	CORDIC->WDATA = *src++;
	sz--;

	/* do all intermediate calcs */
	while(sz--)
	{
		CORDIC->WDATA = *src++;
		CORDIC->WDATA = *src++;
		*dst++ = CORDIC->RDATA;
		*dst++ = CORDIC->RDATA;
	}

	/* get last result */
	*dst++ = CORDIC->RDATA;
	*dst++ = CORDIC->RDATA;
}

/*
 * convert float rectangular to polar. input data is interleaved x,y
 * output data is interleaved phs,mag
 */
void cordic_r2p_float(float32_t *data, int32_t sz)
{
	float32_t *src = data, *dst = data;

	hconfig.Function = CORDIC_FUNCTION_PHASE;
	HAL_CORDIC_Configure(&hcordic, &hconfig);

	/* start first calc */
	CORDIC->WDATA = (NORM * *src++);
	CORDIC->WDATA = (NORM * *src++);
	sz--;

	/* do all intermediate calcs */
	while(sz--)
	{
		CORDIC->WDATA = *src++;
		CORDIC->WDATA = *src++;
		*dst++ = (PI * INV_NORM * (float32_t)CORDIC->RDATA);
		*dst++ = (INV_NORM * (float32_t)CORDIC->RDATA);
	}

	/* get last result */
	*dst++ = (PI * INV_NORM * (float32_t)CORDIC->RDATA);
	*dst++ = (INV_NORM * (float32_t)CORDIC->RDATA);
}

/*
 * convert integer polar to rectangular. data is interleaved mag,phs
 */
void cordic_p2r_float(float32_t *data, int32_t sz)
{
}
