/*
 * qspi.c - external QSPI PSRAM setup for H730 w/ OctoSPI port
 * 12-08-2020 E. Brombaugh
 */

#include "qspi.h"
#include "printf.h"

/* uncomment this to run the memory in QUAD mode */
#define QUAD_MODE

OSPI_HandleTypeDef hospi1;
OSPI_RegularCmdTypeDef sCommand;
OSPI_MemoryMappedTypeDef sMemMappedCfg;

static void Error_Handler(void)
{
  while(1)
  {
  }
}

/*
 * initialize qspi for memory mapped access
 */
void qspi_init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	OSPIM_CfgTypeDef sOspiManagerCfg = {0};

	/* set up GPIO */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    /**OCTOSPI1 GPIO Configuration
    PB2     ------> OCTOSPIM_P1_CLK
    PE11     ------> OCTOSPIM_P1_NCS
    PB13     ------> OCTOSPIM_P1_IO2
    PD11     ------> OCTOSPIM_P1_IO0
    PD12     ------> OCTOSPIM_P1_IO1
    PD13     ------> OCTOSPIM_P1_IO3
    */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_OCTOSPIM_P1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_OCTOSPIM_P1;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_OCTOSPIM_P1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_OCTOSPIM_P1;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	/* turn on OctoSPI */
    __HAL_RCC_OSPI1_CLK_ENABLE();

	/* Reset the OctoSPI memory interface */
	__HAL_RCC_OSPI1_FORCE_RESET();
	__HAL_RCC_OSPI1_RELEASE_RESET();

	/* OCTOSPI1 parameter configuration*/
	hospi1.Instance = OCTOSPI1;
	hospi1.Init.FifoThreshold = 4;
	hospi1.Init.DualQuad = HAL_OSPI_DUALQUAD_DISABLE;
	hospi1.Init.MemoryType = HAL_OSPI_MEMTYPE_MICRON;
	hospi1.Init.DeviceSize = QSPI_ADDR_BITS;
	hospi1.Init.ChipSelectHighTime = 1;
	hospi1.Init.FreeRunningClock = HAL_OSPI_FREERUNCLK_DISABLE;
	hospi1.Init.ClockMode = HAL_OSPI_CLOCK_MODE_0;
	hospi1.Init.WrapSize = HAL_OSPI_WRAP_NOT_SUPPORTED;
	hospi1.Init.ClockPrescaler = 3;
	hospi1.Init.SampleShifting = HAL_OSPI_SAMPLE_SHIFTING_NONE;
	hospi1.Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_DISABLE;
	hospi1.Init.ChipSelectBoundary = 1;
	hospi1.Init.ClkChipSelectHighTime = 0;
	hospi1.Init.DelayBlockBypass = HAL_OSPI_DELAY_BLOCK_BYPASSED;
	hospi1.Init.MaxTran = 0;
	hospi1.Init.Refresh = 0;
	if (HAL_OSPI_Init(&hospi1) != HAL_OK)
	{
		Error_Handler();
	}
	sOspiManagerCfg.ClkPort = 1;
	sOspiManagerCfg.NCSPort = 1;
	sOspiManagerCfg.IOLowPort = HAL_OSPIM_IOPORT_1_LOW;
	if (HAL_OSPIM_Config(&hospi1, &sOspiManagerCfg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		Error_Handler();
	}

	/* reset QSPI memory */
	sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
	sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_4_LINES;
	sCommand.Instruction        = 0xf5;	// exit quad mode
	sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
	sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
	sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
	sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode           = HAL_OSPI_DATA_NONE;
	sCommand.DummyCycles        = 0;
	sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
	sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
	sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
	if (HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		Error_Handler();
	}

	sCommand.InstructionMode   = HAL_OSPI_INSTRUCTION_1_LINE;
	sCommand.Instruction       = 0x66;	// reset enable
	if (HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		Error_Handler();
	}

	sCommand.Instruction       = 0x99;	// reset
	if (HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		Error_Handler();
	}

#ifdef QUAD_MODE
	/* Put it in QSPI mode */
	sCommand.Instruction       = 0x35;	// enter quad mode
	if (HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
			Error_Handler();
	}

	sCommand.InstructionMode	= HAL_OSPI_INSTRUCTION_4_LINES;	// qspi mode uses 4 line instr
#endif

	/* prep for subsequent operations */
	sCommand.AddressSize		= HAL_OSPI_ADDRESS_24_BITS;
	sCommand.AddressMode		= HAL_OSPI_ADDRESS_4_LINES;
	sCommand.DataMode			= HAL_OSPI_DATA_4_LINES;
}

