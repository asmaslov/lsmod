#ifndef __LSMOD_CONFIG_H__
#define __LSMOD_CONFIG_H__

//#define MMA7455L_USED
#define ADXL330_USED

#ifndef F_CPU
  #define F_CPU  20000000
#endif

#define LSMOD_ADDR  0x21
#define PC_ADDR     0xA1

#define LSMOD_COLOR  RGB_ORCHID

#define TRACK_TURNON   0
#define TRACK_HUM      1
#define TRACK_SWING    2
#define TRACK_HIT      3
#define TRACK_CLASH    4
#define TRACK_TURNOFF  5

#endif // __LSMOD_CONFIG_H__
