/**
 ******************************************************************************
  * @file    bsp_driver_sd.c for H7 (based on stm32h743i_eval_sd.c)
  * @brief   This file includes a generic uSD card driver.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* USER CODE BEGIN FirstSection */
/* can be used to modify / undefine following code or add new definitions */
/* USER CODE END FirstSection */
/* Includes ------------------------------------------------------------------*/
#include "bsp_driver_sd.h"
#include "printf.h"
#include "sdio.h"

/* Extern variables ---------------------------------------------------------*/
extern SD_HandleTypeDef hsd1;

/* variables ---------------------------------------------------------*/
static volatile  uint32_t  WriteStatus = 0, ReadStatus = 0;

/* USER CODE BEGIN BeforeInitSection */
/* can be used to modify / undefine following code or add code */
/* USER CODE END BeforeInitSection */
/**
  * @brief  Initializes the SD card device.
  * @retval SD status
  */
uint8_t BSP_SD_Init(void)
{
  uint8_t sd_state = MSD_OK;
  /* Check if the SD card is plugged in the slot */
  if (BSP_SD_IsDetected() != SD_PRESENT)
  {
	//printf("BSP_SD_Init: card not detected (SHOULDN'T HAPPEN)\n\r");
    return MSD_ERROR_SD_NOT_PRESENT;
  }

  //printf("BSP_SD_Init: &hsd1 = 0x%08X\n\r", &hsd1);

  /* HAL SD initialization */
  sd_state = HAL_SD_Init(&hsd1);
  /* Configure SD Bus width (4 bits mode selected) */
  if (sd_state == MSD_OK)
  {
	printf("BSP_SD_Init: HAL_SD_Init OK\n\r");
#if 0
	  /* NOT NEEDED - HAL_SD_Init() does this already */
	/* Enable wide operation */
    if (HAL_SD_ConfigWideBusOperation(&hsd1, SDMMC_BUS_WIDE_4B) != HAL_OK)
    {
	  printf("BSP_SD_Init: HAL_SD_ConfigWideBusOperation Failed\n\r");
      sd_state = MSD_ERROR;
    }
	else
	  printf("BSP_SD_Init: HAL_SD_ConfigWideBusOperation OK\n\r");
#endif

    /* enable IRQs for SDMMC  - priority lower than audio (which is 1) */
    HAL_NVIC_SetPriority(SDMMC1_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(SDMMC1_IRQn);
  }
  else
  {
	  printf("BSP_SD_Init: HAL_SD_Init failed with result %d\n\r", sd_state);
  }

  return sd_state;
}
/* USER CODE BEGIN AfterInitSection */
/* can be used to modify previous code / undefine following code / add code */
/* USER CODE END AfterInitSection */

/**
  * @brief  Configures Interrupt mode for SD detection pin.
  * @retval Returns 0 in success otherwise 1.
  */
uint8_t BSP_SD_ITConfig(void)
{
  /* TBI: add user code here depending on the hardware configuration used */

  return (uint8_t)0;
}

/* USER CODE BEGIN BeforeReadBlocksSection */
/* can be used to modify previous code / undefine following code / add code */
/* USER CODE END BeforeReadBlocksSection */
/**
  * @brief  Reads block(s) from a specified address in an SD card, in polling mode.
  * @param  pData: Pointer to the buffer that will contain the data to transmit
  * @param  ReadAddr: Address from where data is to be read
  * @param  NumOfBlocks: Number of SD blocks to read
  * @param  Timeout: Timeout for read operation
  * @retval SD status
  */
uint8_t BSP_SD_ReadBlocks(uint32_t *pData, uint32_t ReadAddr, uint32_t NumOfBlocks, uint32_t Timeout)
{
  uint8_t sd_state = MSD_OK;

  if (HAL_SD_ReadBlocks(&hsd1, (uint8_t *)pData, ReadAddr, NumOfBlocks, Timeout) != HAL_OK)
  {
      printf("BSP_SD_ReadBlocks: fail, err = %d\n\r", hsd1.ErrorCode);
      //printf("\t pData = 0x%08X, ReadAddr = 0x%08X, NumOfBlocks = %d\n\r", pData, ReadAddr, NumOfBlocks);
      sd_state = MSD_ERROR;
  }
  else
  {
	  //printf("BSP_SD_ReadBlocks: HAL_SD_ReadBlocks OK\n\r");
  }

  return sd_state;
}

/* USER CODE BEGIN BeforeWriteBlocksSection */
/* can be used to modify previous code / undefine following code / add code */
/* USER CODE END BeforeWriteBlocksSection */
/**
  * @brief  Writes block(s) to a specified address in an SD card, in polling mode.
  * @param  pData: Pointer to the buffer that will contain the data to transmit
  * @param  WriteAddr: Address from where data is to be written
  * @param  NumOfBlocks: Number of SD blocks to write
  * @param  Timeout: Timeout for write operation
  * @retval SD status
  */
uint8_t BSP_SD_WriteBlocks(uint32_t *pData, uint32_t WriteAddr, uint32_t NumOfBlocks, uint32_t Timeout)
{
  uint8_t sd_state = MSD_OK;

  if (HAL_SD_WriteBlocks(&hsd1, (uint8_t *)pData, WriteAddr, NumOfBlocks, Timeout) != HAL_OK)
  {
    printf("BSP_SD_WriteBlocks: fail, err = %d\n\r", hsd1.ErrorCode);
    //printf("\t pData = 0x%08X, WriteAddr = 0x%08X, NumOfBlocks = %d\n\r", pData, WriteAddr, NumOfBlocks);
    sd_state = MSD_ERROR;
  }

  return sd_state;
}

/* USER CODE BEGIN BeforeReadDMABlocksSection */
/* can be used to modify previous code / undefine following code / add code */
/* USER CODE END BeforeReadDMABlocksSection */
/**
  * @brief  Reads block(s) from a specified address in an SD card, in DMA mode.
  * @param  pData: Pointer to the buffer that will contain the data to transmit
  * @param  ReadAddr: Address from where data is to be read
  * @param  NumOfBlocks: Number of SD blocks to read
  * @retval SD status
  */
uint8_t BSP_SD_ReadBlocks_DMA(uint32_t *pData, uint32_t ReadAddr, uint32_t NumOfBlocks)
{
  uint8_t sd_state = MSD_OK;
  uint32_t timeout;
  uint32_t alignedAddr;

  ReadStatus = 0;

  /* Read block(s) in DMA transfer mode */
  if (HAL_SD_ReadBlocks_DMA(&hsd1, (uint8_t *)pData, ReadAddr, NumOfBlocks) != HAL_OK)
  {
    sd_state = MSD_ERROR;
  }
  else
  {
      /* Wait that the reading process is completed or a timeout occurs */
      timeout = HAL_GetTick();
      while((ReadStatus == 0) && ((HAL_GetTick() - timeout) < BSP_SD_TIMEOUT))
      {
      }

      /* incase of a timeout return error */
      if (ReadStatus == 0)
      {
        sd_state = MSD_ERROR;
      }
      else
      {
        ReadStatus = 0;
        timeout = HAL_GetTick();

        while((HAL_GetTick() - timeout) < BSP_SD_TIMEOUT)
        {
          if (BSP_SD_GetCardState() == SD_TRANSFER_OK)
          {
            sd_state = MSD_OK;
              /*
                 the SCB_InvalidateDCache_by_Addr() requires a 32-Byte aligned address,
                 adjust the address and the D-Cache size to invalidate accordingly.
               */
              alignedAddr = (uint32_t)pData & ~0x1F;
              SCB_InvalidateDCache_by_Addr((uint32_t*)pData, NumOfBlocks*512 + ((uint32_t)pData - alignedAddr));
             break;
          }
        }
      }
  }

  return sd_state;
}

/* USER CODE BEGIN BeforeWriteDMABlocksSection */
/* can be used to modify previous code / undefine following code / add code */
/* USER CODE END BeforeWriteDMABlocksSection */
/**
  * @brief  Writes block(s) to a specified address in an SD card, in DMA mode.
  * @param  pData: Pointer to the buffer that will contain the data to transmit
  * @param  WriteAddr: Address from where data is to be written
  * @param  NumOfBlocks: Number of SD blocks to write
  * @retval SD status
  */
uint8_t BSP_SD_WriteBlocks_DMA(uint32_t *pData, uint32_t WriteAddr, uint32_t NumOfBlocks)
{
  uint8_t sd_state = MSD_OK;
  uint32_t timeout;
  uint32_t alignedAddr;

  /* flush DCache to destination - no safety overlap so sz >= 8 */
  alignedAddr = (uint32_t)pData & ~0x1F;
  SCB_CleanDCache_by_Addr((uint32_t*)pData, NumOfBlocks*512 + ((uint32_t)pData - alignedAddr));

  /* Write block(s) in DMA transfer mode */
  if (HAL_SD_WriteBlocks_DMA(&hsd1, (uint8_t *)pData, WriteAddr, NumOfBlocks) != HAL_OK)
  {
    sd_state = MSD_ERROR;
  }
  else
  {
    /* Wait that writing process is completed or a timeout occurs */
    timeout = HAL_GetTick();
    while((WriteStatus == 0) && ((HAL_GetTick() - timeout) < BSP_SD_TIMEOUT))
    {
    }

    /* incase of a timeout return error */
    if (WriteStatus == 0)
    {
      sd_state = MSD_ERROR;
    }
    else
    {
      WriteStatus = 0;
      timeout = HAL_GetTick();

      while((HAL_GetTick() - timeout) < BSP_SD_TIMEOUT)
      {
        if (BSP_SD_GetCardState() == SD_TRANSFER_OK)
        {
          sd_state = MSD_OK;
          break;
        }
      }
    }
  }

  return sd_state;
}

/* USER CODE BEGIN BeforeEraseSection */
/* can be used to modify previous code / undefine following code / add code */
/* USER CODE END BeforeEraseSection */
/**
  * @brief  Erases the specified memory area of the given SD card.
  * @param  StartAddr: Start byte address
  * @param  EndAddr: End byte address
  * @retval SD status
  */
uint8_t BSP_SD_Erase(uint32_t StartAddr, uint32_t EndAddr)
{
  uint8_t sd_state = MSD_OK;

  if (HAL_SD_Erase(&hsd1, StartAddr, EndAddr) != HAL_OK)
  {
    sd_state = MSD_ERROR;
  }

  return sd_state;
}

/* USER CODE BEGIN BeforeGetCardStateSection */
/* can be used to modify previous code / undefine following code / add code */
/* USER CODE END BeforeGetCardStateSection */
/**
  * @brief  Gets the current SD card data status.
  * @param  None
  * @retval Data transfer state.
  *          This value can be one of the following values:
  *            @arg  SD_TRANSFER_OK: No data transfer is acting
  *            @arg  SD_TRANSFER_BUSY: Data transfer is acting
  */
uint8_t BSP_SD_GetCardState(void)
{
  return ((HAL_SD_GetCardState(&hsd1) == HAL_SD_CARD_TRANSFER ) ? SD_TRANSFER_OK : SD_TRANSFER_BUSY);
}

/**
  * @brief  Get SD information about specific SD card.
  * @param  CardInfo: Pointer to HAL_SD_CardInfoTypedef structure
  * @retval None
  */
void BSP_SD_GetCardInfo(HAL_SD_CardInfoTypeDef *CardInfo)
{
  /* Get SD card Information */
  HAL_SD_GetCardInfo(&hsd1, CardInfo);
}

/* USER CODE BEGIN BeforeCallBacksSection */
/* can be used to modify previous code / undefine following code / add code */
/* USER CODE END BeforeCallBacksSection */
/**
  * @brief SD Abort callbacks
  * @param hsd: SD handle
  * @retval None
  */
void HAL_SD_AbortCallback(SD_HandleTypeDef *hsd)
{
  BSP_SD_AbortCallback();
}

/**
  * @brief Tx Transfer completed callback
  * @param hsd: SD handle
  * @retval None
  */
void HAL_SD_TxCpltCallback(SD_HandleTypeDef *hsd)
{
  BSP_SD_WriteCpltCallback();
}

/**
  * @brief Rx Transfer completed callback
  * @param hsd: SD handle
  * @retval None
  */
void HAL_SD_RxCpltCallback(SD_HandleTypeDef *hsd)
{
  BSP_SD_ReadCpltCallback();
}

/* USER CODE BEGIN CallBacksSection_C */
/**
  * @brief BSP SD Abort callback
  * @retval None
  */
__weak void BSP_SD_AbortCallback(void)
{

}

/**
  * @brief BSP Tx Transfer completed callback
  * @retval None
  */
__weak void BSP_SD_WriteCpltCallback(void)
{
    WriteStatus = 1;
}

/**
  * @brief BSP Rx Transfer completed callback
  * @retval None
  */
__weak void BSP_SD_ReadCpltCallback(void)
{
    ReadStatus = 1;
}
/* USER CODE END CallBacksSection_C */

/**
 * @brief  Detects if SD card is correctly plugged in the memory slot or not.
 * @param  None
 * @retval Returns if SD is detected or not
 */
uint8_t BSP_SD_IsDetected(void)
{
  if(sdio_detect())
    return SD_PRESENT;
  else
    return SD_NOT_PRESENT;
}

/* USER CODE BEGIN AdditionalCode */
/* user code can be inserted here */
/* USER CODE END AdditionalCode */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
