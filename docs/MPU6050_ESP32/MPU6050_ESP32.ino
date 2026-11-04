/**
 * @file    MPU6050_ESP32.ino
 * @brief   Bit-banged I2C target emulating an MPU-6050 at 0x68.
 *
 * Wiring (STM32F411 -> ESP32 NodeMCU-32S, 38-pin):
 *     PB6 SCL -> GPIO22        PA2 TX -> GPIO16 (UART2 RX)
 *     PB7 SDA -> GPIO21        GND    -> GND
 *     4.7k pull-ups from SCL and SDA to 3V3.
 *
 * UART2 is forwarded to USB, so the ESP32 is also the STM32's terminal.
 * ESP32 messages are prefixed [ESP]; 'b' pauses the bridge.
 *
 * The Wire slave mishandles repeated START, which i2c_read() depends on,
 * so the I2C target is bit-banged. 100 kHz. DMP/FIFO/aux I2C not emulated.
 *
 * Commands: b bridge, m motion, s static, d dump, r reset, v stats, ? help.
 */

#include <Arduino.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <math.h>
#include <soc/gpio_struct.h>

#define SDA_PIN       21
#define SCL_PIN       22
#define MPU_ADDR      0x68
#define SERIAL_BAUD   115200
#define UART_RX_PIN   16
#define UART_TX_PIN   17
#define UART_BAUD     115200
#define UART_RING_SIZE 4096u

enum : uint8_t {
  REG_SMPLRT_DIV        = 0x19,
  REG_CONFIG            = 0x1A,
  REG_GYRO_CONFIG       = 0x1B,
  REG_ACCEL_CONFIG      = 0x1C,
  REG_INT_PIN_CFG       = 0x37,
  REG_INT_ENABLE        = 0x38,
  REG_INT_STATUS        = 0x3A,
  REG_ACCEL_XOUT_H      = 0x3B,
  REG_ACCEL_XOUT_L      = 0x3C,
  REG_ACCEL_YOUT_H      = 0x3D,
  REG_ACCEL_YOUT_L      = 0x3E,
  REG_ACCEL_ZOUT_H      = 0x3F,
  REG_ACCEL_ZOUT_L      = 0x40,
  REG_TEMP_OUT_H        = 0x41,
  REG_TEMP_OUT_L        = 0x42,
  REG_GYRO_XOUT_H       = 0x43,
  REG_GYRO_XOUT_L       = 0x44,
  REG_GYRO_YOUT_H       = 0x45,
  REG_GYRO_YOUT_L       = 0x46,
  REG_GYRO_ZOUT_H       = 0x47,
  REG_GYRO_ZOUT_L       = 0x48,
  REG_SIGNAL_PATH_RESET = 0x68,
  REG_USER_CTRL         = 0x6A,
  REG_PWR_MGMT_1        = 0x6B,
  REG_PWR_MGMT_2        = 0x6C,
  REG_FIFO_COUNT_H      = 0x72,
  REG_FIFO_COUNT_L      = 0x73,
  REG_FIFO_R_W          = 0x74,
  REG_WHO_AM_I          = 0x75
};

/* Register file and pointer. The pointer auto-increments on reads/writes and
 * survives a repeated START; it wraps at 0x7F. */
static volatile uint8_t regs[128];
static volatile uint8_t reg_ptr = 0;

enum BusState : uint8_t {
  ST_IDLE = 0,
  ST_ADDR,
  ST_ACK_ADDR,
  ST_RX,
  ST_ACK_RX,
  ST_TX,
  ST_ACK_TX,
  ST_IGNORE
};

static volatile uint8_t bus_state = ST_IDLE;
static volatile uint8_t bit_index = 0;
static volatile uint8_t rx_shift = 0;
static volatile uint8_t tx_shift = 0;
static volatile bool addressed = false;
static volatile bool read_dir = false;
static volatile bool first_write_byte = true;
static volatile bool bus_active = false;
static volatile uint8_t prev_scl = 1;
static volatile uint8_t prev_sda = 1;

