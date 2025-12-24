/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file   fatfs.c
  * @brief  Code for fatfs applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
#include "fatfs.h"
#include "memory_config.h"  /* SECTION_NC 매크로 - CubeMX 재생성 후 다시 추가 필요 */

uint8_t retSD;    /* Return value for SD */
char SDPath[4];   /* SD logical drive path */

/*
 * [중요] SDMMC IDMA는 RAM_D1만 접근 가능!
 * FATFS 구조체 내부의 win 버퍼(512B)가 DMA 전송에 사용되므로
 * 반드시 RAM_D1 (Non-cacheable) 영역에 배치해야 함
 * CubeMX 재생성 후 SECTION_NC 다시 추가 필요!
 */
SECTION_NC FATFS SDFatFS;    /* File system object for SD logical drive */
SECTION_NC FIL SDFile;       /* File object for SD */

/* USER CODE BEGIN Variables */

/* USER CODE END Variables */

void MX_FATFS_Init(void)
{
  /*## FatFS: Link the SD driver ###########################*/
  retSD = FATFS_LinkDriver(&SD_Driver, SDPath);

  /* USER CODE BEGIN Init */
  /* additional user code for init */
  /* USER CODE END Init */
}

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

/* USER CODE BEGIN Application */

/* USER CODE END Application */
