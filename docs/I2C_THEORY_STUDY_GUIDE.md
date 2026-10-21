# STM32F411 I2C Theory Study Guide

**Target:** WeAct STM32F411CEU6 BlackPill V3.0  
**Peripheral:** I2C1 on PB6 (SCL) / PB7 (SDA), AF4  
**Purpose:** A complete theory companion for the baremetal I2C master driver. Read this before Phase 0.1 and answer the self-check questions at the end without looking back.

> This guide summarizes the STM32F411 I2C peripheral and the I2C bus protocol. It is a study aid, not a replacement for RM0383. When in doubt, the reference manual wins.

---

## 1. I2C Protocol Essentials

I2C is a synchronous, multi-master, multi-slave, two-wire bus.

| Wire | Role |
|---|---|
| SCL | Serial clock, driven by the master |
| SDA | Serial data, bidirectional |

Both lines are **open-drain** with external pull-up resistors. A device can only pull a line low or release it. The bus is therefore a wired-AND: if any device pulls low, the line is low. This is what makes ACK/NACK and multi-master arbitration possible.

### 1.1 START and STOP

| Condition | Definition |
|---|---|
| START (S) | SDA goes high-to-low while SCL is high |
| STOP (P) | SDA goes low-to-high while SCL is high |
| Repeated START (Sr) | A second START without an intervening STOP |

A START or STOP while SCL is high is the only time SDA may change without being clocked. During data bits, SDA must be stable while SCL is high and may change only while SCL is low.

### 1.2 Addressing

A 7-bit address is sent as one byte:

```
byte = (address << 1) | R/W
```

- `R/W = 0` means master write (master transmits).
- `R/W = 1` means master read (master receives).
- The receiver of each byte pulls SDA low on the ninth clock to ACK.
- A NACK is SDA left high on the ninth clock.

A 10-bit address uses two bytes: `11110xx0` followed by the upper address bits for a write, or `11110xx1` for a read. The driver starts with 7-bit addressing only.

### 1.3 Clock stretching

A slave may hold SCL low after the master releases it to buy time. The master must wait until SCL is actually high before continuing. The STM32 I2C peripheral handles this automatically when `NOSTRETCH=0`. Do not disable clock stretching in slave mode unless you fully understand the consequences.

### 1.4 Arbitration

If two masters start at the same time, each checks SDA while transmitting. A master that tries to drive SDA high while another drives it low loses arbitration and must stop driving. The STM32 reports this as `SR1.ARLO`. Arbitration can continue through address and data bytes; it is lost only when a mismatch occurs.

### 1.5 Bus states

| State | Meaning |
|---|---|
| Free | No master is using the bus; `SR2.BUSY=0` |
| Start | START has been generated; `SR1.SB=1` |
| Address | Address byte is being sent or matched; `SR1.ADDR=1` |
| Data | Data bytes are being transferred |
| Stop | STOP has been generated; bus returns to free |

---

## 2. STM32F411 I2C Register Map

The F411 I2C peripheral has ten registers. All are 32-bit and must be accessed with 32-bit loads/stores. Offsets are relative to the peripheral base address.

| Offset | Register | Reset | Purpose |
|---|---|---|---|
| `0x00` | `I2C_CR1` | `0x0000` | Control: enable, START/STOP, ACK, SMBus, reset |
| `0x04` | `I2C_CR2` | `0x0000` | Clock frequency, interrupts, DMA |
| `0x08` | `I2C_OAR1` | `0x0000` | Own address 1 (slave) |
| `0x0C` | `I2C_OAR2` | `0x0000` | Own address 2 (slave) |
| `0x10` | `I2C_DR` | `0x0000` | Data register |
| `0x14` | `I2C_SR1` | `0x0000` | Status flags |
| `0x18` | `I2C_SR2` | `0x0000` | Status flags, bus state |
| `0x1C` | `I2C_CCR` | `0x0000` | Clock control |
| `0x20` | `I2C_TRISE` | `0x0002` | Maximum rise time |
| `0x24` | `I2C_FLTR` | `0x0000` | Noise filters |

> `I2C_CCR` and `I2C_TRISE` must be configured with `PE=0`. `I2C_OAR1`, `I2C_OAR2`, and `I2C_FLTR` also require `PE=0`.

