#ifndef __I2C_H__
#define __I2C_H__

//#include "lsmod_config.h"

#include <inttypes.h>

#define I2C_FREQUENCY_HZ  100000
#define I2C_MAX_ITER      10
#define I2C_TIMEOUT_TC    1000

void I2C_Setup(void);
int I2C_ReadData(uint8_t i2cAddr, uint8_t subAddr, uint8_t* data, int len);
int I2C_WriteData(uint8_t i2cAddr, uint8_t subAddr, uint8_t* data, int len);

#endif // __COMPORT_H__
