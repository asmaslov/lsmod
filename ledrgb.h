#ifndef __LEDRGB_H_
#define __LEDRGB_H_

#include <inttypes.h>
#include <stdbool.h>

#define LEDRGB_TOTAL_LEN  58
#define LEDRGB_REDUCED    32

extern uint32_t LedrgbColor;

void LedrgbInit(void);
void LedrgbLoadColor(void);
void LedrgbSaveColor(void);
void LedrgbOn(uint32_t color);
void LedrgbOff(void);
void LedrgbSet(uint32_t color, uint8_t len);

#endif // __LEDRGB_H_
