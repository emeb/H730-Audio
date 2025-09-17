/*
 * usart.c - uart diag support for H730 Audio
 * 12-04-2020 E. Brombaugh
 */

#ifndef __usart__
#define __usart__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

void setup_usart(void);
void usart_putc(void* p, char c);

#ifdef __cplusplus
}
#endif

#endif
