#ifndef __LSMOD_CONFIG_H__
#define __LSMOD_CONFIG_H__

//#define MMA7455L_USED
#define ADXL330_USED

//#define CLASH_DISABLE
#define SENSOR_DELAY_TICKS  10000

#define VOLTAGE_DELAY_TICKS  100000
#define VOLTAGE_CHAN      3
#define VOLTAGE_ACCUMUL   100
#define VOLTAGE_MUL       125
#define VOLTAGE_MAX       90
#define VOLTAGE_HIGH      86
#define REDUCE_HIGH       2
#define VOLTAGE_AVERAGE   76
#define REDUCE_AVERAGE    4
#define VOLTAGE_LOW       70
#define REDUCE_LOW        16    
#define VOLTAGE_CRITICAL  68

#ifndef F_CPU
  #define F_CPU  20000000
#endif

#define LSMOD_ADDR  0x21
#define PC_ADDR     0xA1

#define TRACK_TURNON   0
#define TRACK_HUM      1
#define TRACK_SWING    2
#define TRACK_HIT      3
#define TRACK_CLASH    4
#define TRACK_TURNOFF  5

#endif // __LSMOD_CONFIG_H__
