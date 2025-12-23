# STM32H743 CP151AD-TW LED 매트릭스 메모리 구성 가이드

## 프로젝트 개요

- **MCU**: STM32H743
- **LED**: CP151AD-TW (16-bit RGB, 48bit/LED + 16bit Gain)
- **매트릭스 크기**: 160 × 64 (10,240개)
- **인터페이스**: FMC 16-bit + 74HCT138 래치 확장
- **추가 기능**: SD-Card, UART, Audio, USB-CDC

---

## 1. CP151AD-TW LED 사양

### 1.1 데이터 구조

```
LED 데이터 (48bit): R[15:0] + G[15:0] + B[15:0]  (MSB first, RGB 순서)
Current Gain (16bit): GR[4:0] + GG[4:0] + GB[4:0] + Reserved
```

### 1.2 타이밍 사양

| 파라미터 | 심볼 | 표준값 | 단위 |
|----------|------|--------|------|
| 비트 주기 | TDATA | 1.25 | µs |
| 코드 0 High | T0H | 0.24 | µs |
| 코드 0 Low | T0L | 0.48 | µs |
| 코드 1 High | T1H | 0.48 | µs |
| 코드 1 Low | T1L | 0.24 | µs |
| 리셋 시간 | Tres | 80 | µs (최소) |

### 1.3 CP151AD-TW 비트 인코딩 (3단계)

```
비트 '0':  [High][Low][Low]   → 0.24µs + 0.48µs + 0.48µs
비트 '1':  [High][High][Low]  → 0.48µs + 0.48µs + 0.24µs

1비트 = 3번의 FMC Write 필요
```

---

## 2. 하드웨어 구성

### 2.1 FMC + 74HCT138 래치 확장

```
                    74HCT138 (Latch Select)
                         │
    FMC A[1:0] ─────────┤ A, B
    FMC NWE ────────────┤ Enable
                         │
                    ┌────┴────┐
                    ▼    ▼    ▼    ▼
               Latch0 Latch1 Latch2 Latch3
                 │      │      │      │
FMC D[15:0] ════╪══════╪══════╪══════╪════
                 │      │      │      │
                 ▼      ▼      ▼      ▼
              16라인  16라인  16라인  16라인
              (0~15) (16~31) (32~47) (48~63)
                 │      │      │      │
                 └──────┴──────┴──────┘
                    각 라인에 160개 LED
```

### 2.2 데이터 흐름

```
    SD-Card                      RAM_D1                         FMC
  ┌─────────┐                 ┌──────────┐                  ┌─────────┐
  │ 미리    │    SDMMC+IDMA   │ Buffer 0 │ ◄── 쓰기 중      │ 74HCT138│
  │ 계산된  │ ──────────────► │ (186KB)  │                  │    │    │
  │ LED     │                 ├──────────┤                  │  Latch  │
  │ 데이터  │                 │ Buffer 1 │ ──► 출력 중 ──► │  0~3    │
  │ 파일    │                 │ (186KB)  │      FMC+DMA     │    │    │
  └─────────┘                 └──────────┘                  │    ▼    │
                                   │                       │ 64 Lines│
                                   │ 버퍼 스왑             │ ×160LED │
                                   ▼                       └─────────┘
                              DMA 완료 시
                              인덱스 교환
```

---

## 3. 버퍼 크기 계산

### 3.1 LED 데이터 계산

| 항목 | 계산 |
|------|------|
| LED당 비트 | 48 bits |
| 라인당 LED | 160개 |
| 라인당 LED 비트 | 160 × 48 = 7,680 bits |
| Current Gain | 16 bits |
| Reset 비트 | 64 bits |
| **라인당 총 비트** | **7,760 bits** |

### 3.2 FMC 버퍼 계산

```
1비트 출력에 필요한 메모리:
- 16bit FMC × 4 래치 × 3단계 = 24 bytes

전체 버퍼:
- 7,760 bits × 24 bytes = 186,240 bytes (단일 버퍼)
- 더블 버퍼: 186,240 × 2 = 372,480 bytes (≈364 KB)
```

