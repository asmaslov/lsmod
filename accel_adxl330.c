#include "accel_adxl330.h"
#include "adc.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <math.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>

/****************************************************************************
 * Private types/enumerations/variables                                     *
 ****************************************************************************/

uint16_t* rawX = NULL;
uint16_t* rawY = NULL;
uint16_t* rawZ = NULL;
static volatile uint8_t xcount, ycount, zcount;
static ADXL330_VALUES accelPrev;
static volatile bool calculated;
static ADXL330_VALUES accelAccum;
static const uint32_t prescale2[8] = {0, 1, 8, 32, 64, 128, 256, 1024};

/****************************************************************************
 * Public types/enumerations/variables                                      *
 ****************************************************************************/

volatile bool Adxl330_HitDetected = false;
volatile bool Adxl330_MotionDetected = false;

ADXL330_VALUES Adxl330_AccelReal;
ADXL330_ANGLES Adxl330_AnglesReal;

/****************************************************************************
 * Private functions                                                        *
 ****************************************************************************/

static void readyX(void)
{
  if (xcount < ADXL330_ACCUMUL)
  {
    xcount++;
    accelAccum.x += *rawX;
    accelAccum.x -= ADXL330_ZERO;
  }  
}

static void readyY(void)
{
  if (ycount < ADXL330_ACCUMUL)
  {
    ycount++;
    accelAccum.y += *rawY;
    accelAccum.y -= ADXL330_ZERO;
  }  
}

static void readyZ(void)
{
  if (zcount < ADXL330_ACCUMUL)
  {
    zcount++;
    accelAccum.z += *rawZ;
    accelAccum.z -= ADXL330_ZERO;
  }  
}

static void timer2Setup(uint16_t freq)
{
  uint8_t div;
  uint32_t ocr;
  
  div = 0;
  do {
    ocr = F_CPU / (freq * prescale2[++div]) - 1;
  } while (ocr > UINT8_MAX);
  TCCR2A = 0x00;
  TCCR2B = 0x00;
  OCR2A = (uint8_t)ocr;
  TCCR2A = (1 << WGM21);
  TCCR2B = (((div >> 2) & 1) << CS22) | (((div >> 1) & 1) << CS21) | (((div >> 0) & 1) << CS20);
  TCNT2 = 0;
  TIMSK2 |= (1 << OCIE2A);
}

/****************************************************************************
 * Interrupt handler functions                                              *
 ****************************************************************************/

ISR(TIMER2_COMPA_vect)
{
  if (xcount == ADXL330_ACCUMUL)
  {
    Adxl330_AccelReal.x = accelAccum.x / ADXL330_ACCUMUL;
    accelAccum.x = 0;
    xcount = 0;
  }
  if (ycount == ADXL330_ACCUMUL)
  {
    Adxl330_AccelReal.y = accelAccum.y / ADXL330_ACCUMUL;
    accelAccum.y = 0;
    ycount = 0;
  }
  if (zcount == ADXL330_ACCUMUL)
  {
    Adxl330_AccelReal.z = accelAccum.z / ADXL330_ACCUMUL;
    accelAccum.z = 0;
    zcount = 0;
  }
  if ((abs(Adxl330_AccelReal.x - accelPrev.x) > ADXL330_HIT) ||
      (abs(Adxl330_AccelReal.y - accelPrev.y) > ADXL330_HIT) ||
      (abs(Adxl330_AccelReal.z - accelPrev.z) > ADXL330_HIT))
  {
    Adxl330_HitDetected = true;
  }
  else
  {
    if ((abs(Adxl330_AccelReal.x - accelPrev.x) > ADXL330_MOTION) ||
        (abs(Adxl330_AccelReal.y - accelPrev.y) > ADXL330_MOTION) ||
        (abs(Adxl330_AccelReal.z - accelPrev.z) > ADXL330_MOTION))
    {
      Adxl330_MotionDetected = true;
    }
  }
  accelPrev.x = Adxl330_AccelReal.x;
  accelPrev.y = Adxl330_AccelReal.y;
  accelPrev.z = Adxl330_AccelReal.z;
  if ((accelPrev.x != 0) || (accelPrev.y != 0) || (accelPrev.z != 0))
  {
    calculated = true;
  }
}

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
  accelAccum.x = 0;
  accelAccum.y = 0;
  accelAccum.y = 0;
  accelPrev.x = 0;
  accelPrev.y = 0;
  accelPrev.z = 0;
  xcount = 0;
  ycount = 0;
  zcount = 0;
  ADC_Init();
  rawX = ADC_ChannelSetup(ADXL330_CHAN_X, readyX);
  rawY = ADC_ChannelSetup(ADXL330_CHAN_Y, readyY);
  rawZ = ADC_ChannelSetup(ADXL330_CHAN_Z, readyZ);
  calculated = false;
  timer2Setup(ADXL330_FREQ_HZ);
  while (!calculated);
  Adxl330_HitDetected = false;
  Adxl330_MotionDetected = false;
}

void Adxl330_Get(ADXL330_ANGLES* angles)
{
  ADXL330_VALUES accel;
  float norm;
  float sinRoll, cosRoll, sinPitch, cosPitch;
  
  accel.x = Adxl330_AccelReal.x / 100;
  accel.y = Adxl330_AccelReal.y / 100;
  accel.z = Adxl330_AccelReal.z / 100;
  norm = sqrt(accel.x * accel.x + accel.y * accel.y + accel.z * accel.z);
  sinRoll = accel.y / norm;
  cosRoll = sqrt(1.0 - (sinRoll * sinRoll));
  sinPitch = accel.x / norm;
  cosPitch = sqrt(1.0 - (sinPitch * sinPitch));
  angles->roll = (float)(asin(sinRoll) * 180 / M_PI);
  angles->pitch = (float)(asin(sinPitch) * 180 / M_PI);
}
