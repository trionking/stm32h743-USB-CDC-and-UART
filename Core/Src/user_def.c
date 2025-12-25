/*
 * user_def.c
 *
 *  Created on: Dec 16, 2025
 *      Author: SIDO
 */


#include "main.h"

#include "user_def.h"
#include "user_com.h"
#include "buffers.h"
#include "usbd_cdc_if.h"

//#include "stdio.h"
#include "fatfs.h"

extern TIM_HandleTypeDef htim7;

extern UART_HandleTypeDef huart4;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_uart4_rx;
extern DMA_HandleTypeDef hdma_uart4_tx;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;
extern DMA_HandleTypeDef hdma_usart2_rx;
extern DMA_HandleTypeDef hdma_usart2_tx;

extern SD_HandleTypeDef hsd1;

// UART1 queue
extern Queue rx_UART1_queue;
extern Queue rx_UART1_line_queue;
extern Queue tx_UART1_queue;

// UART2 queue
extern Queue rx_UART2_queue;
extern Queue rx_UART2_line_queue;
extern Queue tx_UART2_queue;

// UART4 queue
extern Queue rx_UART4_queue;
extern Queue rx_UART4_line_queue;
extern Queue tx_UART4_queue;

// USB queue
extern Queue rx_USB_queue;
extern Queue rx_USB_line_queue;
extern Queue tx_USB_queue;

// 1ms Interrupt callback function
//   HAL_TIM_PeriodElapsedCallback
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	uint16_t tm_q_len;
	static uint16_t cntT_10ms=0;

	if (htim->Instance == TIM7)    // 1ms Timer
	{
		cntT_10ms++;
		if (cntT_10ms > 9)
		{
			cntT_10ms = 0;


			tm_q_len =  Len_queue(&tx_USB_queue);

			if (tm_q_len)    // buffer not empty?
			{
				if (IS_CDC_TxState_Busy() != USBD_BUSY)
				{
					if (tm_q_len > USB_TX_SEND_SIZE)
					{
							Dequeue_bytes(&tx_USB_queue, usb_stat.tx_buffer, USB_TX_SEND_SIZE);
							CDC_Transmit_FS(usb_stat.tx_buffer, USB_TX_SEND_SIZE);
					}
					else
					{
							Dequeue_bytes(&tx_USB_queue, usb_stat.tx_buffer, tm_q_len);
							CDC_Transmit_FS(usb_stat.tx_buffer, tm_q_len);
					}
				}
			}

			// uart tx process from buffer
			tm_q_len =  Len_queue(&tx_UART1_queue);

			if (tm_q_len)    // buffer not empty?
			{
				if (hdma_usart1_tx.State == HAL_DMA_STATE_READY)
				{
					if (tm_q_len > UART_TX_SEND_SIZE)
					{
							Dequeue_bytes(&tx_UART1_queue, uart1_stat.tx_buffer, UART_TX_SEND_SIZE);
							HAL_UART_Transmit_DMA(&huart1, uart1_stat.tx_buffer, UART_TX_SEND_SIZE);
					}
					else
					{
							Dequeue_bytes(&tx_UART1_queue, uart1_stat.tx_buffer, tm_q_len);
							HAL_UART_Transmit_DMA(&huart1, uart1_stat.tx_buffer, tm_q_len);
					}
				}
			}
/*			// uart tx process from buffer
			tm_q_len =  Len_queue(&tx_UART2_queue);

			if (tm_q_len)    // buffer not empty?
			{
				if (hdma_usart2_tx.State == HAL_DMA_STATE_READY)
				{
					if (tm_q_len > DMA_TX_BUFFER_SIZE)
					{
							Dequeue_bytes(&tx_UART2_queue,u2TxBuffer,DMA_TX_BUFFER_SIZE);
							// DMA 전송 전 캐시 플러시
							SCB_CleanDCache_by_Addr((uint32_t *)SDFatFS.win, sizeof(SDFatFS.win));
							HAL_UART_Transmit_DMA(&huart2,u2TxBuffer,DMA_TX_BUFFER_SIZE);
							// DMA 전송 후 캐시 무효화
							SCB_InvalidateDCache_by_Addr((uint32_t *)SDFatFS.win, sizeof(SDFatFS.win));
					}
					else
					{
							Dequeue_bytes(&tx_UART2_queue,u2TxBuffer,tm_q_len);
							// DMA 전송 전 캐시 플러시
							SCB_CleanDCache_by_Addr((uint32_t *)SDFatFS.win, sizeof(SDFatFS.win));
							HAL_UART_Transmit_DMA(&huart2,u2TxBuffer,tm_q_len);
							// DMA 전송 후 캐시 무효화
							SCB_InvalidateDCache_by_Addr((uint32_t *)SDFatFS.win, sizeof(SDFatFS.win));
					}
				}
			}

*/
			// uart tx process from buffer
			tm_q_len =  Len_queue(&tx_UART4_queue);

			if (tm_q_len)    // buffer not empty?
			{
				if (hdma_uart4_tx.State == HAL_DMA_STATE_READY)
				{
					if (tm_q_len > UART_TX_SEND_SIZE)
					{
							Dequeue_bytes(&tx_UART4_queue, uart4_stat.tx_buffer, UART_TX_SEND_SIZE);
							/* RAM_D2 (Non-cacheable) 영역이므로 캐시 작업 불필요 */
							HAL_UART_Transmit_DMA(&huart4, uart4_stat.tx_buffer, UART_TX_SEND_SIZE);
					}
					else
					{
							Dequeue_bytes(&tx_UART4_queue, uart4_stat.tx_buffer, tm_q_len);
							/* RAM_D2 (Non-cacheable) 영역이므로 캐시 작업 불필요 */
							HAL_UART_Transmit_DMA(&huart4, uart4_stat.tx_buffer, tm_q_len);
					}
				}
			}

		}
	}
}

