/*
 * qspi.c - external QSPI PSRAM setup for H730 w/ OctoSPI port
 * 12-08-2020 E. Brombaugh
 */

#ifndef __qspi__
#define __qspi__

#include "stm32h7xx_hal.h"

#define QSPI_ADDR_BITS 23
#define QSPI_SIZE (1<<QSPI_ADDR_BITS)

void qspi_init(void);
void qspi_writebytes(uint32_t addr, uint8_t *data, uint32_t sz);
void qspi_readbytes(uint32_t addr, uint8_t *data, uint32_t sz);
void qspi_memmap(uint8_t enable);
uint8_t qspi_test();

#endif