### 3.3 기타 DMA 버퍼

| 용도 | 크기 |
|------|------|
| UART TX DMA | 16 KB |
| UART RX DMA | 16 KB |
| Audio DMA | 16 KB |
| USB-CDC TX/RX | 8 KB |
| **합계** | **~56 KB** |

---

## 4. STM32H743 메모리 구조

### 4.1 메모리 영역 특성

| 영역 | 주소 | 크기 | 버스 | DMA 접근 | 특징 |
|------|------|------|------|----------|------|
| ITCM | 0x00000000 | 64KB | TCM | MDMA만 | 코드 실행 최적화 |
| DTCM | 0x20000000 | 128KB | TCM | MDMA만 | 데이터 접근 최고속 |
| RAM_D1 (AXI) | 0x24000000 | 512KB | AXI 64-bit | 모든 DMA | 고속, 캐시 효과 좋음 |
| RAM_D2 | 0x30000000 | 288KB | AHB 32-bit | DMA1/2 | 중속 |
| RAM_D3 | 0x38000000 | 64KB | AHB 32-bit | BDMA만 | 저전력 유지 |

### 4.2 버스 성능 비교

| 메모리 | 캐시 히트 | 캐시 미스 | 비고 |
|--------|----------|----------|------|
| DTCM | 0 cycle | 0 cycle | 캐시 bypass, 항상 최고속 |
| RAM_D1 (AXI) | 0 cycle | ~6 cycle | 캐시 효과 좋음 |
| RAM_D2 (AHB) | 0 cycle | ~12 cycle | 캐시 효과 제한적 |

---

## 5. 메모리 배치 전략

### 5.1 최종 메모리 맵

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         STM32H743 메모리 배치                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  DTCM (128KB) @ 0x20000000  [캐시 Bypass - 항상 최고속]                      │
│  ├─ Stack              16 KB                                                │
│  ├─ Heap               32 KB                                                │
│  ├─ .bss (핵심 변수)   ~48 KB                                               │
│  └─ .data              ~16 KB                                               │
│                                                                             │
│  RAM_D1 (512KB) @ 0x24000000                                                │
│  ├─ [0x24000000] Non-Cacheable  384 KB  ← FMC 더블버퍼 (SD-Card 직접 수신)  │
│  └─ [0x24060000] Cacheable      128 KB  ← 확장 BSS (D-Cache 활용)           │
│                                                                             │
│  RAM_D2 (288KB) @ 0x30000000  [Non-Cacheable]                               │
│  ├─ UART DMA 버퍼        32 KB                                              │
│  ├─ Audio DMA 버퍼       16 KB                                              │
│  └─ USB-CDC 버퍼          8 KB                                              │
│                                                                             │
│  RAM_D3 (64KB) @ 0x38000000  [Non-Cacheable]                                │
│  └─ BDMA 전용 / 저전력 유지                                                 │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 5.2 MPU 우선순위를 이용한 캐시 영역 분리

```
MPU Region 번호가 높을수록 우선순위가 높음
→ 겹치는 영역에서는 높은 번호의 설정이 적용됨

Region 0 (낮은 우선순위): RAM_D1 전체 512KB → Non-Cacheable
Region 1 (높은 우선순위): 뒤쪽 128KB → Cacheable (override)

결과:
┌────────────────────────────────────────────────────────────────┐
│ 0x24000000                              0x24060000  0x2407FFFF │
│ ├──────────── 384KB ────────────────────┼────── 128KB ────────┤│
│ │         Non-Cacheable                 │     Cacheable       ││
│ │         (Region 0 적용)               │   (Region 1 적용)   ││
│ │         FMC/SD-Card DMA               │   확장 BSS          ││
│ └───────────────────────────────────────┴─────────────────────┘│
└────────────────────────────────────────────────────────────────┘
```

---

## 6. 링커 스크립트

### 6.1 STM32H743_LED_Matrix.ld

