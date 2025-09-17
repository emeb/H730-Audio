/*
 * shared_spi.c - interface routines for shared SPI port.
 * 04-19-16 E. Brombaugh
 * 08-31-17 E. Brombaugh - updated for F303
 * 04-01-20 E. Brombaugh - updated for H750
 * 12-05-20 E. Brombaugh - updated for H730
 */

#include "shared_spi.h"

#define SPI_MOSI_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE()
#define SPI_MOSI_GPIO_PORT GPIOB
#define SPI_MOSI_PIN GPIO_PIN_15
#define SPI_MOSI_AF GPIO_AF5_SPI2

#define SPI_SCLK_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()
#define SPI_SCLK_GPIO_PORT GPIOA
#define SPI_SCLK_PIN GPIO_PIN_9
#define SPI_SCLK_AF GPIO_AF5_SPI2

#define SPI_CLK_ENABLE() __HAL_RCC_SPI2_CLK_ENABLE()
#define SPI_PORT SPI2

#define SPI_DMA_CLK_ENABLE() __HAL_RCC_DMA1_CLK_ENABLE();
#define SPI_DMA_CHANNEL DMA1_Stream0
#define SPI_DMA_REQUEST DMA_REQUEST_SPI2_TX
#define SPI_DMA_TCFLAG DMA_FLAG_TC0

/* uncomment this to use DMA */
//#define SHARED_SPI_USE_DMA

#ifdef SHARED_SPI_USE_DMA
/* DMA channel handle */
DMA_HandleTypeDef hdma_spi = {0};
#endif

/* SPI port handle */
SPI_HandleTypeDef SpiHandle;

/* ----------------------- Private functions ----------------------- */
/*
 * Read byte from SPI interface
 */
uint8_t Shared_SPI_ReadWriteByte(uint8_t tx)
{
	uint8_t rx;
	uint32_t SPI_CFG1 = SpiHandle.Instance->CFG1;
	
	/* slow down for the F042 */
	MODIFY_REG(SpiHandle.Instance->CFG1, SPI_CFG1_MBR, (3<<SPI_CFG1_MBR_Pos));
	
	/* Set the number of data at current transfer */
	MODIFY_REG(SpiHandle.Instance->CR2, SPI_CR2_TSIZE, 1);

	/* Enable SPI peripheral */
	__HAL_SPI_ENABLE(&SpiHandle);

	/* Master transfer start */
	SET_BIT(SpiHandle.Instance->CR1, SPI_CR1_CSTART);

	/* Wait until TXP flag is set to send data */
	while (!__HAL_SPI_GET_FLAG(&SpiHandle, SPI_FLAG_TXP));

	*((__IO uint8_t *)&SpiHandle.Instance->TXDR) = tx;

	/* Wait for Tx data to be sent */
	while (!__HAL_SPI_GET_FLAG(&SpiHandle, SPI_FLAG_EOT));
	
	/* wait for Rx data to arrive */
	while(!__HAL_SPI_GET_FLAG(&SpiHandle, SPI_FLAG_RXP));
	
	rx = *((__IO uint8_t *)&SpiHandle.Instance->RXDR);
	
	/* clean up */
	__HAL_SPI_CLEAR_EOTFLAG(&SpiHandle);
	__HAL_SPI_CLEAR_TXTFFLAG(&SpiHandle);

	/* Disable SPI peripheral */
	__HAL_SPI_DISABLE(&SpiHandle);
	
	/* restore speed */
	SpiHandle.Instance->CFG1 = SPI_CFG1;
	
	return rx;
}

/*
 * multi-byte write
 */
void Shared_SPI_WriteBytes(uint8_t *pData, uint16_t size)
{
	/* Set the number of data at current transfer */
	MODIFY_REG(SpiHandle.Instance->CR2, SPI_CR2_TSIZE, size);

	/* Enable SPI peripheral */
	__HAL_SPI_ENABLE(&SpiHandle);

	/* Master transfer start */
	SET_BIT(SpiHandle.Instance->CR1, SPI_CR1_CSTART);

	/* send the data */
	while(size--)
	{
		/* Wait until TXP flag is set to send data */
		while (!__HAL_SPI_GET_FLAG(&SpiHandle, SPI_FLAG_TXP));

		*((__IO uint8_t *)&SpiHandle.Instance->TXDR) = (*pData++);
	}

	/* Wait for Tx data to be sent */
	while (!__HAL_SPI_GET_FLAG(&SpiHandle, SPI_FLAG_EOT));

	/* clean up */
	__HAL_SPI_CLEAR_EOTFLAG(&SpiHandle);
	__HAL_SPI_CLEAR_TXTFFLAG(&SpiHandle);

	/* Disable SPI peripheral */
	__HAL_SPI_DISABLE(&SpiHandle);
}

/*
 * Write byte to SPI interface
 */
void Shared_SPI_WriteByte(uint8_t Data)
{
	Shared_SPI_WriteBytes(&Data, 1);
}

/*
 * Write word to SPI interface
 */
void Shared_SPI_WriteWord(uint16_t Data)
{
	Shared_SPI_WriteBytes((uint8_t *)&Data, 2);
}

/**
  * @brief  Custom - Transmit an amount of data in blocking mode
  * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @param  pData: pointer to data buffer
  * @param  Size: amount of data to be sent
  * @retval none
  */
void Shared_SPI_Blocking_PIO_WriteBytes(uint8_t *pData, uint32_t Size)
{
	Shared_SPI_WriteBytes(pData, Size);
}

/**
  * @brief  Custom - Transmit a number of fixed int values
  * @param  hspi: pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @param  Data: Integer data
  * @param  Size: amount of data to be sent
  * @retval none
  */
