#ifndef __LSMOD_CONFIG_H__
#define __LSMOD_CONFIG_H__

//#define MMA7455L_USED
#define ADXL330_USED
//#define WATCHDOG_USED

#ifndef F_CPU
  #define F_CPU  20000000
#endif

#define MAX_TRACKS  6

#define LSMOD_ADDR  0x21
#define PC_ADDR     0xA1

#endif // __LSMOD_CONFIG_H__
