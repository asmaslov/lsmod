#ifndef __ACCEL_ADXL330_H__
#define __ACCEL_ADXL330_H__

#include <inttypes.h>
#include <stdbool.h>

#define ADXL330_CHAN_X  0
#define ADXL330_CHAN_Y  1
#define ADXL330_CHAN_Z  2

#define ADXL330_THRESHOLD  20

typedef struct {
  float x;
  float y;
  float z;
} ADXL330_VALUES;

typedef struct {
  float roll;
  float pitch;
} ADXL330_ANGLES;

extern volatile bool Adxl330_MotionDetected;

extern ADXL330_VALUES Adxl330_AccelReal;
extern ADXL330_ANGLES Adxl330_AnglesReal;

void Adxl330_Init(void);
void Adxl330_Read(ADXL330_VALUES* values);
void Adxl330_Get(ADXL330_ANGLES* angles);

#endif // __ACCEL_ADXL330__
