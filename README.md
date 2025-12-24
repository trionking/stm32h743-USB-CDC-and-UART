# STM32H743 USB CDC & UART 통신 프로젝트

STM32H743ZITx 기반 LED 투명 디스플레이 컨트롤러 프로젝트입니다.
USB CDC (Virtual COM Port)와 다중 UART 통신을 지원하며, STM32H7의 복잡한 메모리 아키텍처 문제를 해결한 레퍼런스 코드를 포함합니다.

## 주요 특징

- **USB CDC** (Virtual COM Port) 지원
- **다중 UART** 통신 (UART1, UART2, UART4)
- **STM32H7 메모리 아키텍처** 최적화
- **DMA 기반** 고속 데이터 전송
- **CubeMX 재생성 대응** 코드 구조

## 하드웨어

| 항목 | 사양 |
|------|------|
| MCU | STM32H743ZITx |
| 코어 | ARM Cortex-M7 @ 480MHz |
| Flash | 2MB |
| RAM | 총 1056KB (아래 상세) |

### RAM 상세 구성

| 영역 | 주소 | 크기 | 버스 | 특징 |
|------|------|------|------|------|
| ITCMRAM | 0x00000000 | 64KB | TCM | 고속 명령어 실행, 0-wait |
| DTCMRAM | 0x20000000 | 128KB | TCM | 고속 데이터, 스택/힙, 0-wait |
| RAM_D1 (AXI SRAM) | 0x24000000 | 512KB | AXI | USB/SDMMC 접근 가능 |
| RAM_D2 (AHB SRAM) | 0x30000000 | 288KB | AHB | DMA1/DMA2 접근 가능 |
| RAM_D3 (AHB SRAM) | 0x38000000 | 64KB | AHB | BDMA 전용, 저전력 모드 유지 |

### 메모리 구성도

```
                    ┌─────────────────────────────────────────────────────────────┐
                    │                         CPU (Cortex-M7)                      │
                    │                           480MHz                             │
                    └───────┬───────────────────────┬───────────────────┬─────────┘
                            │                       │                   │
                       TCM Bus                  AXI Bus              AHB Bus
                      (0-wait)                 (64-bit)             (32-bit)
                            │                       │                   │
         ┌──────────────────┼───────────────────────┼───────────────────┼──────────────────┐
         │                  │                       │                   │                  │
         ▼                  ▼                       ▼                   ▼                  ▼
┌─────────────────┐ ┌─────────────────┐   ┌─────────────────┐  ┌─────────────────┐ ┌─────────────────┐
│     ITCM        │ │     DTCM        │   │     RAM_D1      │  │     RAM_D2      │ │     RAM_D3      │
│   0x00000000    │ │   0x20000000    │   │   0x24000000    │  │   0x30000000    │ │   0x38000000    │
│     64KB        │ │     128KB       │   │     512KB       │  │     288KB       │ │     64KB        │
├─────────────────┤ ├─────────────────┤   ├─────────────────┤  ├─────────────────┤ ├─────────────────┤
│ 고속 코드 실행  │ │ 스택/힙/변수    │   │ NC: 384KB       │  │ UART DMA        │ │ BDMA 전용       │
│ (인터럽트 등)   │ │ (기본 RAM)      │   │ Cached: 128KB   │  │ SPI DMA         │ │                 │
├─────────────────┤ ├─────────────────┤   ├─────────────────┤  │ I2C DMA         │ │ • ADC3          │
│                 │ │                 │   │ • USB OTG       │  │ ADC1/2 DMA      │ │ • LPUART1       │
│ CPU만 접근 가능 │ │ CPU만 접근 가능 │   │ • SDMMC         │  ├─────────────────┤ │ • I2C4          │
│                 │ │                 │   │ • FMC           │  │                 │ │ • SPI6          │
│ DMA 접근 불가   │ │ DMA 접근 불가   │   │ • 대용량 데이터 │  │ DMA1/DMA2 접근  │ │                 │
└─────────────────┘ └─────────────────┘   └────────┬────────┘  └────────┬────────┘ └────────┬────────┘
                                                   │                    │                   │
                                                   ▼                    ▼                   ▼
                                          ┌───────────────┐    ┌───────────────┐   ┌───────────────┐
                                          │   USB OTG     │    │   DMA1/DMA2   │   │     BDMA      │
                                          │   SDMMC       │    │               │   │               │
                                          │   (IDMA)      │    │   D2 도메인   │   │   D3 도메인   │
                                          └───────────────┘    └───────────────┘   └───────────────┘
```

