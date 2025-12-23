# CLAUDE.md

이 파일은 Claude Code (claude.ai/code)가 이 저장소의 코드를 작업할 때 참고하는 가이드입니다.

## 프로젝트 개요

**STM32H743ZITx** 마이크로컨트롤러 기반의 LED 사인 투명 디스플레이 컨트롤러용 STM32CubeIDE 프로젝트입니다. 프로젝트명은 `ld_transp_v100` (LED Transparency v1.0.0)입니다.

## 빌드 명령

**중요**: 빌드할 경우 `.vscode/tasks.json` 파일을 참고할 것. 환경 변수 `STM32_CUBE_IDE_PATH`, `STM32_CUBE_CLT_PATH`는 시스템에 이미 정의되어 있음.

커맨드라인 빌드 (GNU Tools for STM32 13.3.rel1):
```bash
cd Debug && make all      # Debug 빌드
cd Debug && make clean    # 클린
```

STM32CubeIDE에서 빌드:
- **Debug 빌드**: Project > Build Configurations > Set Active > Debug 선택 후, Project > Build Project
- **Release 빌드**: Project > Build Configurations > Set Active > Release 선택 후, Project > Build Project

MCU ARM GCC 툴체인 사용, 컴파일러 정의:
- `USE_PWR_LDO_SUPPLY`
- `USE_HAL_DRIVER`
- `STM32H743xx`

## 아키텍처

### 디렉토리 구조

- `Core/` - 메인 애플리케이션 코드
  - `Core/Src/main.c` - STM32CubeMX 생성 진입점, HAL 초기화
  - `Core/Src/user_def.c` - 사용자 애플리케이션 로직 (`run_proc()`에서 시작)
  - `Core/Inc/user_def.h` - 사용자 정의 함수 선언
- `Drivers/` - STM32 HAL 및 CMSIS 라이브러리 (자동 생성, 수정 금지)
- `Middlewares/` - 서드파티 라이브러리 (FatFs, USB Device Library)
- `FATFS/` - FatFs 설정 및 SD 카드 드라이버
- `USB_DEVICE/` - USB CDC (가상 COM 포트) 설정

### 애플리케이션 흐름

1. `main()`에서 주변장치 초기화 (GPIO, FMC, UART) 후 `run_proc()` 호출
2. `run_proc()` (`user_def.c`)에 메인 애플리케이션 로직 구현
3. 사용자 코드는 `user_def.c` 또는 `Core/Src/`의 새 파일에 작성

### 설정된 주변장치

- **UART4, USART1, USART2**: 115200 baud, 8N1 (시리얼 통신)
- **FMC/SRAM**: 외부 메모리 인터페이스 (Bank 1, 16비트 폭)
- **SDMMC1**: SD 카드 인터페이스 (현재 코드에서 비활성화)
- **USB OTG FS**: CDC 가상 COM 포트 (현재 코드에서 비활성화)
- **GPIO 출력**: `OT_LD_SYS` (PE3), `OT_LD_REV` (PE4), `OT_SD_EN` (PG9)
- **GPIO 입력**: `IN_nSD_CD` (PG12 - SD 카드 감지), `IN_SW0` (PE1 - 스위치)

### 링커 스크립트

- `STM32H743ZITX_FLASH.ld` - 내부 플래시에 프로그램 저장 (기본값)
- `STM32H743ZITX_RAM.ld` - RAM 전용 실행

### 메모리 배치

| 영역 | 주소 | 크기 | 용도 |
|------|------|------|------|
| DTCMRAM | 0x20000000 | 128KB | .data, .bss, 스택(16KB), 힙(32KB) - 캐시 bypass |
| RAM_D1_NC | 0x24000000 | 384KB | FMC/SDMMC DMA 버퍼 (Non-cacheable) |
| RAM_D1_CACHED | 0x24060000 | 128KB | 확장 BSS, 프레임 버퍼 (D-Cache 활용) |
| RAM_D2 | 0x30000000 | 288KB | UART/Audio/USB DMA 버퍼 |
| RAM_D3 | 0x38000000 | 64KB | BDMA 버퍼 |
| ITCMRAM | 0x00000000 | 64KB | 고속 실행 코드 |

