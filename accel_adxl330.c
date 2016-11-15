#include "accel_adxl330.h"
#include "adc.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <math.h>
#include <assert.h>
#include <stddef.h>

/****************************************************************************
 * Private types/enumerations/variables                                     *
 ****************************************************************************/

/****************************************************************************
 * Public types/enumerations/variables                                      *
 ****************************************************************************/

volatile bool Adxl330_MotionDetected = false;

uint16_t* rawX = NULL;
uint16_t* rawY = NULL;
uint16_t* rawZ = NULL;
ADXL330_VALUES Adxl330_AccelReal;
ADXL330_ANGLES Adxl330_AnglesReal;

/****************************************************************************
 * Private functions                                                        *
 ****************************************************************************/

static void readyX(void)
{
  if (*rawX > ADXL330_THRESHOLD)
  {
    Adxl330_MotionDetected = true;
  }
}

static void readyY(void)
{
  if (*rawY > ADXL330_THRESHOLD)
  {
    Adxl330_MotionDetected = true;
  }
}

static void readyZ(void)
{
  if (*rawZ > ADXL330_THRESHOLD)
  {
    Adxl330_MotionDetected = true;
  }
}

/****************************************************************************
 * Interrupt handler functions                                              *
 ****************************************************************************/

/****************************************************************************
 * Public functions                                                         *
 ****************************************************************************/

void Adxl330_Init(void)
{
  Adxl330_AnglesReal.roll = 0;
  Adxl330_AnglesReal.pitch = 0;
  Adxl330_AccelReal.x = 0;
  Adxl330_AccelReal.y = 0;
  Adxl330_AccelReal.z = 0;
  ADC_Init();
  rawX = ADC_ChannelSetup(ADXL330_CHAN_X, readyX);
  rawY = ADC_ChannelSetup(ADXL330_CHAN_Y, readyY);
  rawZ = ADC_ChannelSetup(ADXL330_CHAN_Z, readyZ);
}

void Adxl330_Read(ADXL330_VALUES* values)
{
  values->x = *rawX / 16;
  values->y = *rawY / 16;
  values->z = *rawZ / 16;
}

void Adxl330_Get(ADXL330_ANGLES* angles)
{
  ADXL330_VALUES accel;
  float norm;
  float sinRoll, cosRoll, sinPitch, cosPitch;
  
  Adxl330_Read(&accel);
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
}
