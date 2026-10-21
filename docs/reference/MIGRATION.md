# MIGRATION.md — From F103 reference to F411CEU6

**Purpose:** These reference files are for a *different* MCU. Read them, then write
your own `stm32f411.ld` and `startup_stm32f411.s`. Copying them verbatim produces a
binary that builds cleanly, flashes cleanly, and then **locks the chip up**.

| File | Upstream | MCU | Core | Flash | RAM |
|---|---|---|---|---|---|
| `STM32F103XB_FLASH.ld` | ST `cmsis_device_f1` | STM32F103xB | Cortex-M3 | 128K | 20K |
| `startup_stm32f103xb.s` | ST `cmsis_device_f1` | STM32F103xB | Cortex-M3 | 128K | 20K |
| `libopencm3_linker.ld.S` | libopencm3 `ld/` | generic template | any | macro-driven | macro-driven |
| `f411_external_irqs.inc` | extracted from RM0383 Table 37 | STM32F411xC/E | Cortex-M4F | — | — |
| **YOUR TARGET** | — | **STM32F411CEU6** | **Cortex-M4F** | **512K** | **128K** |

`f411_external_irqs.inc` is the 86-slot IRQ list for your MCU, extracted
straight from RM0383 Table 37 — use the names from it in your startup file.

---

## 1. The seven numbers you must change

Every one of these is verifiable from hardware, not guesswork.

| What | F103 reference | F411 (yours) | Where to confirm |
|---|---|---|---|
| Flash origin | `0x08000000` | `0x08000000` | same — no change |
| Flash length | `128K` | **`512K`** | `st-info: flash: 524288` |
| RAM origin | `0x20000000` | `0x20000000` | same — no change |
| RAM length | `20K` | **`128K`** | `st-info: sram: 131072` |
| `_estack` | `0x20004FFF` | **`0x20020000`** | top of 128K SRAM |
| Vector table words | 67 | **102** | RM0383 Table 37 |
| Named external IRQs | 43 (F103xB) | **57** (in 86 slots) | RM0383 Table 37, max position 85 |

**`20K` → `128K` and `0x20004FFF` → `0x20020000` are the two that bite.**
`_estack` must be the *end* of your RAM region. If you leave it at `0x20004FFF`
and then raise RAM to 128K, the stack still lives in the bottom 20K and your
`_user_heap_stack` check passes for the wrong reason.

**Careful with `_estack` and alignment.** The F103 reference uses `0x20004FFF`
(top of 20K minus 1, an odd value). ARM requires an 8-byte aligned SP at
exception entry. For 128K RAM the top is `0x20020000` — already 8-byte aligned
and **even**, which is what you want. The `-1` in the F103 value is an Ac6
convention, not a requirement.

---

## 2. The vector table: 67 words → 102 words

This is the single biggest structural difference, and it is *not* a find-and-replace.

**F103xB table (67 words, measured from the file):** 16 system exceptions +
43 named external IRQs + 7 zero words + 1 `BootRAM` word.
The 7 zeros are padding so the same file can host the larger F103 densities,
and `BootRAM` supports "boot in RAM mode" on F10x medium-density parts. **The
F411CEU6 has neither** — no RAM boot mode, and a completely different IRQ set.

**F411 table (102 words):** 16 system exceptions + **86** external IRQ slots
(positions 0–85).

### The counting trap

RM0383 Table 37 for STM32F411xC/E lists only **57 rows with a real peripheral
name**. The other 29 IRQ positions are **reserved on this package** — but they
still occupy address slots, because the CPU computes the vector address
arithmetically:

```
address = 0x40 + 4 × position      (position 0 = first external IRQ)
```

Check it: the last external interrupt, `SPI5` at position **85**, sits at
`0x194`. `0x194 / 4 = 101` → table index 101, plus 16 system slots = **102 words**.