---

## 3. Register Details

### 3.1 I2C_CR1 — Control Register 1

| Bit | Name | Meaning | Master use |
|---|---|---|---|
| 15 | `SWRST` | Software reset; resets the peripheral state machine | Recovery only |
| 14 | Reserved | — | — |
| 13 | `ALERT` | SMBus alert | Not used in I2C mode |
| 12 | `PEC` | Packet error checking transfer | Not used in basic I2C |
| 11 | `POS` | ACK/PEC position; used for 2-byte reception | 2-byte read only |
| 10 | `ACK` | ACK enable: 1 = ACK returned, 0 = NACK | Set for all bytes except the last read |
| 9 | `STOP` | Generate a STOP condition | End of transfer |
| 8 | `START` | Generate a START or repeated START | Start of transfer |
| 7 | `NOSTRETCH` | Disable clock stretching in slave mode | Leave 0 |
| 6 | `ENGC` | General call enable | Leave 0 |
| 5 | `ENPEC` | PEC enable | Leave 0 |
| 4 | `ENARP` | ARP enable | Leave 0 |
| 3 | `SMBTYPE` | SMBus device/host select | Leave 0 |
| 2 | Reserved | — | — |
| 1 | `SMBUS` | 0 = I2C mode, 1 = SMBus mode | Must be 0 for I2C |
| 0 | `PE` | Peripheral enable | Set after configuration |

Key points:

- `PE` must be 0 while you configure `CR2`, `CCR`, `TRISE`, `FLTR`, `OAR1`, and `OAR2`.
- `START` is set by software and cleared by hardware once the START condition is generated.
- `STOP` is set by software and cleared by hardware once the STOP condition is generated.
- `ACK` must be 1 for normal reception. Clear it before the last byte of a read.
- `POS` changes the ACK position for 2-byte reads. It is not needed for normal 1-byte or N-byte reads.
- `SWRST` resets the peripheral. Use it only after a bus error or a stuck bus, and reinitialize afterwards.

### 3.2 I2C_CR2 — Control Register 2

| Bit | Name | Meaning |
|---|---|---|
| 15:13 | Reserved | — |
| 12 | `LAST` | DMA last transfer |
| 11 | `DMAEN` | DMA enable |
| 10 | `ITBUFEN` | Buffer interrupt enable |
| 9 | `ITEVTEN` | Event interrupt enable |
| 8 | `ITERREN` | Error interrupt enable |
| 7:6 | Reserved | — |
| 5:0 | `FREQ[5:0]` | PCLK1 frequency in MHz |

`FREQ` is the frequency of the clock feeding the I2C peripheral, not necessarily the system clock. On the F411, I2C1 is on APB1. If APB1 is 50 MHz, `FREQ=50`.

Write `FREQ` with a clear-then-set sequence:

```c
I2C1->CR2 = (I2C1->CR2 & ~0x3FU) | (pclk1_mhz & 0x3FU);
```

Do not OR the new value into the old field without clearing it first. `FREQ` is a 6-bit field, not a set of independent bits.

### 3.3 I2C_OAR1 — Own Address 1

| Bit | Name | Meaning |
|---|---|---|
| 15 | `ADDMODE` | 0 = 7-bit address, 1 = 10-bit address |
| 14 | Must be 1 | Reserved; keep at 1 when writing |
| 13:10 | Reserved | — |
| 9:8 | `ADD[9:8]` | Upper address bits in 10-bit mode |
| 7:1 | `ADD[7:1]` | 7-bit address, or lower 7 bits of a 10-bit address |
| 0 | `ADD0` | 10-bit address LSB; don't care in 7-bit mode |

For a 7-bit slave address, write `(address << 1) | (1U << 14)`. Bit 14 must remain 1. For a master-only driver, `OAR1` is not needed, but it must not be corrupted.

### 3.4 I2C_OAR2 — Own Address 2

| Bit | Name | Meaning |
|---|---|---|
| 15:8 | Reserved | — |
| 7:1 | `ADD2[7:1]` | Second 7-bit slave address |
| 0 | `ENDUAL` | Enable dual addressing |

`OAR2` is slave-only. Leave it at reset value for a master-only driver. If used, configure it with `PE=0`.

