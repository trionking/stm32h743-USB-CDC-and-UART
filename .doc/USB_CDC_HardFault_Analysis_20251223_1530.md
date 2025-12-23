# STM32H743 USB CDC Hard Fault 원인 분석

**작성일**: 2025-12-23
**프로젝트**: ld_transp_v100 (LED Transparency Controller)
**MCU**: STM32H743ZITx

---

## 1. 문제 현상

USB CDC 전송 시 Hard Fault 발생:
```
위치: USB_WritePacket() → PCD_WriteEmptyTxFifo()
오류 라인: USBx_DFIFO((uint32_t)ch_ep_num) = __UNALIGNED_UINT32_READ(pSrc);
```

---

## 2. 근본 원인

### 2.1 STM32H7 메모리 아키텍처의 복잡성

STM32H743은 다중 버스 아키텍처를 가지며, 각 메모리 영역은 특정 버스로만 접근 가능합니다:

| 메모리 영역 | 주소 범위 | 버스 | 접근 가능 장치 |
|------------|----------|------|---------------|
| DTCM | 0x20000000 | TCM 전용 | CPU만 접근 가능 |
| RAM_D1 (AXI SRAM) | 0x24000000 | AXI | CPU, DMA2, USB OTG |
| RAM_D2 (AHB SRAM) | 0x30000000 | AHB | CPU, DMA1, DMA2 |
| RAM_D3 | 0x38000000 | AHB | CPU, BDMA |

### 2.2 USB OTG FS의 버스 제약

```
┌─────────────────────────────────────────────────────────────┐
│                     STM32H743 Bus Matrix                     │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   ┌──────────┐                      ┌──────────────────┐   │
│   │   CPU    │──── TCM Port ───────►│      DTCM        │   │
│   │ Cortex-M7│                      │   (0x20000000)   │   │
│   └────┬─────┘                      └──────────────────┘   │
│        │                                    ✗              │
│        │ AXI                         USB 접근 불가         │
│        ▼                                                   │
│   ┌─────────────────────────────────────────────────┐     │
│   │                   AXI Bus                        │     │
│   └───────┬─────────────────────────────┬───────────┘     │
│           │                             │                  │
│           ▼                             ▼                  │
│   ┌──────────────────┐          ┌──────────────┐          │
│   │     RAM_D1       │◄─────────│  USB OTG FS  │          │
│   │  (0x24000000)    │   AXI    │              │          │
│   │   AXI SRAM       │          └──────────────┘          │
│   └──────────────────┘                 ✓                   │
│          ✓                      USB 접근 가능              │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

**핵심 문제**: USB OTG FS는 AXI 버스 마스터이므로, **DTCM 영역에 접근할 수 없습니다**.

### 2.3 CubeMX 기본 코드의 문제점

STM32CubeMX가 생성하는 USB 관련 버퍼들은 기본적으로 `.bss` 섹션에 배치되어 **DTCM**에 위치합니다:

```c
/* usbd_cdc_if.c - CubeMX 생성 코드 */
uint8_t UserRxBufferFS[APP_RX_DATA_SIZE];  // → DTCM (.bss)
uint8_t UserTxBufferFS[APP_TX_DATA_SIZE];  // → DTCM (.bss)

/* usbd_conf.c - CubeMX 생성 코드 */
void *USBD_static_malloc(uint32_t size)
{
  static uint32_t mem[...];  // → DTCM (.bss)
  return mem;
}
```

---

## 3. 이전 프로젝트와의 차이점

### 3.1 이전에 쉽게 동작한 이유 (추정)

| 요소 | 이전 프로젝트 | 현재 프로젝트 |
|------|--------------|--------------|
| 스택/힙 위치 | RAM_D1 또는 ITCM | DTCM |
| 메모리 분할 | 단순 구조 | 복잡한 MPU 설정 |
| 링커 스크립트 | 기본 또는 수정됨 | 커스텀 (다중 영역) |
| USB 버퍼 배치 | (우연히) RAM_D1 | DTCM (기본값) |

### 3.2 현재 프로젝트에서 어려웠던 이유

1. **복잡한 메모리 레이아웃**
   - 5개의 메모리 영역 (DTCM, ITCM, RAM_D1_NC, RAM_D1_CACHED, RAM_D2, RAM_D3)
   - 각 영역별 캐시 정책 및 접근 제한

2. **다단계 버퍼 참조**
   ```
   사용자 호출: CDC_Transmit_FS(usb_stat.tx_buffer, len)
         ↓
   USBD_CDC_SetTxBuffer() → hcdc->TxBuffer = buffer
         ↓
   USBD_CDC_TransmitPacket() → USBD_LL_Transmit(hcdc->TxBuffer)
         ↓
   HAL_PCD_EP_Transmit() → ep->xfer_buff = buffer
         ↓
   USB 인터럽트 → PCD_WriteEmptyTxFifo()
         ↓
   USB_WritePacket(ep->xfer_buff)  ← Hard Fault!
   ```

3. **CubeMX 코드 재생성 문제**
   - 수정한 코드가 CubeMX 재생성 시 덮어쓰여짐
   - USER CODE 섹션만 보존됨

4. **에러 발생 위치의 모호성**
   - 하드폴트가 HAL 레벨에서 발생
   - 실제 원인은 메모리 배치 문제

---

## 4. 해결 방법

### 4.1 버퍼 배치 변경 (buffers.c)

```c
/* RAM_D1 Non-cacheable 영역에 USB 버퍼 배치 */
SECTION_NC
uint8_t usb_cdc_rx_buffer[2048];

