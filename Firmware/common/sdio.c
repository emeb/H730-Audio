/*
 * sdio.c - H7 Audio SD card setup
 * 07-20-19 E. Brombaugh
 */

#include "sdio.h"
#include "printf.h"

#define SD_PWR_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()
#define SD_PWR_GPIO_PORT GPIOA
#define SD_PWR_PIN GPIO_PIN_10
#define SD_PWR_LOW()   HAL_GPIO_WritePin(SD_PWR_GPIO_PORT, SD_PWR_PIN, GPIO_PIN_RESET)
#define SD_PWR_HIGH()  HAL_GPIO_WritePin(SD_PWR_GPIO_PORT, SD_PWR_PIN, GPIO_PIN_SET)

#define SD_CD_GPIO_CLK_ENABLE() __HAL_RCC_GPIOD_CLK_ENABLE()
#define SD_CD_GPIO_PORT GPIOD
#define SD_CD_PIN GPIO_PIN_1

/* uncomment for connectivity test */
//#define CONN_TST

#ifdef CONN_TST
#define SIG 2
const char *sig_name[] =
{
	"PC8  -> SDMMC1_D0",
	"PC9  -> SDMMC1_D1",
	"PC10 -> SDMMC1_D2",
	"PC11 -> SDMMC1_D3",
	"PC12 -> SDMMC1_CK",
	"PD2  -> SDMMC1_CMD"
};

const GPIO_TypeDef *sig_port[] =
{
	GPIOC,
	GPIOC,
	GPIOC,
	GPIOC,
	GPIOC,
	GPIOD
};

const uint16_t sig_pin[] =
{
	GPIO_PIN_8,
	GPIO_PIN_9,
	GPIO_PIN_10,
	GPIO_PIN_11,
	GPIO_PIN_12,
	GPIO_PIN_2
};
#endif

SD_HandleTypeDef hsd1;

/**
  * @brief  Gets Time from RTC
  * @param  None
  * @retval Time in DWORD
  */
DWORD get_fattime(void)
{
  /* USER CODE BEGIN get_fattime */
  return 0;
  /* USER CODE END get_fattime */
}

/**
  * @brief  DeInitializes the SD card device.
  * @retval SD status
  */
uint8_t BSP_SD_DeInit(void)
{
  uint8_t sd_state = MSD_OK;

  /* HAL SD deinitialization */
  if(HAL_SD_DeInit(&hsd1) != HAL_OK)
  {
	printf("BSP_SD_DeInit: failed\n\r");
    sd_state = MSD_ERROR;
  }
  else
	printf("BSP_SD_DeInit: OK\n\r");

  __HAL_RCC_SDMMC1_CLK_DISABLE();

  return  sd_state;
}

/*
 * initialize SD card
 */
void sdio_init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* Peripheral clock enable */
	__HAL_RCC_SDMMC1_CLK_ENABLE();

	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();

	/* Enable power pin GPIO clock */
	SD_PWR_GPIO_CLK_ENABLE();
	SD_CD_GPIO_CLK_ENABLE();

#ifndef CONN_TST
	/* Power pin configuration */
	GPIO_InitStruct.Pin = SD_PWR_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(SD_PWR_GPIO_PORT, &GPIO_InitStruct);

    /* start up with power off */
    SD_PWR_LOW();

	/* Card Detect configuration */
	GPIO_InitStruct.Pin = SD_CD_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(SD_CD_GPIO_PORT, &GPIO_InitStruct);

	/**SDMMC1 GPIO Configuration
	PC8     ------> SDMMC1_D0
	PC9     ------> SDMMC1_D1
	PC10     ------> SDMMC1_D2
	PC11     ------> SDMMC1_D3
	PC12     ------> SDMMC1_CK
	PD2     ------> SDMMC1_CMD
	*/
	GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
						|GPIO_PIN_12;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	//GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	//GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC1;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_2;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	/* simply sets up the handle but doesn't actually init until FF insists */
	//HAL_SD_DeInit(&hsd1);	// for some reason this causes SAI ISR to hang
							// seems to be due to mmc power state change to off
#if 1
	hsd1.Instance = SDMMC1;
	hsd1.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
	hsd1.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
	hsd1.Init.BusWide = SDMMC_BUS_WIDE_4B;
	//hsd1.Init.BusWide = SDMMC_BUS_WIDE_1B;
	hsd1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
	//hsd1.Init.ClockDiv = 0;
	hsd1.Init.ClockDiv = 3;
	//hsd1.Init.TranceiverPresent = SDMMC_TRANSCEIVER_NOT_PRESENT;
