/*
 * audio.c - H730_audio Audio processing routines for demo
 * 12-06-20 E. Brombaugh
 */

#ifndef __audio__
#define __audio__

#include "main.h"

void Audio_Init(void);
void Audio_Proc(int32_t *dst, int32_t *src, uint16_t sz);

#endif
