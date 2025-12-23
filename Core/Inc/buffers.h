/**
 * @file    buffers.h
 * @brief   메모리 영역별 버퍼 extern 선언
 * @date    2025-12-23
 */

#ifndef BUFFERS_H
#define BUFFERS_H

#include "memory_config.h"
#include "user_com.h"

/*==============================================================================
 * FMC LED 더블버퍼
 *============================================================================*/
extern uint16_t fmc_buffer[2][FMC_BUFFER_SIZE];
extern volatile uint8_t fmc_write_buffer;
extern volatile uint8_t fmc_read_buffer;

/*==============================================================================
 * UART 상태 구조체 (DMA 버퍼 포함, Non-cacheable)
 * 각 구조체 내부:
 *   - tx_buffer[]: DMA TX 버퍼
 *   - rx_buffer[]: DMA RX 버퍼
 *   - line_buf[]: 라인 처리용 버퍼
 *============================================================================*/
extern struct uart1_Stat_ST uart1_stat;   /* UART1 - ESP (512/512) */
extern struct uart2_Stat_ST uart2_stat;   /* UART2 - RS485 (512/8192) */
extern struct uart4_Stat_ST uart4_stat;   /* UART4 - Debug (512/512) */

/*==============================================================================
 * USB 버퍼 (RAM_D1 AXI SRAM, USB OTG FS 접근 가능)
 *============================================================================*/
extern struct usb_Stat_ST usb_stat;
extern uint8_t usb_cdc_rx_buffer[2048];
extern uint8_t usb_cdc_tx_buffer[2048];
extern uint32_t usb_cdc_handle_mem[140];

/*==============================================================================
 * Audio DMA 버퍼
 *============================================================================*/
extern uint8_t audio_buffer[AUDIO_BUFFER_SIZE];

/*==============================================================================
 * 함수 선언
 *============================================================================*/
void Buffer_Swap(void);

#endif /* BUFFERS_H */