```ld
MEMORY
{
  FLASH         (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K
  ITCMRAM       (xrw) : ORIGIN = 0x00000000, LENGTH = 64K
  DTCMRAM       (xrw) : ORIGIN = 0x20000000, LENGTH = 128K
  
  /* RAM_D1 분할: Non-cache (FMC) + Cacheable (확장 BSS) */
  RAM_D1_NC     (xrw) : ORIGIN = 0x24000000, LENGTH = 384K
  RAM_D1_CACHED (xrw) : ORIGIN = 0x24060000, LENGTH = 128K
  
  /* RAM_D2: DMA 전용 Non-cacheable */
  RAM_D2        (xrw) : ORIGIN = 0x30000000, LENGTH = 288K
  
  /* RAM_D3: BDMA/저전력 */
  RAM_D3        (xrw) : ORIGIN = 0x38000000, LENGTH = 64K
}

_Min_Heap_Size  = 0x8000;  /* 32KB */
_Min_Stack_Size = 0x4000;  /* 16KB */

SECTIONS
{
  /* ============== FLASH ============== */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.rodata)
    *(.rodata*)
    *(.glue_7)
    *(.glue_7t)
    KEEP(*(.init))
    KEEP(*(.fini))
    . = ALIGN(4);
  } >FLASH

  .ARM : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
  .ARM.exidx : { *(.ARM.exidx* .gnu.linkonce.armexidx.*) } >FLASH

  /* ============== ITCM - 고속 실행 코드 ============== */
  .itcm_text :
  {
    . = ALIGN(4);
    _sitcm_text = .;
    *(.itcm_text)
    *(.itcm_text*)
    . = ALIGN(4);
    _eitcm_text = .;
  } >ITCMRAM AT>FLASH

  _siitcm = LOADADDR(.itcm_text);

  /* ============== DTCM - 핵심 데이터 (캐시 bypass, 최고속) ============== */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    *(.RamFunc)
    *(.RamFunc*)
    . = ALIGN(4);
    _edata = .;
  } >DTCMRAM AT>FLASH

  _sidata = LOADADDR(.data);

  .bss (NOLOAD) :
  {
    . = ALIGN(4);
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } >DTCMRAM

  ._user_heap_stack (NOLOAD) :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >DTCMRAM

  _estack = ORIGIN(DTCMRAM) + LENGTH(DTCMRAM);

  /* ============== RAM_D1 Non-Cacheable - FMC 버퍼 ============== */
  .ram_d1_nc (NOLOAD) :
  {
    . = ALIGN(32);
    _sram_d1_nc = .;
    *(.fmc_buffer)
    *(.fmc_buffer*)
    *(.sdmmc_buffer)
    *(.ram_d1_nc)
    . = ALIGN(32);
    _eram_d1_nc = .;
  } >RAM_D1_NC

  /* ============== RAM_D1 Cacheable - 확장 BSS ============== */
  .ram_d1_cached (NOLOAD) :
  {
    . = ALIGN(32);
    _sram_d1_cached = .;
    *(.ram_d1_cached)
    *(.ram_d1_cached*)
    *(.large_bss)
    *(.frame_buffer)
    . = ALIGN(32);
    _eram_d1_cached = .;
  } >RAM_D1_CACHED

  /* ============== RAM_D2 - DMA 버퍼 (Non-cacheable) ============== */
  .ram_d2 (NOLOAD) :
  {
    . = ALIGN(32);
    _sram_d2 = .;
    *(.ram_d2)
    *(.ram_d2*)
    *(.uart_dma)
    *(.audio_dma)
    *(.usb_buffer)
    *(.dma_buffer)
    . = ALIGN(32);
    _eram_d2 = .;
  } >RAM_D2

  /* ============== RAM_D3 - BDMA (Non-cacheable) ============== */
  .ram_d3 (NOLOAD) :
  {
    . = ALIGN(32);
    _sram_d3 = .;
    *(.ram_d3)
    *(.ram_d3*)
    *(.bdma_buffer)
    . = ALIGN(32);
    _eram_d3 = .;
  } >RAM_D3

  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
```

---

## 7. MPU 설정

### 7.1 mpu_config.c

