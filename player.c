#include "player.h"

#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>

#include "debug.h"

/****************************************************************************
 * Private types/enumerations/variables                                     *
 ****************************************************************************/

static const uint16_t prescale1[6] = {0, 1, 8, 64, 256, 1024};
static uint8_t div1;

static uint32_t EEMEM tracksAddrMem[PLAYER_MAX_TRACKS];
static uint32_t EEMEM tracksLenMem[PLAYER_MAX_TRACKS];

static uint8_t sound;
static bool test = false;
static uint8_t testSound[PLAYER_BUFFER_SIZE];
static uint32_t trackPos = 0;
static uint32_t trackLen = 0;

/****************************************************************************
 * Public types/enumerations/variables                                      *
 ****************************************************************************/

volatile bool PlayerActive= false;
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
  trackPos++;
  if (test)
  {
    if (trackPos < PLAYER_BUFFER_SIZE)
    {
      OCR1A = testSound[trackPos];
    }
    else
    {
      PlayerStop();
    }
  }
  else
  {
    if (trackPos < trackLen)
    {
      OCR1A = sound;
      DataflashReadContiniousNext();
    }
    else
    {
      PlayerStop();
    }
  }
}

/****************************************************************************
 * Public functions                                                         *
 ****************************************************************************/

void PlayerInit(void)
{
  uint8_t div;
  uint32_t icr;

  TIMSK1 &= ~(1 << OCIE1A);
  TCCR1A = 0x00;
  TCCR1B = 0x00;
  div = 0;
  do {
    icr = F_CPU / (PLAYER_FREQ_HZ * prescale1[++div]) - 1;
  } while (icr > UINT16_MAX);
  ICR1 = (uint16_t)icr; 
  div1 = div;
  TCCR1A = (1 << COM1A1) | (0 << COM1A0) | (1 << WGM11) | (0 << WGM10);
  TCCR1B = (1 << WGM13) | (1 << WGM12);
  DDRB |= (1 << DDB1);
}

void PlayerLoadMem(void)
{
  uint8_t i;
  
  for (i = 0; i < PLAYER_MAX_TRACKS; i++)
  {
    PlayerTracksAddr[i] = eeprom_read_dword(&tracksAddrMem[i]);
    PlayerTracksLen[i] = eeprom_read_dword(&tracksLenMem[i]);
  }
}

void PlayerSaveMem(void)
{
  uint8_t i;

  eeprom_write_dword(&tracksAddrMem[0], PlayerTracksAddr[0]);
  eeprom_busy_wait();
  eeprom_write_dword(&tracksLenMem[0], PlayerTracksLen[0]);
  eeprom_busy_wait();

  //TODO: Fix strange bug with memory save, it saves shit
  /*for (i = 0; i < PLAYER_MAX_TRACKS; i++)
  {
    eeprom_write_dword(&tracksAddrMem[i], PlayerTracksAddr[i]);
    eeprom_busy_wait();
    eeprom_write_dword(&tracksLenMem[i], PlayerTracksLen[i]);
    eeprom_busy_wait();
  }*/
}

void PlayerTest(void)
{
  uint16_t i;
  double val;
  
  PlayerActive = true;
  for (i = 0; i < PLAYER_BUFFER_SIZE; i++)
  {
    val = i;
    val = 127 * sin(2 * M_PI * val * PLAYER_TEST_FREQ_HZ / PLAYER_FREQ_HZ);
    testSound[i] = 0x80 + (int8_t)val;
  }
  test = true;
  trackLen = PLAYER_BUFFER_SIZE;
  trackPos = 0;
  OCR1A = testSound[0];
  TCCR1B |= (((div1 >> 2) & 1) << CS12) | (((div1 >> 1) & 1) << CS11) | (((div1 >> 0) & 1) << CS10);
  TCNT1 = 0;
  TIMSK1 |= (1 << TOIE1);
}

void PlayerStart(uint8_t track)
{ 
  assert(track < PLAYER_MAX_TRACKS);
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

void PlayerStop(void)
{
  if (test)
  {
    test = false;
  }
  else
  {
    DataflashReadContiniousStop();
  }    
  TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10));
  TIMSK1 &= ~(1 << TOIE1);
  PlayerActive = false;
}
