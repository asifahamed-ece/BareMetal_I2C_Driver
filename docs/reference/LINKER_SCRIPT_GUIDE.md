# Writing `stm32f411.ld` — a field-by-field guide

**You write this file. This document explains every field so you can fill it in
with understanding instead of guesswork.**

Verified on this machine: `arm-none-eabi-gcc 16.2.0`, `ld 2.47`, STM32F411CEU6.

---

## 0. What this file actually is

Your C compiler does not know your chip exists. `gcc` turns `main.c` into an
object file full of **sections** (`.text`, `.data`, `.bss`, `.rodata`, …) — it
does not know or care where those bytes will live.

The **linker** is what turns object files into a flat binary image. It needs to
know three things that only *you* can tell it:

1. **How much memory do I have, and where?** (Flash at `0x08000000`, 512 KB; RAM
   at `0x20000000`, 128 KB)
2. **Which section goes where?** (vector table first, then code, then data)
3. **What are the boundary addresses?** (so the startup code knows what to copy)

`stm32f411.ld` is the answer to those three questions. It is a *declarative
contract between the linker and the physical silicon*. Get it wrong and the
compiler is perfectly happy — the failure appears on the board instead.

### The one rule that governs everything

> **The CPU resets by reading two words from address `0x00000000`.**
> Word 0 = initial stack pointer. Word 1 = address of the reset handler.
> `0x00000000` is an **alias** of flash `0x08000000` (RM0383 §2.2.3).

So the vector table must be the **very first bytes in flash**. Everything else in
the file is a consequence of that.

---

## 1. The file skeleton

```ld
/* stm32f411.ld — STM32F411CEU6 (512 KB Flash, 128 KB SRAM) */

ENTRY(Reset_Handler)                    /* (1) */

_estack = ORIGIN(RAM) + LENGTH(RAM);    /* (2) */
_Min_Stack_Size = 0x1000;               /* (3) */

MEMORY                                  /* (4) */
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

SECTIONS                                /* (5) */
{
  .isr_vector : { KEEP(*(.isr_vector)) } > FLASH

  .text : { *(.text*) *(.rodata*) } > FLASH

  _sidata = LOADADDR(.data);
  .data : { _sdata = .; *(.data*) _edata = .; } > RAM AT> FLASH

  .bss : { _sbss = .; *(.bss*) *(COMMON) _ebss = .; } > RAM

  ._user_heap_stack : { . = ALIGN(8); . = . + _Min_Stack_Size; . = ALIGN(8); } > RAM
}
```

Now each piece.

---

## 2. `ENTRY(Reset_Handler)` — (1)

Tells the linker which symbol is the program's entry point, and records it in
the ELF header.

```
$ arm-none-eabi-readelf -h build/app.elf | grep Entry
  Entry point address:  0x80001af
```

**Why it matters:** the entry point is where the debugger jumps to when you
"reset and run". It is *not* how the chip boots — the chip always boots via the
vector table at `0x00000000`. `ENTRY` just has to agree with it.

The name must match the symbol your startup file defines. ST convention:
`Reset_Handler`.

---

## 3. `_estack` — (2)

The initial stack pointer value. The stack grows **downward** from here, so
`_estack` must be the **highest** valid RAM address.

```
RAM: 0x20000000 ................. 0x2001FFFF
      ^ _sbss/_sdata              ^ _estack = 0x20020000
```

**Write it as `ORIGIN(RAM) + LENGTH(RAM)`, not as a literal.** This is the
libopencm3 trick, and it is worth copying:

```ld
_estack = ORIGIN(RAM) + LENGTH(RAM);
```

Now `_estack` **cannot drift out of sync** with the `MEMORY` block. The F103
reference hardcodes `_estack = 0x20004FFF` — change the RAM size and you must
remember to change two places. Change one place here and you can't get it
wrong.

> The F103 value is `0x20004FFF` (top of 20K RAM, minus 1). An odd value. ARM
> wants an 8-byte aligned SP at exception entry. For your 128K RAM the top is
> `0x20020000` — already 8-byte aligned and even. Use
> `ORIGIN(RAM) + LENGTH(RAM)`.