```c
#include "stm32h7xx_hal.h"

void MPU_Config(void)
{
    MPU_Region_InitTypeDef MPU_InitStruct = {0};
    
    HAL_MPU_Disable();
    
    /*--------------------------------------------------------------------------
     * Region 0: RAM_D1 전체 512KB → Non-Cacheable (낮은 우선순위)
     *------------------------------------------------------------------------*/
    MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
    MPU_InitStruct.Number           = MPU_REGION_NUMBER0;
    MPU_InitStruct.BaseAddress      = 0x24000000;
    MPU_InitStruct.Size             = MPU_REGION_SIZE_512KB;
    MPU_InitStruct.SubRegionDisable = 0x00;
    MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL1;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_DISABLE;
    MPU_InitStruct.IsShareable      = MPU_ACCESS_SHAREABLE;
    MPU_InitStruct.IsCacheable      = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    
    /*--------------------------------------------------------------------------
     * Region 1: 뒤쪽 128KB → Cacheable (높은 우선순위, Region 0 override)
     * 0x24060000 ~ 0x2407FFFF
     *------------------------------------------------------------------------*/
    MPU_InitStruct.Number           = MPU_REGION_NUMBER1;
    MPU_InitStruct.BaseAddress      = 0x24060000;
    MPU_InitStruct.Size             = MPU_REGION_SIZE_128KB;
    MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL1;
    MPU_InitStruct.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;
    MPU_InitStruct.IsCacheable      = MPU_ACCESS_CACHEABLE;
    MPU_InitStruct.IsBufferable     = MPU_ACCESS_BUFFERABLE;  /* Write-back */
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    
    /*--------------------------------------------------------------------------
     * Region 2: RAM_D2 전체 → Non-Cacheable
     *------------------------------------------------------------------------*/
    MPU_InitStruct.Number           = MPU_REGION_NUMBER2;
    MPU_InitStruct.BaseAddress      = 0x30000000;
    MPU_InitStruct.Size             = MPU_REGION_SIZE_512KB;  /* 288KB 커버 */
    MPU_InitStruct.IsShareable      = MPU_ACCESS_SHAREABLE;
    MPU_InitStruct.IsCacheable      = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    
    /*--------------------------------------------------------------------------
     * Region 3: RAM_D3 → Non-Cacheable
     *------------------------------------------------------------------------*/
    MPU_InitStruct.Number           = MPU_REGION_NUMBER3;
    MPU_InitStruct.BaseAddress      = 0x38000000;
    MPU_InitStruct.Size             = MPU_REGION_SIZE_64KB;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

void Cache_Enable(void)
{
    /* I-Cache 활성화 */
    SCB_EnableICache();
    
    /* D-Cache 활성화 */
    SCB_EnableDCache();
}

void SystemInit_PostMPU(void)
{
    MPU_Config();
    Cache_Enable();
}
```

---

## 8. 헤더 파일

### 8.1 memory_config.h

```c
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
 *============================================================================*/
#define UART_TX_BUFFER_SIZE     (16 * 1024)
#define UART_RX_BUFFER_SIZE     (16 * 1024)
#define AUDIO_BUFFER_SIZE       (16 * 1024)
#define USB_TX_BUFFER_SIZE      (4 * 1024)
#define USB_RX_BUFFER_SIZE      (4 * 1024)

#endif /* MEMORY_CONFIG_H */
```

---

## 9. 버퍼 선언

### 9.1 buffers.c