static volatile uint32_t stat_starts = 0;
static volatile uint32_t stat_stops = 0;
static volatile uint32_t stat_addr_acks = 0;
static volatile uint32_t stat_addr_nacks = 0;
static volatile uint32_t stat_rx_bytes = 0;
static volatile uint32_t stat_tx_bytes = 0;
static volatile uint32_t stat_master_nacks = 0;
static volatile uint32_t edge_count = 0;

static bool motion_mode = false;
static uint32_t last_sample_us = 0;
static portMUX_TYPE g_mux = portMUX_INITIALIZER_UNLOCKED;

/* UART2 -> USB ring: the bridge task is the producer, loop() the consumer. */
static uint8_t uart_ring[UART_RING_SIZE];
static volatile uint32_t uart_head = 0;
static volatile uint32_t uart_tail = 0;
static volatile uint32_t uart_dropped = 0;
static volatile bool bridge_enabled = true;

/* Open-drain: 1 releases the line, 0 pulls it low. */
#define SDA_LOW()      (GPIO.out_w1tc = (1u << SDA_PIN))
#define SDA_REL()      (GPIO.out_w1ts = (1u << SDA_PIN))
#define SCL_LOW()      (GPIO.out_w1tc = (1u << SCL_PIN))
#define SCL_REL()      (GPIO.out_w1ts = (1u << SCL_PIN))
#define SDA_IS_HIGH()  (((GPIO.in >> SDA_PIN) & 1u) != 0)
#define SCL_IS_HIGH()  (((GPIO.in >> SCL_PIN) & 1u) != 0)

/* ------------------------------------------------------------------------- */
/* Register model                                                            */
/* ------------------------------------------------------------------------- */

static void IRAM_ATTR reset_registers(void) {
  for (uint16_t i = 0; i < sizeof(regs); i++) {
    regs[i] = 0;
  }
  regs[REG_PWR_MGMT_1] = 0x40;   /* asleep after reset */
  regs[REG_WHO_AM_I]   = 0x68;
  reg_ptr = 0;
}

static uint8_t IRAM_ATTR read_reg(uint8_t r) {
  r &= 0x7F;
  switch (r) {
    case REG_INT_STATUS: {
      uint8_t v = regs[r];         /* read-to-clear */
      regs[r] = 0;
      return v;
    }
    case REG_WHO_AM_I:
      return 0x68;
    default:
      return regs[r];
  }
}

static void IRAM_ATTR write_reg(uint8_t r, uint8_t v) {
  r &= 0x7F;
  switch (r) {
    case REG_WHO_AM_I:
    case REG_INT_STATUS:
    case REG_FIFO_COUNT_H:
    case REG_FIFO_COUNT_L:
    case REG_FIFO_R_W:
      return;                      /* read-only */

    case REG_PWR_MGMT_1:
      if (v & 0x80) {              /* DEVICE_RESET self-clears */
        reset_registers();
        return;
      }
      regs[r] = (uint8_t)(v & 0x7F);
      return;

    case REG_SIGNAL_PATH_RESET:
      return;

    case REG_USER_CTRL:
      regs[r] = (uint8_t)(v & 0xF0); /* low nibble is reset strobes */
      return;

    default:
      if (r >= REG_ACCEL_XOUT_H && r <= REG_GYRO_ZOUT_L) {
        return;                    /* sensor data is read-only */
      }
      regs[r] = v;
      return;
  }
}

/* ------------------------------------------------------------------------- */
/* I2C target                                                                */
/* ------------------------------------------------------------------------- */

static inline void IRAM_ATTR bus_start(void) {
  bus_state = ST_ADDR;
  bit_index = 0;
  rx_shift = 0;
  addressed = false;
  read_dir = false;
  first_write_byte = true;
  SDA_REL();
  SCL_REL();
  bus_active = true;
  stat_starts = stat_starts + 1u;
}

static inline void IRAM_ATTR bus_stop(void) {
  bus_state = ST_IDLE;
  SDA_REL();
  SCL_REL();
  bus_active = false;
  stat_stops = stat_stops + 1u;
}

/* SCL low: put the next SDA bit on the wire, then release SCL. Holding SCL
 * until the end of this handler is clock stretching, which gives the ISR
 * time to work without changing the master's protocol. */
