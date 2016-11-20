#include "player.h"

#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>

/****************************************************************************
 * Private types/enumerations/variables                                     *
 ****************************************************************************/

static const uint16_t prescale1[6] = {0, 1, 8, 64, 256, 1024};
static uint8_t div1;

static uint32_t EEMEM tracksAddrMem[PLAYER_MAX_TRACKS];
static uint32_t EEMEM tracksLenMem[PLAYER_MAX_TRACKS];

static uint16_t soundBuffer[PLAYER_BUFFER_SIZE];
static uint8_t trackIdx = 0;
static uint32_t trackPos = 0;
static uint8_t len = 0;

/****************************************************************************
 * Public types/enumerations/variables                                      *
 ****************************************************************************/

volatile bool PlayerActive;
uint32_t PlayerTracksAddr[PLAYER_MAX_TRACKS];
uint32_t PlayerTracksLen[PLAYER_MAX_TRACKS];

/****************************************************************************
 * Private functions                                                        *
 ****************************************************************************/

/****************************************************************************
 * Interrupt handler functions                                              *
 ****************************************************************************/

/****************************************************************************
 * Public functions                                                         *
 ****************************************************************************/

void PlayerInit(void)
{
  uint8_t div;
  uint32_t ocr;

  TIMSK1 &= ~(1 << OCIE1A);
  TCCR1A = 0x00;
  TCCR1B = 0x00;
  div = 0;
  do {
	  ocr = F_CPU / (PLAYER_FREQ_HZ * prescale1[++div]) - 1;
  } while (ocr > UINT16_MAX);
  OCR1A = (uint16_t)ocr;
  div1 = div;
	TCCR1A = (1 << COM1A1) | (0 << COM1A0) | (1 << WGM11) | (1 << WGM10);
  TCCR1B = (0 << WGM13) | (1 << WGM12);
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
  
  for (i = 0; i < PLAYER_MAX_TRACKS; i++)
  {
    eeprom_write_dword(&tracksAddrMem[i], PlayerTracksAddr[i]);
    eeprom_write_dword(&tracksLenMem[i], PlayerTracksLen[i]);
  }
}

void PlayerTest(void)
{
	PlayerActive = true;
}

/*static void PlayerStart(void)
{
	uint8_t i;
	
	for (i = 0; i < SOUND_BUFFER_SIZE; i++)
	{
		soundBuffer[2 * i + 1] = i + 0x8000;
	}
	//DataflashRead((tracksAddr[0] + trackPos), soundBuffer, SOUND_BUFFER_SIZE);
	
	
	//OCR1A = soundBuffer[0];
	OCR1A = 0x34;
  TCCR1B |= (((div1 >> 2) & 1) << CS12) | (((div1 >> 1) & 1) << CS11) | (((div1 >> 0) & 1) << CS10);
  TCNT1 = 0;
  //TIMSK1 |= (1 << OCIE1A);
  led2(true);
}

static void PlayerStop(void)
{
	TIMSK1 &= ~(1 << OCIE1A);
  TCCR1B &= ~((((div1 >> 2) & 1) << CS12) | (((div1 >> 1) & 1) << CS11) | (((div1 >> 0) & 1) << CS10));
  play = false;
  led2(false);
}

ISR(TIMER1_COMPA_vect)
{
	debug(1);
	trackPos++;
	if (trackPos == SOUND_BUFFER_SIZE)
	{
		trackPos = 0;
	}
	//OCR1A = soundBuffer[trackPos];
	//OCR1AL = soundBuffer[2 * trackPos + 1];
}

/*ISR(TIMER1_OVF_vect)
{
	if (trackPos == tracksLen[0])
	{
		PlayerStop();
	}
}*/