### 메모리 주소 맵

```
  0x00000000 ┌────────────┐
             │   ITCM     │ 64KB  ← 고속 코드
  0x00010000 └────────────┘

  0x08000000 ┌────────────┐
             │   FLASH    │ 2MB   ← 프로그램 저장
  0x08200000 └────────────┘

  0x20000000 ┌────────────┐
             │   DTCM     │ 128KB ← 스택(16KB) + 힙(32KB) + 변수(~80KB)
  0x20020000 └────────────┘

  0x24000000 ┌────────────┐
             │ RAM_D1 NC  │ 384KB ← USB, FMC, SDMMC 버퍼 (Non-cacheable)
  0x24060000 ├────────────┤
             │ RAM_D1 C   │ 128KB ← 대용량 데이터 (Cacheable)
  0x24080000 └────────────┘

  0x30000000 ┌────────────┐
             │   RAM_D2   │ 288KB ← UART/SPI/I2C DMA 버퍼
  0x30048000 └────────────┘

  0x38000000 ┌────────────┐
             │   RAM_D3   │ 64KB  ← ADC3, LPUART1, I2C4, SPI6 (BDMA)
  0x38010000 └────────────┘
```

## 메모리 구성

STM32H7의 다중 메모리 영역을 활용한 최적화 구성:

| 영역 | 주소 | 크기 | 용도 |
|------|------|------|------|
| DTCMRAM | 0x20000000 | 128KB | 스택, 힙, 일반 변수 |
| RAM_D1_NC | 0x24000000 | 384KB | USB, FMC 버퍼 (Non-cacheable) |
| RAM_D1_CACHED | 0x24060000 | 128KB | 대용량 데이터 |
| RAM_D2 | 0x30000000 | 288KB | UART DMA 버퍼 |
| RAM_D3 | 0x38000000 | 64KB | BDMA 버퍼 (D3 도메인 전용) |

### 링커 스크립트로 RAM 자동 할당 설정

기본적으로 일반 변수는 DTCM에 할당됩니다. 자동으로 다른 RAM에 할당하려면 링커 스크립트를 수정할 수 있습니다.

#### 방법 1: 기본 RAM을 RAM_D1_CACHED로 변경

`STM32H743ZITX_FLASH.ld`에서:

```ld
/* 변경 전 */
MEMORY
{
  RAM (xrw)   : ORIGIN = 0x20000000, LENGTH = 128K   /* DTCM */
}

/* 변경 후 - RAM_D1_CACHED를 기본으로 */
MEMORY
{
  DTCM (xrw)  : ORIGIN = 0x20000000, LENGTH = 128K
  RAM (xrw)   : ORIGIN = 0x24060000, LENGTH = 128K   /* RAM_D1_CACHED */
}
```

이렇게 하면 일반 변수가 자동으로 RAM_D1_CACHED에 할당됩니다.

#### 방법 2: DTCM + RAM_D1 연결 (Overflow)

```ld
SECTIONS
{
  .bss :
  {
    *(.bss*)
  } >DTCM AT> DTCM

  /* DTCM 초과 시 RAM_D1_CACHED로 */
  .bss_ext :
  {
    *(.large_bss*)
  } >RAM_D1_CACHED
}
```

#### 현실적인 선택

| 방법 | 장점 | 단점 |
|------|------|------|
| 현재 (수동 지정) | 메모리 위치 명확 | 매번 속성 지정 필요 |
| 기본 RAM 변경 | 자동 할당 | 스택도 이동됨, 약간 느림 |
| 스택만 DTCM 유지 | 성능 + 편의성 | 링커 스크립트 복잡 |

> **추천**: 현재처럼 DTCM을 기본으로 두고, 큰 버퍼만 `SECTION_CACHED`로 명시하는 게 가장 안전합니다. 어차피 큰 버퍼는 몇 개 안 되니까요.

## 주변장치별 메모리 제한 (중요!)

STM32H7은 버스 구조상 **특정 주변장치는 특정 RAM 영역만 접근 가능**합니다.

### DMA 컨트롤러별 접근 가능 메모리

