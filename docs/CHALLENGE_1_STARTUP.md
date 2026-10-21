# Challenge 1 — Write `startup_stm32f411.s`

**You write this file. I review it. Do not paste ST's `startup_stm32f103xb.s` —
read it, understand it, then write yours for the F411.**

The linker script (`stm32f411.ld`) is done and verified. It defines the
addresses. **Your startup file is the code that uses them.**

---

## Part A — Understand before you write

Answer these in your own words first. I will ask you about them.

1. On reset, the CPU reads two words from `0x00000000`. What are they, and
   which two of your linker-script symbols do they come from?
2. Your linker script defines `_sidata`, `_sdata`, `_edata`, `_sbss`, `_ebss`,
   `_estack`. Which of these are *addresses in RAM* and which are *addresses in
   flash*? Why does one of them live in flash?
3. Why does `.data` need a copy loop at all? What would the CPU read from
   `data_thing` if the copy never happened?
4. What is the difference between `.data` and `.bss` from the *startup code's*
   point of view? Why does one need copying and the other only zeroing?
5. Your linker script reserves 4 KB for the stack. Where does the stack grow
   from, and which symbol tells the CPU where to start?
6. Your build uses `-mfloat-abi=hard`. The F411 has an FPU. The F103 does not.
   What must your startup code do that the F103's does not — and what happens
   if you forget?

---

## Part B — The algorithm

Write the file to implement **exactly** this. Do not optimise it. Do not skip
steps. You can use the F103 file as a syntax reference.

### B1. The file header

```asm
  .syntax unified
  .cpu cortex-m4
  .fpu fpv4-sp-d16
  .thumb
```

### B2. The vector table — 102 words

```asm
  .section .isr_vector,"a",%progbits
```

**The `"a"` is not optional.** Without it the section is not allocated, the
linker does not advance FLASH for it, `.text` overlaps it at `0x08000000`,
and `.isr_vector` ends up in no program header so it is never flashed. The
build succeeds. The chip locks up. (Root-caused on this board already.)

The table must contain, **in this exact order**:

| Words | Content |
|---|---|
| 0 | `_estack` — the initial stack pointer |
| 1 | `Reset_Handler` |
| 2 | `NMI_Handler` |
| 3 | `HardFault_Handler` |
| 4 | `MemManage_Handler` |
| 5 | `BusFault_Handler` |
| 6 | `UsageFault_Handler` |
| 7–10 | `0` (reserved) |
| 11 | `SVC_Handler` |
| 12 | `DebugMon_Handler` |
| 13 | `0` (reserved) |
| 14 | `PendSV_Handler` |
| 15 | `SysTick_Handler` |
| 16–101 | the **86 external IRQ slots**, positions 0–85 |

**Total = 102 words = 408 bytes = 0x198.**

For words 16–101 use the names in `docs/reference/f411_external_irqs.inc`.
That file is the authoritative 86-slot list extracted from RM0383 Table 37 —
57 named, 29 reserved. **Reserved slots still need a value:** use
`Default_Handler`, never `0`. A `0` entry is not a valid handler address; the
CPU would jump to the vector table itself and fault.

> **The counting trap.** RM0383 Table 37 names only 57 of the 86 slots. Count
> the **highest position (85)**, not the number of rows. The address of slot
> *n* is `0x40 + 4n`, so slot 85 sits at `0x194` → index 101 → +16 system =
> 102 words. Reserved slots still occupy address space.

### B3. Weak aliases

Every handler name used in the table must exist, even if your app never
defines it. For each one:

```asm
  .weak NMI_Handler
  .thumb_set NMI_Handler, Default_Handler
```

You need this for **all 9 system handlers + all 57 named IRQs** (the reserved
slots point straight at `Default_Handler`, so they need no alias).

> 66 blocks. Write a `.macro` so you are not typing them by hand:
> ```asm
> .macro def_handler name
>   .weak \name
>   .thumb_set \name, Default_Handler
> .endm
> ```
> Your macro, your choice — but understand what the F103 file is doing before
> you abbreviate it.

### B4. `Default_Handler`

```asm
  .section .text.Default_Handler,"ax",%progbits
  .type Default_Handler, %function
Default_Handler:
Infinite_Loop:
  b Infinite_Loop
  .size Default_Handler, .-Default_Handler
```

An infinite loop — the CPU parks there so a debugger can inspect the state
that caused the fault.

### B5. `Reset_Handler` — the algorithm

```
 1.  Enable the FPU:
       CPACR is at 0xE000ED88 (PM0214 §4.6.1).
       Read it, OR in (0xF << 20)  ->  bits 23:20 = CP11/CP10 = 0b11 (full access)
       Write it back, then DSB, then ISB.
       PM0214 §4.6.6 gives the exact sequence. Use it.

 2.  Copy .data from flash to RAM:
       src  = _sidata   (in FLASH - the initialisers)
       dst  = _sdata    (in RAM)
       end  = _edata
       loop while dst < end:  *dst++ = *src++     (word at a time)

 3.  Zero .bss:
       start = _sbss
       end   = _ebss
       loop while p < end:  *p++ = 0

 4.  Call main:
       bl main
       then loop forever   (main must never return)
```

