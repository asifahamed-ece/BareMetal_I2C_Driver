# Where the addresses come from — page references

**The BlackPill board has no memory of its own.** The STM32F411CEU6 MCU inside it
does. The board is just a carrier that exposes pins and power. So:

- **Memory map, flash layout, vector table** → **Reference Manual (RM0383)**
- **Pin names, package pinout, AF table** → **Datasheet**
- **Core peripherals (SysTick, NVIC, CPACR)** → **Programming Manual (PM0214)**

Rendered page images are in this folder. Open `index.html` in a browser.

Source PDFs: `~/Downloads/STM32F411CEU6/`

---

## The four pages that matter for this project

| # | What | Document | Section | Printed page | PDF page | Image |
|---|---|---|---|---|---|---|
| 1 | Memory map vs boot mode | RM0383 | §2.2.3, **Table 3** | 42 | 42 | `rm0383_p42_memory_map.png` |
| 2 | Flash sector layout | RM0383 | §2.2.3, **Table 4** | 44 | 44 | `rm0383_p44_flash_sectors.png` |
| 3 | Vector table (102 words) | RM0383 | §10.1.3, **Table 37** | 201–204 | 201–204 | `rm0383_p20*_vector_table_*.png` |
| 4 | Pin definitions (pin name, type, functions) | Datasheet | **Table 8** | 44–46 | 43–47 | open the PDF |
| 5 | **Alternate function → AF number mapping** | Datasheet | **Table 9** | 47–52 | 47–52 | open the PDF |

---

## Page 1 — Memory map vs boot mode (RM0383 p42)

This is the page that answers "where does memory live?" and "what does BOOT0 do?".

The relevant rows of **Table 3**:

```
Address range            Contents
0x2000 0000 - 0x2002 0000   SRAM1 (128 KB)      <- your RAM
0x1FFF 0000 - 0x1FFF 77FF   System memory       <- the ROM DFU bootloader
0x0800 0000 - 0x0807 FFFF   Flash memory        <- your flash
0x0000 0000 - 0x0007 FFFF   Flash (512 KB) ALIASED   <- what the CPU sees at reset
```

**The last row is the one that matters most.** The CPU's reset vector fetch
happens at `0x00000000`, and `0x00000000` is an **alias** of `0x08000000`.
That is the entire reason the vector table must be the first bytes in flash,
and the reason `0x08000000` appears in your linker script's `MEMORY` block.

Also on this page: the list of peripherals the embedded bootloader can use
(I2C1 on PB6/PB7 — which is exactly the pin pair your OLED is on).

---

## Page 2 — Flash sector layout (RM0383 p44)

**Table 4. Flash module organization (STM32F411xC/E)**

```
Sector 0   0x0800 0000 - 0x0800 3FFF   16 Kbytes
Sector 1   0x0800 4000 - 0x0800 7FFF   16 Kbytes
Sector 2   0x0800 8000 - 0x0800 BFFF   16 Kbytes
Sector 3   0x0800 C000 - 0x0800 FFFF   16 Kbytes
Sector 4   0x0801 0000 - 0x0801 FFFF   64 Kbytes
Sector 5   0x0802 0000 - 0x0803 FFFF  128 Kbytes
Sector 6   0x0804 0000 - 0x0805 FFFF  128 Kbytes
Sector 7   0x0806 0000 - 0x0807 FFFF  128 Kbytes
```

Why you care:

- `st-flash` / OpenOCD's flash driver **erase by sector**. A program in sector 0
  gets erased when you write sector 0.
- Flash is erased in **whole sectors** — you cannot rewrite a single byte
  without erasing its sector. This is the same property NOR flash has, and it
  is why the F411's flash wear is a real consideration for a long-running
  logger.
- Your 512 KB is 8 sectors. `st-info --probe` reported
  `flash: 524288 (pagesize: 16384)` — 16 KB pages, matching sectors 0–3.

---

## Page 3 — Vector table (RM0383 p201–204, Table 37)

**The table you need to reproduce in `startup_stm32f411.s`.**

Layout:

| Table index | Content |
|---|---|
| 0 | Initial SP (`_estack`) |
| 1 | Reset handler |
| 2–15 | System exceptions: NMI, HardFault, MemManage, BusFault, UsageFault, 4×reserved, SVC, DebugMon, reserved, PendSV, SysTick |
| 16–101 | **86 external IRQ slots**, positions 0–85 |

**The address formula:**

```
address of external IRQ n  =  0x40 + 4 × n
```

Check it: `SPI5` is the last one, position **85** → `0x40 + 340 = 0x194`.
`0x194 / 4 = 101` → table index 101, + 16 system slots = **102 words = 408 bytes = 0x198**.

**The trap:** Table 37 names only **57** of the 86 slots. The other 29
positions are *reserved on the F411xC/E* — but they still occupy address
space, so the table must still have 102 words. The reserved positions are:

