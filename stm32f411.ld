/* ===========================================================================
 * stm32f411.ld — Linker script for the STM32F411CEU6
 * ===========================================================================
 *
 * TARGET      : STM32F411CEU6 (WeAct BlackPill V3.0)
 * CORE        : Cortex-M4F (ARMv7E-M, single-precision FPU)
 *
 * MEMORY MAP  (RM0383 §2.2.3 — verified against the silicon with
 *               `st-info --probe`: flash: 524288, sram: 131072)
 *   FLASH  512 KB  @ 0x08000000   (0x08000000 - 0x0807FFFF)
 *   SRAM1  128 KB  @ 0x20000000   (0x20000000 - 0x2001FFFF)
 *
 * WHAT THIS FILE DOES
 *   The compiler emits *sections*; it has no idea where they will live. This
 *   script tells the linker:
 *     1. how much memory exists, and where          (MEMORY block)
 *     2. which section goes where, in what order     (SECTIONS block)
 *     3. the boundary addresses the startup code needs (the _s* symbols)
 *
 * THE ONE RULE THAT GOVERNS EVERYTHING
 *   The CPU resets by reading two words from address 0x00000000:
 *     word 0 = initial stack pointer
 *     word 1 = address of the reset handler
 *   0x00000000 is an ALIAS of flash 0x08000000 (RM0383 §2.2.3).
 *   Therefore .isr_vector MUST be the first bytes in flash, and MUST be
 *   listed first in the SECTIONS block below.
 *
 * SYMBOL CONTRACT WITH startup_stm32f411.s
 *   This file defines the addresses; the startup file reads them. The names
 *   below follow the ST convention (not the libopencm3 one) and must match
 *   exactly in both files:
 *
 *     _estack   initial stack pointer (top of RAM)
 *     _sidata   LOAD address of .data  -> the copy SOURCE  (in flash)
 *     _sdata    START address of .data -> the copy DEST    (in RAM)
 *     _edata    END address of .data
 *     _sbss     START address of .bss
 *     _ebss     END address of .bss
 *
 * ---------------------------------------------------------------------------
 */

ENTRY(Reset_Handler)

/* ---------------------------------------------------------------------------
 * Initial stack pointer.
 *
 * The stack grows DOWNWARD from here, so this must be the highest valid RAM
 * address. It is derived from the RAM region rather than hardcoded so that it
 * can never drift out of sync with the MEMORY block below. (The F103
 * reference hardcodes 0x20004FFF; change the RAM size there and you must
 * remember to change two places. Here there is only one place.)
 *
 * Result: 0x20000000 + 128K = 0x20020000, which is 8-byte aligned as the ARM
 * procedure-call standard requires.
 * ------------------------------------------------------------------------- */
_estack = ORIGIN(RAM) + LENGTH(RAM);

/* ---------------------------------------------------------------------------
 * Space reserved for the stack, and a link-time check that it fits.
 *
 * This is NOT a runtime limit. Reserving it means that if code + data + stack
 * no longer fit in RAM, the LINK FAILS with a clear "region RAM overflowed"
 * error instead of the program crashing mysteriously at run time.
 *
 * 4 KB is deliberate: Phase 1 adds an I2C driver with nested call frames and
 * buffers. 1 KB is uncomfortably tight; 8 KB wastes RAM on a 128 KB part.
 * ------------------------------------------------------------------------- */
_Min_Stack_Size = 0x1000;

/* ---------------------------------------------------------------------------
 * No heap. This project allocates nothing dynamically (C11, no malloc), so
 * there is no _Min_Heap_Size. If you ever add malloc, add the reservation
 * here too, or the linker will happily let the heap eat your stack.
 * ------------------------------------------------------------------------- */

/* ===========================================================================
 * MEMORY REGIONS
 *
 *   ORIGIN  = start address
 *   LENGTH  = size
 *   (rwx)   = permissions the linker enforces. r=read w=write x=execute.
 *            RAM is rwx so that a mistake placing code in RAM is caught.
 * ========================================================================= */
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

/* ===========================================================================
 * OUTPUT SECTIONS
 *
 * ld walks this block TOP TO BOTTOM and assigns addresses in that order.
 * The order here IS the memory layout. If .isr_vector is not first, the
 * vector table is not at 0x08000000 and the chip resets to garbage — with
 * no error and no warning from the toolchain.
 * ========================================================================= */