static inline void IRAM_ATTR on_scl_fall(void) {
  switch (bus_state) {
    case ST_TX:
      if (tx_shift & 0x80) {
        SDA_REL();
      } else {
        SDA_LOW();
      }
      break;

    case ST_ACK_ADDR:
      if (addressed) {
        SDA_LOW();                 /* ACK address */
      } else {
        SDA_REL();                 /* NACK address */
      }
      break;

    case ST_ACK_RX:
      SDA_LOW();                   /* ACK received byte */
      break;

    case ST_ACK_TX:
      SDA_REL();                   /* let master drive ACK/NACK */
      break;

    default:
      SDA_REL();
      break;
  }
  SCL_REL();
}

/* SCL high: sample SDA and advance the byte state machine. */
static inline void IRAM_ATTR on_scl_rise(void) {
  switch (bus_state) {
    case ST_ADDR:
      rx_shift = (uint8_t)((rx_shift << 1) | (SDA_IS_HIGH() ? 1u : 0u));
      bit_index = (uint8_t)(bit_index + 1u);
      if (bit_index == 8) {
        uint8_t addr = (uint8_t)(rx_shift >> 1);
        read_dir = (rx_shift & 0x01u) != 0;
        addressed = (addr == MPU_ADDR);
        bit_index = 0;
        if (addressed) {
          stat_addr_acks = stat_addr_acks + 1u;
          bus_state = ST_ACK_ADDR;
        } else {
          stat_addr_nacks = stat_addr_nacks + 1u;
          bus_state = ST_IGNORE;
        }
      }
      break;

    case ST_ACK_ADDR:
      bit_index = 0;
      rx_shift = 0;
      if (read_dir) {
        tx_shift = read_reg(reg_ptr);
        reg_ptr = (uint8_t)((reg_ptr + 1u) & 0x7Fu);
        bus_state = ST_TX;
      } else {
        first_write_byte = true;
        bus_state = ST_RX;
      }
      break;

    case ST_RX:
      rx_shift = (uint8_t)((rx_shift << 1) | (SDA_IS_HIGH() ? 1u : 0u));
      bit_index = (uint8_t)(bit_index + 1u);
      if (bit_index == 8) {
        if (first_write_byte) {
          reg_ptr = (uint8_t)(rx_shift & 0x7Fu);
          first_write_byte = false;
        } else {
          write_reg(reg_ptr, rx_shift);
          reg_ptr = (uint8_t)((reg_ptr + 1u) & 0x7Fu);
        }
        stat_rx_bytes = stat_rx_bytes + 1u;
        rx_shift = 0;
        bit_index = 0;
        bus_state = ST_ACK_RX;
      }
      break;

    case ST_ACK_RX:
      bit_index = 0;
      rx_shift = 0;
      bus_state = ST_RX;
      break;

    case ST_TX:
      tx_shift = (uint8_t)(tx_shift << 1);
      bit_index = (uint8_t)(bit_index + 1u);
      if (bit_index == 8) {
        bit_index = 0;
        stat_tx_bytes = stat_tx_bytes + 1u;
        bus_state = ST_ACK_TX;
      }
      break;

    case ST_ACK_TX:
      if (SDA_IS_HIGH()) {           /* NACK: master is done */
        stat_master_nacks = stat_master_nacks + 1u;
        bus_state = ST_IDLE;
      } else {                       /* ACK: send next byte */
        tx_shift = read_reg(reg_ptr);
        reg_ptr = (uint8_t)((reg_ptr + 1u) & 0x7Fu);
        bit_index = 0;
        bus_state = ST_TX;
      }
      break;

    default:
      break;
  }
}

/* Both pins share this handler. SDA changing while SCL is high is the only
 * place START/Sr and STOP can occur; every other edge is a clock edge. */
static void IRAM_ATTR i2c_isr(void) {
  uint8_t scl = SCL_IS_HIGH() ? 1u : 0u;
  uint8_t sda = SDA_IS_HIGH() ? 1u : 0u;

  edge_count = edge_count + 1u;

  if (prev_scl && scl) {
    if (prev_sda && !sda) {
      bus_start();
      prev_scl = scl;
      prev_sda = sda;
      return;
    }
    if (!prev_sda && sda) {
      bus_stop();
      prev_scl = scl;
      prev_sda = sda;
      return;
    }
  }

  if (prev_scl && !scl) {
    on_scl_fall();
  } else if (!prev_scl && scl) {
    on_scl_rise();
  }

  prev_scl = scl;
  prev_sda = sda;
}