### 3.5 I2C_DR — Data Register

| Bit | Name | Meaning |
|---|---|---|
| 15:8 | Reserved | — |
| 7:0 | `DR[7:0]` | Byte to transmit or byte received |

- Writing `DR` in master transmit mode starts transmission of that byte.
- Reading `DR` in master receive mode returns the received byte and clears `RxNE`.
- `TxE=1` means `DR` is empty and another byte can be written.
- `RxNE=1` means `DR` contains a received byte.

### 3.6 I2C_SR1 — Status Register 1

| Bit | Flag | Set when | Cleared by | Master action |
|---|---|---|---|---|
| 15 | `SMBALERT` | SMBus alert | Write 0 | Not used in I2C |
| 14 | `TIMEOUT` | SMBus timeout | Write 0 | Not used in I2C |
| 13 | Reserved | — | — | — |
| 12 | `PECERR` | PEC error | Write 0 | Not used in basic I2C |
| 11 | `OVR` | Overrun/underrun | Write 0 | Recover |
| 10 | `AF` | Acknowledge failure (NACK) | Write 0 | Abort, STOP, report NACK |
| 9 | `ARLO` | Arbitration lost | Write 0 | Recover or retry |
| 8 | `BERR` | Bus error (misplaced START/STOP) | Write 0 | Recover |
| 7 | `TxE` | Data register empty | Write `DR` | Load next byte |
| 6 | `RxNE` | Data register not empty | Read `DR` | Read received byte |
| 5 | Reserved | — | — | — |
| 4 | `STOPF` | STOP detected (slave) | Read `SR1`, then write `CR1` | Slave only |
| 3 | `ADD10` | 10-bit address header sent | Read `SR1`, then write `DR` | 10-bit only |
| 2 | `BTF` | Byte transfer finished | Read `SR1`, then read/write `DR` | Wait before STOP |
| 1 | `ADDR` | Address sent or matched | Read `SR1`, then read `SR2` | Clear before data |
| 0 | `SB` | START generated | Read `SR1`, then write `DR` | Send address |

Critical sequences:

- **Clear `SB`:** read `SR1`, then write the address to `DR`.
- **Clear `ADDR`:** read `SR1`, then read `SR2`.
- **Clear `BTF`:** read `SR1`, then read or write `DR`.
- **Clear `AF`, `BERR`, `ARLO`, `OVR`:** write 0 to the flag.
- **Clear `TxE`:** write the next byte to `DR`.
- **Clear `RxNE`:** read `DR`.

`ADDR` is the one that trips up most drivers. Reading `SR2` at the wrong moment can clear it. Clear `ADDR` deliberately, then proceed.

### 3.7 I2C_SR2 — Status Register 2

| Bit | Name | Meaning |
|---|---|---|
| 15:8 | `PEC[7:0]` | Packet error checking byte |
| 7 | `DUALF` | Dual flag: received address matched `OAR2` |
| 6 | `SMBHOST` | SMBus host header detected |
| 5 | `SMBDEFAULT` | SMBus default address detected |
| 4 | `GENCALL` | General call address detected |
| 3 | Reserved | — |
| 2 | `TRA` | 1 = transmitter, 0 = receiver |
| 1 | `BUSY` | 1 = bus busy |
| 0 | `MSL` | 1 = master mode, 0 = slave mode |

`SR2` is read-only. It reports the bus state and is part of the `ADDR` clear sequence. Do not use `SR2.BUSY` as a substitute for `SR1.SB`; they mean different things.

- `BUSY=1` means the bus is not free.
- `MSL=1` means this peripheral is currently the master.
- `TRA=1` means the master is transmitting; `TRA=0` means it is receiving.

### 3.8 I2C_CCR — Clock Control Register

| Bit | Name | Meaning |
|---|---|---|
| 15 | `F/S` | 0 = standard mode, 1 = fast mode |
| 14 | `DUTY` | 0 = 2:1 duty, 1 = 16:9 duty (fast mode only) |
| 13:12 | Reserved | — |
| 11:0 | `CCR[11:0]` | Clock divider |

`CCR` is a **divider**, not a bitmask. The value 80 at 16 MHz is not "bits 6 and 4 set"; it is the number 80 that produces a 100 kHz SCL.