/* original pure HAL with error checking */
/*
 * writing to QSPI must be handled in indirect mode
 */
void qspi_writebytes(uint32_t addr, uint8_t *data, uint32_t sz)
{
	HAL_StatusTypeDef stat;

	sCommand.Address			= addr;
	sCommand.Instruction 		= 0x38;		// SPI QUAD WRITE
	sCommand.DummyCycles		= 0;
	sCommand.NbData				= sz;

	if ((stat = HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE)) != HAL_OK)
	{
		printf("qspi_writebytes: HAL_OSPI_Command status = 0x%08X\n\r", stat);
		Error_Handler();
	}

	if ((stat = HAL_OSPI_Transmit(&hospi1, data, HAL_OSPI_TIMEOUT_DEFAULT_VALUE)) != HAL_OK)
	{
		printf("qspi_writebytes: HAL_OSPI_Transmit status = 0x%08X\n\r", stat);
		Error_Handler();
	}
}

/*
 * reading QSPI in batch mode
 */
void qspi_readbytes(uint32_t addr, uint8_t *data, uint32_t sz)
{
	HAL_StatusTypeDef stat;

	sCommand.Address			= addr;
	sCommand.Instruction 		= 0xEB;		// SPI QUAD READ
	sCommand.DummyCycles		= 6;
	sCommand.NbData				= sz;

	if ((stat = HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE)) != HAL_OK)
	{
		printf("qspi_readbytes: HAL_OSPI_Command status = 0x%08X\n\r", stat);
		Error_Handler();
	}

	if ((stat = HAL_OSPI_Receive(&hospi1, data, HAL_OSPI_TIMEOUT_DEFAULT_VALUE)) != HAL_OK)
	{
		printf("qspi_readbytes: HAL_OSPI_Receive status = 0x%08X\n\r", stat);
		Error_Handler();
	}
}

/*
 * put QSPI into memory mapped read mode
 */
