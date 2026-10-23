/**
 * @file    stm32f411.ld
 * @brief   Linker script for the STM32F411CEU6 (Cortex-M4F).
 *
 * The compiler emits sections with no idea where they live. This script says
 * how much memory exists, which section goes where, and what the boundary
 * addresses are.
 *
 * Memory map verified against silicon (RM0383 §2.2.3):
 *   FLASH  512 KB @ 0x08000000
 *   SRAM1  128 KB @ 0x20000000
 *
 * @note .isr_vector MUST be first. On reset the CPU reads SP from
 *       0x00000000 and the reset vector from 0x00000004, and 0x00000000 is
 *       an alias of 0x08000000.
 *
 * @note Symbols shared with startup_stm32f411.s. Both files must agree:
 *         _estack  initial SP (top of RAM)
 *         _sidata  load address of .data  (copy source, flash)
 *         _sdata   start of .data         (copy destination, RAM)
 *         _edata   end of .data
 *         _sbss    start of .bss
 *         _ebss    end of .bss
 */

ENTRY(Reset_Handler)

/**
 * @brief Initial stack pointer.
 *
 * The stack grows down, so this is the highest valid RAM address. Derived
 * from the RAM region so it cannot drift out of sync with MEMORY below.
 * 0x20000000 + 128K = 0x20020000, already 8-byte aligned.
 */
_estack = ORIGIN(RAM) + LENGTH(RAM);

/**
 * @brief Stack reservation.
 *
 * Not a runtime limit — it makes the LINK fail with "region RAM overflowed"
 * if code + data + stack stop fitting, instead of the program corrupting
 * memory at run time.
 */
_Min_Stack_Size = 0x1000;

/**
 * @brief Memory regions.
 *
 * (rwx) are permissions the linker enforces. RAM is rwx so that placing code
 * in RAM is caught rather than silently allowed.
 */
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

/**
 * @brief Output sections.
 *
 * ld walks this block top to bottom and assigns addresses in that order, so
 * the order here IS the memory layout.
 */
SECTIONS
{
  /**
   * @brief Vector table. First bytes in flash.
   *
   * KEEP() stops --gc-sections from discarding it as "unused". The matching
   * input section in startup_stm32f411.s must carry the "a" (alloc)
   * attribute; without it the table is never flashed and the chip locks up.
   * Verify: arm-none-eabi-readelf -S build/app.elf | grep isr_vector
   */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } > FLASH

  /**
   * @brief Program code. *(.text*) also catches .text.main, .text.foo, ...
   */
  .text :
  {
    . = ALIGN(4);
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)
    KEEP(*(.init))
    KEEP(*(.fini))
    . = ALIGN(4);
  } > FLASH

  /**
   * @brief Read-only data: string literals, const globals. Stays in flash.
   */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } > FLASH

  /* ARM unwind tables. Empty in pure C. */
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

  /* Static constructor and destructor tables. Empty in plain C. */
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

  /**
   * @brief Initialised writable globals — the VMA/LMA split.
   *
   * A global such as `int counter = 5;` is writable, so it must live in RAM.
   * The initial value has to survive a power cycle, so it is stored in flash.
   * One variable therefore needs two addresses:
   *
   *   VMA  set by "> RAM"       where it RUNS
   *   LMA  set by "AT> FLASH"   where it is LOADED FROM
   *
   * @warning Omitting "AT> FLASH" is a silent failure: the linker believes
   *          the initialisers load from RAM, where they do not exist.
   */
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

  /**
   * @brief Uninitialised globals.
   *
   * NOBITS in the ELF: occupies address space but contributes zero bytes to
   * the flash image. The startup code zeroes it. *(COMMON) covers the legacy
   * -fcommon behaviour and is empty on GCC 10+.
   */
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

  /**
   * @brief Carve the stack out of RAM so the link can prove it fits.
   *
   * '.' is the location counter; advancing it reserves the space. ALIGN(8)
   * because the ARM ABI requires an 8-byte aligned SP at exception entry.
   *
   * @note No heap: this project never calls malloc. If that changes, add a
   *       heap reservation here or the heap will eat the stack.
   */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE(end = .);
    PROVIDE(_end = .);
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } > RAM

  /**
   * @brief Discarded sections.
   *
   * @warning Do not add libc.a / libm.a / libgcc.a here. The F103 reference
   *          does, because Ac6 System Workbench links them separately. Here
   *          it would discard memcpy, memset and the division helpers.
   */
  /DISCARD/ :
  {
    *(.ARM.attributes)
  }
}
