# I2C Peripheral Driver — Mental Model & Implementation Flow

**Purpose:** A reusable mental model for implementing an I2C master from scratch, and for approaching any communication peripheral (SPI, UART, etc.) with the same discipline.

**Project context:** STM32F411CEU6 BlackPill, baremetal C, no HAL. The same model applies to the H743 or any other MCU after replacing the low-level layer.

---

## The one-line model

> A communication driver is a **layered translation** of a protocol specification into a **hardware state machine**, with **bounded waits** and **explicit error recovery**.

Everything below is an expansion of that sentence.

---

## Stage 0 — Understand the protocol before the peripheral

Do not start with registers. Start with a one-page protocol sheet:

- **Physical layer:** open-drain, wired-AND, pull-ups, bus capacitance, clock stretching.
- **Frame format:** START, address + R/W, ACK/NACK, data, repeated START, STOP.
- **Roles:** who is master, who is slave, what multi-master arbitration means.
- **Timing:** standard/fast mode, setup/hold, rise time, clock frequency.
- **Errors:** NACK, arbitration loss, bus error, stuck bus, timeout.

For I2C, draw this until you can do it from memory:

```text
Write:  S | addr+W | A | data0 | A | data1 | A | ... | P
Read:   S | addr+W | A | reg   | A | Sr | addr+R | A | data | A | ... | N | P
```

Where `S` = START, `Sr` = repeated START, `A` = ACK, `N` = NACK, `P` = STOP.

**Deliverable:** a protocol sheet with the frame, the legal sequences, and the error conditions. This is your specification. Registers are an implementation detail.

---

## Stage 1 — Map the MCU peripheral to that specification

Read the peripheral chapter with four questions:

1. **What are the physical pins and their alternate functions?**
   - For the F411 BlackPill: PB6/PB7, AF4, open-drain, external pull-ups.
2. **What are the control, status, and data registers?**
   - `CR1` (enable, START, STOP, ACK), `CR2` (clock, interrupts, DMA), `DR` (data), `SR1`/`SR2` (flags), `CCR`/`TRISE` (timing).
3. **What is the happy-path sequence for each operation?**
   - Master transmitter, master receiver, repeated START, 2-byte reception. The reference manual gives these as numbered procedures — extract them.
4. **How is each flag set and cleared?**
   - This is where most bugs live. Make a table: flag, set condition, clear method, required action.

Example for I2C:

| Flag | Set when | Cleared by | Action |
|---|---|---|---|
| `SB` | START generated | Read SR1, write DR | Send address |
| `ADDR` | Address matched | Read SR1, then SR2 | Clear, then TX/RX |
| `TXE` | DR empty | Write DR | Load next byte |
| `RXNE` | DR full | Read DR | Consume byte |
| `AF` | NACK received | Write 0 to AF | Abort / STOP |
| `BERR` | Misplaced START/STOP | Write 0 to BERR | Recover |
| `ARLO` | Arbitration lost | Write 0 to ARLO | Retry / recover |
| `BUSY` | Bus busy | Hardware | Wait before START |

**Deliverable:** a register/flag map and the extracted sequences. If you cannot fill this table, you do not understand the peripheral yet.

---

## Stage 2 — Bring up the electrical layer first

Before writing a protocol engine:

1. Verify both lines idle high.
2. Verify pull-ups (external 4.7 kΩ; internal ~40 kΩ is too weak for a real bus).
3. Configure pins: AF mode, open-drain, high speed, AF4.
4. Enable clocks: GPIO port + I2C peripheral.
5. Set `FREQ`, `CCR`, `TRISE` from the actual PCLK1.
6. Run an **address scanner** (`0x08`–`0x77`).

If the scanner finds `0x3C` and `0x68`, the electrical and addressing layers work. If not, stop and debug hardware before writing more code.

**Deliverable:** a probe scan with a logic-analyzer capture.

---

## Stage 3 — Build the low-level “vocabulary”

The LL layer should expose **one function per hardware action**, with no protocol logic:

```c
i2c_status_t ll_wait_flag(flag, timeout);
i2c_status_t ll_start(void);
i2c_status_t ll_stop(void);
i2c_status_t ll_write_dr(uint8_t byte);
i2c_status_t ll_read_dr(uint8_t *byte);
i2c_status_t ll_clear_addr(void);
i2c_status_t ll_set_ack(bool ack);
```

Rules:

- Only this layer touches `I2C1_*` registers.
- Every wait takes a timeout and returns a status.
- No `while (flag == 0);` anywhere.
- No device knowledge (no “OLED” or “MPU6050” here).
- No protocol knowledge (no “register read” here) — just the hardware verbs.

This is the layer you can test with a scope and a single byte.

