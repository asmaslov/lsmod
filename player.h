#ifndef __PLAYER_H_
#define __PLAYER_H_

#include "lsmod_config.h"

#include <inttypes.h>
#include <stdbool.h>

#define PLAYER_MAX_TRACKS  6
#define PLAYER_FREQ_HZ     44100

extern volatile bool PlayerActive;
extern uint32_t PlayerTracksAddr[PLAYER_MAX_TRACKS];
extern uint32_t PlayerTracksLen[PLAYER_MAX_TRACKS];
extern volatile uint32_t PlayerTrackPos;
extern uint16_t PlayerMaxValue;

void PlayerInit(void);
void PlayerLoadMem(void);
void PlayerSaveMem(void);
void PlayerStart(uint8_t track);
void PlayerStop(void);

#endif // __PLAYER_H_