Standard mode:

```
CCR = f_PCLK1 / (2 * f_SCL)
```

Fast mode, `DUTY=0`:

```
CCR = f_PCLK1 / (3 * f_SCL)
```

Fast mode, `DUTY=1`:

```
CCR = f_PCLK1 / (25 * f_SCL)
```

Minimum values: `CCR >= 4` in standard and fast mode, except `DUTY=1` where the minimum is 1.

Worked examples:

| PCLK1 | Mode | Target | CCR | Actual SCL |
|---|---|---|---|---|
| 16 MHz | Standard | 100 kHz | 80 | 100 kHz |
| 16 MHz | Fast, DUTY=0 | 400 kHz | 14 | 381 kHz |
| 50 MHz | Standard | 100 kHz | 250 | 100 kHz |
| 50 MHz | Fast, DUTY=0 | 400 kHz | 42 | 397 kHz |

At 16 MHz, 400 kHz fast mode is coarse: `CCR=13` gives about 410 kHz, which is above the 400 kHz limit; `CCR=14` gives about 381 kHz, which is safe. Start at 100 kHz and only move to 400 kHz after the bus is proven.

### 3.9 I2C_TRISE — Rise Time Register

| Bit | Name | Meaning |
|---|---|---|
| 15:6 | Reserved | — |
| 5:0 | `TRISE[5:0]` | Maximum SCL rise time in PCLK1 ticks, plus one |

Formula:

```
TRISE = (t_r_max / T_PCLK1) + 1
```

Where `t_r_max` is the maximum allowed SCL rise time:

| Mode | `t_r_max` | Source |
|---|---|---|
| Standard mode, 100 kHz | 1000 ns | I2C-bus specification (NXP UM10204) |
| Fast mode, 400 kHz | 300 ns | I2C-bus specification (NXP UM10204) |

These are **I2C-bus specification limits**, not ST-invented values. ST repeats them in the STM32 datasheet's I2C characteristics table (Table 58) and refers to them in the RM's `TRISE` description. The actual rise time on your board depends on the pull-up resistors and bus capacitance; `TRISE` tells the peripheral the worst case it must tolerate.

Worked examples:

| PCLK1 | Mode | `T_PCLK1` | Calculation | `TRISE` |
|---|---|---|---|---|
| 16 MHz | Standard | 62.5 ns | 1000 / 62.5 + 1 = 17 | 17 |
| 16 MHz | Fast | 62.5 ns | 300 / 62.5 + 1 = 5.8 → 5 | 5 |
| 50 MHz | Standard | 20 ns | 1000 / 20 + 1 = 51 | 51 |
| 50 MHz | Fast | 20 ns | 300 / 20 + 1 = 16 | 16 |

`TRISE` reset value is `0x0002`. It must be configured with `PE=0`. Use integer truncation only when the result still meets the minimum rise time; when in doubt, round up.

### 3.10 I2C_FLTR — Filter Register

| Bit | Name | Meaning |
|---|---|---|
| 15:5 | Reserved | — |
| 4 | `ANOFF` | 0 = analog filter on, 1 = analog filter off |
| 3:0 | `DNF[3:0]` | Digital noise filter: 0 = off, 1–15 = filter spikes up to `DNF` PCLK1 periods |

The analog filter rejects short spikes. The digital filter adds delay. For a first driver, leave `FLTR` at reset (analog filter on, digital filter off). Configure it with `PE=0` if you change it.

---

## 4. Master Transfer Sequences

These are the flag-level sequences the driver must implement. Every wait must have a timeout.

### 4.1 Master Write

```
1. Wait until SR2.BUSY = 0.
2. Set CR1.START = 1.
3. Wait until SR1.SB = 1.
4. Write DR = (address << 1) | 0.
5. Wait until SR1.ADDR = 1.
6. Read SR1, then read SR2. This clears ADDR.
7. For each data byte:
   a. Wait until SR1.TxE = 1.
   b. Write DR = byte.
   c. If SR1.AF = 1: write 0 to AF, set STOP, return NACK.
8. Wait until SR1.BTF = 1.
9. Set CR1.STOP = 1.
10. Wait until SR2.BUSY = 0 (optional, with timeout).
```