#endif
#else
	GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
						|GPIO_PIN_12;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_2;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	printf("Testing SDIO connection %s...\n\r", sig_name[SIG]);
	while(1)
	{
		HAL_GPIO_WritePin(sig_port[SIG], sig_pin[SIG], GPIO_PIN_RESET);
		HAL_Delay(1);
		HAL_GPIO_WritePin(sig_port[SIG], sig_pin[SIG], GPIO_PIN_SET);
		HAL_Delay(1);
	}
#endif
}

/*
 * enable/disable SD card power
 */
void sdio_power(uint8_t state)
{
    /* set pin */
    if(state)
        SD_PWR_HIGH();
    else
        SD_PWR_LOW();

    /* wait 50ms to stabilize (Per Paul) */
    HAL_Delay(50);
}

/*
 * check SD CS line to see if it's pulled up
 */
uint8_t sdio_detect(void)
{
    uint8_t result = 0;

    /* get state of CD pin - pulled down -> card present */
    if(HAL_GPIO_ReadPin(SD_CD_GPIO_PORT, SD_CD_PIN) == GPIO_PIN_RESET)
    {
		result = 1;
	}

	return result;
}

/*
 * emergency halt SD ops in progress - used for power down alarm
 */
HAL_StatusTypeDef sdio_abort(void)
{
	return HAL_SD_Abort(&hsd1);
}

/*
 * simple test of SD card
 */
uint8_t sdio_test(void)
{
	FATFS Fatfs;			/* File system object for each logical drive */
	char SDPath[4];   		/* SD logical drive path */
	DIR Dir;				/* Directory object */
	FILINFO Finfo;
	FRESULT fres;
	uint8_t result = 0;
	uint8_t retSD;    		/* Return value for SD */

	HAL_Delay(100);

	retSD = FATFS_LinkDriver(&SD_Driver, SDPath);
	printf("sdio_test: FATFS_LinkDriver result = %d\n\r", retSD);

	fres = f_mount(&Fatfs, (TCHAR const*)SDPath, 0);
	printf("sdio_test: f_mount result = %d\n\r", fres);

	fres = f_opendir(&Dir, "");
	printf("sdio_test: f_opendir : result = %d\n\r", fres);

	if(fres == 0)
	{
		/* dump top-level directory */
		int32_t p1 = 0;
		uint32_t s1 = 0, s2 = 0;
		for(;;) {
			fres = f_readdir(&Dir, &Finfo);
#if 1
			if ((fres != FR_OK) || !Finfo.fname[0]) break;
#else
			if (fres != FR_OK)
			{
				printf("err - fres = %d\n\r", fres);
				break;
			}
			if (!Finfo.fname[0])
			{
				printf("err - fname[0] = 0\n\r");
				break;
			}
#endif
			if (Finfo.fattrib & AM_DIR) {
				s2++;
			} else {
				s1++; p1 += Finfo.fsize;
			}
#if 1
			printf("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  %s\n\r",
					(Finfo.fattrib & AM_DIR) ? 'D' : '-',
					(Finfo.fattrib & AM_RDO) ? 'R' : '-',
					(Finfo.fattrib & AM_HID) ? 'H' : '-',
					(Finfo.fattrib & AM_SYS) ? 'S' : '-',
					(Finfo.fattrib & AM_ARC) ? 'A' : '-',
					(Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15, Finfo.fdate & 31,
					(Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63,
					Finfo.fsize, &(Finfo.fname[0]));
#else
			printf("%s\n\r", &(Finfo.fname[0]));
#endif
		}
		printf("%4u File(s),%10lu bytes total\n%4u Dir(s)\n\r", s1, p1, s2);
		//if (f_getfree("\\", (DWORD*)&p1, &fs) == FR_OK)
		//	printf(", %10lu bytes free\n", p1 * fs->csize * 512);

		fres = f_closedir(&Dir);
		printf("sdio_test: f_closedir result = %d\n\r", fres);

		result = 1;
	}

	retSD = FATFS_UnLinkDriver(SDPath);
	printf("sdio_test: FATFS_UnLinkDriver result = %d\n\r", retSD);

	//BSP_SD_DeInit();

	return result;
}

/**
  * @brief  This function handles SD interrupt request.
  * @param  None
  * @retval None
  */
void SDMMC1_IRQHandler(void)
{
   HAL_SD_IRQHandler(&hsd1);
}