SECTIONS
{
  /* -------------------------------------------------------------------------
   * .isr_vector — THE VECTOR TABLE. Must be the first bytes in flash.
   *
   * KEEP() protects it from --gc-sections, which is free to discard
   * "unused" sections. Without KEEP, the linker may remove the vector table
   * and the chip will reset to 0x00000000.
   *
   * NOTE: the corresponding input section must be declared with the "a"
   * (alloc) attribute in startup_stm32f411.s:
   *     .section .isr_vector,"a",%progbits
   * Without "a" the section is not marked loadable, the FLASH region is not
   * advanced for it, .text lands on top of it at 0x08000000, and
   * .isr_vector ends up in no program header — so it is never flashed at
   * all. The build succeeds; the chip locks up. This was root-caused on this
   * board, so check it:
   *     arm-none-eabi-readelf -S build/app.elf | grep isr_vector   # needs "A"
   * ---------------------------------------------------------------------- */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } > FLASH

  /* -------------------------------------------------------------------------
   * .text — program code.
   *   *(.text*)  is a glob: every input section whose name starts with
   *              .text (.text, .text.main, .text.foo, ...).
   * ---------------------------------------------------------------------- */
  .text :
  {
    . = ALIGN(4);
    *(.text*)
    *(.glue_7)         /* ARM<->Thumb interworking veneers */
    *(.glue_7t)
    *(.eh_frame)
    KEEP(*(.init))
    KEEP(*(.fini))
    . = ALIGN(4);
  } > FLASH

  /* -------------------------------------------------------------------------
   * .rodata — read-only data: string literals, const globals.
   * It lives in FLASH because it never changes. Putting it in RAM would
   * waste RAM and break const semantics.
   * ---------------------------------------------------------------------- */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } > FLASH

  /* ARM unwind tables. Empty in pure C, but keeping them here costs nothing
   * and avoids a surprise if C++ exceptions are ever enabled. */
  .ARM.extab :
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } > FLASH

  .ARM.exidx :
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } > FLASH

  /* C/C++ static constructor and destructor tables. Empty in plain C, but
   * if __libc_init_array is ever called these must be kept. */
  .preinit_array :
  {
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array*))
    PROVIDE_HIDDEN(__preinit_array_end = .);
  } > FLASH

  .init_array :
  {
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array*))
    PROVIDE_HIDDEN(__init_array_end = .);
  } > FLASH

  .fini_array :
  {
    PROVIDE_HIDDEN(__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array*))
    PROVIDE_HIDDEN(__fini_array_end = .);
  } > FLASH

  /* -------------------------------------------------------------------------
   * .data — initialised, writable globals.
   *
   * This is the VMA/LMA split, and it is the part most people get wrong.
   *
   * A global like `int counter = 5;` is writable, so the CPU must read it
   * from RAM. But the initial value `5` has to be stored somewhere, and only
   * flash survives a power cycle. So one variable needs TWO addresses:
   *
   *   VMA (virtual)  = 0x20000000  (RAM)  set by "> RAM"   where it RUNS
   *   LMA (load)      = 0x0800xxxx  (FLASH) set by "AT> FLASH" where it is
   *                                                LOADED FROM
   *
   *   FLASH:  [vector][.text][.rodata][ 5 ]   <- the "5" lives here
   *                   |
   *                   |  startup code copies this (LMA -> VMA)
   *                   v
   *   RAM:    [counter = 5 ][ uninitialised .bss ]
   *
   *   _sidata = LOADADDR(.data) saves the LMA so the startup code knows
   *   where to copy FROM. _sdata/_edata give the destination range.
   *
   * OMITTING "AT> FLASH" IS A SILENT FAILURE: the linker then believes the
   * initialisers load from RAM, where they do not exist. You get garbage
   * values, or a spurious "region RAM overflow" error.
   * ---------------------------------------------------------------------- */
  _sidata = LOADADDR(.data);

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } > RAM AT> FLASH

  /* -------------------------------------------------------------------------
   * .bss — uninitialised globals. They need RAM but no flash space; the
   * startup code simply zeroes them.
   *
   * .bss is NOBITS in the ELF: it occupies address space but contributes
   * ZERO bytes to the flash image. That is why a 128 KB RAM part can have a
   * .bss larger than the whole binary.
   *
   * *(COMMON) covers the legacy -fcommon GCC behaviour. GCC 10+ defaults to
   * -fno-common, so uninitialised globals land straight in .bss and COMMON
   * is empty. Harmless to keep.
   * ---------------------------------------------------------------------- */
  .bss :
  {
    . = ALIGN(4);
    _sbss = .;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
  } > RAM

  /* -------------------------------------------------------------------------
   * ._user_heap_stack — reserve the stack at the top of RAM.
   *
   * '.' is the current address (the location counter). This block moves it
   * forward by _Min_Stack_Size, carving the stack out of RAM so the linker
   * can prove code + data + stack still fit.
   *
   * ALIGN(8) because the ARM procedure-call standard requires the stack
   * pointer to be 8-byte aligned at exception entry.
   *
   * There is no heap: this project does not use malloc. If you add it, add a
   * _Min_Heap_Size reservation here too.
   * ---------------------------------------------------------------------- */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE(end = .);
    PROVIDE(_end = .);
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } > RAM

  /* -------------------------------------------------------------------------
   * /DISCARD/ — throw these away.
   *
   * .ARM.attributes is toolchain metadata. .ARM.exidx* is C++ exception
   * index data (already placed above; this catches any stragglers).
   *
   * NOTE: do NOT copy the F103 reference's /DISCARD/ block, which drops
   * libc.a, libm.a and libgcc.a entirely. That is correct for Ac6 System
   * Workbench, which links those separately, but for this build it would
   * discard memcpy, memset and the compiler's division helpers.
   * ---------------------------------------------------------------------- */
  /DISCARD/ :
  {
    *(.ARM.attributes)
  }
}