So: **count the highest position, not the number of rows.** The F103 file gets
away with a short table because it has a different, smaller IRQ set. If you copy
the F103 67-word table onto an F411, an interrupt that exists on your part
(positions 74–80, 82–85) lands on the wrong entry — or off the end entirely.

The full reserved-position list for the F411, straight from Table 37:
```
named positions : 0-18, 23-38, 40-42, 47, 49-51, 56-60, 67-73, 81, 84, 85   (57)
reserved        : 19-22, 39, 43-46, 48, 52-55, 61-66, 74-80, 82, 83       (29)
total slots     : 86
```

### Reserved slots still need a value

Fill every unused slot with `Default_Handler`. An empty slot is `0x00000000`,
which is *not* a valid handler address — the CPU would jump to the vector
table itself and fault.

The F103 file shows the pattern for its own reserved tail: seven `.word 0`
entries before `BootRAM`. Yours will have **29** reserved slots scattered
through positions 19–85 (not one contiguous block).

### Verify the count

```sh
# the vector table must be exactly 102 words = 408 bytes = 0x198
arm-none-eabi-size -A build/app.elf | grep isr_vector
# expect:  .isr_vector   00000198
```

If you get `0x174` (372) you have 93 words. If you get `0x19c` you have 103.

---

## 3. Startup file: what changes and what doesn't

### Changes

| Item | F103 reference | F411 |
|---|---|---|
| `.cpu` directive | `.cpu cortex-m3` | **`.cpu cortex-m4`** |
| `.fpu` directive | `.fpu softvfp` | **`.fpu fpv4-sp-d16`** (or omit) |
| `bl SystemInit` | present | **omit** — you have no CMSIS `system_stm32f4xx.c` |
| `bl __libc_init_array` | present | your call (see below) |
| `bl main` then `bx lr` | present | keep, or `bl main` + infinite loop |
| Vector table | 74 words | **102 words** |
| IRQ names | 43 F103 names | **F411 names from RM0383 Table 37** |
| `.word BootRAM` | present | **remove** — F411 has no RAM boot mode |

**`.cpu cortex-m3` will still assemble and link.** The assembler does not police
the `-mcpu` in a `.s` file against your C flags, and the M3/M4 instruction sets
are nearly identical. So this error is silent too. Set it right.

### The one trap the F103 file gets right — and you must copy

```asm
  .section .isr_vector,"a",%progbits
```

The `"a"` means **allocatable**. Without it the section is not marked loadable,
and the linker will:

1. not advance the FLASH region for it,
2. place `.text` at `0x08000000` as well — **overlapping** the vector table,
3. leave `.isr_vector` in **no program header**, so it is never written to flash.

The build succeeds. The flash succeeds. The chip then reads `0xFFFFFFFF` as its
vector table → `PC = 0xFFFFFFFE`, `MSP = 0xFFFFFFFC` → double fault → lockup.

This was observed and root-caused on this very board. **Check it every build:**

```sh
arm-none-eabi-readelf -S build/app.elf | grep isr_vector   # must show flag "A"
arm-none-eabi-objdump -h build/app.elf                      # .text must NOT be 0x08000000
```

Correct placement for the F411:

```
.isr_vector   0x198  @ 0x08000000
.text         0xxxx  @ 0x08000198     <- 0x198 = 408 bytes of vector table
```

### `__libc_init_array` — decide, don't copy

The F103 file calls `bl __libc_init_array` to run C++ static constructors and
`__attribute__((constructor))` functions. You are writing C11 with no C++ and
no constructors. Calling it is *harmless if the symbol exists* (newlib provides
it) but pulls in a dependency you do not need. **Your call** — but decide it, and
be able to say why.

### Weak aliases

The F103 file ends with ~60 blocks of:

```asm
  .weak NMI_Handler
  .thumb_set NMI_Handler,Default_Handler
```

This means: "if the application never defines `NMI_Handler`, use
`Default_Handler`." The vector table references `NMI_Handler`; the weak alias
provides it. If your app *does* define it, the strong definition wins
automatically — no linker script change needed.

