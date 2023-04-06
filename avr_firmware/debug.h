#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <avr/io.h>
#include <inttypes.h>
#include <util/delay.h>

#include "comport.h"

static inline void debug(uint8_t on)
{
  if (on == 0)
  {
    PORTD |= (1 << PD4);
  }
  else
  {
    PORTD &= ~(1 << PD4);
  }
}

static inline void debugout(char ch)
{
  ComportDebug(ch);
}

static inline void debugstr(char *str)
{
  ComportDebugString(str);
}

static inline void deblink(int t)
{
  int i;
  for (i = 0; i < t; i++)
  {
    debug(1);
    _delay_ms(150);
    debug(0);
    _delay_ms(150);
  }
}

static inline void debinv(void)
{
  if (PIND & (1 << PIND4))
  {
    debug(1);
  }
  else
  {
    debug(0);
  }
}

#endif // __DEBUG_H__