---

## Stage 4 — Build the transaction engine

Now write the protocol logic as a sequence of LL verbs. For polling, a linear function is fine:

```c
status i2c_write(addr, data, len, timeout) {
    wait_bus_idle(timeout);
    start();
    wait_flag(SB, timeout);
    write_dr((addr << 1) | 0);
    wait_flag(ADDR, timeout);
    clear_addr();

    for (i = 0; i < len; i++) {
        wait_flag(TXE, timeout);
        write_dr(data[i]);
        if (flag(AF)) return I2C_ERR_NACK;
    }

    wait_flag(BTF, timeout);
    stop();
    return I2C_OK;
}
```

For reads, the order of ACK/NACK and STOP is the subtle part. The reference manual's master-receiver procedure is authoritative; the 1-byte and 2-byte cases are special:

- **1-byte read:** clear ADDR, disable ACK, set STOP, then read DR.
- **2-byte read:** use `POS` and the documented sequence.
- **N-byte read:** ACK all but the last, NACK the last, then STOP.

This is why you write the state machine on paper before coding.

**Deliverable:** `write`, `read`, `write_read`, `probe` functions that return status codes.

---

## Stage 5 — Define the API contract before implementation

Write the header first and review it:

- **Handle:** opaque `i2c_bus_t`, no globals.
- **Status:** `I2C_OK`, `NACK`, `TIMEOUT`, `BUSY`, `ARLO`, `BERR`, `BUS`.
- **Timeouts:** every blocking call takes one.
- **Addressing:** 7-bit, validated (`0x08`–`0x77`); 10-bit later.
- **Operations:** `init`, `write`, `read`, `write_read`, `probe`, `recover`.
- **Ownership:** who may call from an ISR; whether transfers are reentrant.
- **Invariants:** one STOP per transaction; bus idle before START; last read byte NACKed.

The contract is the design. If the header is vague, the implementation will be too.

---

## Stage 6 — Design error handling as a first-class feature

Every communication driver eventually meets a broken bus. Decide now:

| Error | Cause | Response |
|---|---|---|
| NACK | Device absent / not ready | Return `I2C_ERR_NACK`, issue STOP |
| Timeout | Flag never set | Abort, STOP, recover |
| BERR | Misplaced START/STOP | `SWRST`, re-init |
| ARLO | Another master won | Retry or report |
| Stuck bus | Slave holding SDA/SCL | 9 clocks + STOP, then `SWRST` |

Rules:

- Every wait has a deadline.
- On error, return the bus to idle.
- Never leave the peripheral enabled in a bad state.
- Log `SR1`/`SR2` and the state at the point of failure.

---

## Stage 7 — Verify with evidence, not optimism

Use a ladder:

1. **Compiles clean** (`-Wall -Wextra -Werror`).
2. **Known device ID** (`WHO_AM_I == 0x68`).
3. **Logic-analyzer capture** matches the frame.
4. **Fault injection** passes (wrong address, unplug, SDA held low).
5. **Stress test** passes (10k transfers, random lengths).

The capture is the ground truth. If the code “works” but the waveform is wrong, the code is wrong.

---

## Stage 8 — Interrupts and DMA are the same state machine

Polling, interrupts, and DMA differ only in **who advances the state**:

- **Polling:** the CPU waits on flags.
- **Interrupts:** the ISR reacts to flags and advances the state machine.
- **DMA:** the DMA moves bytes; the ISR still handles address/START/STOP/errors.

So design the state machine once:

```text
IDLE → START → ADDR → TX/RX → STOP → IDLE
                  └──── error ────→ RECOVER
```

Events: `SB`, `ADDR`, `TXE`, `RXNE`, `BTF`, `AF`, `BERR`, `ARLO`, timeout.  
Actions: write DR, read DR, set ACK/NACK, set START/STOP, clear flags.

A table of **state × event → action → next state** is the implementation plan. The ISR is just a lookup into that table.

---

## Stage 9 — Put device drivers on top, never inside

```text
app
 ├── mpu6050   (knows registers 0x75, 0x3B, ...)
 ├── ssd1306   (knows commands 0x21, 0x22, 0x40, ...)
 └── i2c_master (knows frames, flags, timeouts)
      └── i2c1_ll (knows F411 registers)
```

- Device drivers use only the public I2C API.
- The I2C core knows nothing about device registers.
- Swapping the MCU changes the LL layer, not the device drivers.
- Swapping the device changes the device driver, not the bus core.

This boundary is what makes the driver reusable.

---

## Generalizing to SPI and UART

The same stages apply; only the protocol details change:

| Aspect | I2C | SPI | UART |
|---|---|---|---|
| Physical | 2 wires, open-drain, pull-ups | 4 wires, push-pull, chip select | 2 wires, point-to-point |
| Addressing | 7/10-bit address | Chip-select line | None |
| Acknowledgment | ACK/NACK per byte | None | Optional parity |
| Duplex | Half-duplex | Full-duplex | Full-duplex |
| Clock | Master-generated, stretching | Master-generated, CPOL/CPHA | Baud divisor |
| Error detection | NACK, BERR, ARLO | Overrun, mode fault | Framing, parity, overrun |
| Multi-master | Yes, arbitration | No | No |

The transferable pattern is always:

```text
physical config → frame format → transaction engine → error/recovery → API → device drivers → app
```

If you can describe a protocol as **frames + states + errors**, you can implement it with this model.

---

## Coding checklist for any communication driver

- [ ] Protocol sheet drawn (frame, sequences, errors).
- [ ] Register/flag table extracted from the reference manual.
- [ ] Pin/AF/clock/timing calculated from the actual clock tree.
- [ ] LL layer contains only hardware verbs.
- [ ] Every wait has a timeout.
- [ ] Every error returns a status and restores the bus.
- [ ] State machine written as a table before code.
- [ ] Last-byte ACK/NACK and repeated START handled explicitly.
- [ ] No magic numbers; masks and shifts are named.
- [ ] `volatile` on registers; read-modify-write for shared fields.
- [ ] Reserved bits preserved; flag-clear semantics follow the reference manual.
- [ ] ISR short; no `printf`, no blocking, no dynamic allocation.
- [ ] DMA/cache maintenance considered on Cortex-M7 (H743).
- [ ] Test evidence: UART log, analyzer capture, fault injection, stress.
- [ ] Device drivers use only the public API.
- [ ] Portability boundary (LL interface) identified before adding a second MCU.

---

## Applied to this project

The current `main.c` is a **linear polling sequence** with no timeouts and device logic mixed into the bus code. That is a fine starting point, but the mental model says:

1. Extract the LL verbs (`start`, `stop`, `write_dr`, `read_dr`, `wait_flag`).
2. Add a status type and a deadline to every wait.
3. Write the protocol sheet and state table before adding read support.
4. Add `i2c_probe` and `WHO_AM_I` as the first real tests.
5. Then refactor the SSD1306 onto the new API, and add the MPU6050.
6. Only then convert the same states to interrupt-driven, and later DMA.

That is the Phase 1 → Phase 3 path in `docs/PLAN.md`. The key is to build the **protocol engine** once, then change only the event source — polling, interrupt, or DMA — without rewriting the logic.

---

## Appendix A — `I2C_CCR` and the `80U` value

Reference: RM0383 §18.6.8, `I2C_CCR`, offset `0x1C`, reset `0x0000`.

### Register layout

```text
15    14     13:12      11:0
F/S   DUTY   Reserved   CCR[11:0]
```

| Bit(s) | Name | Meaning |
|---|---|---|
| 15 | `F/S` | 0 = standard mode (Sm), 1 = fast mode (Fm) |
| 14 | `DUTY` | 0 = Fm `t_low/t_high = 2`, 1 = `16/9` |
| 13:12 | Reserved | Keep at reset value (0) |
| 11:0 | `CCR[11:0]` | SCL clock divider in master mode |

`CCR` is **not a bitmask of enables**. It is a 12-bit binary divider. Writing 80 means “divide by 80,” not “enable bits 6 and 4.”

### Timing formulas

Standard mode (`F/S = 0`):

```text
t_high = CCR × T_PCLK1
t_low  = CCR × T_PCLK1
f_SCL  = PCLK1 / (2 × CCR)
```

Fast mode, `DUTY = 0` (`t_low/t_high = 2`):

```text
t_high = CCR × T_PCLK1
t_low  = 2 × CCR × T_PCLK1
f_SCL  = PCLK1 / (3 × CCR)
```

Fast mode, `DUTY = 1` (`t_low/t_high = 16/9`):

```text
t_high = 9 × CCR × T_PCLK1
t_low  = 16 × CCR × T_PCLK1
f_SCL  = PCLK1 / (25 × CCR)
```

The minimum allowed `CCR` is `0x04` in Sm/Fm, except in fast DUTY mode where it is `0x01`.

### Why 80 at 16 MHz and 100 kHz

The current code assumes `PCLK1 = 16 MHz`, so `T_PCLK1 = 62.5 ns`.

For standard mode, 100 kHz means a 10 µs period with a 5 µs high phase and a 5 µs low phase:

```text
CCR = PCLK1 / (2 × f_SCL)
    = 16,000,000 / (2 × 100,000)
    = 80
```

Check:

```text
t_high = 80 × 62.5 ns = 5,000 ns = 5 µs
t_low  = 80 × 62.5 ns = 5,000 ns = 5 µs
period = 10 µs  →  100 kHz
```

This matches the reference manual's own example: at `FREQ = 8` (8 MHz, `T_PCLK1 = 125 ns`), `CCR = 0x28` (40) gives `40 × 125 ns = 5 µs` high and low, i.e. 100 kHz.

### What the 80 looks like in the register

```text
80 decimal = 0x050 = 0000 0101 0000  (12-bit field)
                     bit11 ... bit0
```

Bits 6 and 4 are set:

```text
bit 6 = 64
bit 4 = 16
64 + 16 = 80
```

But the correct interpretation is “the 12-bit field holds the number 80,” not “two independent features are enabled.”

With `F/S = 0` and `DUTY = 0`, the full register is:

```text
0x0000_0050
```

- bit 15 `F/S = 0` → standard mode
- bit 14 `DUTY = 0` → ignored in standard mode
- bits 13:12 = 00
- bits 11:0 = 0x050 = 80

### Why the mode bits matter

The same `CCR = 80` produces a different frequency if `F/S` or `DUTY` changes:

| Mode | Formula | Result at 16 MHz, CCR = 80 |
|---|---|---|
| Sm (`F/S=0`) | `16 MHz / (2 × 80)` | **100 kHz** |
| Fm, `DUTY=0` | `16 MHz / (3 × 80)` | ~66.7 kHz |
| Fm, `DUTY=1` | `16 MHz / (25 × 80)` | 8 kHz |

So `F/S` and `DUTY` are not decoration; they change how the divider is interpreted.

### The code, line by line

```c
I2C1_CCR &= ~(0xFFFU);        // clear CCR[11:0], leave F/S and DUTY untouched
I2C1_CCR |= (80U & 0xFFFU);   // set CCR = 80
```

- `0xFFF` is the 12-bit mask for `CCR[11:0]`.
- `80U & 0xFFF` is still 80; the mask guards against a value larger than 12 bits.
- `|=` ORs the value; because the previous line cleared the field, this is effectively an assignment.
- `F/S` and `DUTY` are not cleared by `0xFFF`. They stay at their reset value of 0, which is why standard mode is selected. If the peripheral had previously been in fast mode, `F/S` would remain 1 and 80 would be interpreted with the fast-mode formula. A robust init clears the mode bits too:

```c
uint32_t ccr = I2C1_CCR;
ccr &= ~((1U << 15) | (1U << 14) | 0xFFFU);  // clear F/S, DUTY, CCR
ccr |= (0U << 15) | (0U << 14) | (80U & 0xFFFU);
I2C1_CCR = ccr;                               // single write
```

### Coupling with `FREQ` and `TRISE`

`CCR` alone does not define the bus speed. Three values must agree:

1. **`I2C_CR2.FREQ[5:0]`** = PCLK1 in MHz. At 16 MHz, `FREQ = 16`.
2. **`I2C_CCR.CCR[11:0]`** = divider from the formula above. At 16 MHz and 100 kHz, `CCR = 80`.
3. **`I2C_TRISE.TRISE[5:0]`** = maximum SCL rise time divided by `T_PCLK1`, plus 1.

For standard mode, the maximum rise time is 1000 ns:

```text
TRISE = (1000 ns / 62.5 ns) + 1 = 16 + 1 = 17
```

That is why the current code writes `17`. If PCLK1 changes, all three values must be recomputed:

| PCLK1 | 100 kHz `CCR` | Sm `TRISE` | 400 kHz `CCR` (Fm, DUTY=0) | Fm `TRISE` |
|---|---|---|---|---|
| 16 MHz | 80 | 17 | 13 (≈410 kHz) or 14 (≈381 kHz) | 5 |
| 50 MHz | 250 | 51 | 42 (≈397 kHz) | 16 |

At 16 MHz, 400 kHz has poor resolution: `CCR = 16e6 / (3 × 400e3) = 13.33`. `CCR = 13` gives ~410 kHz and `CCR = 14` gives ~381 kHz. Running APB1 at 50 MHz gives much finer resolution and is one reason to move to the 100 MHz PLL configuration in Phase 0.

### Rules to remember

- `CCR` is a divider value, not a set of enable bits.
- `F/S` selects standard vs fast mode; `DUTY` selects the fast-mode duty cycle.
- `CCR` must be written while `PE = 0`.
- `CCR`, `FREQ`, and `TRISE` are a matched set derived from PCLK1.
- Changing the clock tree without recomputing all three silently changes the bus speed.
- The real SCL frequency can differ slightly because of analog filter delay and rise/fall times; verify with a logic analyzer.