| DMA | DTCM | RAM_D1 | RAM_D2 | RAM_D3 |
|-----|------|--------|--------|--------|
| MDMA | ✅ | ✅ | ✅ | ✅ |
| DMA1/DMA2 | ❌ | ✅ | ✅ | ❌ |
| BDMA | ❌ | ❌ | ❌ | ✅ |
| SDMMC IDMA | ❌ | ✅ | ❌ | ❌ |

### 주변장치별 필수 RAM 영역

| 주변장치 | 사용 DMA | 필수 RAM | 비고 |
|----------|----------|----------|------|
| **USB OTG FS/HS** | - | **RAM_D1** | AXI 버스 직접 접근 |
| **SDMMC1/2** | IDMA | **RAM_D1** | 내장 IDMA 사용 |
| **ADC3** | BDMA | **RAM_D3** | D3 도메인 전용 |
| **LPUART1** | BDMA | **RAM_D3** | D3 도메인 전용 |
| **I2C4** | BDMA | **RAM_D3** | D3 도메인 전용 |
| **SPI6** | BDMA | **RAM_D3** | D3 도메인 전용 |
| ADC1/2 | DMA1/2 | RAM_D1 또는 RAM_D2 | |
| UART/USART | DMA1/2 | RAM_D1 또는 RAM_D2 | |
| SPI1~5 | DMA1/2 | RAM_D1 또는 RAM_D2 | |
| I2C1~3 | DMA1/2 | RAM_D1 또는 RAM_D2 | |

> **주의**: 잘못된 메모리 영역에 버퍼를 배치하면 **Hard Fault** 또는 **데이터 전송 실패**가 발생합니다.

### D3 도메인 특징

D3 도메인 주변장치(ADC3, LPUART1, I2C4, SPI6)는:
- **BDMA만 사용 가능** (DMA1/DMA2 사용 불가)
- **RAM_D3에만 버퍼 배치 가능**
- **저전력 모드(Standby)에서도 동작 가능** → 웨이크업 트리거로 활용

```c
// ADC3 DMA 버퍼 예시
__attribute__((section(".ram_d3"), aligned(32)))
uint16_t adc3_buffer[256];
```

## USB CDC 설정 (중요!)

### 1. Internal DMA 설정

현재 프로젝트에서는 USB OTG FS Internal DMA를 **비활성화**하여 사용 중입니다.

```c
// usbd_conf.c
hpcd_USB_OTG_FS.Init.dma_enable = DISABLE;
```

> **참고**: Internal DMA 활성화 시 추가 설정이 필요할 수 있습니다.
> DMA 활성화를 원하는 경우 ST 공식 문서 및 예제를 참고하세요.

### 2. 메모리 제약 - Hard Fault 방지

**USB OTG FS는 AXI 버스만 사용하므로 DTCM(0x20000000)에 접근 불가**

```
CPU ────► DTCM     ✓ 접근 가능
USB OTG ──► DTCM   ✗ Hard Fault!
USB OTG ──► RAM_D1 ✓ AXI 버스로 접근 가능
```

### 해결 방법

1. **USB 버퍼를 RAM_D1에 배치** (`buffers.c`)
```c
SECTION_NC  // RAM_D1 Non-cacheable
uint8_t usb_cdc_tx_buffer[2048];
uint8_t usb_cdc_rx_buffer[2048];
```

2. **CDC_Transmit_FS에서 RAM_D1 버퍼로 복사 후 전송** (`usbd_cdc_if.c`)
```c
memcpy(usb_cdc_tx_buffer, Buf, Len);
USBD_CDC_SetTxBuffer(&hUsbDeviceFS, usb_cdc_tx_buffer, Len);
```

3. **USBD_static_malloc을 RAM_D1 버퍼 반환하도록 구현** (`buffers.c`)
```c
void *USBD_static_malloc(uint32_t size) {
    return (void *)usb_cdc_handle_mem;  // RAM_D1 버퍼
}
```

## 프로젝트 구조

```
├── Core/
│   ├── Src/
│   │   ├── main.c           # 메인 진입점
│   │   ├── user_def.c       # 사용자 로직
│   │   ├── user_com.c       # UART/USB 통신
│   │   └── buffers.c        # DMA 버퍼 (메모리 배치)
│   └── Inc/
│       ├── memory_config.h  # 메모리 섹션 매크로
│       └── buffers.h        # 버퍼 extern 선언
├── USB_DEVICE/
│   ├── App/usbd_cdc_if.c    # CDC 인터페이스 (수정됨)
│   └── Target/usbd_conf.c   # USB 설정
├── Drivers/                  # HAL 드라이버
├── Middlewares/              # USB, FatFs 라이브러리
└── .doc/                     # 기술 문서
```