---

## 4. `_Min_Stack_Size` — (3)

How much RAM to **reserve** for the stack at the top of RAM.

This isn't a runtime limit — it's a **link-time check**. By carving
`_Min_Stack_Size` out of RAM, if your code + data + stack don't fit, the linker
fails with a clear "region RAM overflowed" instead of you discovering it as a
random crash three days later.

```ld
_Min_Stack_Size = 0x1000;   /* 4 KB */
```

Choose 4 KB. Your Phase 1 I2C work will use nested call frames and buffers;
1 KB is uncomfortably tight, 8 KB is wasted RAM on a part with 128 KB.

---

## 5. `MEMORY { ... }` — (4)

Declares the physical memory regions. Two fields per region:

| Field | Meaning | Your value |
|---|---|---|
| `ORIGIN` | Start address | `0x08000000` / `0x20000000` |
| `LENGTH` | Size | `512K` / `128K` |

The attributes in parentheses are **permissions the linker checks**:

| Attr | Meaning | Why |
|---|---|---|
| `r` | readable | code, constants |
| `w` | writable | `.data`, `.bss` — RAM is writable |
| `x` | executable | code |
| `!` | do not execute | — |

`FLASH (rx)` and `RAM (rwx)` is the standard pairing. `RAM (rwx)` also lets the
linker catch a mistake where you accidentally put code in RAM.

**These are not guesses.** Both were confirmed against the silicon:
```
st-info: flash: 524288  (512 KiB)
st-info: sram:  131072  (128 KiB)
```

---

## 6. `SECTIONS { ... }` — (5)

The heart of the file. It says: *for each output section, in this order, put
these input sections here, in this memory region.*

### The anatomy of one section

```ld
  .text : { *(.text*) *(.rodata*) } > FLASH
  ^name  ^ ^contents^             ^region
```

| Piece | Meaning |
|---|---|
| `.text` | **output section** name — the name in the final ELF |
| `*(.text*)` | **input section pattern** — pull in every input section whose name matches `.text*` |
| `> FLASH` | place it in the FLASH region |

### The `*()` wildcard

Object files contain many sections named after the function they hold
(`.text.main`, `.text.foo`, `.text.__aeabi_...`). The pattern `*(.text*)` means
"every input section whose name starts with `.text`". The `*` is a glob
wildcard, and the parentheses are required.

### Why the order in `SECTIONS` matters

`ld` walks your `SECTIONS` block **top to bottom** and assigns addresses in
that order. That is why `.isr_vector` must be listed first.

Proof — I built the same program with `.text` first and the vector table
landed in the wrong place:

```
.text         0x1c  @ 0x08000000      <-- code at the front
.isr_vector   0x198 @ 0x0800001c      <-- vector table NOT at 0x08000000
```

The chip would reset to garbage. **No error, no warning. It just doesn't work.**

### `KEEP(...)`

```ld
  .isr_vector : { KEEP(*(.isr_vector)) } > FLASH
```

`KEEP` prevents `--gc-sections` (garbage collection, which we use to strip
unused code) from deleting the section. Without it, the linker is free to
discard the vector table as "unused" — and it does.

**`KEEP` on `.isr_vector` is not optional.**

---

## 7. The three sections that need real thought

### `.text` — code and constants

```ld
  .text : { *(.text*) *(.rodata*) } > FLASH
```

`.rodata*` (string literals, `const` globals) goes here too, because it lives in
flash. Putting it in `> RAM` would waste RAM and break `const` semantics.

> Keep them separate as `.text` and `.rodata` output sections, or combine them
> like the F103 reference does? **Either works.** Combining is simpler; the
> ST reference keeps them separate. Pick one and be consistent — the only
> requirement is that the *total* lands in FLASH.

### `.data` — the VMA/LMA split

This is the concept most people get wrong. Read it twice.

```ld
  _sidata = LOADADDR(.data);
  .data : { _sdata = .; *(.data*) _edata = .; } > RAM AT> FLASH
```

A variable like `int counter = 5;` is a **global with a non-zero initial
value**. Two things must be true:

