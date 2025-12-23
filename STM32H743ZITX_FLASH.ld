/*
******************************************************************************
**
**  File        : LinkerScript.ld
**
**  Author      : STM32CubeIDE
**
**  Abstract    : Linker script for STM32H7 series
**                2048Kbytes FLASH and 1056Kbytes RAM
**
**                Set heap size, stack size and stack location according
**                to application requirements.
**
**                Set memory bank area and size if external memory is used.
**
**  Target      : STMicroelectronics STM32
**
**  Distribution: The file is distributed as is, without any warranty
**                of any kind.
**
*****************************************************************************
** @attention
**
** Copyright (c) 2025 STMicroelectronics.
** All rights reserved.
**
** This software is licensed under terms that can be found in the LICENSE file
** in the root directory of this software component.
** If no LICENSE file comes with this software, it is provided AS-IS.
**
****************************************************************************
*/

/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
_estack = ORIGIN(DTCMRAM) + LENGTH(DTCMRAM);    /* end of DTCMRAM */

/* Generate a link error if heap and stack don't fit into RAM */
_Min_Heap_Size = 0x8000;   /* 32KB */
_Min_Stack_Size = 0x4000;   /* 16KB */

/* Specify the memory areas */
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

/* Define output sections */
SECTIONS
{
  /* ============== FLASH ============== */
  /* The startup code goes first into FLASH */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector)) /* Startup code */
    . = ALIGN(4);
  } >FLASH

  /* The program code and other data goes into FLASH */
  .text :
  {
    . = ALIGN(4);
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */
    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;        /* define a global symbols at end of code */
  } >FLASH

  /* Constant data goes into FLASH */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    . = ALIGN(4);
  } >FLASH

  .ARM.extab (READONLY) : /* The READONLY keyword is only supported in GCC11 and later, remove it if using GCC10 or earlier. */
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } >FLASH

  .ARM (READONLY) : /* The READONLY keyword is only supported in GCC11 and later, remove it if using GCC10 or earlier. */
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  .preinit_array (READONLY) : /* The READONLY keyword is only supported in GCC11 and later, remove it if using GCC10 or earlier. */
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >FLASH

  .init_array (READONLY) : /* The READONLY keyword is only supported in GCC11 and later, remove it if using GCC10 or earlier. */
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >FLASH

  .fini_array (READONLY) : /* The READONLY keyword is only supported in GCC11 and later, remove it if using GCC10 or earlier. */
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >FLASH

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
  /* used by the startup to initialize data */
  _sidata = LOADADDR(.data);

  /* Initialized data sections goes into DTCMRAM, load LMA copy after code */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;        /* create a global symbol at data start */
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */
    *(.RamFunc)        /* .RamFunc sections */
    *(.RamFunc*)       /* .RamFunc* sections */

    . = ALIGN(4);
    _edata = .;        /* define a global symbol at data end */
  } >DTCMRAM AT>FLASH

  /* Uninitialized data section */
  .bss (NOLOAD) :
  {
    . = ALIGN(4);
    /* This is used by the startup in order to initialize the .bss section */
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;         /* define a global symbol at bss end */
    __bss_end__ = _ebss;
  } >DTCMRAM

  /* User_heap_stack section, used to check that there is enough RAM left */
  ._user_heap_stack (NOLOAD) :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >DTCMRAM

  /* ============== RAM_D1 Non-Cacheable - FMC 버퍼 ============== */
  .ram_d1_nc :
  {
    . = ALIGN(32);
    _sram_d1_nc = .;
    KEEP(*(.fmc_buffer))
    KEEP(*(.fmc_buffer*))
    KEEP(*(.sdmmc_buffer))
    KEEP(*(.ram_d1_nc))
    . = ALIGN(32);
    _eram_d1_nc = .;
  } >RAM_D1_NC

  /* ============== RAM_D1 Cacheable - 확장 BSS ============== */
  .ram_d1_cached :
  {
    . = ALIGN(32);
    _sram_d1_cached = .;
    KEEP(*(.ram_d1_cached))
    KEEP(*(.ram_d1_cached*))
    KEEP(*(.large_bss))
    KEEP(*(.frame_buffer))
    . = ALIGN(32);
    _eram_d1_cached = .;
  } >RAM_D1_CACHED

  /* ============== RAM_D2 - DMA 버퍼 (Non-cacheable) ============== */
  .ram_d2 :
  {
    . = ALIGN(32);
    _sram_d2 = .;
    KEEP(*(.ram_d2))
    KEEP(*(.ram_d2*))
    KEEP(*(.uart_dma))
    KEEP(*(.audio_dma))
    KEEP(*(.usb_buffer))
    KEEP(*(.dma_buffer))
    . = ALIGN(32);
    _eram_d2 = .;
  } >RAM_D2

  /* ============== RAM_D3 - BDMA (Non-cacheable) ============== */
  .ram_d3 :
  {
    . = ALIGN(32);
    _sram_d3 = .;
    KEEP(*(.ram_d3))
    KEEP(*(.ram_d3*))
    KEEP(*(.bdma_buffer))
    . = ALIGN(32);
    _eram_d3 = .;
  } >RAM_D3

  /* Remove information from the standard libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