### 메모리 섹션 매크로 (C 코드에서 사용)

```c
// FMC 버퍼 (Non-cacheable)
__attribute__((section(".fmc_buffer"), aligned(32))) uint16_t buf[SIZE];

// DMA 버퍼
__attribute__((section(".uart_dma"), aligned(32))) uint8_t uart_buf[SIZE];
__attribute__((section(".audio_dma"), aligned(32))) uint8_t audio_buf[SIZE];
__attribute__((section(".usb_buffer"), aligned(32))) uint8_t usb_buf[SIZE];

// ITCM 고속 함수
__attribute__((section(".itcm_text"))) void fast_func(void);

// 확장 BSS (Cacheable)
__attribute__((section(".large_bss"), aligned(32))) uint8_t large_data[SIZE];
```

### LED 매트릭스 하드웨어

- **LED**: CP151AD-TW (48bit RGB + 16bit Current Gain)
- **매트릭스**: 160 × 64 (10,240개)
- **인터페이스**: FMC 16-bit + 74HCT138 래치 확장 (4개 래치, 각 16라인)
- **FMC 더블버퍼**: 약 364KB (SD-Card에서 직접 수신)

### DMA 접근 제한

| 메모리 | DMA1/DMA2 | MDMA | BDMA | SDMMC IDMA |
|--------|-----------|------|------|------------|
| DTCMRAM | ❌ | ✅ | ❌ | ❌ |
| RAM_D1 | ✅ | ✅ | ❌ | ✅ |
| RAM_D2 | ✅ | ✅ | ❌ | ✅ |
| RAM_D3 | ❌ | ✅ | ✅ | ❌ |

### USB OTG FS 메모리 제약 (중요!)

**USB OTG FS는 AXI 버스만 사용하므로 DTCM(0x20000000)에 접근 불가!**

```
CPU ────► DTCM     ✓ 접근 가능
USB OTG ──► DTCM   ✗ Hard Fault 발생!
USB OTG ──► RAM_D1 ✓ AXI 버스로 접근 가능
```

**필수 조치사항:**
1. USB 관련 모든 버퍼는 `SECTION_NC` (RAM_D1)에 배치
2. `CDC_Transmit_FS()`에서 RAM_D1 버퍼로 복사 후 전송
3. `USBD_static_malloc()`은 `buffers.c`에서 RAM_D1 버퍼 반환하도록 구현

**관련 파일:**
- `buffers.c`: `usb_cdc_tx_buffer`, `usb_cdc_rx_buffer`, `usb_cdc_handle_mem`
- `usbd_cdc_if.c`: USER CODE BEGIN 7 섹션에서 `memcpy` 후 전송
- `usbd_conf.c`: 원본 `USBD_static_malloc` 함수 `#if 0`으로 비활성화

**CubeMX 재생성 후 체크리스트:**
- [ ] `usbd_conf.c`의 `USBD_static_malloc` 함수가 `#if 0`으로 비활성화 되어있는지 확인

상세 분석: `.doc/USB_CDC_HardFault_Analysis_20251223_1530.md`

## 개발 가이드라인

### STM32CubeMX 연동

`.ioc` 파일 (`ld_transp_v100.ioc`)에 MCU 설정 포함. 코드 재생성 시:
- `/* USER CODE BEGIN */`과 `/* USER CODE END */` 마커 사이의 사용자 코드는 보존됨
- STM32CubeMX 생성 파일에는 이 마커 내부에만 코드 추가
- 복잡한 로직은 `user_def.c`와 같은 별도 파일에 작성 권장

### 클럭 설정

시스템 클럭: 400 MHz (8 MHz HSE에서 PLL 통해 생성)
- SYSCLK: 400 MHz
- AHB: 200 MHz
- APB1/APB2/APB3/APB4: 100 MHz

## 참고 문서

- `.doc/STM32H743_LED_Matrix_Memory_Config.md` - 메모리 구성 상세 가이드