void Out_esp32_rst_pin(uint8_t ctrl)
{
	if (ctrl)
	{
		HAL_GPIO_WritePin(OT_ESP_RST_GPIO_Port, OT_ESP_RST_Pin, GPIO_PIN_SET);
	}
	else
	{
		HAL_GPIO_WritePin(OT_ESP_RST_GPIO_Port, OT_ESP_RST_Pin, GPIO_PIN_RESET);
	}
}

void usr_MX_SDMMC1_SD_Init(void)
{

  hsd1.Instance = SDMMC1;
  hsd1.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
  hsd1.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
  hsd1.Init.BusWide = SDMMC_BUS_WIDE_4B;
  hsd1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd1.Init.ClockDiv = 2;
}

void SD_GPIO_Low_Out(void)
{
	  GPIO_InitTypeDef GPIO_InitStruct = {0};

	  /*Configure GPIO pin Output Level */
	  HAL_GPIO_WritePin(SDMMC1_D0_GPIO_Port, SDMMC1_D0_Pin, GPIO_PIN_RESET);
	  HAL_GPIO_WritePin(SDMMC1_D1_GPIO_Port, SDMMC1_D1_Pin, GPIO_PIN_RESET);
	  HAL_GPIO_WritePin(SDMMC1_D2_GPIO_Port, SDMMC1_D2_Pin, GPIO_PIN_RESET);
	  HAL_GPIO_WritePin(SDMMC1_D3_GPIO_Port, SDMMC1_D3_Pin, GPIO_PIN_RESET);
	  HAL_GPIO_WritePin(SDMMC1_CMD_GPIO_Port, SDMMC1_CMD_Pin, GPIO_PIN_RESET);
	  HAL_GPIO_WritePin(SDMMC1_CK_GPIO_Port, SDMMC1_CK_Pin, GPIO_PIN_RESET);

	  GPIO_InitStruct.Pin = SDMMC1_D0_Pin|SDMMC1_D1_Pin|SDMMC1_D2_Pin|SDMMC1_D3_Pin|SDMMC1_CK_Pin;
	  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	  //GPIO_InitStruct.Pull = GPIO_NOPULL;
	  GPIO_InitStruct.Pull = GPIO_PULLUP;
	  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	  GPIO_InitStruct.Pin = SDMMC1_CMD_Pin;
	  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	  //GPIO_InitStruct.Pull = GPIO_NOPULL;
	  GPIO_InitStruct.Pull = GPIO_PULLUP;
	  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	  HAL_GPIO_Init(SDMMC1_CMD_GPIO_Port, &GPIO_InitStruct);
}

void SD_GPIO_Float(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

	HAL_SD_MspDeInit(&hsd1);

  GPIO_InitStruct.Pin = SDMMC1_D0_Pin|SDMMC1_D1_Pin|SDMMC1_D2_Pin|SDMMC1_D3_Pin|SDMMC1_CK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  //GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = SDMMC1_CMD_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  //GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SDMMC1_CMD_GPIO_Port, &GPIO_InitStruct);

}

