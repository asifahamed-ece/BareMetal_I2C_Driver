# BareMetal I2C Driver — Learning & Build Plan

**Status:** Draft for review  
**Date:** 2026-10-20  
**Repository:** `~/Projects/BareMetal_I2C_Driver`  
**Target board:** WeAct STM32F411CEU6 BlackPill V3.0  
**Test devices:** SSD1306 OLED (write-only) and MPU6050 (GY-521)  
**Toolchain:** `arm-none-eabi-gcc`, Make, OpenOCD + ST-Link, no HAL  
**Working mode:** You write the code; the mentor guides, reviews, and debugs with you.

---

## 1. Purpose

Turn the current single-file SSD1306 demo into a real, reusable baremetal I2C master driver — and use the process to build deep, working knowledge of:

- the clock tree and how peripheral timing is derived from it
- GPIO alternate functions, open-drain outputs, and bus electrical behaviour
- the I2C protocol at the register level: START, address, ACK/NACK, data, repeated START, STOP
- bounded waits, error codes, and bus recovery
- NVIC, interrupts, and event-driven state machines
- DMA transfers and peripheral-to-memory data flow
- debugging with SWD, UART logs, and a logic analyzer

Success is **not** "the OLED lights up". Success is:

1. You can explain every register write without looking at a guide.
2. The driver returns errors instead of hanging.
3. It works with at least two different I2C devices.
4. It survives fault injection: wrong address, unplugged device, stuck bus.
5. The build, flash, and test process is reproducible from a clean clone.

---

## 2. What this project is and is not

**It is:**

- a learning-first, no-HAL baremetal project
- a generic 7-bit I2C master, first polling, then interrupt-driven, then DMA
- device drivers for the SSD1306 and MPU6050 on top of the generic master
- an evidence-based test effort: UART logs, logic-analyzer captures, stress results

**It is not:**

- a HAL clone or a copy of ST's `stm32f4xx_hal_i2c.c`
- a multi-master or SMBus implementation in the first release
- a port to every board you own; the H743/ESP32 ports are optional later phases
- an OLED project with I2C bolted on — the bus driver is the product, the OLED is a client

**Important hardware fact:** the SSD1306's I2C interface is **write-only**. There is no read path into its GDDRAM or registers. "Read support" in this project means reading from other targets, starting with the MPU6050. The display keeps a shadow framebuffer in MCU RAM.

---

## 3. Starting point

Current repository:

- `src/main.c` — I2C1 init, polling write, SSD1306 init/framebuffer, SysTick delay, demo.
- `src/blackpill_stm32f411.h` — hand-written register map, with some defects.
- `docs/` — images and a demo GIF; no build system.
- Known weaknesses: no timeouts, no NACK handling, no read path, no repeated START, hard-coded 16 MHz, monolithic structure, no reproducible build.

Plan: preserve the current working state as a reference, build the new structure alongside it, then retire the old `main.c` once the new demo matches it.

Recommended git steps before Phase 0:

```sh
git tag v0-demo
git switch -c driver-v1
```

---

## 4. Decisions and constraints

| Area | Decision | Reason |
|---|---|---|
| MCU | STM32F411CEU6 | Owned; Cortex-M4; well documented |
| Build | Make + `arm-none-eabi-gcc` | Transparent; no IDE magic |
| Flash/debug | OpenOCD + ST-Link | SWD breakpoints; cheap and scriptable |
| HAL | None | Learning goal |
| Language | C11, no dynamic allocation | Baremetal discipline |
| Addressing | 7-bit first; 10-bit in Phase 4 | YAGNI |
| Clock | HSI 16 MHz explicit first; PLL to 100 MHz in Phase 0 | Learn the clock tree before depending on it |
| I2C speed | 100 kHz first; 400 kHz after timing is verified | Conservative bring-up |
| First read target | MPU6050 | Real register reads with a known ID |
| Display | SSD1306, write-only | Existing hardware |
| API style | Blocking first, async later | Progressive complexity |