/* ------------------------------------------------------------------------- */
/* Sensor model                                                              */
/* ------------------------------------------------------------------------- */

static int16_t to_i16(float v) {
  if (v > 32767.0f) return 32767;
  if (v < -32768.0f) return -32768;
  return (int16_t)(v >= 0.0f ? v + 0.5f : v - 0.5f);
}

static void put_i16(uint8_t reg, int16_t v) {
  regs[reg]     = (uint8_t)((uint16_t)v >> 8);
  regs[reg + 1] = (uint8_t)((uint16_t)v & 0xFFu);
}

/* Sample rate = gyro output rate / (1 + SMPLRT_DIV); gyro runs at 8 kHz with
 * DLPF off and 1 kHz otherwise. */
static uint32_t sample_interval_us(void) {
  uint8_t dlpf = (uint8_t)(regs[REG_CONFIG] & 0x07u);
  uint32_t base = (dlpf == 0) ? 8000u : 1000u;
  uint32_t div = (uint32_t)regs[REG_SMPLRT_DIV] + 1u;
  uint32_t rate = base / div;
  if (rate == 0) rate = 1;
  uint32_t us = 1000000u / rate;
  if (us < 125u) us = 125u;
  return us;
}

static void update_sensor(uint32_t now_ms) {
  uint8_t pwr1 = regs[REG_PWR_MGMT_1];
  uint8_t pwr2 = regs[REG_PWR_MGMT_2];
  uint8_t afs  = (uint8_t)((regs[REG_ACCEL_CONFIG] >> 3) & 0x03u);
  uint8_t fs   = (uint8_t)((regs[REG_GYRO_CONFIG] >> 3) & 0x03u);

  float ax_g = 0.0f, ay_g = 0.0f, az_g = 1.0f;
  float gx_dps = 0.0f, gy_dps = 0.0f, gz_dps = 0.0f;

  if (motion_mode) {
    float seconds = (float)now_ms / 1000.0f;
    float angle = 2.0f * 3.14159265f * 0.10f * seconds;
    ax_g = sinf(angle);
    az_g = cosf(angle);
    gy_dps = 36.0f;                /* 0.1 Hz tilt = 36 dps peak */
  }

  const float accel_lsb = 16384.0f / (float)(1u << afs);
  const float gyro_lsb  = 131.0f / (float)(1u << fs);

  int16_t ax = to_i16(ax_g * accel_lsb);
  int16_t ay = to_i16(ay_g * accel_lsb);
  int16_t az = to_i16(az_g * accel_lsb);
  int16_t gx = to_i16(gx_dps * gyro_lsb);
  int16_t gy = to_i16(gy_dps * gyro_lsb);
  int16_t gz = to_i16(gz_dps * gyro_lsb);
  int16_t temp = (pwr1 & 0x08u) ? 0 : to_i16((25.0f - 36.53f) * 340.0f);

  if (pwr2 & 0x08u) ax = 0;
  if (pwr2 & 0x10u) ay = 0;
  if (pwr2 & 0x20u) az = 0;
  if (pwr2 & 0x01u) gx = 0;
  if (pwr2 & 0x02u) gy = 0;
  if (pwr2 & 0x04u) gz = 0;

  /* Publish all 14 bytes at once so a burst read cannot tear a sample. */
  portENTER_CRITICAL(&g_mux);
  put_i16(REG_ACCEL_XOUT_H, ax);
  put_i16(REG_ACCEL_YOUT_H, ay);
  put_i16(REG_ACCEL_ZOUT_H, az);
  put_i16(REG_TEMP_OUT_H, temp);
  put_i16(REG_GYRO_XOUT_H, gx);
  put_i16(REG_GYRO_YOUT_H, gy);
  put_i16(REG_GYRO_ZOUT_H, gz);
  regs[REG_INT_STATUS] |= 0x01u;
  portEXIT_CRITICAL(&g_mux);
}