```
19, 20, 21, 22, 39, 43, 44, 45, 46, 48, 52, 53, 54, 55,
61, 62, 63, 64, 65, 66, 74, 75, 76, 77, 78, 79, 80, 82, 83
```

Fill them with `Default_Handler`, **never `0`**.

Also note two shared-vector positions on the F411:
- position **41** — `EXTI17 / RTC_Alarm`
- position **42** — `EXTI18 / OTG_FS_WKUP`

Two sources share one vector. Name it for whichever you actually use.

A ready-made list of all 86 slots is in
[`../reference/f411_external_irqs.inc`](../reference/f411_external_irqs.inc).

---

## Datasheet — pins and alternate functions

| What | PDF page |
|---|---|
| Table 1 — device summary (Flash 512 KB, SRAM 128 KB) | 1, 6 |
| Pin definition / package pinout | 6 |
| **Table 8** — pin definitions (pin name, type, all functions per pin) | 43–47 |
| **Table 9** — alternate function mapping (**function → AF number**) | 47–52 |

**Two different tables, two different jobs.** This trips people up:

- **Table 8** tells you *which functions a pin can have* (PB6 can be SPI3_MOSI,
  USART1_TX, TIM4_CH1, **I2C1_SCL**, …) — it does **not** give the AF number.
- **Table 9** gives the **AF number** for each (pin, function) pair. This is the
  table that tells you what to write into `GPIOx_AFRL` / `GPIOx_AFRH`.

**Table 9 is the one for the I2C work.** On PDF page 48, the `PB6` row reads:

```
                        AF00  AF01  AF02  AF03  AF04  AF05 ...
PB6                     -     -     -     -   I2C1_SCL  - ...
```

So `PB6 → I2C1_SCL` is in the **AF04** column. Same for `PB7 → I2C1_SDA`.
**AF4** — that is the value that goes into `GPIOx_AFRL`.

(Verified by coordinate extraction, not eyeballed: on PDF page 48 the `AF04`
column header sits at x=338.6 and the `I2C1_SCL` cell on the PB6 row spans
x=331.0–363.2.)

For the SWD pins, Table 8 on PDF page 44 shows:

```
PA13   JTMS-SWDIO
PA14   JTCK-SWCLK
```

Both **AF0** — the SWD interface is on its dedicated alternate function and is
not remapped, which is why your ST-Link works on the default pin mapping.

---

## Programming Manual (PM0214) — the core peripherals

These are **not** in RM0383, because they belong to the ARM core, not to ST.

| What | Section | PDF page |
|---|---|---|
| Vector table offsets, exception model | §2.3.4 | ~50 |
| SysTick | §4.5 | ~246 |
| **CPACR — enabling the FPU** | **§4.6.1** | **253** |
| FPU enable sequence (code example) | §4.6.6 | **257** |

**§4.6.6 is the exact sequence your `Reset_Handler` implements:**

```asm
LDR.W    R0, =0xE000ED88     ; CPACR
LDR      R1, [R0]
ORR      R1, R1, #(0xF << 20)   ; CP11/CP10 = full access
STR      R1, [R0]
DSB                          ; wait for the store to complete
ISB                          ; reset the pipeline - FPU now enabled
```

And §4.6.1 explains the field encoding: bits 23:20 hold `CP11`/`CP10`, each
2 bits, where `0b11` = full access, `0b00` = access denied (any attempt
raises a NOCP UsageFault).

**This is why the FPU enable must be the FIRST thing in `Reset_Handler`.** The
F411's FPU is disabled from reset, and this project compiles with
`-mfloat-abi=hard`. Any float instruction before the CPACR write would fault.

---

## Quick index — which document answers which question

| Question | Document | Where |
|---|---|---|
| Where is flash? | RM0383 | Table 3, p42 |
| Where is RAM? | RM0383 | Table 3, p42 |
| How do I erase flash? | RM0383 | Table 4, p44 |
| What is the vector table? | RM0383 | Table 37, p201 |
| Which IRQ is I2C1_EV? | RM0383 | Table 37, p203 (position 31) |
| What is the AF number for PB6/PB7? | Datasheet | **Table 9**, p48 |
| What is PC13's alternate function? | Datasheet | Table 8, p43 |
| How do I enable the FPU? | PM0214 | §4.6.6, p257 |
| How do I set up SysTick? | PM0214 | §4.5, ~p246 |
| How do I configure the NVIC? | PM0214 | §4.3 |
| What are the RCC registers? | RM0383 | §6.3, p118–136 |

---

## Verifying these numbers against your actual board

The reference manual states the *part's* capabilities. To confirm *your* chip:

```sh
st-info --probe
#   flash:      524288 (pagesize: 16384)   <- matches Table 4 sector size
#   sram:       131072                      <- matches Table 3
#   chipid:     0x431                       <- STM32F411xC/E
```

That is how the numbers in `stm32f411.ld` were confirmed — not read off a
datasheet and hoped for.