- The CPU must be able to **read it** → it must live in **RAM**
- The **initial value `5`** must be stored somewhere → in **flash**

One variable, **two addresses**:

| | Address | Set by | Meaning |
|---|---|---|---|
| **VMA** (Virtual) | `0x20000000` (RAM) | `> RAM` | where it **runs** |
| **LMA** (Load) | `0x080001b4` (flash) | `AT> FLASH` | where it's **loaded from** |

`AT> FLASH` is what creates the second address. `LOADADDR(.data)` returns the
LMA, and `_sidata` saves it so the startup code knows where to copy from.

```
FLASH:  [vector][.text][.rodata][ 5 ]   <- the "5" lives here
              |
              |  startup code copies this
              v
RAM:    [counter = 5 ][  uninitialised  ]
```

**The startup code** (which you write next) does:
```asm
    ldr r0, =_sdata      /* destination: RAM  */
    ldr r1, =_edata
    ldr r2, =_sidata     /* source:    FLASH */
    /* copy loop */
```

**Forgetting `AT> FLASH` is a silent failure.** The linker then thinks the
initializers load from RAM, where they don't exist. You get either garbage
values or a "region RAM overflow" error.

The four symbols this creates:

| Symbol | Meaning | Defined by |
|---|---|---|
| `_sidata` | LMA of `.data` — the copy **source** | `LOADADDR(.data)` |
| `_sdata` | VMA start of `.data` — the copy **destination** | `_sdata = .;` |
| `_edata` | VMA end of `.data` | `_edata = .;` |
| `_sbss` / `_ebss` | VMA range of `.bss` | `_sbss = .;` / `_ebss = .;` |

> **Naming trap.** The libopencm3 template uses `_data`, `_edata`, `_ebss`,
> `_data_loadaddr`, `_stack`. The ST convention is `_sdata`, `_edata`, `_sbss`,
> `_ebss`, `_sidata`, `_estack`. **Never mix them** — the linker script and the
> startup file must agree, or you get undefined references. Use the ST names
> (the F103 reference and `docs/PLAN.md` both use them).

### `.bss` — zero-initialised

```ld
  .bss : { _sbss = .; *(.bss*) *(COMMON) _ebss = .; } > RAM
```

Globals declared without an initialiser. They need RAM but **no flash space** —
the startup code just zeroes them.

`*(COMMON)` handles a legacy GCC behaviour. Modern GCC (10+) defaults to
`-fno-common`, so uninitialised globals go straight to `.bss` and `COMMON` is
empty. **Harmless to keep** — it costs nothing and covers the `-fcommon` case.

`.bss` is special: it is `NOBITS` in the ELF — it occupies address space but
**no bytes in the flash image**. That's why a 128 KB RAM part can have a
`.bss` bigger than the whole file.

---

## 8. `.` — the location counter

Inside a section body, `.` means "the current address".

```ld
  ._user_heap_stack :
  {
    . = ALIGN(8);              /* round up to 8-byte boundary */
    . = . + _Min_Stack_Size;   /* reserve the stack          */
    . = ALIGN(8);
  } > RAM
```

- `. = ALIGN(8)` — move the cursor to the next 8-byte boundary
- `. = . + _Min_Stack_Size` — move it forward by 4 KB
- `ALIGN(8)` is a *function*; `. = ALIGN(8)` is an *assignment*. The `.` is what
  makes it move.

**Why 8-byte alignment?** The ARM procedure-call standard requires 8-byte
alignment for the stack pointer. `Reset_Handler` sets SP to `_estack`; if that
isn't 8-aligned, a `push`/`pop` in a function could fault.

---

## 9. `/DISCARD/` — what to throw away

```ld
  /DISCARD/ : { *(.ARM.exidx*) *(.ARM.attributes) }
```

Some sections exist only for the toolchain and should not be in your binary.
`.ARM.attributes` is metadata for the linker. `.ARM.exidx` is C++ exception
index data — empty in pure C, but harmless to discard.

**Do NOT copy the F103 reference's `/DISCARD/` block verbatim:**
```ld
  /DISCARD/ : { libc.a ( * ) libm.a ( * ) libgcc.a ( * ) }
```
That discards libc, libm and libgcc entirely — correct for Ac6 System
Workbench (which links them separately), **fatal for you**. You would lose
`memcpy`, `memset` and the compiler's division helpers.

