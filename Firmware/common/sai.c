/*
 * sai.c - H7 SAI setup
 * 06-19-19 E. Brombaugh
 */

#include "sai.h"
#include "audio.h"
#include "cyclesleep.h"
#include "printf.h"

/* buffer size management */
volatile uint16_t sai_dummy_buffsz, sai_curr_buffsz, sai_next_buffsz;

/* SAI HAL handles */
SAI_HandleTypeDef hsai_tx0, hsai_rx0;
DMA_HandleTypeDef hdma_sai_tx0, hdma_sai_rx0;

/* Diagnostic flag */
#define ENABLE_SAIDIAG
#ifdef ENABLE_SAIDIAG
#define FLAG_0  GPIOB->BSRR=(1<<(7+16))
#define FLAG_1  GPIOB->BSRR=(1<<7)
#else
#define FLAG_0
#define FLAG_1
#endif


/* DMA buffers for 24-bit I2S */
__IO int32_t 	tx0_buffer[MAX_DMA_BUFFSZ+8] __attribute__ ((aligned (32))) __attribute__ ((section (".ram_d3_data"))),
				rx0_buffer[MAX_DMA_BUFFSZ+8] __attribute__ ((aligned (32))) __attribute__ ((section (".ram_d3_data")));

/*
 * Initializes IOs used by the SAI
 */
void SAI_GPIO_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Connect pins to SAI peripheral  */
	// PE2  - SAI1_MCLK_A
	// PE3  - SAI1_SD_B = main ADC data
	// PE4  - SAI1_FS_A
	// PE5  - SAI1_SCK_A
	// PE6  - SAI1_SD_A = main DAC data

	/* Enable SAI GPIO Clock */
	__HAL_RCC_GPIOE_CLK_ENABLE();

	GPIO_InitStructure.Pin = GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6;
	GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStructure.Speed = GPIO_SPEED_MEDIUM;	// needed for signal integrity over wires
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Alternate = GPIO_AF6_SAI1;
	HAL_GPIO_Init(GPIOE, &GPIO_InitStructure);

#ifdef ENABLE_SAIDIAG
	/* Enable GPIO B Clock */
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/* Enable PB7 for diagnostic output */
	GPIO_InitStructure.Pin =  GPIO_PIN_7;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
	GPIO_InitStructure.Pull = GPIO_NOPULL ;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
#endif
}

/* some defines to set common SAI comm params */
/* new settings for 24-bit */
#define MY_SAI_DATASIZE SAI_DATASIZE_32
#define MY_SAI_FRAMELENGTH 64
#define MY_SAI_ACTIVEFRAMELENGTH 32
#define MY_SAI_SLOTSIZE SAI_SLOTSIZE_32B

/*
 * Initializes the Audio Codec audio interface (I2S) as Master
 *
 * This function assumes that the I2S input clock (through PLL_R in
 * Devices RevA/Z and through dedicated PLLI2S_R in Devices RevB/Y)
 * is already configured and ready to be used.
 * AudioFreq: Audio frequency to be configured for the I2S peripheral.
 */
