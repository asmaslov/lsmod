#include "accel_mma7455l.h"
#include "i2c.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <math.h>
#include <assert.h>

/****************************************************************************
 * Private types/enumerations/variables                                     *
 ****************************************************************************/

/****************************************************************************
 * Public types/enumerations/variables                                      *
 ****************************************************************************/

bool Mma7455l_Connected = false;
bool Mma7455l_Error = false;
volatile bool Mma7455l_MotionDetected = false;

MMA7455L_VALUES Mma7455l_AccelReal;
MMA7455L_ANGLES Mma7455l_AnglesReal;

/****************************************************************************
 * Private functions                                                        *
 ****************************************************************************/

static bool writeReg(uint8_t regAddr, uint8_t data)
{
  if (I2C_WriteData(MMA7455L_I2C_ADDR, regAddr, &data, 1) != 1)
  {
    return false;
  }
  return true;
}

static bool readReg(uint8_t regAddr, uint8_t *dest, uint16_t count)
{
  if (I2C_ReadData(MMA7455L_I2C_ADDR, regAddr, dest, count) != count)
  {
    return false;
  }
  return true;
}

static void clearInt(void)
{
  writeReg(MMA7455L_INTRST,
           MMA7455L_INTRST_CLRINT1 |
           MMA7455L_INTRST_CLRINT2);
  writeReg(MMA7455L_INTRST,
           ~MMA7455L_INTRST_CLRINT1 &
           ~MMA7455L_INTRST_CLRINT2);
}

/****************************************************************************
 * Interrupt handler functions                                              *
 ****************************************************************************/

ISR(INT0_vect)
{
  Mma7455l_MotionDetected = true;
  clearInt();
}

/****************************************************************************
 * Public functions                                                         *
 ****************************************************************************/

bool Mma7455l_Init(void)
{
  uint8_t reg;
  
  Mma7455l_AnglesReal.roll = 0;
  Mma7455l_AnglesReal.pitch = 0;
  Mma7455l_AccelReal.x = 0;
  Mma7455l_AccelReal.y = 0;
  Mma7455l_AccelReal.z = 0;
  I2C_Setup();
  if (!writeReg(MMA7455L_MCTL,
                MMA7455L_MCTL_MODE_LEVEL_DETECT |
                MMA7455L_MCTL_GLVL_2G))
  {
    return false;
  }
  if (!readReg(MMA7455L_MCTL, &reg, 1))
  {
    return false;
  }
  if (!writeReg(MMA7455L_CTL1,
                MMA7455L_CTL1_INTREG_LEVEL_DETECT))
  {
    return false;
  }
  if (!readReg(MMA7455L_CTL1, &reg, 1))
  {
    return false;
  }
  if (!writeReg(MMA7455L_LDTH,
                MMA7455L_THRESHOLD))
  {
    return false;
  }
  if (!readReg(MMA7455L_LDTH, &reg, 1))
  {
    return false;
  }
  EICRA = (0 << ISC00) | (0 << ISC01);
  EIMSK = (1 << INT0);
  Mma7455l_Connected = true;
  return true;
}

bool Mma7455l_Read(MMA7455L_VALUES* values)
{
  uint8_t buffer[6], ctrl, stat;
  int16_t rawX, rawY, rawZ;
  float sens;
  
  if (readReg(MMA7455L_XOUTL, buffer, 6) &&
      readReg(MMA7455L_MCTL, &ctrl, 1) &&
      readReg(MMA7455L_STATUS, &stat, 1))
  {
    switch(ctrl & MMA7455L_MCTL_GLVL_MASK)
    {
      case MMA7455L_MCTL_GLVL_2G:
        sens = MMA7455L_SENSITIVITY_2G;
        break;
      case MMA7455L_MCTL_GLVL_4G:
        sens = MMA7455L_SENSITIVITY_4G;
        break;
      case MMA7455L_MCTL_GLVL_8G:
        sens = MMA7455L_SENSITIVITY_8G;
        break;
    }
    rawX = ((((uint16_t)buffer[1] << 8) | buffer[0]) & 0x1FF) * (buffer[1] & (1 << 1)) ? -1 : 1;
    rawY = ((((uint16_t)buffer[3] << 8) | buffer[2]) & 0x1FF) * (buffer[3] & (1 << 1)) ? -1 : 1;
    rawZ = ((((uint16_t)buffer[5] << 8) | buffer[4]) & 0x1FF) * (buffer[5] & (1 << 1)) ? -1 : 1;
    values->x = rawX * sens / 16;
    values->y = rawY * sens / 16;
    values->z = rawZ * sens / 16;
    return true;
  }
  return false;
}

bool Mma7455l_Get(MMA7455L_ANGLES* angles)
{
  MMA7455L_VALUES accel;
  float norm;
  float sinRoll, cosRoll, sinPitch, cosPitch;
  
  if (Mma7455l_Read(&accel))
  {
    Mma7455l_Error = false;
    accel.x /= 100;
    accel.y /= 100;
    accel.z /= 100;
    norm = sqrt(accel.x * accel.x + accel.y * accel.y + accel.z * accel.z);
    sinRoll = accel.y / norm;
    cosRoll = sqrt(1.0 - (sinRoll * sinRoll));
    sinPitch = accel.x / norm;
    cosPitch = sqrt(1.0 - (sinPitch * sinPitch));
    angles->roll = (float)(asin(sinRoll) * 180 / M_PI);
    angles->pitch = (float)(asin(sinPitch) * 180 / M_PI);
    return true;
  }
  Mma7455l_Error = true;
  return false;
}
