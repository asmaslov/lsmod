#ifndef __ADC_H_
#define __ADC_H_

#include <inttypes.h>
#include <stdbool.h>

#define ADC_TOTAL_CHANNELS  8

#define ADC_VREF  ((0 << REFS0) | (0 << REFS1))  // AREF pin
#define ADC_ADTS  ((0 << ADTS2) | (0 << ADTS1) | (0 << ADTS0))  // Free running
#define ADC_ADPS  ((1 << ADPS2) | (0 << ADPS1) | (1 << ADPS0))  // 625 kHz (20 MHz clock)
#define ADC_MAX_VALUE  0x400

typedef void (*ADCHandler)(void);

void ADC_Init(void);
uint16_t* ADC_ChannelSetup(uint8_t ch, ADCHandler hnd);
uint16_t ADC_Read(uint8_t ch);

#endif // __ADC_H_