void SAI_AudioInterface_Master_Init(uint32_t AudioFreq)
{
	/* Enable the CODEC_SAI peripheral clock */
	__HAL_RCC_SAI1_CLK_ENABLE();

	/* Configure SAI1_Block_A for Master Out
	LSBFirst: Disabled
	DataSize: 16 */
	__HAL_SAI_RESET_HANDLE_STATE(&hsai_tx0);
	hsai_tx0.Instance = SAI1_Block_A;
	hsai_tx0.Init.AudioFrequency = AudioFreq;
	hsai_tx0.Init.AudioMode = SAI_MODEMASTER_TX;
	hsai_tx0.Init.NoDivider = SAI_MASTERDIVIDER_ENABLED;
	hsai_tx0.Init.Protocol = SAI_FREE_PROTOCOL;
	hsai_tx0.Init.DataSize = MY_SAI_DATASIZE;
	hsai_tx0.Init.FirstBit = SAI_FIRSTBIT_MSB;
	hsai_tx0.Init.ClockStrobing = SAI_CLOCKSTROBING_RISINGEDGE;
	hsai_tx0.Init.Synchro = SAI_ASYNCHRONOUS;
	hsai_tx0.Init.SynchroExt = SAI_SYNCEXT_OUTBLOCKA_ENABLE;
	hsai_tx0.Init.OutputDrive = SAI_OUTPUTDRIVE_ENABLED;
	hsai_tx0.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_1QF;

	/* Configure SAI1_Block_A Frame
	Frame Length: 32
	Frame active Length: 16
	FS Definition: Start frame + Channel Side identification
	FS Polarity: FS active Low
	FS Offset: FS asserted one bit before the first bit of slot 0 */
	hsai_tx0.FrameInit.FrameLength = MY_SAI_FRAMELENGTH;
	hsai_tx0.FrameInit.ActiveFrameLength = MY_SAI_ACTIVEFRAMELENGTH;
	hsai_tx0.FrameInit.FSDefinition = SAI_FS_CHANNEL_IDENTIFICATION;
	hsai_tx0.FrameInit.FSPolarity = SAI_FS_ACTIVE_LOW;
	hsai_tx0.FrameInit.FSOffset = SAI_FS_BEFOREFIRSTBIT;

	/* Configure SAI Block_x Slot
	Slot First Bit Offset: 0
	Slot Size  : 16
	Slot Number: 2
	Slot Active: All slot actives */
	hsai_tx0.SlotInit.FirstBitOffset = 0;
	hsai_tx0.SlotInit.SlotSize = MY_SAI_SLOTSIZE;
	hsai_tx0.SlotInit.SlotNumber = 2;
	hsai_tx0.SlotInit.SlotActive = SAI_SLOTACTIVE_ALL;

	HAL_SAI_DeInit(&hsai_tx0);
	if(HAL_SAI_Init(&hsai_tx0) != HAL_OK)
		printf("SAI1_A init failed\n\r");

	/* Configure SAI1_Block_B for Master Out
	LSBFirst: Disabled
	DataSize: 16 */
	__HAL_SAI_RESET_HANDLE_STATE(&hsai_rx0);
	hsai_rx0.Instance = SAI1_Block_B;
	hsai_rx0.Init.AudioFrequency = AudioFreq;
	hsai_rx0.Init.AudioMode = SAI_MODESLAVE_RX;
	hsai_rx0.Init.NoDivider = SAI_MASTERDIVIDER_ENABLED;
	hsai_rx0.Init.Protocol = SAI_FREE_PROTOCOL;
	hsai_rx0.Init.DataSize = MY_SAI_DATASIZE;
	hsai_rx0.Init.FirstBit = SAI_FIRSTBIT_MSB;
	hsai_rx0.Init.ClockStrobing = SAI_CLOCKSTROBING_RISINGEDGE;
	hsai_rx0.Init.Synchro = SAI_SYNCHRONOUS;
	hsai_rx0.Init.SynchroExt = SAI_SYNCEXT_OUTBLOCKA_ENABLE;
	hsai_rx0.Init.OutputDrive = SAI_OUTPUTDRIVE_ENABLED;
	hsai_rx0.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_1QF;

	/* Configure SAI1_Block_B Frame
	Frame Length: 32
	Frame active Length: 16
	FS Definition: Start frame + Channel Side identification
	FS Polarity: FS active Low
	FS Offset: FS asserted one bit before the first bit of slot 0 */
	hsai_rx0.FrameInit.FrameLength = MY_SAI_FRAMELENGTH;
	hsai_rx0.FrameInit.ActiveFrameLength = MY_SAI_ACTIVEFRAMELENGTH;
	hsai_rx0.FrameInit.FSDefinition = SAI_FS_CHANNEL_IDENTIFICATION;
	hsai_rx0.FrameInit.FSPolarity = SAI_FS_ACTIVE_LOW;
	hsai_rx0.FrameInit.FSOffset = SAI_FS_BEFOREFIRSTBIT;

	/* Configure SAI1 Block_B Slot
	Slot First Bit Offset: 0
	Slot Size  : 16
	Slot Number: 2
	Slot Active: All slot actives */
	hsai_rx0.SlotInit.FirstBitOffset = 0;
	hsai_rx0.SlotInit.SlotSize = MY_SAI_SLOTSIZE;
	hsai_rx0.SlotInit.SlotNumber = 2;
	hsai_rx0.SlotInit.SlotActive = SAI_SLOTACTIVE_ALL;

	HAL_SAI_DeInit(&hsai_rx0);
	if(HAL_SAI_Init(&hsai_rx0) != HAL_OK)
		printf("SAI1_B init failed\n\r");
}

/* some defines to set common SAI DMA params */
/* new settings for 24-bit */
#define MY_SAI_DMA_PDATASIZE DMA_PDATAALIGN_WORD
#define MY_SAI_DMA_MDATASIZE DMA_MDATAALIGN_WORD

/*
 * Init I2S channel for DMA with IRQ per block
 */