/* ------------------------------------------------------------------------- */
/* Console                                                                   */
/* ------------------------------------------------------------------------- */

static bool verbose = false;   /* clean STM32 terminal by default; 'v' enables */

static void print_help(void) {
  Serial.println("[ESP] b bridge  m motion  s static  d dump  r reset  v stats  ? help");
}

static void dump_regs(void) {
  Serial.printf("[ESP] reg_ptr=0x%02X  PWR_MGMT_1=0x%02X\n",
                (unsigned)reg_ptr, (unsigned)regs[REG_PWR_MGMT_1]);
  for (uint8_t row = 0; row < 8; row++) {
    Serial.printf("[ESP] %02X:", (unsigned)(row * 16u));
    for (uint8_t col = 0; col < 16; col++) {
      Serial.printf(" %02X", (unsigned)regs[(row * 16u) + col]);
    }
    Serial.println();
  }
}

static void print_stats(void) {
  noInterrupts();
  uint32_t s  = stat_starts;
  uint32_t p  = stat_stops;
  uint32_t aa = stat_addr_acks;
  uint32_t an = stat_addr_nacks;
  uint32_t rx = stat_rx_bytes;
  uint32_t tx = stat_tx_bytes;
  uint32_t mn = stat_master_nacks;
  uint8_t  st = bus_state;
  uint8_t  rp = reg_ptr;
  uint32_t ud = uart_dropped;
  bool     br = bridge_enabled;
  interrupts();

  Serial.printf("[ESP] bus S=%lu P=%lu ACK=%lu NAK=%lu RX=%lu TX=%lu MN=%lu st=%u rp=0x%02X br=%u drop=%lu\n",
                (unsigned long)s, (unsigned long)p,
                (unsigned long)aa, (unsigned long)an,
                (unsigned long)rx, (unsigned long)tx,
                (unsigned long)mn, (unsigned)st, (unsigned)rp,
                (unsigned)br, (unsigned long)ud);
}

static void handle_serial(void) {
  while (Serial.available() > 0) {
    switch (Serial.read()) {
      case 'b':
        bridge_enabled = !bridge_enabled;
        Serial.printf("[ESP] bridge %s\n", bridge_enabled ? "on" : "off");
        break;
      case 'm':
        motion_mode = true;
        Serial.println("[ESP] motion");
        break;
      case 's':
        motion_mode = false;
        Serial.println("[ESP] static");
        break;
      case 'd':
        dump_regs();
        break;
      case 'r':
        portENTER_CRITICAL(&g_mux);
        reset_registers();
        portEXIT_CRITICAL(&g_mux);
        last_sample_us = micros();
        Serial.println("[ESP] reset");
        break;
      case 'v':
        verbose = !verbose;
        Serial.printf("[ESP] stats %s\n", verbose ? "on" : "off");
        break;
      case 'h':
      case '?':
        print_help();
        break;
      default:
        break;
    }
  }
}

/* ------------------------------------------------------------------------- */
/* Housekeeping                                                              */
/* ------------------------------------------------------------------------- */

/* A master that ACKs the last read byte and then STOPs can wedge the bus
 * behind our next data bit. Release the lines if nothing moves for 100 ms. */
static void recover_stuck_bus(void) {
  portENTER_CRITICAL(&g_mux);
  bus_state = ST_IDLE;
  SDA_REL();
  SCL_REL();
  bus_active = false;
  portEXIT_CRITICAL(&g_mux);
}

static void housekeeping(void) {
  static uint32_t last_edges = 0;
  static uint32_t last_edge_ms = 0;
  static uint32_t last_stats_ms = 0;

  uint32_t now_ms = millis();
  uint32_t edges = edge_count;

  if (edges != last_edges) {
    last_edges = edges;
    last_edge_ms = now_ms;
  } else if (bus_active && (uint32_t)(now_ms - last_edge_ms) > 100u) {
    recover_stuck_bus();
    Serial.println("[ESP] bus recovery");
    last_edge_ms = now_ms;
  }

  if (verbose && (uint32_t)(now_ms - last_stats_ms) >= 1000u) {
    last_stats_ms = now_ms;
    print_stats();
  }
}

