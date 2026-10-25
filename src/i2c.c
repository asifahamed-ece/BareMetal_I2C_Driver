/**
 * @file    i2c.c
 * @brief   Baremetal I2C1 master: polling driven, 100 kHz standard mode.
 *
 * Reference: RM0383 §18 (p472); register descriptions §18.6 (p496+).
 *
 * The shape of every transaction is the same three-step dance:
 *   1. set START, wait for SB   — "the bus is mine, clock is running"
 *   2. write the address, wait for ADDR (ack) or AF (nack)
 *   3. move bytes while TXE/BTF say the shift register has room
 * Every wait is bounded, because an unbounded wait on a stuck bus is how a
 * firmware hangs with no output and no fault — the worst kind of bug.
 */

#include "blackpill_stm32f411.h"
#include "i2c.h"
#include "timebase.h"

#define SCL_PIN        6U           ///< PB6, I2C1_SCL, alternate function 4
#define SDA_PIN        7U           ///< PB7, I2C1_SDA, alternate function 4
#define PCLK1          50000000U    ///< APB1 = 50 MHz, from clock_init_100Mhz()
#define I2C_TIMEOUT_US 2000U        ///< Abandon a stuck bus after 2 ms
#define I2C_SPEED_HZ   100000U      ///< Standard mode ceiling is 100 kHz

/**
 * @brief   Configure PB6/PB7 and bring I2C1 up as a 100 kHz master.
 *
 * @note    Order matters: the peripheral must be disabled (PE = 0) before any
 *          timing register is touched, and enabled (PE = 1) only once every
 *          other bit is in place.
 */
void i2c_init(void)
{
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC_APB1ENR |= RCC_APB1ENR_I2C1EN;

    /* PA-style: clear the two MODER fields, then set both to 0b10 = alternate function. */
    GPIOB_MODER &= ~((0x3U << GPIO_PIN_MODE_SHIFT(SCL_PIN)) | (0x3U << GPIO_PIN_MODE_SHIFT(SDA_PIN)));
    GPIOB_MODER |= ((0x2U << GPIO_PIN_MODE_SHIFT(SCL_PIN)) | (0x2U << GPIO_PIN_MODE_SHIFT(SDA_PIN)));

    /*
     * Internal pull-ups are ~30-50 kOhm. With a few hundred pF of bus
     * capacitance that cannot meet the 1000 ns rise time standard mode
     * requires, so they only stop the line floating before the external
     * 4.7 kOhm resistors take over. Keep them; do not rely on them.
     */
    GPIOB_PUPDR &= ~((0x3U << GPIO_PIN_MODE_SHIFT(SCL_PIN)) | (0x3U << GPIO_PIN_MODE_SHIFT(SDA_PIN)));
    GPIOB_PUPDR |=  ((0x1U << GPIO_PIN_MODE_SHIFT(SCL_PIN)) | (0x1U << GPIO_PIN_MODE_SHIFT(SDA_PIN)));

    /*
     * Open-drain is not a preference, it is the bus contract: every device
     * may only pull a line low and lets the pull-up raise it. Push-pull would
     * have two devices driving opposite levels into the same wire.
     */
    GPIOB_OTYPER |= ((1U << SCL_PIN) | (1U << SDA_PIN));

    /* High speed setting = fast slew on both edges; 100 kHz is slow, but the
     * edges are what the rise-time budget is spent on, so leave it fast. */
    GPIOB_OSPEEDR |= ((0x3U << GPIO_PIN_MODE_SHIFT(SCL_PIN)) | (0x3U << GPIO_PIN_MODE_SHIFT(SDA_PIN)));

    /* AFRL holds AF numbers for pins 0-7, four bits each: PB6 -> bits 27:24,
     * PB7 -> bits 31:28. AF4 is I2C1 on this part (datasheet Table 9). */
    GPIOB_AFRL &= ~((0xFU << (SCL_PIN * 4U)) | (0xFU << (SDA_PIN * 4U)));
    GPIOB_AFRL |=  ((0x4U << (SCL_PIN * 4U)) | (0x4U << (SDA_PIN * 4U)));

    I2C1_CR1 &= ~I2C_CR1_PE;    /* Timing registers are read-only while PE = 1. */

    /* SWRST drops the peripheral state machine back to idle. Useful when
     * re-initialising after a wedged bus; it does not touch the pins. */
    I2C1_CR1 |= I2C_CR1_SWRST;
    I2C1_CR1 &= ~I2C_CR1_SWRST;

    /*
     * FREQ (CR2 bits 5:0) tells the peripheral how fast APB1 actually is, so
     * it can count its own timing in PCLK1 cycles. The value is in MHz and
     * must be 2..50. Lie here and every derived timing below is wrong.
     */
    I2C1_CR2 = ((I2C1_CR2 & ~0x3FU) | 50U);

    /*
     * Standard mode splits one SCL period into equal high and low halves,
     * each CCR x T_PCLK1 long:
     *     SCL = 1 / (2 x CCR x T_PCLK1)  ->  CCR = PCLK1 / (2 x SCL)
     *     50 000 000 / 200 000 = 250
     * Fast mode (F/S = 1) uses a different duty split and is not used here.
     */
    uint32_t ccr_value = PCLK1 / (2U * I2C_SPEED_HZ);
    I2C1_CCR &= ~(0xFFFU);
    I2C1_CCR |= (ccr_value << 0U);

    /*
     * TRISE is the longest rise time the bus is allowed, expressed in
     * T_PCLK1 units, plus one:
     *     standard mode allows 1000 ns  ->  1000 ns / 20 ns + 1 = 51
     * PCLK1 / 1 MHz + 1 happens to produce exactly that at 50 MHz.
     */
    uint32_t trise_value = (PCLK1 / 1000000U) + 1U;
    I2C1_TRISE &= ~(0x3FU << 0U);
    I2C1_TRISE |= (trise_value << 0U);

    I2C1_CR1 |= I2C_CR1_ACK;    /* Acknowledge every byte we receive. */
    I2C1_CR1 |= I2C_CR1_PE;     /* Enable last: everything above is now live. */
}