void sai_start(void)
{
	DMA_Stream_TypeDef *stream_tx0, *stream_rx0;

	/* make sure SAI is disabled */
	__HAL_SAI_DISABLE(&hsai_tx0);
	__HAL_SAI_DISABLE(&hsai_rx0);

	/* Enable the DMA clock */
	__HAL_RCC_DMA2_CLK_ENABLE();

	/* Configure the TX DMA Request for SAI1_A */
	hdma_sai_tx0.Instance                 = DMA2_Stream1;
    hdma_sai_tx0.Init.Request             = DMA_REQUEST_SAI1_A;
	hdma_sai_tx0.Init.Direction           = DMA_MEMORY_TO_PERIPH;
	hdma_sai_tx0.Init.PeriphInc           = DMA_PINC_DISABLE;
	hdma_sai_tx0.Init.MemInc              = DMA_MINC_ENABLE;
	hdma_sai_tx0.Init.PeriphDataAlignment = MY_SAI_DMA_PDATASIZE;
	hdma_sai_tx0.Init.MemDataAlignment    = MY_SAI_DMA_MDATASIZE;
	hdma_sai_tx0.Init.Mode                = DMA_CIRCULAR;
	hdma_sai_tx0.Init.Priority            = DMA_PRIORITY_HIGH;
	hdma_sai_tx0.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
	hdma_sai_tx0.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
	hdma_sai_tx0.Init.MemBurst            = DMA_MBURST_SINGLE;
	hdma_sai_tx0.Init.PeriphBurst         = DMA_PBURST_SINGLE;

    /* Configure the RX DMA Request for SAI1_B */
    hdma_sai_rx0.Instance                 = DMA2_Stream2;
    hdma_sai_rx0.Init.Request             = DMA_REQUEST_SAI1_B;
    hdma_sai_rx0.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_sai_rx0.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_sai_rx0.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_sai_rx0.Init.PeriphDataAlignment = MY_SAI_DMA_PDATASIZE;
    hdma_sai_rx0.Init.MemDataAlignment    = MY_SAI_DMA_MDATASIZE;
    hdma_sai_rx0.Init.Mode                = DMA_CIRCULAR;
    hdma_sai_rx0.Init.Priority            = DMA_PRIORITY_HIGH;
    hdma_sai_rx0.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    hdma_sai_rx0.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    hdma_sai_rx0.Init.MemBurst            = DMA_MBURST_SINGLE;
    hdma_sai_rx0.Init.PeriphBurst         = DMA_MBURST_SINGLE;

	/* hook up DMA to SAI */
    __HAL_LINKDMA(&hsai_tx0, hdmatx, hdma_sai_tx0);
    __HAL_LINKDMA(&hsai_rx0, hdmarx, hdma_sai_rx0);

	/* Deinitialize the Stream for new transfer */
	HAL_DMA_DeInit(&hdma_sai_tx0);
	HAL_DMA_DeInit(&hdma_sai_rx0);

	/* Configure the DMA Stream */
	HAL_DMA_Init(&hdma_sai_tx0);
	HAL_DMA_Init(&hdma_sai_rx0);

	/* Configure the source, destination address and the data length */
	stream_tx0 = hdma_sai_tx0.Instance;
	stream_rx0 = hdma_sai_rx0.Instance;

	/* Clear DBM bit */
	stream_tx0->CR &= (uint32_t)(~DMA_SxCR_DBM);
	stream_rx0->CR &= (uint32_t)(~DMA_SxCR_DBM);

	/* Configure DMA Stream data length */
	stream_tx0->NDTR = (uint32_t)sai_curr_buffsz;
	stream_rx0->NDTR = (uint32_t)sai_curr_buffsz;

	/* Configure DMA Stream peripheral address */
	stream_tx0->PAR = (uint32_t)&hsai_tx0.Instance->DR;
	stream_rx0->PAR = (uint32_t)&hsai_rx0.Instance->DR;

	/* Configure DMA Stream memory address */
	stream_tx0->M0AR = (uint32_t)&tx0_buffer;
	stream_rx0->M0AR = (uint32_t)&rx0_buffer;

	/* Enable the RX half-transfer and transfer complete interrupts */
	__HAL_DMA_ENABLE_IT(&hdma_sai_rx0, DMA_IT_TC | DMA_IT_HT);

	/* SAI RX DMA IRQ Channel configuration */
	HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 1, 1);
	HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);

	/* Enable SAI DMA requests */
    hsai_tx0.Instance->CR1 |= SAI_xCR1_DMAEN;
    hsai_rx0.Instance->CR1 |= SAI_xCR1_DMAEN;

	/* Enable DMA */
	__HAL_DMA_ENABLE(&hdma_sai_tx0);
	__HAL_DMA_ENABLE(&hdma_sai_rx0);

	/* Enable slave, then master to keep sync */
	__HAL_SAI_ENABLE(&hsai_tx0);
	__HAL_SAI_ENABLE(&hsai_rx0);
}

/*
 * halt the sai background processes
 */
void sai_stop(void)
{
	__HAL_SAI_DISABLE(&hsai_tx0);
	__HAL_SAI_DISABLE(&hsai_rx0);
	HAL_SAI_DMAStop(&hsai_tx0);
	HAL_SAI_DMAStop(&hsai_rx0);
	HAL_NVIC_DisableIRQ(DMA2_Stream2_IRQn);
}

