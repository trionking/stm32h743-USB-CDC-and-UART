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


void init_proc(void)
{
	/* Queue 및 통신 초기화를 먼저 수행 */
	init_UART_COM();
	init_USB_COM();

	/* 모든 초기화 완료 후 타이머 시작 */
	HAL_TIM_Base_Start_IT(&htim7);

	HAL_Delay(5000);
}


void run_proc(void)
{
	init_proc();

	//test_gpio();
	//test_uart();
	test_usb_cdc();

	while(1)
	{

	}
}