SECTION_NC
uint8_t usb_cdc_tx_buffer[2048];

SECTION_NC
uint32_t usb_cdc_handle_mem[140];  // USBD_CDC_HandleTypeDef용

/* USBD_static_malloc 대체 */
void *USBD_static_malloc(uint32_t size)
{
    (void)size;
    return (void *)usb_cdc_handle_mem;  // RAM_D1 버퍼 반환
}
```

### 4.2 전송 함수 수정 (usbd_cdc_if.c)

```c
/* USER CODE BEGIN 7 */
USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;
if (hcdc == NULL) return USBD_FAIL;
if (hcdc->TxState != 0) return USBD_BUSY;

/* 버퍼 크기 제한 */
if (Len > sizeof(usb_cdc_tx_buffer)){
  Len = sizeof(usb_cdc_tx_buffer);
}

/* RAM_D1 버퍼로 복사 후 전송 */
memcpy(usb_cdc_tx_buffer, Buf, Len);
USBD_CDC_SetTxBuffer(&hUsbDeviceFS, usb_cdc_tx_buffer, Len);
result = USBD_CDC_TransmitPacket(&hUsbDeviceFS);
/* USER CODE END 7 */
```

### 4.3 CubeMX 재생성 대응

| 파일 | 수정 위치 | 조치 |
|------|----------|------|
| usbd_cdc_if.c | USER CODE BEGIN 7 | 자동 보존 |
| usbd_conf.c | USBD_static_malloc | `#if 0`으로 비활성화 필요 |
| buffers.c/h | 별도 파일 | CubeMX 영향 없음 |

---

## 5. 메모리 맵 요약

```
0x00000000 ┌──────────────────────┐
           │       ITCM           │ 64KB - 고속 실행 코드
0x00010000 └──────────────────────┘

0x20000000 ┌──────────────────────┐
           │       DTCM           │ 128KB - 스택, 힙, 일반 변수
           │  (USB 접근 불가!)    │ ← 문제 원인
0x20020000 └──────────────────────┘

0x24000000 ┌──────────────────────┐
           │   RAM_D1_NC          │ 384KB - FMC, USB 버퍼
           │  (Non-cacheable)     │ ← USB 버퍼 배치 위치
0x24060000 ├──────────────────────┤
           │   RAM_D1_CACHED      │ 128KB - 대용량 데이터
0x24080000 └──────────────────────┘

0x30000000 ┌──────────────────────┐
           │       RAM_D2         │ 288KB - UART DMA 버퍼
           │  (USB 접근 불가)     │
0x30048000 └──────────────────────┘

0x38000000 ┌──────────────────────┐
           │       RAM_D3         │ 64KB - BDMA
0x38010000 └──────────────────────┘
```

---

## 6. 교훈 및 권장사항

### 6.1 STM32H7 USB 개발 시 주의사항

1. **USB 버퍼는 반드시 RAM_D1(AXI SRAM)에 배치**
2. **DMA 비활성화(FIFO 모드)에서도 메모리 영역 제약 존재**
3. **CubeMX 재생성에 대비한 코드 구조 설계 필요**

### 6.2 디버깅 팁

- Hard Fault 발생 시 `pSrc` 주소 확인 → 0x20xxxxxx면 DTCM 문제
- MAP 파일에서 심볼 주소 확인
- `SECTION_NC` 매크로로 명시적 메모리 배치

### 6.3 체크리스트

- [ ] USB TX/RX 버퍼 → RAM_D1
- [ ] USBD_CDC_HandleTypeDef → RAM_D1 (via USBD_static_malloc)
- [ ] usb_stat 구조체 → RAM_D1 (SECTION_NC)
- [ ] CDC_Transmit_FS에서 RAM_D1 버퍼로 복사 후 전송

---

## 7. 관련 파일

| 파일 | 역할 |
|------|------|
| `Core/Src/buffers.c` | RAM_D1 버퍼 정의 |
| `Core/Inc/buffers.h` | extern 선언 |
| `Core/Inc/memory_config.h` | SECTION 매크로 정의 |
| `USB_DEVICE/App/usbd_cdc_if.c` | CDC 인터페이스 (수정됨) |
| `USB_DEVICE/Target/usbd_conf.c` | USB 설정 (malloc 비활성화) |
| `STM32H743ZITX_FLASH.ld` | 링커 스크립트 |

---

**결론**: STM32H7의 다중 버스 아키텍처에서 USB OTG FS는 AXI 버스만 사용하므로, DTCM에 배치된 버퍼에 접근할 수 없습니다. 이전 프로젝트에서는 우연히 버퍼가 접근 가능한 영역에 있었거나, 단순한 메모리 구조를 사용했을 가능성이 높습니다.