Why wait for `BTF` before `STOP`? `TxE=1` only means `DR` is empty; the byte may still be shifting out. Setting `STOP` too early can truncate the last byte. `BTF=1` means the byte transfer has actually finished.

### 4.2 Master Read

```
1. Wait until SR2.BUSY = 0.
2. Set CR1.START = 1.
3. Wait until SR1.SB = 1.
4. Write DR = (address << 1) | 1.
5. Wait until SR1.ADDR = 1.
6. Read SR1, then read SR2. This clears ADDR.
7. For i = 0 to length-1:
   a. If i == length-1, clear CR1.ACK (NACK the last byte).
   b. Wait until SR1.RxNE = 1.
   c. data[i] = DR.
8. Set CR1.STOP = 1.
```

For a single-byte read, the RM's recommended order is: clear `ADDR`, clear `ACK`, set `STOP`, then read `DR`. For a two-byte read, use `POS` and the special sequence in RM0383 §18.4.6. Get the general N-byte case working first, then handle 1-byte and 2-byte reads explicitly.

### 4.3 Register Read (Write-Then-Read with Repeated START)

This is the sequence needed for the MPU6050 and most register-based sensors:

```
S | address+W | ACK | register | ACK | Sr | address+R | ACK | data | NACK | P
```

Flag-level:

```
1. Wait BUSY=0.
2. START=1; wait SB=1.
3. DR = (address << 1) | 0; wait ADDR=1; clear ADDR.
4. Wait TxE=1; DR = register; wait BTF=1.
5. START=1 again (repeated START); wait SB=1.
6. DR = (address << 1) | 1; wait ADDR=1; clear ADDR.
7. Read the required bytes; NACK the last one.
8. STOP=1.
```

The repeated START keeps the bus owned by the master between the register write and the data read. It prevents another master from taking the bus in the gap. It also matches the register-read protocol of most I2C devices.

### 4.4 1-Byte and 2-Byte Read Notes

- **1-byte read:** after clearing `ADDR`, clear `ACK`, set `STOP`, then read `DR`. If you wait for `RxNE` before NACK/STOP, the slave may already have started the next byte.
- **2-byte read:** set `POS=1` before clearing `ADDR`; after the first `RxNE`, clear `POS` and clear `ACK`; after the second `RxNE`, read both bytes and set `STOP`.
- **N-byte read (N >= 3):** keep `ACK=1` for all bytes except the last; clear `ACK` before the last `RxNE`; read the last byte; set `STOP`.

Do not try to generalize the 1-byte and 2-byte cases with the same loop until the basic N-byte path is tested.

---

## 5. Timing and Clock Configuration

The I2C peripheral must know the APB1 clock and the bus timing. All three values must agree:

1. `CR2.FREQ` = PCLK1 in MHz.
2. `CCR` = divider for the target SCL.
3. `TRISE` = maximum rise time in PCLK1 ticks plus one.

At 16 MHz HSI (the current demo):

| Parameter | Value |
|---|---|
| `FREQ` | 16 |
| `CCR` | 80 |
| `TRISE` | 17 |
| `F/S` | 0 (standard mode) |
| Target SCL | 100 kHz |

At 50 MHz APB1 (after the PLL is configured):

| Parameter | Value |
|---|---|
| `FREQ` | 50 |
| `CCR` | 250 |
| `TRISE` | 51 |
| `F/S` | 0 (standard mode) |
| Target SCL | 100 kHz |

If you change the clock tree, you must recompute all three. A driver that hardcodes `CCR=80` and `TRISE=17` will silently run at the wrong speed after a clock change.

---

## 6. Error Handling and Bus Recovery

| Flag | Meaning | Clear | Driver response |
|---|---|---|---|
| `AF` | NACK from the addressed device | Write 0 | Set STOP, return `I2C_ERR_NACK` |
| `BERR` | Misplaced START/STOP | Write 0 | Reset the peripheral, recover the bus |
| `ARLO` | Lost arbitration | Write 0 | Retry the transfer or return an error |
| `OVR` | Overrun/underrun | Write 0 | Reset the transfer, recover |
| `TIMEOUT` | SMBus timeout | Write 0 | Not used in basic I2C |
| `BUSY` stuck | A slave is holding SCL/SDA low | — | Bus recovery, then reinit |