/*
 * Main entry point to set up SAI
 */
void sai_init(uint16_t bufsz)
{
	uint16_t i;

	/* fill DMA buffers w/ known data */
	for(i=0;i<MAX_DMA_BUFFSZ+8;i++)
	{
		tx0_buffer[i] = 0;
		rx0_buffer[i] = 0;
	}

	/* initial buffer size */
	sai_curr_buffsz = bufsz;
	sai_next_buffsz = sai_curr_buffsz;

	/* start up */
	SAI_GPIO_Config();
	SAI_AudioInterface_Master_Init(FSAMPLE);
	sai_start();
}

/*
 * return sample rate
 */
uint32_t sai_get_samplerate(void)
{
	uint32_t freq;

	freq = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_SAI1);
	freq /= (SAI1_Block_A->CR1 & SAI_xCR1_MCKDIV_Msk)>>SAI_xCR1_MCKDIV_Pos;

	if(!(SAI1_Block_A->CR1 & SAI_xCR1_NODIV_Msk))
	{
		/* MCLK enabled */
		if(SAI1_Block_A->CR1 & SAI_xCR1_OSR_Msk)
		{
			freq /= 512;
		}
		else
		{
			freq /= 256;
		}
	}

	return freq;
}

/*
 * return current bufsz
 */
uint16_t sai_get_bufsz(void)
{
	return sai_curr_buffsz;
}

/*
 * set desired bufsz
 */
uint8_t sai_set_bufsz(uint16_t bufsz)
{
	printf("sai_set_bufsz: %d requested\n\r", bufsz);
	if((bufsz>MAX_DMA_BUFFSZ) || (bufsz&1) || (bufsz == 0))
		return 1;
	else
	{
		sai_next_buffsz = bufsz;
		printf("sai_set_bufsz: sai_next_buffsz = %d\n\r", sai_next_buffsz);
		return 0;
	}
}

/*
 * This function handles SAI RX DMA block interrupt for 24-bit data.
 */
void DMA2_Stream2_IRQHandler(void)  __attribute__ ((section (".itcm_code")));
void DMA2_Stream2_IRQHandler(void)
{
	int32_t *dst0, *src0;
	DMA_Stream_TypeDef *dma_stream;

	/* raise activity flag */
	FLAG_1;

	/* start audio loading calc */
	start_meas();

	/* Transfer complete interrupt */
	if (__HAL_DMA_GET_FLAG(&hdma_sai_rx0, DMA_FLAG_TCIF2_6) != RESET)
	{
		/* Clear the Interrupt flag */
		__HAL_DMA_CLEAR_FLAG(&hdma_sai_rx0, DMA_FLAG_TCIF2_6);

		/* check if buffer size update required */
		if(sai_next_buffsz != sai_curr_buffsz)
		{
			/* change buffer size */
			sai_curr_buffsz = sai_next_buffsz;

			/* Disable DMA */
			__HAL_DMA_DISABLE(&hdma_sai_tx0);
			__HAL_DMA_DISABLE(&hdma_sai_rx0);

			/* Update buffer size regs */
			dma_stream = hdma_sai_tx0.Instance; dma_stream->NDTR = (uint32_t)sai_curr_buffsz;
			dma_stream = hdma_sai_rx0.Instance; dma_stream->NDTR = (uint32_t)sai_curr_buffsz;

			/* Re-enable DMA */
			__HAL_DMA_ENABLE(&hdma_sai_tx0);
			__HAL_DMA_ENABLE(&hdma_sai_rx0);
		}

		/* Point to 2nd half of buffers */
		src0 = (int32_t *)(rx0_buffer) + sai_curr_buffsz/2;
		dst0 = (int32_t *)(tx0_buffer) + sai_curr_buffsz/2;

		/* Process the buffer */
		Audio_Proc(dst0, src0, sai_curr_buffsz/2);
	}

	/* Half Transfer complete interrupt */
	if (__HAL_DMA_GET_FLAG(&hdma_sai_rx0, DMA_FLAG_HTIF2_6) != RESET)
	{
		/* Clear the Interrupt flag */
		__HAL_DMA_CLEAR_FLAG(&hdma_sai_rx0, DMA_FLAG_HTIF2_6);

		/* Point to 1st half of buffers */
		src0 = (int32_t *)(rx0_buffer);
		dst0 = (int32_t *)(tx0_buffer);

		/* Process the buffer */
		Audio_Proc(dst0, src0, sai_curr_buffsz/2);
	}

	/* finish audio loading calc */
	end_meas();

	/* drop activity flag */
	FLAG_0;
}