```c
#include "memory_config.h"

/*==============================================================================
 * RAM_D1 영역 - FMC LED 더블버퍼 (SD-Card에서 직접 수신)
 * 총 사용량: 186,240 × 2 = 372,480 bytes (≈364 KB)
 *============================================================================*/
SECTION_FMC
uint16_t fmc_buffer[2][FMC_BUFFER_SIZE];   /* 더블 버퍼 */

/* 버퍼 인덱스 (0 또는 1) */
volatile uint8_t fmc_write_buffer = 0;     /* SD-Card가 쓰는 버퍼 */
volatile uint8_t fmc_read_buffer = 1;      /* FMC가 읽는 버퍼 */

/*==============================================================================
 * RAM_D2 영역 - DMA 버퍼들
 * 총 사용량: 32 + 16 + 8 = 56 KB
 *============================================================================*/
/* UART DMA 버퍼 */
SECTION_UART_DMA
uint8_t uart_tx_buffer[UART_TX_BUFFER_SIZE];

SECTION_UART_DMA
uint8_t uart_rx_buffer[UART_RX_BUFFER_SIZE];

/* Audio DMA 버퍼 */
SECTION_AUDIO_DMA
uint8_t audio_buffer[AUDIO_BUFFER_SIZE];

/* USB-CDC 버퍼 */
SECTION_USB
uint8_t usb_tx_buffer[USB_TX_BUFFER_SIZE];

SECTION_USB
uint8_t usb_rx_buffer[USB_RX_BUFFER_SIZE];

/*==============================================================================
 * ITCM 영역 - 고속 실행 함수 (선택적)
 *============================================================================*/
SECTION_ITCM
void FMC_Output_IRQHandler(void)
{
    /* 시간 크리티컬한 FMC 출력 처리 */
    /* ITCM에서 실행되어 Flash 접근 지연 없음 */
}

SECTION_ITCM
void DMA_Complete_Callback(void)
{
    /* 버퍼 스왑 */
    uint8_t temp = fmc_write_buffer;
    fmc_write_buffer = fmc_read_buffer;
    fmc_read_buffer = temp;
}
```

---

## 10. 메모리 사용량 요약

| 영역 | 크기 | 캐시 | 사용량 | 여유 | 용도 |
|------|------|------|--------|------|------|
| ITCM | 64 KB | I-Cache | ~4 KB | ~60 KB | 크리티컬 코드 |
| DTCM | 128 KB | Bypass | ~80 KB | ~48 KB | 스택/힙/핵심 BSS |
| RAM_D1 (NC) | 384 KB | ❌ 없음 | ~364 KB | ~20 KB | FMC 더블버퍼 |
| RAM_D1 (Cached) | 128 KB | ✅ D-Cache | 가변 | ~128 KB | 확장 BSS |
| RAM_D2 | 288 KB | ❌ 없음 | ~56 KB | ~232 KB | DMA 버퍼들 |
| RAM_D3 | 64 KB | ❌ 없음 | 예비 | ~64 KB | BDMA |
| **합계** | **1,056 KB** | | **~504 KB** | **~552 KB** | |

---

## 11. 주의사항

### 11.1 DMA 접근 제한

| 컨트롤러 | DTCM/ITCM | RAM_D1 | RAM_D2 | RAM_D3 |
|----------|-----------|--------|--------|--------|
| DMA1/DMA2 | ❌ 불가 | ✅ 가능 | ✅ 가능 | ❌ 불가 |
| MDMA | ✅ 가능 | ✅ 가능 | ✅ 가능 | ✅ 가능 |
| BDMA | ❌ 불가 | ❌ 불가 | ❌ 불가 | ✅ 가능 |
| SDMMC IDMA | ❌ 불가 | ✅ 가능 | ✅ 가능 | ❌ 불가 |

### 11.2 MPU Region 크기 제한

MPU Region은 2^N 크기만 설정 가능:
- 32B, 64B, 128B, 256B, 512B
- 1KB, 2KB, 4KB, 8KB, 16KB, 32KB, 64KB, 128KB, 256KB, 512KB, 1MB...

### 11.3 32바이트 정렬

D-Cache 라인 크기가 32바이트이므로, DMA 버퍼는 반드시 32바이트 정렬 필요:

```c
__attribute__((aligned(32))) uint8_t buffer[SIZE];
```

---

## 12. 참고 자료

- STM32H743 Reference Manual (RM0433)
- STM32H7 AN4838: Managing memory protection unit in STM32 MCUs
- STM32H7 AN4839: Level 1 cache on STM32F7 and STM32H7 Series
- CP151AD-TW Datasheet (Sanken Optoelectronics)