/**
 * @brief   Probe one 7-bit address by attempting START, address, STOP.
 * @param   addr 7-bit address, 0x08..0x77.
 * @return  1 if the address was acknowledged, 0 otherwise.
 *
 * @note    There is no "is there a device?" register — probing is the only
 *          honest way to ask. Each probe costs one full transaction.
 */
uint8_t i2c_probe(uint8_t addr)
{
    /* BUSY lives in SR2, not SR1. The macro is misnamed I2C1_SR1_BUSY but its
     * value (bit 1) is correct and is being applied to the right register. */
    uint32_t start = timebase_us();
    while (I2C1_SR2 & I2C1_SR1_BUSY) {
        if ((timebase_us() - start) > I2C_TIMEOUT_US) return 0;
    }

    /* Setting START makes the peripheral take control of the bus as soon as
     * it sees it free; the hardware clears the bit itself afterwards. */
    I2C1_CR1 |= I2C_CR1_START;

    /* SB set = START is on the wire and SCL is being stretched low. */
    start = timebase_us();
    while (!(I2C1_SR1 & I2C_SR1_SB)) {
        if ((timebase_us() - start) > I2C_TIMEOUT_US) return 0;
    }

    /* Address frame: 7 bits shifted up, bit 0 = 0 for write. The hardware
     * emits it in the same clock stretch, so DR may be written immediately. */
    I2C1_DR = (uint32_t)((addr << 1) | 0U);

    /* Two possible outcomes, and both are answers: ADDR means someone
     * acknowledged, AF means nobody did. Wait for either, not just for ADDR. */
    start = timebase_us();
    while (!(I2C1_SR1 & I2C_SR1_ADDR) && !(I2C1_SR1 & I2C1_SR1_ACK_FAILURE)) {
        if ((timebase_us() - start) > I2C_TIMEOUT_US) return 0;
    }

    uint8_t found = 0;
    if (I2C1_SR1 & I2C_SR1_ADDR) {
        found = 1;
        /* ADDR is cleared by reading SR1 and then SR2. The while condition
         * above already read SR1, so this read completes the pair and lets
         * SCL be released. Skip it and the bus stays stretched forever. */
        (void)I2C1_SR2;
    } else {
        /* AF is cleared by writing 0 to it, which is what this mask does. */
        I2C1_SR1 &= ~I2C1_SR1_ACK_FAILURE;
    }

    I2C1_CR1 |= I2C_CR1_STOP;

    return found;
}

