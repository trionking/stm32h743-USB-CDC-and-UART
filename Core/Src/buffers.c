/**
 * @file    buffers.c
 * @brief   메모리 영역별 버퍼 선언
 * @date    2025-12-23
 */

#include "memory_config.h"
#include "user_com.h"

/*==============================================================================
 * RAM_D1 영역 - FMC LED 더블버퍼 (SD-Card에서 직접 수신)
 * 총 사용량: 186,240 × 2 = 372,480 bytes (약 364 KB)
 *============================================================================*/
SECTION_FMC
uint16_t fmc_buffer[2][FMC_BUFFER_SIZE];   /* 더블 버퍼 */

/* 버퍼 인덱스 (0 또는 1) */
volatile uint8_t fmc_write_buffer = 0;     /* SD-Card가 쓰는 버퍼 */
volatile uint8_t fmc_read_buffer = 1;      /* FMC가 읽는 버퍼 */

/*==============================================================================
 * RAM_D2 영역 - UART 상태 구조체 (DMA 버퍼 포함, Non-cacheable)
 *============================================================================*/
SECTION_DMA
struct uart1_Stat_ST uart1_stat;   /* UART1 - ESP 통신 (512/512) */

SECTION_DMA
struct uart2_Stat_ST uart2_stat;   /* UART2 - RS485 통신 (512/8192) */

SECTION_DMA
struct uart4_Stat_ST uart4_stat;   /* UART4 - 디버그 포트 (512/512) */

/*==============================================================================
 * RAM_D1 영역 - USB 버퍼 (AXI SRAM, USB OTG FS 접근 가능)
 * 주의: USB OTG FS는 AXI 버스를 사용하므로 RAM_D2(AHB)/DTCM에 접근 불가
 *============================================================================*/
SECTION_NC
struct usb_Stat_ST usb_stat;

/* USB CDC 버퍼 - CubeMX 자동생성 버퍼 대신 사용 */
SECTION_NC
uint8_t usb_cdc_rx_buffer[2048];

SECTION_NC
uint8_t usb_cdc_tx_buffer[2048];

/* USBD_CDC_HandleTypeDef용 메모리 (약 540 bytes) */
SECTION_NC
uint32_t usb_cdc_handle_mem[140];

/*==============================================================================
 * RAM_D2 영역 - Audio DMA 버퍼 (Non-cacheable)
 *============================================================================*/
SECTION_AUDIO_DMA
uint8_t audio_buffer[AUDIO_BUFFER_SIZE];

/*==============================================================================
 * 버퍼 스왑 함수
 *============================================================================*/
void Buffer_Swap(void)
{
    uint8_t temp = fmc_write_buffer;
    fmc_write_buffer = fmc_read_buffer;
    fmc_read_buffer = temp;
}

/*==============================================================================
 * USB 메모리 할당 함수 (usbd_conf.c의 함수를 대체)
 * CubeMX 재생성 후 usbd_conf.c에서 USBD_static_malloc 함수를 주석 처리 필요
 *============================================================================*/
void *USBD_static_malloc(uint32_t size)
{
    (void)size;
    return (void *)usb_cdc_handle_mem;  /* RAM_D1에 배치된 버퍼 반환 */
}