void Shared_SPI_Blocking_PIO_WriteWord(uint16_t Data, uint32_t Size)
{
	while(Size--)
	{
		Shared_SPI_WriteBytes((uint8_t *)&Data, 2);
	}
}

#ifdef SHARED_SPI_USE_DMA
/*
 * Setup DMA
 */
void Shared_SPI_InitDMA(void)
{
	// turn on DMA clock
	SPI_DMA_CLK_ENABLE();
	
    // Common
	hdma_spi.Instance                 = SPI_DMA_CHANNEL;
    hdma_spi.Init.Request             = SPI_DMA_REQUEST;
	hdma_spi.Init.Direction           = DMA_MEMORY_TO_PERIPH;
	hdma_spi.Init.PeriphInc           = DMA_PINC_DISABLE;
	hdma_spi.Init.MemInc              = DMA_MINC_ENABLE;
	hdma_spi.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	hdma_spi.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
	hdma_spi.Init.Mode                = DMA_NORMAL;
	hdma_spi.Init.Priority            = DMA_PRIORITY_LOW;
    	
    HAL_DMA_Init(&hdma_spi);

    /* Associate the initialized DMA handle to the the SPI handle */
    __HAL_LINKDMA(&SpiHandle, hdmatx, hdma_spi);
}

/*
 * Start DMA multi-write - used both in blocking and non-blocking
 */
void Shared_SPI_start_DMA_WriteBytes(uint8_t *buffer, int32_t count)
{
    /* Setup buffer loc / len */
    hdma_spi.Instance->CNDTR = (uint32_t)count;
    hdma_spi.Instance->CMAR = (uint32_t)buffer;
	hdma_spi.Instance->CPAR  = (uint32_t)&SpiHandle.Instance->DR;
 
    /* Enable SPI_DMA_TX */
	__HAL_DMA_ENABLE(&hdma_spi);
	
    /* Enable SPI DMA TX request */
    SpiHandle.Instance->CR2 |= SPI_CR2_TXDMAEN;
}

/*
 * Finish DMA multi-write - used both in blocking and non-blocking
 */
void Shared_SPI_end_DMA_WriteBytes(void)
{
    /* Wait until DMA count == 0 */
    while(hdma_spi.Instance->CNDTR != 0)
	{
	}
	
	/* wait for tx fifo to drain */
	while((SpiHandle.Instance->SR & SPI_FLAG_FTLVL) != SPI_FTLVL_EMPTY)
	{
	}

	/* wait for not busy */
	while(__HAL_SPI_GET_FLAG(&SpiHandle,SPI_FLAG_BSY) != RESET)
	{
	}

	/* drain rx fifo */
	while((SpiHandle.Instance->SR & SPI_FLAG_FRLVL) != SPI_FRLVL_EMPTY)
	{
		__IO uint8_t tmpreg = *((__IO uint8_t*)&SpiHandle.Instance->DR);
		UNUSED(tmpreg); /* To avoid GCC warning */
	}
	
	/* Clear OVERUN flag because received is not read */
	__HAL_SPI_CLEAR_OVRFLAG(&SpiHandle);

    /* DISABLE SPI DMA TX request */
    SpiHandle.Instance->CR2 &= ~SPI_CR2_TXDMAEN;
 
    /* DISABLE SPI_DMA_TX */
    __HAL_DMA_DISABLE(&hdma_spi);
	
    /* Clear DMA TransferComplete Flag */
	__HAL_DMA_CLEAR_FLAG(&hdma_spi, SPI_DMA_TCFLAG);
}
#endif

/*
 * Initialize SPI interface to LCD
 */
void Shared_SPI_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable MOSI pin Clock */
	SPI_MOSI_GPIO_CLK_ENABLE();
	
	/* Enable MOSI pin for AF output */
	GPIO_InitStructure.Pin =  SPI_MOSI_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStructure.Pull = GPIO_NOPULL ;
	GPIO_InitStructure.Alternate = SPI_MOSI_AF;
	HAL_GPIO_Init(SPI_MOSI_GPIO_PORT, &GPIO_InitStructure);
	
	/* Enable SCLK pin Clock */
	SPI_SCLK_GPIO_CLK_ENABLE();
	
	/* Enable SCLK pin for AF output */
	GPIO_InitStructure.Pin =  SPI_SCLK_PIN;
	GPIO_InitStructure.Alternate = SPI_SCLK_AF;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(SPI_SCLK_GPIO_PORT, &GPIO_InitStructure);
	
	/* Enable SPI Port Clock */
	SPI_CLK_ENABLE();
  
	/* Set up SPI port */
	SpiHandle.Instance = SPI_PORT;
	SpiHandle.Init.Mode = SPI_MODE_MASTER;
	SpiHandle.Init.Direction = SPI_DIRECTION_2LINES;
	SpiHandle.Init.DataSize = SPI_DATASIZE_8BIT;
	SpiHandle.Init.CLKPolarity = SPI_POLARITY_HIGH;
	SpiHandle.Init.CLKPhase = SPI_PHASE_1EDGE;
	SpiHandle.Init.NSS = SPI_NSS_HARD_OUTPUT;
	SpiHandle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
	SpiHandle.Init.FirstBit = SPI_FIRSTBIT_MSB;
	SpiHandle.Init.TIMode = SPI_TIMODE_DISABLE;
	SpiHandle.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	SpiHandle.Init.CRCPolynomial = 7;
	SpiHandle.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
	SpiHandle.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
	HAL_SPI_Init(&SpiHandle);
	
#ifdef SHARED_SPI_USE_DMA
	/* set up SPI DMA */
	Shared_SPI_InitDMA();
#endif
}