void sd_card_ctl(uint8_t ctl_val)	// 1: stm32, 0: esp32
{
	uint32_t sd_tick;
	FRESULT tm_res;

	if (ctl_val)	// sdcard On
	{
		sd_tick = HAL_GetTick();
		while(1)
		{
			// sd-card Power Off
			HAL_SD_MspDeInit(&hsd1);
		  HAL_GPIO_WritePin(OT_SD_EN_GPIO_Port, OT_SD_EN_Pin, GPIO_PIN_RESET);
		  SD_GPIO_Low_Out();
		  HAL_Delay(50);
		  HAL_GPIO_WritePin(OT_SD_EN_GPIO_Port, OT_SD_EN_Pin, GPIO_PIN_SET);
			HAL_SD_MspInit(&hsd1);
			HAL_Delay(50);

			tm_res = BSP_SD_Init();
			if (tm_res == FR_OK)
			{
				break;
			}
			else
			{
				if ((HAL_GetTick()-sd_tick) > 2000)
				{
					sd_tick = HAL_GetTick();
					printf_USBC(PR_RED,"SD Card Error %d, Please Insert SD Card\r\n", tm_res);
				}
			}
		}
	}
	else	// sd card Off
	{
		// sd card desable
		HAL_SD_MspDeInit(&hsd1);
	  HAL_GPIO_WritePin(OT_SD_EN_GPIO_Port, OT_SD_EN_Pin, GPIO_PIN_RESET);
	  SD_GPIO_Low_Out();
	  HAL_Delay(50);
	  SD_GPIO_Float();
	  HAL_GPIO_WritePin(OT_SD_EN_GPIO_Port, OT_SD_EN_Pin, GPIO_PIN_SET);
	}
  HAL_Delay(100);
}

void sd_scan_dir(void)
{
	TCHAR path[200] = "0:/"; // 명시적으로 루트 디렉토리 설정
	DIR root_dir;
	FILINFO file_info;

	// SD 카드 마운트
	retSD = f_mount(&SDFatFS, (TCHAR const*)SDPath, 1);
	//printf_USBC(PR_WHT,"SD Mount : res f_mount : %02X\n\r", retSD);

	if (retSD == FR_OK)
	{
		// 디렉토리 열기
		retSD = f_opendir(&root_dir, path);
		//printf_USBC(PR_WHT,"res f_opendir : %02X\n\r", retSD);

		if (retSD == FR_OK)
		{
			while (1)
			{
				// 디렉토리 내 파일 읽기
				retSD = f_readdir(&root_dir, &file_info);
				if (retSD != FR_OK || file_info.fname[0] == 0)
						break;

								TCHAR *fn;
								fn = file_info.fname;

								printf_USBC(PR_WHT,"%c%c%c%c ",
														 ((file_info.fattrib & AM_DIR) ? 'D' : '-'),
														 ((file_info.fattrib & AM_RDO) ? 'R' : '-'),
														 ((file_info.fattrib & AM_SYS) ? 'S' : '-'),
														 ((file_info.fattrib & AM_HID) ? 'H' : '-'));

								printf_USBC(PR_WHT,"%10ld ", file_info.fsize);
								printf_USBC(PR_WHT,"%s\r\n", fn);
								HAL_Delay(2);

			}
		}
		else
		{
			printf_USBC(PR_WHT,"f_opendir 실패: res = %02X\n\r", retSD);
		}

		// SD 카드 언마운트
		retSD = f_mount(0, SDPath, 0);
		//printf_USBC(PR_WHT,"SD Unmount : res f_mount : %02X\n\r", retSD);
	}
	else
	{
		printf_USBC(PR_WHT,"SD Mount 실패: res f_mount : %02X\n\r", retSD);
	}
}

