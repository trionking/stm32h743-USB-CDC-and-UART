/*
 * user_com.h
 *
 *  Created on: Nov 2, 2021
 *      Author: trion
 */

#ifndef INC_USER_COM_H_
#define INC_USER_COM_H_
#include <ctype.h>
#include <stdbool.h>

#include "main.h"
#include "ring_buffer.h"
#include "user_def.h"

#define PR_NOP    99        // No style
#define PR_INI    0         // initial value
#define PR_BLK    30        // Black
#define PR_RED    31        // Red
#define PR_GRN    32        // Green
#define PR_YEL    33        // Yellow
#define PR_BLE    34        // Blue
#define PR_MAG    35        // Magenta
#define PR_CYN    36        // Cyan
#define PR_WHT    37        // White

/*==============================================================================
 * UART DMA 버퍼 크기 정의
 *============================================================================*/
#define U1_TX_BUF_SIZE    512
#define U1_RX_BUF_SIZE    512
#define U2_TX_BUF_SIZE    512
#define U2_RX_BUF_SIZE    8192
#define U4_TX_BUF_SIZE    512
#define U4_RX_BUF_SIZE    512

#define USB_TX_BUF_SIZE   512
#define USB_RX_BUF_SIZE   512


#define UART_TX_SEND_SIZE    100	// 10ms 당 보낼 최대 바이트 수 : 115200 bps / 10 / 100 = 115
#define USB_TX_SEND_SIZE    512	// 10ms 당 보낼 최대 바이트 수 : 12M bps / 10 / 100 = 12000, 대충 여유 있게 512


/* 하위 호환성 유지 */
#define DMA_RX_BUFFER_SIZE    512
#define DMA_TX_BUFFER_SIZE    512
#define U2_DMA_RX_BUFFER_SIZE 8192

/*==============================================================================
 * UART 상태 구조체 매크로 (TX/RX DMA 버퍼 포함)
 *============================================================================*/
#define DECLARE_UART_STAT_STRUCT(name, tx_size, rx_size)  \
struct name##_Stat_ST {                                    \
    uint8_t f_uart_rcvd;       /* DMA RX 완료 플래그 */    \
    uint8_t cnt_rx;                                        \
    uint8_t tx_buffer[tx_size];/* DMA TX 버퍼 */           \
    uint8_t rx_buffer[rx_size];/* DMA RX 버퍼 */           \
    uint8_t line_buf[rx_size + 2]; /* 라인 버퍼 */         \
    uint32_t dma_rx_len;                                   \
    uint16_t rx_CRC16;                                     \
    uint16_t tx_CRC16;                                     \
    uint16_t cal_CRC16;                                    \
}

/* UART별 구조체 타입 선언 */
DECLARE_UART_STAT_STRUCT(uart1, U1_TX_BUF_SIZE, U1_RX_BUF_SIZE);
DECLARE_UART_STAT_STRUCT(uart2, U2_TX_BUF_SIZE, U2_RX_BUF_SIZE);
DECLARE_UART_STAT_STRUCT(uart4, U4_TX_BUF_SIZE, U4_RX_BUF_SIZE);

/*==============================================================================
 * USB 상태 구조체
 *============================================================================*/
struct usb_Stat_ST {
    uint8_t f_rcvd;
    uint8_t tx_buffer[USB_TX_BUF_SIZE + 2];
    uint8_t rx_buffer[USB_RX_BUF_SIZE + 2];
    uint32_t dma_rx_len;
    uint16_t rx_CRC16;
    uint16_t tx_CRC16;
    uint16_t cal_CRC16;
};

typedef enum
{
  NOT_LINE = 0,
  RCV_LINE = 1,
  WNG_LINE = 2,
  PAS_LINE = 3,
} COM_Idy_Typ;

#define USB_ECHO
#define USB_ETX		'\r'

//#define UART2_ECHO
#define UART2_ETX		'\n'

//#define UART1_ECHO
#define UART1_ETX		'\r'

//#define UART4_ECHO
#define UART4_ETX		'\r'

extern union flash_UT ufl_UT;

uint8_t atoh(char in_ascii);
uint8_t atod(char in_ascii);
uint32_t atoh_str(char *in_asc_str,uint8_t len);
uint16_t safe_atoi(const char *str, uint16_t len);
uint32_t atod_str(char *in_asc_str,uint8_t len);
bool is_float(const char *str);
bool is_numeric(const char *str);

void init_USB_COM(void);

void UART_baudrate_set(UART_HandleTypeDef *tm_hart,uint32_t baud_rate);
void init_UART_COM(void);

void printf_USBC(uint8_t color,const char *str_buf,...);
void printf_UARTC(UART_HandleTypeDef *h_tmUART,uint8_t color,const char *str_buf,...);
void printf_UARTC_485(UART_HandleTypeDef *h_tmUART,uint8_t color,const char *str_buf,...);

void ESP_Printf(uint8_t color,const char *str_buf,...);
COM_Idy_Typ ESP_GetLine(uint8_t *line_buf);
void DBG_Printf(uint8_t color,const char *str_buf,...);
COM_Idy_Typ DBUG_GetLine(uint8_t *line_buf);
COM_Idy_Typ TF_DBUG_GetLine(uint8_t *line_buf);
COM_Idy_Typ USB_GetLine(uint8_t *line_buf);
COM_Idy_Typ UART2_GetLine(uint8_t *line_buf);


#endif /* INC_USER_COM_H_ */