/**
 * @brief   Write @p len bytes to a 7-bit addressed slave.
 * @param   addr 7-bit address (0x3C for SSD1306).
 * @param   data Bytes to send; `data[0]` goes first.
 * @param   len  Byte count, must be <= 255.
 * @return  1 on success, 0 on NACK or timeout.
 *
 * @note    Every failure path issues STOP before returning so a NACK or a
 *          stuck flag cannot leave the bus claimed by a master that is gone.
 * @warning The loop counter is `uint8_t`; `len` above 255 wraps and never
 *          terminates. Callers today stay well below that.
 */
uint8_t i2c_write(uint8_t addr, uint8_t *data, uint32_t len)
{
    uint32_t start = timebase_us();
    while (I2C1_SR2 & I2C1_SR1_BUSY) {
        if ((timebase_us() - start) > I2C_TIMEOUT_US) return 0;
    }

    I2C1_CR1 |= I2C_CR1_START;
    start = timebase_us();
    while (!(I2C1_SR1 & I2C_SR1_SB)) {
        if ((timebase_us() - start) > I2C_TIMEOUT_US) return 0;
    }

    I2C1_DR = (uint32_t)((addr << 1) | 0U);     /* bit 0 = 0: write direction */

    start = timebase_us();
    while (!(I2C1_SR1 & I2C_SR1_ADDR) && !(I2C1_SR1 & I2C1_SR1_ACK_FAILURE)) {
        if ((timebase_us() - start) > I2C_TIMEOUT_US) {
            I2C1_CR1 |= I2C_CR1_STOP;           /* Release the bus before bailing out. */
            return 0;
        }
    }

    if (I2C1_SR1 & I2C_SR1_ADDR) {
        (void)I2C1_SR2;     /* SR1-then-SR2 read: required to clear ADDR. */
    } else {
        I2C1_SR1 &= ~I2C1_SR1_ACK_FAILURE;
        I2C1_CR1 |= I2C_CR1_STOP;
        return 0;
    }

    for (uint8_t i = 0; i < len; i++) {
        /* TXE = DR is empty and can take another byte. It does not mean the
         * byte has been acknowledged yet, only that there is room to queue. */
        start = timebase_us();
        while (!(I2C1_SR1 & I2C_SR1_TXE)) {
            if ((timebase_us() - start) > I2C_TIMEOUT_US) {
                I2C1_CR1 |= I2C_CR1_STOP;
                return 0;
            }
        }
        I2C1_DR = data[i];
    }

    /*
     * TXE only says DR is empty; the last byte may still be shifting out.
     * BTF set means both DR and the shift register are done, which is the
     * only safe moment to raise STOP. Stop early and the final byte is cut
     * mid-frame — a classic "the last OLED column looks wrong" bug.
     */
    start = timebase_us();
    while (!(I2C1_SR1 & I2C1_SR1_BTF)) {
        if ((timebase_us() - start) > I2C_TIMEOUT_US) {
            I2C1_CR1 |= I2C_CR1_STOP;
            return 0;
        }
    }

    I2C1_CR1 |= I2C_CR1_STOP;
    return 1;
}
