/*
 * main.c - H730 Audio
 * 12-04-20 E. Brombaugh
 ******************************************************************************
 * Changelog
 *
 * date: 2020-12-04 V0.0
 * Initial creation
 *
 */

#include "main.h"
#include "usart.h"
#include "printf.h"
#include "led.h"
#include "systick.h"
#include "st7789.h"
#include "gfx.h"
#include "adc.h"
#include "shared_i2c.h"
#include "eeprom.h"
#include "audio.h"
#include "codec.h"
#include "sai.h"
#include "cyclesleep.h"
#include "qspi.h"
#include "sdio.h"

/* build version in simple format */
const char *fwVersionStr = "V0.0";

/* build time */
const char *bdate = __DATE__;
const char *btime = __TIME__;

/**
* @brief  CPU L1-Cache enable.
* @param  None
* @retval None
*/
static void CPU_CACHE_Enable(void)
{
  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  while(1)
  {
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
  /** Macro to configure the PLL clock source
  */
  __HAL_RCC_PLL_PLLSOURCE_CONFIG(RCC_PLLSOURCE_HSE);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 275;
  RCC_OscInitStruct.PLL.PLLP = 1;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_1;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_DFSDM1
                              |RCC_PERIPHCLK_SPI2|RCC_PERIPHCLK_SAI1
                              |RCC_PERIPHCLK_SDMMC|RCC_PERIPHCLK_I2C2
                              |RCC_PERIPHCLK_ADC|RCC_PERIPHCLK_I2C1
                              |RCC_PERIPHCLK_USB|RCC_PERIPHCLK_OSPI;
  PeriphClkInitStruct.PLL2.PLL2M = 4;
  PeriphClkInitStruct.PLL2.PLL2N = 196;
  PeriphClkInitStruct.PLL2.PLL2P = 4;
  PeriphClkInitStruct.PLL2.PLL2Q = 2;
  PeriphClkInitStruct.PLL2.PLL2R = 2;
  PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_1;
  PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
  PeriphClkInitStruct.PLL2.PLL2FRACN = 4981;
  PeriphClkInitStruct.PLL3.PLL3M = 4;
  PeriphClkInitStruct.PLL3.PLL3N = 96;
  PeriphClkInitStruct.PLL3.PLL3P = 4;
  PeriphClkInitStruct.PLL3.PLL3Q = 4;
  PeriphClkInitStruct.PLL3.PLL3R = 2;
  PeriphClkInitStruct.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_1;
  PeriphClkInitStruct.PLL3.PLL3VCOSEL = RCC_PLL3VCOWIDE;
  PeriphClkInitStruct.PLL3.PLL3FRACN = 0;
  PeriphClkInitStruct.OspiClockSelection = RCC_OSPICLKSOURCE_D1HCLK;
  PeriphClkInitStruct.SdmmcClockSelection = RCC_SDMMCCLKSOURCE_PLL;
  PeriphClkInitStruct.Sai1ClockSelection = RCC_SAI1CLKSOURCE_PLL2;
  PeriphClkInitStruct.Spi123ClockSelection = RCC_SPI123CLKSOURCE_PLL;
  PeriphClkInitStruct.Dfsdm1ClockSelection = RCC_DFSDM1CLKSOURCE_D2PCLK1;
  PeriphClkInitStruct.Usart16ClockSelection = RCC_USART16910CLKSOURCE_D2PCLK2;
  PeriphClkInitStruct.I2c1235ClockSelection = RCC_I2C1235CLKSOURCE_D2PCLK1;
  PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_PLL3;
  PeriphClkInitStruct.AdcClockSelection = RCC_ADCCLKSOURCE_PLL2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Enable USB Voltage detector
  */
  HAL_PWREx_EnableUSBVoltageDetector();
}

int main(void)
{
	int16_t enc;
    int8_t i;
    uint32_t act, tot;

    /* Enable the CPU Cache */
    CPU_CACHE_Enable();

	/* init systick handler before letting HAL start it */
	systick_init();

    /* STM32H7xx HAL library initialization:
       - Systick timer is configured by default as source of time base, but user
         can eventually implement his proper time base source (a general purpose
         timer for example or other time source), keeping in mind that Time base
         duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
         handled in milliseconds basis.
       - Set NVIC Group Priority to 4
       - Low Level Initialization
     */
    HAL_Init();

    /* Configure the system clock to 550 MHz */
    SystemClock_Config();

    /* start cyclesleep */
	cyccnt_enable();

	/* init the UART for diagnostics */
	setup_usart();
	init_printf(0,usart_putc);
	printf("\n\n\rH730 Audio Main\n\r");
	printf("CPUID: 0x%08X\n\r", SCB->CPUID);
	printf("IDCODE: 0x%08X\n\r", DBGMCU->IDCODE);
	printf("Version: %s\n\r", fwVersionStr);
	printf("Build Date: %s\n\r", bdate);
	printf("Build Time: %s\n\r", btime);
	printf("\n");
	printf("SYSCLK = %d\n\r", HAL_RCC_GetSysClockFreq());
	printf("\n");

	/* initialize LEDs */
	LEDInit();
	printf("LED initialized\n\r");

	/* initialize the Quad SPI RAM */
	qspi_init();
	printf("QSPI initialized\n\r");

#if 1
	if(qspi_test())
		printf("QSPI Test Failed\n\r");
	else
		printf("QSPI Test Passed\n\r");
#endif

	/* initialize the SD card power control */
	sdio_init();
	printf("SD Card slot initialized\n\r");

#if 1
	/* Check if SD card works */
	if(sdio_detect())
	{
		printf("SD Card Present\n\r");
		sdio_power(1);
		sdio_test();
		sdio_power(0);
	}
	else
		printf("No SD Card\n\r");
#endif

    /* initialize I2C bus processing */
    shared_i2c_init();
    printf("I2C initialized\n\r");

    /* initialize eeprom */
    eeprom_init();
    printf("EEPROM initialized\n\r");

/* initialize the spi port and LCD */
	gfx_init(&ST7789_drvr);
	gfx_clrscreen();
	gfx_drawstr(0, 0, "Hello World!");
	gfx_set_forecolor(GFX_RED);
	gfx_fillcircle(40, 60, 30);
	gfx_set_forecolor(GFX_GREEN);
	gfx_fillcircle(70, 90, 30);
	gfx_set_forecolor(GFX_BLUE);
	gfx_fillcircle(100, 120, 30);
	ST7789_backlight(1);
	printf("GFX initialized\n\r");

    /* initialize audio processing */
	ADC_Init(GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7);
	printf("ADC initialized\n\r");

    /* initialize audio processing */
	Audio_Init();
	printf("Audio initialized\n\r");

	/* setup codec */
	Codec_Init();
	printf("Codec initialized\n\r");

	/* init Codec */
	if(Codec_Reset()==0)
		printf("Codec reset\n\r");
	else
		printf("Codec did not reset properly\n\r");

	/* initialize SAI port */
	sai_init(FAST_DMA_BUFFSZ);
	printf("SAI initialized\n\r");
	printf("SAI Sample Rate = %d \n\r", sai_get_samplerate());
    printf("SAI Buffer size = %d \n\r", sai_get_bufsz());

	/* unmute */
	Codec_Mute(0);
	printf("Codec unmuted\n\r");

    /* Infinite loop */
	printf("Looping...\n\r");
	while(1)
	{
        /* Audio load */
        get_meas(&act, &tot);
        printf("%2d %% ", 100*act/tot);

        /* Dump ADC and button state */
        for(i=0;i<ADC_NUMCHLS;i++)
        {
            printf("%03X ", ADC_getraw(i));
        }

		enc += systick_get_enc();
		printf("%1d %d  \r", systick_get_button(ENC_E), enc);

		LEDToggle(0);
		HAL_Delay(100);
	}

	/* should never get here */
	return 0;
}
