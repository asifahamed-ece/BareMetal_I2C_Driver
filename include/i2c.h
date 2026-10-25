#ifndef I2C_H
#define I2C_H

void i2c_init();
uint8_t i2c_probe(uint8_t);

uint8_t i2c_write(uint8_t addr, uint8_t *data, uint32_t len);

#endif