You need one such block for **every** handler name you put in the vector table.
For 86 IRQs that is 86 blocks. Write a `.macro` to avoid typing them:

```asm
.macro def_irq name
  .weak \name
  .thumb_set \name,Default_Handler
.endm
```

Then `def_irq I2C1_EV_IRQHandler` etc. Your macro, your call — but understand
what the F103 file is doing before you abbreviate it.

---

## 4. Linker script: what changes

### `MEMORY` block

```ld
MEMORY
{
FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K    /* was 128K */
RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K    /* was 20K  */
}
```

### `_estack`

```ld
_estack = 0x20020000;    /* was 0x20004FFF */
```

### The `/DISCARD/` block — **do not copy this**

```ld
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }
```

The Ac6 System Workbench script discards libc/libm/libgcc because SWB links
those separately. **You are not using SWB.** If you copy this you discard
`memcpy`, `memset` and the compiler's division helpers — the link will fail with
undefined references, or worse, succeed and misbehave.

### `.ARM.extab` / `.ARM.exidx`

Keep them if you link C++ exceptions. For pure C with no exceptions they are
empty, but keeping the section costs nothing and prevents a surprise later.

### `.user_heap_stack`

The F103 script reserves `_Min_Heap_Size` + `_Min_Stack_Size` at the top of RAM
so the linker errors if RAM overflows. That is a genuinely good idea. You have
no heap (no `malloc`), so you only need the stack part. Decide the size —
4 KB is comfortable for the I2C work coming in Phase 1.

### `AT> FLASH` and `_sidata`

```ld
  _sidata = LOADADDR(.data);
  .data : { ... } >RAM AT> FLASH
```

`>RAM` = where it **runs** (VMA). `AT> FLASH` = where it is **loaded from**
(LMA). `_sidata` is the LMA, and the startup code copies LMA→VMA. This is
identical between F103 and F411 — the only difference is the flash size, so
`_sidata` lands at a different address.

---

## 5. The libopencm3 template — a different philosophy, worth understanding

`libopencm3_linker.ld.S` is **not** an STM32 file. It is a *parameterised
template*: a Python script (`genlink.py`) reads a device database
(`ld/devices.data`) and substitutes `-D_ROM=128K -D_RAM=20K` style macros.

### Its naming convention differs — this is the trap

| libopencm3 | ST convention (F103 file) | Meaning |
|---|---|---|
| `_data` | `_sdata` | start of `.data` in RAM |
| `_edata` | `_edata` | end of `.data` in RAM |
| `_ebss` | `_ebss` | end of `.bss` |
| `_data_loadaddr` | `_sidata` | LMA of `.data` (source for the copy) |
| `_stack` | `_estack` | initial stack pointer |
| — | `_sbss` | start of `.bss` |

**Do not mix the two conventions.** If your linker script exports `_data` and
your startup file reads `_sdata`, the link fails with undefined references. Pick
the ST convention (`_sdata`/`_edata`/`_sbss`/`_ebss`/`_sidata`/`_estack`) since
that is what `docs/PLAN.md` and the reference startup file use, and write the
matching names in **both** files.

### Its vector table is inside `.text`

```ld
  .text : {
    *(.vectors)    /* Vector table */
    *(.text*)
    ...
  } >rom
```

libopencm3 puts the vector table in a `.vectors` section *inside* `.text` rather
than in a separate `.isr_vector` output section. It works. The ST approach
(a separate `.isr_vector` output section, placed first) is more explicit about
"this must be at address 0" and is what you should use — it makes the failure
mode visible in `objdump -h`.

### It uses `PROVIDE` for the stack

```ld
PROVIDE(_stack = ORIGIN(ram) + LENGTH(ram));
```

This *derives* the stack top from the RAM region instead of hardcoding it. It
cannot drift out of sync with `MEMORY`. Worth copying.

