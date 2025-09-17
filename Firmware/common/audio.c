/*
 * audio.c - H730_audio Audio processing routines for demo
 * 12-06-20 E. Brombaugh
 */

#include <stdlib.h>
#include "audio.h"
#include "sai.h"

/* stereo input/output mix buffers */
int16_t sxin[MAX_STEREO_BUFSZ];
int16_t raw_out[MAX_STEREO_BUFSZ];

/*
 * initialize audio processing
 */
void Audio_Init(void)
{
}

/*
 * explore 32-bit mixing - 32x12+32x12->32
 * makes proper use of smlal
 */
int32_t mix32(int32_t in1, int32_t a1, int32_t in2, int32_t a2)
{
	int64_t mix_tmp ;
	int32_t out;

	mix_tmp = (int64_t)in1 * (int64_t)a1;
	mix_tmp += (int64_t)in2 * (int64_t)a2;
	out = mix_tmp >> 20;
	return __SSAT(out,24)<<8;
}

/*
 * This function is called by the RX DMA IRQ to process audio data.
 */
void Audio_Proc(int32_t *dst, int32_t *src, uint16_t sz) __attribute__ ((section (".itcm_code")));
void Audio_Proc(int32_t *dst, int32_t *src, uint16_t sz)
{
	uint16_t i;
	uint8_t clip = 0;
	int16_t mix;
	uint32_t * inval_addr = (uint32_t *)((uint32_t)src & ~0x1f);
	uint32_t * clean_addr = (uint32_t *)((uint32_t)dst & ~0x1f);

	/* invalidate Dcache of sources - no safety overlap so sz >= 8 */
	SCB_InvalidateDCache_by_Addr(inval_addr, 4*sz);

	/* convert 24(left justified in 32)->16 and compute signal levels */
	for(i=0;i<sz;i++)
	{
		/* cut input down to 16-bits */
		sxin[i] = src[i]>>16;

		/* check for clipping */
		if(sxin[i] > 32700)
			clip++;
	}

	/* just pass thru for now */
#if 1
	memcpy(raw_out, sxin, sz*sizeof(int16_t));
#else
	for(i=0;i<sz;i++)
		raw_out[i] = sxin[i];
#endif

	/* wet/dry mix and convert back to 24(left justified in 32) */
	mix = 4095;
	for(i=0;i<sz;i++)
	{
		/* check for clipping */
		if(raw_out[i] > 32700)
			clip++;

		/* wet/dry mix */
		dst[i] = mix32(raw_out[i]<<16, mix, src[i], 4095-mix);
	}

	/* flush DCache to destination - no safety overlap so sz >= 8 */
	SCB_CleanDCache_by_Addr(clean_addr, 4*sz);
}
