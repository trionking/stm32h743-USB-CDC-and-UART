/**
 * @file    memory_config.h
 * @brief   STM32H743 메모리 영역 및 섹션 매크로 정의
 * @date    2025-12-23
 */

#ifndef MEMORY_CONFIG_H
#define MEMORY_CONFIG_H

#include <stdint.h>

/*==============================================================================
 * 메모리 영역 주소 정의
 *============================================================================*/
#define DTCM_BASE           0x20000000
#define RAM_D1_NC_BASE      0x24000000   /* Non-cacheable */
#define RAM_D1_CACHED_BASE  0x24060000   /* Cacheable */
#define RAM_D2_BASE         0x30000000   /* Non-cacheable */
#define RAM_D3_BASE         0x38000000   /* Non-cacheable */

/*==============================================================================
 * 섹션 매크로 (캐시 정책별)
 *============================================================================*/
/* DTCM - 캐시 bypass, 최고속 */
#define SECTION_DTCM        __attribute__((section(".bss")))

/* RAM_D1 Non-Cacheable - FMC/SD-Card DMA */
#define SECTION_NC          __attribute__((section(".ram_d1_nc"), aligned(32)))
#define SECTION_FMC         __attribute__((section(".fmc_buffer"), aligned(32)))
#define SECTION_SDMMC       __attribute__((section(".sdmmc_buffer"), aligned(32)))

/* RAM_D1 Cacheable - 성능 필요한 큰 데이터 */
#define SECTION_CACHED      __attribute__((section(".ram_d1_cached"), aligned(32)))
#define SECTION_LARGE_BSS   __attribute__((section(".large_bss"), aligned(32)))

/* RAM_D2 - DMA 버퍼 (Non-cacheable) */
#define SECTION_DMA         __attribute__((section(".ram_d2"), aligned(32)))
#define SECTION_UART_DMA    __attribute__((section(".uart_dma"), aligned(32)))
#define SECTION_AUDIO_DMA   __attribute__((section(".audio_dma"), aligned(32)))
#define SECTION_USB         __attribute__((section(".usb_buffer"), aligned(32)))

/* RAM_D3 - BDMA */
#define SECTION_BDMA        __attribute__((section(".bdma_buffer"), aligned(32)))

/* ITCM - 고속 실행 코드 */
#define SECTION_ITCM        __attribute__((section(".itcm_text")))

/*==============================================================================
 * CP151AD-TW LED Matrix 설정
 *============================================================================*/
#define NUM_LEDS_PER_LINE       160
#define NUM_LINES               64
#define NUM_LATCHES             4
#define BITS_PER_LED            48
#define GAIN_BITS               16
#define RESET_BITS              64
#define PHASES_PER_BIT          3

#define BITS_PER_LINE           (NUM_LEDS_PER_LINE * BITS_PER_LED + GAIN_BITS)
#define TOTAL_BITS_WITH_RESET   (BITS_PER_LINE + RESET_BITS)
#define FMC_BUFFER_SIZE         (TOTAL_BITS_WITH_RESET * NUM_LATCHES * PHASES_PER_BIT)
#define FMC_BUFFER_BYTES        (FMC_BUFFER_SIZE * sizeof(uint16_t))

/*==============================================================================
 * DMA 버퍼 크기
 * 참고: UART 버퍼 크기는 user_com.h에서 정의됨
 *============================================================================*/
#define AUDIO_BUFFER_SIZE       (16 * 1024)

#endif /* MEMORY_CONFIG_H */
