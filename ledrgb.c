#include "ledrgb.h"

#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <math.h>

/****************************************************************************
 * Private types/enumerations/variables                                     *
 ****************************************************************************/

static uint32_t EEMEM colorMem;

/****************************************************************************
 * Public types/enumerations/variables                                      *
 ****************************************************************************/

uint32_t LedrgbColor;

/****************************************************************************
 * Private functions                                                        *
 ****************************************************************************/

static inline void _delay_ns(double __ns)
{
  double __tmp;
  
#if __HAS_DELAY_CYCLES && defined(__OPTIMIZE__) && !defined(__DELAY_BACKWARD_COMPATIBLE__)
  uint32_t __ticks_dc;
  
  __tmp = ((F_CPU) / 1e9) * __ns;
#if defined(__DELAY_ROUND_DOWN__)
  __ticks_dc = (uint32_t)fabs(__tmp);
#elif defined(__DELAY_ROUND_CLOSEST__)
  __ticks_dc = (uint32_t)(fabs(__tmp)+0.5);
#else
  __ticks_dc = (uint32_t)(ceil(fabs(__tmp)));
#endif
  __builtin_avr_delay_cycles(__ticks_dc);

#elif !__HAS_DELAY_CYCLES || (__HAS_DELAY_CYCLES && !defined(__OPTIMIZE__)) || defined (__DELAY_BACKWARD_COMPATIBLE__)
  uint8_t __ticks;

  __tmp = ((F_CPU) / 3e9) * __ns;
  if (__tmp < 1.0)
  {
    __ticks = 1;
  }    
  else if (__tmp > 255)
  {
    _delay_us(__ns / 1000.0);
    return;
  }
  else
  {
    __ticks = (uint8_t)__tmp;
  }    
  _delay_loop_1(__ticks);
#endif
}

static void bit0(void)
{
  PORTD |= (1 << PD6);
  _delay_ns(400);
  PORTD &= ~(1 << PD6);
  _delay_ns(1600);
}
static void bit1(void)
{
  PORTD |= (1 << PD6);
  _delay_ns(1200);
  PORTD &= ~(1 << PD6);
  _delay_ns(800);
}

static void setColor(uint32_t color)
{
  int8_t i, j;
  uint8_t grb[3];
  
  grb[0] = (uint8_t)(color >> 8);
  grb[1] = (uint8_t)(color >> 16);
  grb[2] = (uint8_t)color;
  
  for (i = 0; i < 3; i++)
  {
    for (j = 7; j >= 0; j--)
    {
      if (grb[i] & (1 << j))
      {
        bit1();
      }
      else
      {
        bit0();
      }
    }
  }
}

static void bitStop(void)
{
  PORTD &= ~(1 << PD6);
  _delay_us(50);
}

#ifdef LEDRGB_REDUCED
static uint32_t reduce(uint32_t val)
{
  uint8_t bytes[3];
  uint32_t res;
  
  bytes[0] = (uint8_t)val;
  bytes[0] /= LEDRGB_REDUCED;
  bytes[1] = (uint8_t)(val >> 8);
  bytes[1] /= LEDRGB_REDUCED;
  bytes[2] = (uint8_t)(val >> 16);
  bytes[2] /= LEDRGB_REDUCED;
  res = 0;
  res += bytes[2];
  res <<= 8;
  res += bytes[1];
  res <<= 8;
  res += bytes[0];
  return res;
}
#endif

/****************************************************************************
 * Public functions                                                         *
 ****************************************************************************/

void LedrgbInit(void)
{
  PORTD &= ~(1 << PD6);
  DDRD |= (1 << DDD6);
}

void LedrgbLoadColor(void)
{
  cli();
  LedrgbColor = eeprom_read_dword(&colorMem);
#ifdef LEDRGB_REDUCED
  LedrgbColor = reduce(LedrgbColor);
#endif
  sei();
}

void LedrgbSaveColor(void)
{
  cli();
  eeprom_write_dword(&colorMem, LedrgbColor);
  sei();
}

void LedrgbOn(uint32_t color)
{
  uint8_t i;
  
  cli();
  for (i = 0; i < LEDRGB_TOTAL_LEN; ++i)
  {
    setColor(color);
  }
  sei();
  bitStop();
}

void LedrgbOff(void)
{
  uint8_t i;
  
  cli();
  for (i = 0; i < LEDRGB_TOTAL_LEN; ++i)
  {
    setColor(0);
  }
  sei();
  bitStop();
}

void LedrgbSet(uint32_t color, uint8_t len)
{
  uint8_t i;
  
  if (len > LEDRGB_TOTAL_LEN)
  {
    len = LEDRGB_TOTAL_LEN;
  }
  if (len > 0)
  {
    cli();
    for (i = 1; i <= len; ++i)
    {
      setColor(color);
    }
    if (len < LEDRGB_TOTAL_LEN)
    {
      for (i = len + 1; i <= LEDRGB_TOTAL_LEN; ++i)
      {
        setColor(0);
      }
    }
    sei();
    bitStop();
  }
  else
  {
    LedrgbOff();
  }
}