void sd_read_write_test(void)
{
	UINT bytes_written, bytes_read;
	char write_buf[128] = "STM32H743 SD Card Read/Write Test!\r\nThis is a test message.\r\n";
	char read_buf[128] = {0};
	const char *test_file = "0:/test.txt";

	// SD 카드 마운트
	retSD = f_mount(&SDFatFS, (TCHAR const*)SDPath, 1);
	if (retSD != FR_OK)
	{
		printf_USBC(PR_RED, "SD Mount 실패: %02X\r\n", retSD);
		return;
	}
	printf_USBC(PR_GRN, "SD Mount 성공\r\n");

	// 파일 쓰기 테스트
	retSD = f_open(&SDFile, test_file, FA_CREATE_ALWAYS | FA_WRITE);
	if (retSD != FR_OK)
	{
		printf_USBC(PR_RED, "파일 열기(쓰기) 실패: %02X\r\n", retSD);
		f_mount(0, SDPath, 0);
		return;
	}

	retSD = f_write(&SDFile, write_buf, strlen(write_buf), &bytes_written);
	if (retSD == FR_OK)
	{
		printf_USBC(PR_GRN, "파일 쓰기 성공: %d bytes\r\n", bytes_written);
	}
	else
	{
		printf_USBC(PR_RED, "파일 쓰기 실패: %02X\r\n", retSD);
	}
	f_close(&SDFile);

	// 파일 읽기 테스트
	retSD = f_open(&SDFile, test_file, FA_READ);
	if (retSD != FR_OK)
	{
		printf_USBC(PR_RED, "파일 열기(읽기) 실패: %02X\r\n", retSD);
		f_mount(0, SDPath, 0);
		return;
	}

	retSD = f_read(&SDFile, read_buf, sizeof(read_buf) - 1, &bytes_read);
	if (retSD == FR_OK)
	{
		read_buf[bytes_read] = '\0';  // null 종료
		printf_USBC(PR_GRN, "파일 읽기 성공: %d bytes\r\n", bytes_read);
		printf_USBC(PR_CYN, "내용: %s", read_buf);
	}
	else
	{
		printf_USBC(PR_RED, "파일 읽기 실패: %02X\r\n", retSD);
	}
	f_close(&SDFile);

	// 데이터 검증
	if (strcmp(write_buf, read_buf) == 0)
	{
		printf_USBC(PR_GRN, "\r\n[OK] 읽기/쓰기 데이터 일치!\r\n");
	}
	else
	{
		printf_USBC(PR_RED, "\r\n[FAIL] 읽기/쓰기 데이터 불일치!\r\n");
	}

	// SD 카드 언마운트
	f_mount(0, SDPath, 0);
	printf_USBC(PR_WHT, "SD Unmount 완료\r\n");
}

void test_gpio(void)
{
	uint32_t usr_tick;

	usr_tick = HAL_GetTick();

	while(1)
	{
		if ((HAL_GetTick()-usr_tick) > 300)
		{
			usr_tick = HAL_GetTick();

			HAL_GPIO_TogglePin(OT_LD_SYS_GPIO_Port, OT_LD_SYS_Pin);
		}
	}
}

void test_uart(void)
{
	uint32_t usr_tick;

	usr_tick = HAL_GetTick();

	while(1)
	{
		if ((HAL_GetTick()-usr_tick) > 300)
		{
			usr_tick = HAL_GetTick();

			HAL_GPIO_TogglePin(OT_LD_SYS_GPIO_Port, OT_LD_SYS_Pin);
			printf_UARTC(&huart4, PR_YEL, "UART4 Test from STM32H723\r\n");
		}
	}
}

void test_usb_cdc(void)
{
	uint32_t usr_tick;

	usr_tick = HAL_GetTick();

	while(1)
	{
		if ((HAL_GetTick()-usr_tick) > 300)
		{
			usr_tick = HAL_GetTick();

			HAL_GPIO_TogglePin(OT_LD_SYS_GPIO_Port, OT_LD_SYS_Pin);
			printf_USBC(PR_INI, "USB-CDC Test from STM32H723\r\n");
		}
	}
}

void test_sd_card(void)
{
  usr_MX_SDMMC1_SD_Init();

  sd_card_ctl(1);	// stm32

  sd_scan_dir();
  sd_read_write_test();
}

void init_proc(void)
{
	/* Queue 및 통신 초기화를 먼저 수행 */
	Out_esp32_rst_pin(1); //esp32 disable
	init_UART_COM();
	init_USB_COM();

	/* 모든 초기화 완료 후 타이머 시작 */
	HAL_TIM_Base_Start_IT(&htim7);

	HAL_Delay(2000);
}


void run_proc(void)
{
	init_proc();

	//test_gpio();
	//test_uart();
	//test_usb_cdc();
	test_sd_card();

	while(1)
	{

	}
}