void qspi_memmap(uint8_t enable)
{
	if((hospi1.State != HAL_OSPI_STATE_BUSY_MEM_MAPPED) && enable)
	{
		/* turn on mem mapped mode */
		sCommand.Instruction 		= 0xEB;		// SPI QUAD READ
		sCommand.DummyCycles		= 6;

		if (HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		{
			Error_Handler();
		}

		sMemMappedCfg.TimeOutActivation = HAL_OSPI_TIMEOUT_COUNTER_DISABLE;

		if (HAL_OSPI_MemoryMapped(&hospi1, &sMemMappedCfg) != HAL_OK)
		{
			Error_Handler();
		}
	}
	else if((hospi1.State == HAL_OSPI_STATE_BUSY_MEM_MAPPED) && !enable)
	{
		/* turn off mem mapped mode */
		uint32_t Tickstart = HAL_GetTick();

		/* Update QSPI state */
		hospi1.State = HAL_OSPI_STATE_ABORT;

		/* Configure QSPI: CR register with Abort request */
		SET_BIT(hospi1.Instance->CR, OCTOSPI_CR_ABORT);

		/* wait for not busy */
		while(READ_BIT(hospi1.Instance->SR, OCTOSPI_SR_BUSY))
		{
			if((HAL_GetTick() - Tickstart) > HAL_OSPI_TIMEOUT_DEFAULT_VALUE)
			{
				Error_Handler();
			}
		}

		/* disable mem map mode */
		MODIFY_REG(hospi1.Instance->CR, OCTOSPI_CR_FMODE, 0);

		/* Update QSPI state */
		hospi1.State = HAL_OSPI_STATE_READY;
	}
}

/*
 * tests for QSPI
 */

//#define QSPI_TEST_LEN QSPI_SIZE/4
//#define QSPI_TEST_LEN (1<<16)
#define QSPI_TEST_LEN (1<<4)

/* PRN generator info */
#define NOISE_POLY_TAP0 31
#define NOISE_POLY_TAP1 21
#define NOISE_POLY_TAP2 1
#define NOISE_POLY_TAP3 0
uint32_t prn_lfsr;

/**
  * @brief  PRN into destination
  * @param  seed - seed for noise generator
  * @retval none
  */
void prn_seed(uint32_t seed)
{
	prn_lfsr = seed;
}

/**
  * @brief  32-bit PRN
  * @retval 32-bit unsigned result
  */
uint8_t prn_gen(void)
{
	uint8_t bit;
	uint8_t new_data;

	/* generate 32 new bits */
	for(bit=0;bit<8;bit++)
	{
		new_data = ((prn_lfsr>>NOISE_POLY_TAP0) ^
					(prn_lfsr>>NOISE_POLY_TAP1) ^
					(prn_lfsr>>NOISE_POLY_TAP2) ^
					(prn_lfsr>>NOISE_POLY_TAP3));
		prn_lfsr = (prn_lfsr<<1) | (new_data&1);
	}

	return prn_lfsr;
}

/*
 * test various QSPI access methods
 */
uint8_t qspi_test(void)
{
	uint8_t i, txbuffer[QSPI_TEST_LEN], rxbuffer[QSPI_TEST_LEN], err, retval = 0, *membyte;

	printf("Writing %d count\n\r", QSPI_TEST_LEN);
	for(i=0;i<QSPI_TEST_LEN;i++)
		txbuffer[i] = i;

#if 0
	// infinite writes for testing
	while(1)
#endif
	qspi_writebytes(0, txbuffer, QSPI_TEST_LEN);

	printf("Reading %d count\n\r", QSPI_TEST_LEN);
	qspi_readbytes(0, rxbuffer, QSPI_TEST_LEN);

	err = 0;
	for(i=0;i<QSPI_TEST_LEN;i++)
	{
		if(txbuffer[i] != rxbuffer[i])
		{
			err++;
			printf("%1d : %02X -> %02X\n\r", i, txbuffer[i], rxbuffer[i]);
		}
	}
	if(err==0)
		printf("Read %d count passed.\n\r", QSPI_TEST_LEN);
	else
	{
		retval++;
		printf("Read %d count failed.\n\r", QSPI_TEST_LEN);
	}

	qspi_memmap(1);
	printf("Mem map reading %d count\n\r", QSPI_TEST_LEN);
	err = 0;
	membyte = (uint8_t *)OCTOSPI1_BASE;
	for(i=0;i<QSPI_TEST_LEN;i++)
	{
		if(txbuffer[i] != membyte[i])
		{
			err++;
			printf("%1d : %02X -> %02X\n\r", i, txbuffer[i], membyte[i]);
		}
	}
	qspi_memmap(0);
	if(err==0)
		printf("Mem map read %d count passed.\n\r", QSPI_TEST_LEN);
	else
	{
		retval++;
		printf("Mem map read %d count failed.\n\r", QSPI_TEST_LEN);
	}

	printf("Writing %d prn\n\r", QSPI_TEST_LEN);
	prn_seed(0x12345678);
	for(i=0;i<QSPI_TEST_LEN;i++)
		txbuffer[i] = prn_gen();
	qspi_writebytes(0, txbuffer, QSPI_TEST_LEN);

	printf("Reading %d prn\n\r", QSPI_TEST_LEN);
	qspi_readbytes(0, rxbuffer, QSPI_TEST_LEN);

	err = 0;
	prn_seed(0x12345678);
	for(i=0;i<QSPI_TEST_LEN;i++)
	{
		if(txbuffer[i] != rxbuffer[i])
		{
			printf("%1d : %02X -> %02X\n\r", i, txbuffer[i], rxbuffer[i]);
			err++;
		}
	}
	if(err==0)
		printf("Read %d prn passed.\n\r", QSPI_TEST_LEN);
	else
	{
		retval++;
		printf("Read %d prn failed.\n\r", QSPI_TEST_LEN);
	}

	qspi_memmap(1);
	printf("Mem map reading %d prn\n\r", QSPI_TEST_LEN);
	err = 0;
	membyte = (uint8_t *)OCTOSPI1_BASE;
	SCB_InvalidateDCache_by_Addr((uint32_t *)membyte, QSPI_TEST_LEN);
	for(i=0;i<QSPI_TEST_LEN;i++)
	{
		if(txbuffer[i] != membyte[i])
		{
			err++;
			printf("%1d : %02X -> %02X\n\r", i, txbuffer[i], membyte[i]);
		}
	}
	qspi_memmap(0);
	if(err==0)
		printf("Mem map read %d prn passed.\n\r", QSPI_TEST_LEN);
	else
	{
		retval++;
		printf("Mem map read %d prn failed.\n\r", QSPI_TEST_LEN);
	}

	return 0;
}
