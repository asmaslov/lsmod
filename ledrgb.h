#ifndef __LEDRGB_H_
#define __LEDRGB_H_

#include <inttypes.h>
#include <stdbool.h>

#define RGB_RED          0xFF0000
#define RGB_ORANGE_RED   0xFF4500
#define RGB_ORANGE       0xFFA500
#define RGB_GOLDEN_ROD   0xDAA520
#define RGB_YELLOW       0xFFFF00
#define RGB_GREEN        0x00FF00
#define RGB_LIME_GREEN   0x32CD32
#define RGB_TURQUOISE    0x40E0D0
#define RGB_DODGER_BLUE  0x1E90FF
#define RGB_BLUE         0x0000FF
#define RGB_BLUE_VIOLET  0x8A2BE2
#define RGB_PURPLE       0x800080
#define RGB_ORCHID       0xDA70D6
#define RGB_LAVENDER     0xE6E6FA
#define RGB_SILVER       0xC0C0C0

#define LEDRGB_MAX_LEN  100

void LedrgbInit(void);
void LedrgbOn(uint32_t color);
void LedrgbOff(void);
void LedrgbSet(uint32_t color, uint8_t len);

#endif // __LEDRGB_H_
