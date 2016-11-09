#ifndef __LSMOD_CONFIG_H__
#define __LSMOD_CONFIG_H__

#ifndef F_CPU
  #define F_CPU  20000000
#endif

#define LSMOD_ADDR  0x21
#define PC_ADDR     0xA1

/*#define HEAD_FIRE_TRIGGER_ACTIVATE  (1 << PC6)
#define HEAD_FIRE_TRIGGER_FIRE      (1 << PC7)

#define HEAD_FIRE_SINGLE_ACTIVATE_DELAY_MS  1800
#define HEAD_FIRE_SINGLE_FIRE_TIME_MS       530*/

#endif // __LSMOD_CONFIG_H__
