#include "adc.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <assert.h>
#include <stdlib.h>

#include "debug.h"

/****************************************************************************
 * Private types/enumerations/variables                                     *
 ****************************************************************************/

static bool initialized = false;
static ADCHandler handlers[ADC_TOTAL_CHANNELS];
static uint16_t values[ADC_TOTAL_CHANNELS];

/****************************************************************************
 * Public types/enumerations/variables                                      *
 ****************************************************************************/

/****************************************************************************
 * Private functions                                                        *
 ****************************************************************************/

static void dummyHandler(void)
{
  
}

/****************************************************************************
 * Interrupt handler functions                                              *
 ****************************************************************************/

ISR(ADC_vect)
{
  uint8_t ch;
  
  ch = ADMUX & 0xF;
  values[ch] = ADCW;
  handlers[ch]();
  if(++ch >= ADC_TOTAL_CHANNELS)
  {
    ch = 0;
  }
  ADMUX = ADC_VREF | ch;
  ADCSRA |= (1 << ADSC);
}

/****************************************************************************
 * Public functions                                                         *
 ****************************************************************************/

void ADC_Init(void)
{
  int i;
  
  if (!initialized)
  {
    DIDR0 = 0x00;
    ADMUX = ADC_VREF;
    ADCSRA = (1 << ADEN) | (1 << ADIE) | ADC_ADPS;
    ADCSRB = ADC_ADTS;
    for (i = 0; i < 8; i++)
    {
      values[i] = 0;
      handlers[i] = dummyHandler;
    }
    ADCSRA |= (1 << ADSC);
    initialized = true;
  }  
}

uint16_t* ADC_ChannelSetup(uint8_t ch, ADCHandler hnd)
{
  assert(initialized);
  assert(ch < ADC_TOTAL_CHANNELS);
  DIDR0 |= (1 << ch);
  if (hnd)
  {
    handlers[ch] = hnd;
  }

  return &values[ch];
}

uint16_t ADC_Read(uint8_t ch)
{
  assert(initialized);
  return values[ch];
}