**Order matters.** FPU enable must be step 1 — if any code before it touched a
float, you would already have faulted. The `.data` copy must precede the
`.bss` zeroing is not strictly required, but keep the standard order.

### B6. Do you call `__libc_init_array`?

The F103 file calls it (it runs C++ static constructors). **Decide, and be
able to justify it.** You are writing C11 with no C++ and no
`__attribute__((constructor))`. Either choice is defensible — but it must be a
decision, not an accident. State your choice in a comment at the top of the
file.

---

## Part C — The outcome you must achieve

### C1. The file must build with

```sh
arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
  -nostartfiles -T stm32f411.ld --specs=nano.specs --specs=nosys.specs \
  -c startup_stm32f411.s -o build/startup_stm32f411.o
```

No warnings, no errors.

### C2. The vector table must be exactly 102 words

```sh
arm-none-eabi-size -A build/app.elf | grep isr_vector
#   .isr_vector          00000198      <- 0x198 = 408 bytes = 102 words
```

| Size | Meaning |
|---|---|
| `0x198` | **correct** — 102 words |
| `0x174` | 93 words — you are short 9 |
| `0x19C` | 103 words — one too many |
| `0x000` | the section is empty — the `"a"` flag is missing |

### C3. `.isr_vector` must be allocated and first

```sh
arm-none-eabi-readelf -S build/app.elf | grep isr_vector
#   .isr_vector  PROGBITS  08000000  ...  A      <- the A flag is REQUIRED

arm-none-eabi-objdump -h build/app.elf | grep -E "isr_vector|\.text"
#   .isr_vector  00000198  08000000
#   .text        00000xxx  08000198   <- 0x198 later, NOT 0x08000000
```

### C4. The symbols must match the linker script

```sh
arm-none-eabi-nm build/app.elf | grep -E "_estack|_sidata|_sdata|_edata|_sbss|_ebss"
#   20020000 R _estack     <- top of 128K RAM
#   080001c8 A _sidata     <- in FLASH (this is the copy source)
#   20000000 D _sdata      <- in RAM
```

If any of these is **undefined**, your startup file and the linker script
disagree on a name. Check for typos and for mixing conventions (libopencm3's
`_data`/`_stack` vs ST's `_sdata`/`_estack`).

### C5. The program must reach `main`

Flash it and step through:

```sh
arm-none-eabi-objdump -d build/app.elf | head -40    # see the Reset_Handler code
```

Then, with the ST-Link connected:

```sh
openocd -f interface/stlink-dap.cfg -c "transport select dapdirect_swd" \
  -f target/stm32f4x.cfg \
  -c "reset_config srst_only srst_nogate srst_open_drain connect_assert_srst" \
  -c "adapter speed 100" -c "init; reset halt; reg pc; shutdown"
```

The reported `pc` should be inside `Reset_Handler`, and `msp` should be
`0x20020000`.

**If `pc` is `0xfffffffe`** → the vector table is not in flash. Check the
`"a"` flag (C3).
**If `pc` is `0x08000xxx`** → the chip booted; the problem is now inside your
code, and it is an ordinary debugging problem.

---

## Part D — Definition of done

Send me the file, and I will check:

- [ ] `.syntax unified`, `.cpu cortex-m4`, `.fpu fpv4-sp-d16`, `.thumb`
- [ ] `.section .isr_vector,"a",%progbits` — the `"a"` present
- [ ] vector table is exactly 102 words, correct order, correct names
- [ ] 86 external slots, names from `f411_external_irqs.inc`, 29 reserved →
      `Default_Handler` (not `0`)
- [ ] weak alias + `.thumb_set` for every handler name in the table
- [ ] `Default_Handler` is an infinite loop
- [ ] FPU enable (CPACR `0xE000ED88`, `(0xF << 20)`, DSB, ISB) is **first**
- [ ] `.data` copy loop: `_sidata` → `_sdata`.._edata`
- [ ] `.bss` zero loop: `_sbss`..`_ebss`
- [ ] `bl main`, then an infinite loop
- [ ] a comment stating whether `__libc_init_array` is called, and why
- [ ] all C1–C4 checks pass
- [ ] you can answer all six Part-A questions out loud, no notes

---

## Bonus (do this only after everything above works)

HardFault on purpose: in `main`, do `*(volatile uint32_t *)0 = 1;`.
The chip should land in `Default_Handler`. Attach the debugger and read
`pc` and `lr` — `lr` should point at the faulting instruction. That is how
you will debug real crashes in Phase 1.