---

## 10. Your four decisions

Write the file, then be able to answer these:

1. **Where does `.data`'s LMA go?**
   `AT> FLASH` alone means "immediately after the previous section in flash",
   which lands it right after `.text`/`.rodata`. Alternative: give it an
   explicit address. The default is right — use it, and say why.

2. **How big is the stack, and how is `_estack` derived?**
   4 KB, and `ORIGIN(RAM) + LENGTH(RAM)` so it can't drift.

3. **Are `.text` and `.rodata` one output section or two?**
   Either. Say which you chose.

4. **Do you call `__libc_init_array`?**
   The F103 startup calls it to run C++ static constructors. You have no C++ and
   no `__attribute__((constructor))` functions. Calling it is harmless (newlib
   provides the symbol) but pulls in a dependency you don't need. **Your call —
   but decide it, don't inherit it by accident.**

---

## 11. How to verify before you flash

```sh
# 1. Vector table is exactly 102 words = 408 bytes = 0x198
arm-none-eabi-size -A build/app.elf | grep isr_vector
#   .isr_vector          00000198

# 2. It is ALLOCATED (needs "A" in the flags) and .text does NOT overlap it
arm-none-eabi-readelf -S build/app.elf | grep isr_vector
#   .isr_vector  PROGBITS  08000000  ...  A
arm-none-eabi-objdump -h build/app.elf | grep -E "isr_vector|\.text"
#   .isr_vector  00000198  08000000
#   .text        00000xxx  08000198      <-- 0x198 bytes later, NOT 0x08000000

# 3. Stack and boundary symbols are sane
arm-none-eabi-nm build/app.elf | grep -E "_estack|_sdata|_edata|_sbss|_ebss|_sidata"
#   20020000 R _estack        <-- top of 128K RAM
#   080001b4 A _sidata        <-- inside FLASH

# 4. Nothing in RAM is a surprise
arm-none-eabi-size build/app.elf
```

If check 2 shows `.text` at `0x08000000`, your `.isr_vector` section is missing
the `"a"` alloc flag in the **startup file** — the build will succeed and the
chip will lock up. That exact bug was root-caused on this board already.

---

## 12. Quick syntax reference

| Syntax | Meaning |
|---|---|
| `MEMORY { }` | declare physical regions |
| `SECTIONS { }` | declare output sections and their placement |
| `ENTRY(sym)` | program entry point |
| `ORIGIN(region)` | region's start address |
| `LENGTH(region)` | region's size |
| `> REGION` | assign the section's VMA to that region |
| `AT> REGION` | assign the section's LMA to that region |
| `LOADADDR(sec)` | the LMA of a section |
| `KEEP(pat)` | protect from `--gc-sections` |
| `*(pat)` | include all input sections matching the glob |
| `.` | current address within a section |
| `. = ALIGN(n)` | align the current address to n bytes |
| `. = . + n` | advance the current address by n |
| `PROVIDE(sym, expr)` | define a symbol only if not already defined |
| `/DISCARD/ { }` | throw these sections away |
| `sym = .;` | define a symbol at the current address |

---

## 13. One-page summary

```
ENTRY(Reset_Handler)                 entry point, must match startup file
_estack = ORIGIN(RAM)+LENGTH(RAM)    top of RAM, derived so it can't drift
_Min_Stack_Size = 0x1000             4KB reserve -> link-time overflow check

MEMORY {
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

SECTIONS {
  .isr_vector : { KEEP(*(.isr_vector)) } > FLASH      # FIRST. always.
  .text       : { *(.text*) *(.rodata*) } > FLASH
  _sidata = LOADADDR(.data);
  .data       : { _sdata = .; *(.data*) _edata = .; } > RAM AT> FLASH
  .bss        : { _sbss = .; *(.bss*) *(COMMON) _ebss = .; } > RAM
  ._user_heap_stack : { . = ALIGN(8); . = . + _Min_Stack_Size; . = ALIGN(8); } > RAM
}
```