**Assumptions to confirm:**

- You have an ST-Link (or the BlackPill's SWD header) and a USB-serial adapter for UART logs.
- The MPU6050 module is 3.3 V compatible and has onboard pull-ups (verify the schematic; many GY-521 boards use 4.7 kΩ).
- You can obtain a logic analyzer (recommended) or use a spare MCU as an I2C slave/fault injector.

---

## 5. End-state architecture

```text
app (main.c)
 ├── mpu6050 (device driver)
 ├── ssd1306 (device driver)
 └── i2c_master (public API: write / read / write_read / probe / recover)
      └── i2c1_ll (F411 registers, timing, IRQ/DMA)
           └── board (clock, GPIO/AF, pins, timebase, UART)
```

Rules:

- `i2c_master` knows nothing about the SSD1306 or MPU6050.
- Device drivers know nothing about F411 registers.
- Only the LL layer touches `I2C1_*` registers.
- Public headers expose opaque handles and status codes; register types do not leak.
- Every wait is bounded. No infinite loop is allowed in the driver.

---

## 6. Public API sketch

This is a starting point, not final. It will be reviewed before Phase 1.

```c
typedef enum {
    I2C_OK = 0,
    I2C_ERR_PARAM,
    I2C_ERR_NACK,
    I2C_ERR_TIMEOUT,
    I2C_ERR_BUSY,
    I2C_ERR_ARLO,
    I2C_ERR_BERR,
    I2C_ERR_BUS
} i2c_status_t;

typedef struct i2c_bus i2c_bus_t;

i2c_status_t i2c_init(i2c_bus_t *bus, const i2c_config_t *cfg);

i2c_status_t i2c_write(i2c_bus_t *bus, uint8_t addr7,
                       const uint8_t *data, size_t len,
                       uint32_t timeout_ms);

i2c_status_t i2c_read(i2c_bus_t *bus, uint8_t addr7,
                      uint8_t *data, size_t len,
                      uint32_t timeout_ms);

i2c_status_t i2c_write_read(i2c_bus_t *bus, uint8_t addr7,
                            const uint8_t *wdata, size_t wlen,
                            uint8_t *rdata, size_t rlen,
                            uint32_t timeout_ms);

i2c_status_t i2c_probe(i2c_bus_t *bus, uint8_t addr7, uint32_t timeout_ms);
i2c_status_t i2c_bus_recover(i2c_bus_t *bus);
```

Phase 3 adds an asynchronous form:

```c
i2c_status_t i2c_transfer_async(i2c_bus_t *bus,
                                const i2c_transfer_t *xfer,
                                void (*done)(i2c_bus_t *, i2c_status_t, void *),
                                void *ctx);
```

---

## 7. Repository layout

```text
BareMetal_I2C_Driver/
├── Makefile
├── stm32f411.ld
├── startup_stm32f411.s
├── include/
│   ├── stm32f411.h          # cleaned register map
│   ├── board.h
│   ├── clock.h
│   ├── timebase.h
│   ├── uart.h
│   ├── gpio.h
│   ├── i2c.h
│   └── ...
├── src/
│   ├── main.c
│   ├── clock.c
│   ├── timebase.c
│   ├── uart.c
│   ├── gpio.c
│   ├── i2c1_ll.c
│   ├── i2c_master.c
│   └── ...
├── devices/
│   ├── ssd1306.c / ssd1306.h
│   └── mpu6050.c / mpu6050.h
├── tests/
│   ├── test_i2c_polling.c
│   ├── test_i2c_irq.c
│   └── ...
├── docs/
│   ├── PLAN.md              # this document
│   ├── journal.md
│   ├── waveforms/
│   └── ...
└── tools/
    ├── openocd.cfg
    └── ...
```

The existing `docs/` directory is lowercase; use it rather than creating a second `Docs/`.

---

## 8. Build, flash, and debug

- `make` builds ELF/hex/bin; `make flash` runs OpenOCD; `make size` shows section sizes; `make debug` starts GDB.
- Compiler flags: `-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -Wall -Wextra -Werror -Og -g3`.
- Startup: write your own vector table and `Reset_Handler`; copy `.data`, zero `.bss`, call `main`.
- Linker: FLASH 512 KB at `0x08000000`, RAM 128 KB at `0x20000000`, stack top at end of RAM.
- No newlib `printf` at first; write a small UART formatter. Revisit newlib nano only if needed.
- Debug: SWD breakpoints, watchpoints, and a HardFault handler that prints `PC`, `LR`, and `xPSR`.

---

## 9. Phase 0 — Foundations (brush-up)

**Goal:** a board support layer you understand, before touching I2C.

### 0.1 Toolchain and blinky

- Makefile, startup file, linker script, OpenOCD config.
- Prove: LED blinks; a GDB breakpoint hits; `make size` reports sane sections.

### 0.2 Clock tree

- Read RM0383 Chapter 6 (RCC), especially PLL configuration.
- Configure HSI 16 MHz explicitly, then the PLL to 100 MHz (AHB 100, APB1 50, APB2 100).
- Verify: MCO output, or SysTick timing, or a correct UART baud rate.
- Explain: PLLM/PLLN/PLLP/PLLQ, flash latency, voltage regulator, APB prescalers.

### 0.3 Timebase

- SysTick 1 ms interrupt; DWT `CYCCNT` or TIM2 for microseconds.
- Implement `timebase_ms()`, `timebase_us()`, `delay_us()`.
- Prove: toggle a GPIO every 1 ms and measure it.

### 0.4 UART

- USART2 (PA2/PA3) or USART1 (PA9/PA10), 115200 8N1.
- Polled TX first, then RX interrupt; implement a minimal `uart_printf`.
- Prove: typed characters echo; logs appear in a terminal.

### 0.5 GPIO helper

- `gpio_mode`, `gpio_af`, `gpio_set`, `gpio_get`, open-drain support.
- Prove: reuse it for LED, button, UART, and I2C pins.

### 0.6 NVIC and EXTI

- Button on EXTI, debounce in the ISR or via the tick.
- Read PM0214 for NVIC/SysTick — these are core peripherals, not in RM0383.
- Prove: one interrupt per press; priority behaves as expected.

### 0.7 Timer

- TIM2 update interrupt at 1 kHz, or input capture to measure a pulse.
- Prove: interrupt cadence; captured width.

### 0.8 Fault handling

- HardFault handler that prints `PC`, `LR`, `xPSR`.
- Provoke a divide-by-zero or null dereference.
- Prove: decode the fault with GDB.

**Deliverable:** `clock`, `timebase`, `uart`, `gpio` modules and a test log.  
**Explain-back:** clock tree, AHB vs APB1, why SysTick is 24-bit, NVIC priority.

---

## 10. Phase 1 — Polling I2C master

**Goal:** a correct, bounded, generic master.

### 1.1 Read the protocol

- RM0383 Chapter 18, pages 472–504: START/STOP, addressing, ACK/NACK, clock stretching, SR1/SR2.
- Datasheet Table 9 for PB6/PB7 AF4.
- Draw the state machine on paper before writing code.

### 1.2 Bus bring-up

- PB6/PB7 AF4, open-drain, external 4.7 kΩ pull-ups.
- 100 kHz derived from PCLK1; compute `FREQ`, `CCR`, `TRISE`.
- Implement `i2c_probe` scanning `0x08`–`0x77`.
- Prove: OLED (`0x3C`) and MPU6050 (`0x68`) appear; no false positives.
- Capture the scan on a logic analyzer.

### 1.3 Byte primitives

- `start()`, `send_addr()`, `write_byte()`, `read_byte(ack)`, `stop()`, `repeated_start()`.
- Clear `ADDR` by reading `SR1` then `SR2`.
- Prove: write one byte to the OLED; read `WHO_AM_I` from the MPU6050.
- Capture START, address, ACK, data, STOP.

### 1.4 Timeouts and errors

- Bound every poll with the microsecond timebase.
- Check `AF` (NACK), `BUSY` before START, `BERR`, `ARLO`.
- Return `i2c_status_t`; no silent failures.
- Prove: wrong address returns `I2C_ERR_NACK`, not a hang; unplugged device returns `I2C_ERR_TIMEOUT`.

### 1.5 High-level transfers

- `write`, `read`, `write_read`; zero-length and bounds checks.
- Repeated START for register reads.
- Prove: read MPU6050 registers; write OLED commands.

### 1.6 Bus recovery

- Nine SCL pulses plus STOP; `SWRST`; re-init.
- Prove: hold SDA low with a jumper or another MCU, recover, next transfer works.

### 1.7 Stress and fault injection

- 10k random transfers; wrong addresses; disconnect during a transfer; injected noise.
- Prove: no hang, no corruption; error counters match expectations.

**Deliverable:** `i2c1_ll.c`, `i2c_master.c`, `i2c.h`, test log, waveforms.  
**Explain-back:** why open-drain, the ADDR clear sequence, CCR/TRISE math, repeated START, recovery.

---

## 11. Phase 2 — Device drivers and application

### 2.1 SSD1306

- Init sequence, charge pump, addressing mode, framebuffer, flush, primitives.
- Keep it write-only and document that limitation.
- Prove: reproduce the existing demo through the new API.

### 2.2 MPU6050

- `WHO_AM_I`, reset, wake, accel/gyro configuration, burst read, scaling.
- Prove: stable readings; tilt the board and watch the values change.

### 2.3 Application

- Read the IMU at 10–50 Hz; display accel/gyro on the OLED; UART log; button pages.
- Prove: end-to-end demo; unplug the sensor and get an error screen, not a hang.

**Deliverable:** `devices/` drivers and the application.  
**Explain-back:** register maps, burst reads, signed data, scaling.

---

## 12. Phase 3 — Interrupt-driven master

### 3.1 I2C interrupts

- `ITEVTEN`, `ITBUFEN`, `ITERREN`; event and error flags; NVIC priority.
- Read RM0383 §18.4 and Table 70.

### 3.2 State machine

- States: `IDLE`, `START`, `ADDR_W`, `ADDR_R`, `TX`, `RX`, `STOP`, `ERROR`.
- The ISR handles flags; the main thread starts transfers.
- Prove: the same test suite passes; a GPIO toggles during a transfer, proving the CPU is not blocked.

### 3.3 Async API

- `i2c_transfer_async` with a completion callback; busy flag; queue depth 1 first.
- Timeout via SysTick; abort path.
- Prove: back-to-back transfers; timeout recovery; no race.

### 3.4 Concurrency

- `volatile` state, critical sections, no blocking in the ISR.
- Prove: stress with interrupts; logic analyzer shows correct frames.

**Deliverable:** async driver and a blocking-vs-IRQ CPU-time comparison.  
**Explain-back:** ISR flow, flag clearing, priority, reentrancy.

---

## 13. Phase 4 — Full stack

### 4.1 DMA

- DMA1 streams for I2C1 TX/RX; verify the request mapping in RM Table 35.
- Memory/peripheral increments, transfer complete, error flags.
- Prove: 1 KB OLED flush via DMA; CPU free during the transfer.

### 4.2 10-bit addressing

- Header `11110xx`; read and write; test with a slave MCU.
- Prove: 10-bit scan and transfer.

### 4.3 Multi-master

- `BUSY`, `ARLO`, own address; two masters on one bus.
- Prove: arbitration loss handled; bus recovers.

### 4.4 400 kHz fast mode

- `CCR`/`TRISE` for fast mode; rise time; pull-up sizing.
- Prove: stable at 400 kHz; 10k transfers without errors.

### 4.5 Portability (optional)

- Abstract the LL interface; port to the H743 (I2Cv2) or ESP32.
- Only after the F411 driver is stable and documented.

### 4.6 Power

- Disable peripheral clocks when idle; use `__WFI()`; measure current.

**Deliverable:** full driver and port notes.

---

## 14. Phase 5 — Documentation and release

- README: features, wiring, build/flash, API, limitations.
- Doxygen comments with register references.
- Test report: what passed, waveforms, known issues.
- Distill the journal into a short "what I learned" document.
- Tag `v1.0`.
- Optional: blog post or demo video.

---

## 15. Testing strategy

**Evidence levels:**

1. Compiles with `-Wall -Wextra -Werror`.
2. UART log shows the expected status codes.
3. Logic-analyzer capture matches the protocol.
4. Fault injection passes.
5. Stress test passes.

**Test matrix:**

| Test | Phase | Pass criterion |
|---|---|---|
| Probe scan | 1 | `0x3C` and `0x68` found; no false positives |
| `WHO_AM_I` | 1 | Returns `0x68` |
| OLED init | 2 | Display matches the old demo |
| IMU read | 2 | Values change with motion |
| NACK | 1 | `I2C_ERR_NACK`, no hang |
| Timeout | 1 | `I2C_ERR_TIMEOUT`, bus recovers |
| SDA stuck | 1 | Recovery succeeds |
| Async stress | 3 | 10k transfers, no lost frames |
| DMA flush | 4 | 1 KB correct, CPU free |
| 400 kHz | 4 | 10k transfers, no errors |

No "done" without evidence. Keep captures in `docs/waveforms/`.

---

## 16. Mentoring contract

- You write all production code.
- The mentor provides: reading assignments with page ranges, Socratic questions, pseudo-code, review comments, and debugging guidance.
- No complete driver will be pasted. If you are stuck for more than 30 minutes, you get a hint; if still stuck, a minimal snippet with an explanation.
- Every session ends with: what changed, evidence, what is next, and one explain-back question.
- Use the `systematic-debugging` skill when something fails, and `verification-before-completion` before claiming done.
- Commit at least once per milestone; one-line messages.
- Keep `docs/journal.md`; it is part of the deliverable.

---

## 17. Milestone acceptance checklist

For every milestone:

- [ ] Code compiles clean with `-Wall -Wextra -Werror`.
- [ ] Test or evidence recorded.
- [ ] Explain-back answered without notes.
- [ ] Journal entry written.
- [ ] Commit made.
- [ ] No `TODO`/`FIXME` left without a tracked issue.

---

## 18. Risks and mitigations

| Risk | Mitigation |
|---|---|
| No build-system experience | Phase 0.1; start from a minimal, known-good blinky |
| Weak internal pull-ups | External 4.7 kΩ; verify with a scope |
| 5 V logic on 3.3 V pins | Check levels; MPU6050 and OLED are 3.3 V |
| Bus lockup | Recovery routine; test with SDA held low |
| Clock miscalculation | Derive from PCLK1; verify with an analyzer |
| Interrupt races | Keep the ISR short; `volatile`; critical sections |
| Wrong DMA request mapping | Verify RM Table 35; test with a small buffer first |
| H743 port scope creep | Defer to Phase 4.5; do not block v1.0 |
| No logic analyzer | Use a spare MCU as slave/fault injector, UART logs; recommend a cheap 8-channel analyzer |

---

## 19. Open questions

1. How many hours per week can you commit? This affects pacing, not order.
2. Do you have a logic analyzer or oscilloscope? If not, do you want to budget for one?
3. Is the MPU6050 module a GY-521 with onboard pull-ups? Check the schematic.
4. Do you want to keep the existing `main.c` as a reference or rewrite from scratch?
5. Any deadline or demo date?

---

## 20. First session (next step)

1. Tag the current repo `v0-demo`.
2. Create the branch `driver-v1`.
3. Add the Makefile, startup file, and linker script; blink the LED.
4. Write the first journal entry.
5. Bring the repo back for review before Phase 0.2.

**No I2C code until Phase 0 is signed off.**