## 빌드 방법

### STM32CubeIDE
1. File > Import > Existing Projects into Workspace
2. 프로젝트 폴더 선택
3. Project > Build Project

### 커맨드라인
```bash
cd Debug
make all
```

## CubeMX 재생성 시 주의사항

CubeMX로 코드 재생성 후 **반드시** 아래 사항을 확인하고 수정해야 합니다.

### 1. USBD_static_malloc 함수 비활성화 (필수!)

**파일**: `USB_DEVICE/Target/usbd_conf.c`

CubeMX가 재생성하는 `USBD_static_malloc` 함수는 DTCM 영역에 버퍼를 할당하므로 **Hard Fault**를 유발합니다.
이 함수를 `#if 0`으로 비활성화해야 `buffers.c`의 RAM_D1 버전이 사용됩니다.

**수정 전** (CubeMX 재생성 직후):
```c
void *USBD_static_malloc(uint32_t size)
{
  UNUSED(size);
  static uint32_t mem[(sizeof(USBD_CDC_HandleTypeDef)/4)+1];/* On 32-bit boundary */
  return mem;
}
```

**수정 후**:
```c
/* [주의] CubeMX 재생성 후 이 함수를 #if 0으로 비활성화할 것!
 * buffers.c에서 RAM_D1 영역의 버퍼를 사용하는 버전으로 대체됨
 * USB OTG FS는 DTCM 접근 불가 - RAM_D1 필수
 */
#if 0  /* buffers.c에서 정의됨 */
void *USBD_static_malloc(uint32_t size)
{
  UNUSED(size);
  static uint32_t mem[(sizeof(USBD_CDC_HandleTypeDef)/4)+1];/* On 32-bit boundary */
  return mem;
}
#endif
```

> **위치**: 파일 끝부분 (약 640~660번째 줄 근처)

### 2. USB Internal DMA 설정 확인

**파일**: `USB_DEVICE/Target/usbd_conf.c`

CubeMX에서 "Enable internal IP DMA" 옵션을 변경하면 `dma_enable` 값이 바뀝니다.
현재 프로젝트는 **Internal DMA 비활성화** 상태에서 동작 확인되었습니다.

```c
// HAL_PCD_MspInit 함수 내부 (약 350번째 줄)
hpcd_USB_OTG_FS.Init.dma_enable = DISABLE;  // 현재 설정
```

| CubeMX 설정 | 생성되는 코드 | 상태 |
|-------------|---------------|------|
| Enable internal IP DMA: **체크 해제** | `dma_enable = DISABLE` | ✅ 동작 확인됨 |
| Enable internal IP DMA: **체크** | `dma_enable = ENABLE` | ⚠️ 추가 설정 필요 |

> **참고**: Internal DMA 활성화 시 추가 설정이 필요할 수 있습니다.
> 활성화를 원하는 경우 ST 공식 문서 및 예제를 참고하세요.

### 체크리스트

CubeMX 재생성 후:

- [ ] `usbd_conf.c`에서 `USBD_static_malloc` 함수를 `#if 0`으로 감싸기
- [ ] `usbd_conf.c`의 `dma_enable` 값 확인 (DISABLE 권장)
- [ ] 빌드 후 USB CDC 포트 생성 확인

`USER CODE` 섹션 내의 코드는 자동으로 보존됩니다.

## 문서

- [메모리 구성 가이드](.doc/STM32H743_LED_Matrix_Memory_Config.md)
- [USB CDC Hard Fault 분석](.doc/USB_CDC_HardFault_Analysis_20251223_1530.md)

## 라이선스

이 프로젝트는 ST 드라이버 및 미들웨어의 라이선스를 따릅니다.

## 참고 자료

- [STM32H743 Reference Manual (RM0433)](https://www.st.com/resource/en/reference_manual/rm0433-stm32h742-stm32h743753-and-stm32h750-value-line-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [STM32H7 Memory Architecture AN4891](https://www.st.com/resource/en/application_note/an4891-stm32h7-memory-configuration-stmicroelectronics.pdf)
