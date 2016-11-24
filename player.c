#include "player.h"
#include "dataflash_at45db321b.h"

#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>

/****************************************************************************
 * Private types/enumerations/variables                                     *
 ****************************************************************************/

static const uint16_t prescale1[6] = {0, 1, 8, 64, 256, 1024};
static uint8_t div1;

uint32_t EEMEM tracksAddrMem[PLAYER_MAX_TRACKS];
uint32_t EEMEM tracksLenMem[PLAYER_MAX_TRACKS];

static uint8_t sound;
static uint32_t trackPos = 0;
static uint32_t trackLen = 0;

/****************************************************************************
 * Public types/enumerations/variables                                      *
 ****************************************************************************/

volatile bool PlayerActive = false;
uint32_t PlayerTracksAddr[PLAYER_MAX_TRACKS];
uint32_t PlayerTracksLen[PLAYER_MAX_TRACKS];

/****************************************************************************
 * Private functions                                                        *
 ****************************************************************************/

/****************************************************************************
 * Interrupt handler functions                                              *
 ****************************************************************************/

ISR(TIMER1_OVF_vect)
{
  if (++trackPos < trackLen)
  {
    OCR1A = sound;
    DataflashReadContiniousNext();
  }
  else
  {
    PlayerStop();
  }
}

/****************************************************************************
 * Public functions                                                         *
 ****************************************************************************/

void PlayerInit(void)
{
  uint32_t icr;
  uint8_t div;

  TIMSK1 &= ~(1 << OCIE1A);
  TCCR1A = 0x00;
  TCCR1B = 0x00;
  div = 0;
  do {
    icr = F_CPU / (PLAYER_FREQ_HZ * prescale1[++div]) - 1;
  } while (icr > UINT16_MAX);
  div1 = div;
  ICR1 = (uint16_t)icr;
  TCCR1A = (1 << COM1A1) | (1 << COM1A0) | (1 << WGM11) | (0 << WGM10);
  TCCR1B = (1 << WGM13) | (1 << WGM12);
  DDRB |= (1 << DDB1);
}

void PlayerLoadMem(void)
{
  cli();
  eeprom_read_block(PlayerTracksAddr, tracksAddrMem, sizeof(uint32_t) * PLAYER_MAX_TRACKS);
  eeprom_read_block(PlayerTracksLen, tracksLenMem, sizeof(uint32_t) * PLAYER_MAX_TRACKS);
  sei();
}

void PlayerSaveMem(void)
{
  cli();
  eeprom_update_block(PlayerTracksAddr, tracksAddrMem, sizeof(uint32_t) * PLAYER_MAX_TRACKS);
  eeprom_update_block(PlayerTracksLen, tracksLenMem, sizeof(uint32_t) * PLAYER_MAX_TRACKS);
  sei();
}

void PlayerStart(uint8_t track)
{ 
  assert(track < PLAYER_MAX_TRACKS);
  if (PlayerTracksLen[track] != 0)
  {
    PlayerActive = true;
    DataflashReadContinious(PlayerTracksAddr[track], &sound);
    trackLen = PlayerTracksLen[track];
    trackPos = 0;
    OCR1A = sound;
    DataflashReadContiniousNext();
    TCCR1B |= (((div1 >> 2) & 1) << CS12) | (((div1 >> 1) & 1) << CS11) | (((div1 >> 0) & 1) << CS10);
    TCNT1 = 0;
    TIMSK1 |= (1 << TOIE1);
  }
}

void PlayerStop(void)
{
  TIMSK1 &= ~(1 << TOIE1);
  TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10));
  DataflashReadContiniousStop();
  PlayerActive = false;
}