### Its conditional regions

`#if defined(_CCM)` / `_EEP` / `_XSRAM` / `_XDRAM` blocks exist for MCUs with
core-coupled memory, EEPROM, or external SDRAM. **The F411CEU6 has none of
these** — it has plain Flash and plain SRAM. Skip them.

### The device database entry format

```
stm32f103?b* stm32f1 ROM=128K RAM=20K
```

So a hypothetical F411 entry would be:

```
stm32f411?e* stm32f4 ROM=512K RAM=128K
```

Note the pattern matching: `?` = exactly one char, `*` = zero or more.
`stm32f411?e*` matches `stm32f411ce` + `u6`. This is the trick that lets one
template serve hundreds of parts.

---

## 6. Build flags — F103 reference vs F411

The F103 startup file says `.cpu cortex-m3` / `.fpu softvfp`. Your C flags must
match your MCU:

```sh
-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
```

The F411 has a single-precision FPU. `hard` ABI means float arguments are passed
in VFP registers — **which requires the FPU to be enabled at runtime**. That
means setting `SCB->CPACR` (bits 20–23) in your `Reset_Handler` *before* any
float code runs, or you get a HardFault the moment you use a float.

The F103 has **no FPU at all**, so the reference teaches you nothing here. This
is a real difference you must design for, and it belongs in the startup file.

Toolchain facts already verified on this machine:

- `arm-none-eabi-gcc 16.2.0`, `ld 2.47`
- newlib installed at `/usr/arm-none-eabi` (headers + `libc.a`)
- hard-float multilib present: `thumb/v7e-m+fp/hard`
- working link line:
  `-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -nostartfiles -T <ld> --specs=nano.specs --specs=nosys.specs`

---

## 7. Checklist before you flash

```sh
# 1. vector table is 102 words and is allocated
arm-none-eabi-size -A build/app.elf | grep isr_vector     # .isr_vector  00000198
arm-none-eabi-readelf -S build/app.elf | grep isr_vector  # must show flag "A"

# 2. .text does NOT overlap the vector table
arm-none-eabi-objdump -h build/app.elf | grep -E "isr_vector|\.text"
# .isr_vector @ 0x08000000
# .text       @ 0x08000198   <-- 408 bytes later

# 3. entry point and stack are right
arm-none-eabi-nm build/app.elf | grep -E "_estack|Reset_Handler"

# 4. nothing unexpected in RAM
arm-none-eabi-size build/app.elf
```

Then flash, and if the LED does not blink, attach the debugger and look at
`pc`/`msp` first:

- `pc = 0xFFFFFFFE`, `msp = 0xFFFFFFFC` → the vector table is not in flash
  (section not allocated, or the table is not at `0x08000000`)
- `pc = 0x08000xxx` → the chip booted and crashed *inside your code*; now it is
  a normal C debugging problem
- `msp` not near `0x20020000` → `_estack` is wrong

---

## 8. One-page summary of the F103 → F411 delta

```
F103xB (Cortex-M3)              F411CEU6 (Cortex-M4F)
---------------------------     ---------------------------
FLASH 128K @ 0x08000000    ->   FLASH 512K @ 0x08000000
RAM    20K @ 0x20000000    ->   RAM  128K @ 0x20000000
_estack 0x20004FFF         ->   _estack 0x20020000
.cpu cortex-m3             ->   .cpu cortex-m4 (+ FPU enable)
.fpu softvfp               ->   .fpu fpv4-sp-d16
67 vector words            ->   102 vector words
43 external IRQs           ->   57 named in 86 slots (29 reserved)
F103 IRQ names             ->   F411 IRQ names (RM0383 Table 37)
bl SystemInit              ->   (removed - no CMSIS)
libc/libm/libgcc /DISCARD/ ->   (removed - not SWB)
_sdata/_edata/_sbss/_ebss  ->   same names (keep ST convention)
_sidata = LOADADDR(.data)  ->   same
```
