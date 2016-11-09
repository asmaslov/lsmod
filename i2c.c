#include "i2c.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include <util/twi.h>

void I2C_Setup(void)
{
  uint32_t rate;
  
  TWSR &= ~((1 << TWPS0) | (1 << TWPS1));
  rate = (F_CPU / I2C_FREQUENCY_HZ - 16) / 2;
  TWBR = (uint8_t)rate;
}

int I2C_ReadData(uint8_t i2cAddr, uint8_t subAddr, uint8_t *data, int len)
{
  uint8_t twcr, twsr;
  int n = 0, t, total = 0;

  cli();
restart:
  TWCR &= ~(1 << TWEN);
  if (n++ >= I2C_MAX_ITER)
  {
    sei();
    return -1;
  }
begin:
  TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);  // Send start condition
  t = I2C_TIMEOUT_TC;
  while (!(TWCR & (1 << TWINT)) && (--t > 0));
  twsr = TW_STATUS;
  switch (twsr)
  {
    case TW_REP_START:
    case TW_START:
      break;
    case TW_MT_ARB_LOST:
      goto begin;
    default:
      sei();
      return -1;
  }
  TWDR = i2cAddr | TW_WRITE;  // Send address byte with write flag
  TWCR = (1 << TWINT) | (1 << TWEN);
  t = I2C_TIMEOUT_TC;
  while (!(TWCR & (1 << TWINT)) && (--t > 0));
  twsr = TW_STATUS;
  switch (twsr)
  {
    case TW_MT_SLA_ACK:
      break;
    case TW_MT_SLA_NACK:
      goto restart;
    case TW_MT_ARB_LOST:
      goto begin;
    default:
      goto error;
  }
  TWDR = subAddr;  // Send sub-address byte
  TWCR = (1 << TWINT) | (1 << TWEN);
  t = I2C_TIMEOUT_TC;
  while (!(TWCR & (1 << TWINT)) && (--t > 0));
  twsr = TW_STATUS;
  switch (twsr)
  {
    case TW_MT_DATA_ACK:
      break;
    case TW_MT_DATA_NACK:
      goto quit;
    case TW_MT_ARB_LOST:
      goto begin;
    default:
      goto error;
  }
  TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);  // Send repeated start condition
  t = I2C_TIMEOUT_TC;
  while (!(TWCR & (1 << TWINT)) && (--t > 0));
  twsr = TW_STATUS;
  switch (twsr)
  {
    case TW_START:  
    case TW_REP_START:
      break;
    case TW_MT_ARB_LOST:
      goto begin;
    default:
      goto error;
  }
  TWDR = i2cAddr | TW_READ;  // Send address byte with read flag
  TWCR = (1 << TWINT) | (1 << TWEN);
  t = I2C_TIMEOUT_TC;
  while (!(TWCR & (1 << TWINT)) && (--t > 0));
  twsr = TW_STATUS;
  switch (twsr)
  {
    case TW_MR_SLA_ACK:
      break;
    case TW_MR_SLA_NACK:
      goto quit;
    case TW_MR_ARB_LOST:
      goto begin;
    default:
      goto error;
  }
  while(len-- > 0)
  {
    if (len != 0)
    {
      twcr = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
    }
    else
    {
      twcr = (1 << TWINT) | (1 << TWEN);  // Last byte
    }  
    TWCR = twcr;
    t = I2C_TIMEOUT_TC;
    while (!(TWCR & (1 << TWINT)) && (--t > 0));
    twsr = TW_STATUS;
    switch (twsr)
    {
      case TW_MR_DATA_NACK:
        len = 0;  //  Force end of loop
      case TW_MR_DATA_ACK:
        *data++ = TWDR;  //  Get data byte
        total++;
        break;
      default:
        goto error;
    }
  }
quit:
  TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);  // Send stop condition
  sei();
  return total;
error:
  total = -1;
  goto quit;
}

int I2C_WriteData(uint8_t i2cAddr, uint8_t subAddr, uint8_t* data, int len)
{
  uint8_t twsr;
  int n = 0, t, total = 0;
  
  cli();
restart:
  TWCR &= ~(1 << TWEN);
  if (n++ >= I2C_MAX_ITER)
  {
    sei();
    return -1;
  }
begin:
  TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);  // Send start condition
  t = I2C_TIMEOUT_TC;
  while (!(TWCR & (1 << TWINT)) && (--t > 0));
  twsr = TW_STATUS;
  switch (twsr)
  {
    case TW_REP_START:
    case TW_START:
      break;
    case TW_MT_ARB_LOST:
      goto begin;
    default:
      sei();
      return -1;
  }
  TWDR = i2cAddr | TW_WRITE;  // Send address byte with write flag
  TWCR = (1 << TWINT) | (1 << TWEN);
  t = I2C_TIMEOUT_TC;
  while (!(TWCR & (1 << TWINT)) && (--t > 0));
  twsr = TW_STATUS;
  switch (twsr)
  {
    case TW_MT_SLA_ACK:
      break;
    case TW_MT_SLA_NACK:
      goto restart;
    case TW_MT_ARB_LOST:
      goto begin;
    default:
      goto error;
  }
  TWDR = subAddr;  // Send sub-address byte
  TWCR = (1 << TWINT) | (1 << TWEN);
  t = I2C_TIMEOUT_TC;
  while (!(TWCR & (1 << TWINT)) && (--t > 0));
  twsr = TW_STATUS;
  switch (twsr)
  {
    case TW_MT_DATA_ACK:
      break;
    case TW_MT_DATA_NACK:
      goto quit;
    case TW_MT_ARB_LOST:
      goto begin;
    default:
      goto error;
  }
  while (len-- > 0)
  {
    TWDR = *data++;  // Send data byte
    TWCR = (1 << TWINT) | (1 << TWEN);
    t = I2C_TIMEOUT_TC;
    while (!(TWCR & (1 << TWINT)) && (--t > 0));
    twsr = TW_STATUS;
    switch (twsr)
    {
      case TW_MT_DATA_NACK:
        goto error;
      case TW_MT_DATA_ACK:
        total++;
        break;
      default:
        goto error;
    }
  }
quit:
  TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);  // Send stop condition
  sei();
  return total;
error:
  total = -1;
  goto quit;
}