A robust driver needs:

- A timeout on every flag wait.
- A distinct error code for NACK, bus error, arbitration loss, and timeout.
- A `i2c_bus_recover` function that:
  1. Disables the I2C peripheral (`PE=0`).
  2. Configures SCL and SDA as GPIO open-drain outputs.
  3. Clocks SCL up to nine times while SDA is released, allowing a stuck slave to finish its byte.
  4. Generates a manual STOP (SDA low, SCL high, SDA high).
  5. Reinitializes the I2C peripheral.
- A software reset (`SWRST`) only after the bus recovery sequence, not as the first response to every error.

---

## 7. Common Pitfalls

1. **Not clearing `ADDR` correctly.** Read `SR1`, then `SR2`. Do not read `SR2` for other reasons before this sequence.
2. **Confusing `TxE` and `BTF`.** `TxE` means "load another byte"; `BTF` means "the byte is actually done." Wait for `BTF` before `STOP`.
3. **Not NACKing the last read byte.** If `ACK` stays 1, the slave keeps sending and the master may never generate a clean STOP.
4. **Configuring with `PE=1`.** `CCR`, `TRISE`, `FLTR`, `OAR1`, and `OAR2` require `PE=0`.
5. **Forgetting `OAR1` bit 14.** In 7-bit mode, keep bit 14 at 1 when writing `OAR1`.
6. **Leaving `SMBUS=1`.** Bit 1 of `CR1` must be 0 for I2C mode.
7. **Hardcoding `FREQ`/`CCR`/`TRISE`.** They must match the actual PCLK1.
8. **Using internal pull-ups.** They are far too weak for I2C. Use external 4.7 kΩ (or 2.2 kΩ for fast mode) and check bus capacitance.
9. **Treating `CCR=80` as a bitmask.** It is a divider. The value 80 happens to have bits 6 and 4 set, but that is arithmetic, not configuration.
10. **Setting `STOP` before the last byte is acknowledged.** Wait for `BTF` in transmit mode and `RxNE` in receive mode.
11. **Ignoring a stuck `BUSY` bit.** If `BUSY` never clears, recover the bus instead of spinning forever.
12. **Assuming 5 V tolerance.** Use 3.3 V I2C devices unless the datasheet explicitly permits 5 V on the pins.

---

## 8. Self-Check Questions

Answer these without looking back. If you cannot answer one, reread the relevant section.

1. Why must `FREQ[5:0]` be written with a clear-then-set sequence, and what value do you write at 16 MHz? At 50 MHz?
2. What does `CCR=80` mean at 16 MHz and 100 kHz? Why is it not a bitmask?
3. Why is `TRISE=17` at 16 MHz for 100 kHz? Where do 1000 ns and 300 ns come from, and are they ST-invented values?
4. What is the difference between `TxE` and `BTF`? Why must you wait for `BTF` before setting `STOP` in a write?
5. What exact sequence clears `ADDR`? Why must `SR2` be read, and what can go wrong if you read it at the wrong time?
6. What does `AF` mean, how is it cleared, and what should the driver do when it sees it?
7. Why must `PE=0` while configuring `CCR`, `TRISE`, `FLTR`, `OAR1`, and `OAR2`?
8. How do you generate a repeated START for a register read? Why is it better than a STOP followed by a new START?
9. What is the difference between `BUSY` and `MSL`? How do you recover a bus where `BUSY` never clears?

---

## 9. References

- **RM0383**, STM32F411 reference manual:
  - §18.3.3, I2C registers
  - §18.4, I2C functional description
  - §18.6, I2C register descriptions
  - Pages 472–504
- **STM32F411CEU6 datasheet**:
  - Table 9, alternate-function mapping (PB6/PB7 = AF4)
  - Table 58, I2C characteristics (1000 ns standard, 300 ns fast)
- **NXP UM10204**, I2C-bus specification and user manual
- **PM0214**, STM32 Cortex-M4 programming manual (SysTick, NVIC, fault handling)

---

*End of study guide. The next practical step is Phase 0.1: Makefile, startup file, linker script, OpenOCD config, and blinky.*