/* ------------------------------------------------------------------------- */
/* UART2 -> USB bridge                                                       */
/* ------------------------------------------------------------------------- */

/* The task only fills the ring; loop() is the single writer to USB Serial,
 * so the bridge and the console cannot interleave inside one write. */
static inline void uart_ring_push(uint8_t b) {
  uint32_t next = (uart_head + 1u) & (UART_RING_SIZE - 1u);
  if (next == uart_tail) {
    uart_dropped = uart_dropped + 1u;
    return;
  }
  uart_ring[uart_head] = b;
  uart_head = next;
}

static void uart_bridge_task(void *arg) {
  (void)arg;
  for (;;) {
    int avail = Serial2.available();
    while (avail-- > 0) {
      int c = Serial2.read();
      if (c < 0) break;
      if (!bridge_enabled) continue;
      uart_ring_push((uint8_t)c);
    }
    vTaskDelay(1);
  }
}

static void drain_uart_ring(void) {
  while (uart_tail != uart_head) {
    uint8_t b = uart_ring[uart_tail];
    uart_tail = (uart_tail + 1u) & (UART_RING_SIZE - 1u);
    if (bridge_enabled) Serial.write(b);
  }
}

/* ------------------------------------------------------------------------- */
/* Setup / loop                                                              */
/* ------------------------------------------------------------------------- */

static void pins_init(void) {
  /* Latch high before enabling the open-drain outputs, or the pins glitch low. */
  GPIO.out_w1ts = (1u << SDA_PIN) | (1u << SCL_PIN);

  gpio_config_t cfg = {};
  cfg.pin_bit_mask = (1ULL << SDA_PIN) | (1ULL << SCL_PIN);
  cfg.mode         = GPIO_MODE_INPUT_OUTPUT_OD;
  cfg.pull_up_en   = GPIO_PULLUP_ENABLE;
  cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
  cfg.intr_type    = GPIO_INTR_DISABLE;
  gpio_config(&cfg);

  SDA_REL();
  SCL_REL();
  delayMicroseconds(100);

  prev_scl = SCL_IS_HIGH() ? 1 : 0;
  prev_sda = SDA_IS_HIGH() ? 1 : 0;

  attachInterrupt(digitalPinToInterrupt(SDA_PIN), i2c_isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(SCL_PIN), i2c_isr, CHANGE);
}

void setup() {
  pins_init();
  reset_registers();
  last_sample_us = micros();

  Serial.begin(SERIAL_BAUD);

  /* Start UART2 before the USB settle so early STM32 bytes are buffered.
   * Do not call pinMode() on 16/17; Serial2 owns them via the GPIO matrix. */
  Serial2.setRxBufferSize(1024);
  Serial2.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

  if (xTaskCreatePinnedToCore(uart_bridge_task, "uart_bridge", 4096,
                              nullptr, 1, nullptr, 0) != pdPASS) {
    Serial.println("[ESP] UART task creation failed");
  }

  delay(300);

  Serial.println();
  Serial.printf("[ESP] Fake MPU-6050 0x%02X  SDA=%d SCL=%d\n",
                MPU_ADDR, SDA_PIN, SCL_PIN);
  Serial.printf("[ESP] UART2 RX=%d TX=%d @%d -> USB\n",
                UART_RX_PIN, UART_TX_PIN, UART_BAUD);
  Serial.println("[ESP] cmds: b bridge  m motion  s static  d dump  r reset  v stats  ? help");
  Serial.println("[ESP] reset the STM32 now");
}

void loop() {
  drain_uart_ring();

  uint32_t now_us = micros();

  /* Update only between transactions so a 14-byte burst stays coherent. */
  if (!bus_active && (uint32_t)(now_us - last_sample_us) >= sample_interval_us()) {
    last_sample_us = now_us;
    if (!(regs[REG_PWR_MGMT_1] & 0x40u)) {
      update_sensor(millis());
    }
  }

  handle_serial();
  housekeeping();
  delay(1);   /* yield to the idle task; I2C runs in the GPIO ISR */
}
