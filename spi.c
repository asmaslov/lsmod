#include "spi.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stddef.h>

/****************************************************************************
 * Private types/enumerations/variables                                     *
 ****************************************************************************/

static bool continious = false;
static uint8_t* data = NULL;
static uint8_t* read;
static uint8_t tx_len, rx_len, cnt;

/****************************************************************************
 * Public types/enumerations/variables                                      *
 ****************************************************************************/

volatile bool SPI_TransferCompleted;

/****************************************************************************
 * Private functions                                                        *
 ****************************************************************************/

static void chipSelect(bool select)
{
  if (select)
  {
    PORTB &= ~(1 << PB2);
  }
  else
  {
    PORTB |= (1 << PB2);
  }   
}

/****************************************************************************
 * Interrupt handler functions                                              *
 ****************************************************************************/

ISR(SPI_STC_vect)
{
  if (cnt < (tx_len + rx_len))
  {
    *(data + cnt) = SPDR;
    cnt++;
  }
  if (cnt < tx_len)
  {
    SPDR = *(data + cnt);
  }
  else
  {
    if (cnt < (tx_len + rx_len))
    {
      SPDR = 0xFF;
    }
    else
    {
      if (continious)
      {
        read = SPDR;
      }
      else
      {      
        chipSelect(false);
      }      
      SPI_TransferCompleted = true;
    }    
  }
}

/****************************************************************************
 * Public functions                                                         *
 ****************************************************************************/

void SPI_Init(void)
{
  uint8_t dummy;
  
  PORTB |= (1 << PB2);
  DDRB |= (1 << DDB2) | (1 << DDB3) | (1 << DDB5);
  dummy = SPSR;
  dummy = dummy;
  SPCR = (0 << CPOL) | (0 << CPHA) | (1 << SPE) | (1 << SPIE) | (1 << MSTR);
  // TODO: Calculate bits according to rate
  SPCR |= (0 << SPR0) | (1 << SPR1); // 312.5 Kbit (20 MHz clock)
  //SPCR |= (1 << SPR0) | (1 << SPR1); // 156.250 Kbit (20 MHz clock)
  SPI_TransferCompleted = false;
}

void SPI_WriteRead(uint8_t* dat, uint8_t tx_ln, uint8_t rx_ln)
{
  cnt = 0;
  data = dat;
  tx_len = tx_ln;
  rx_len = rx_ln;
  SPI_TransferCompleted = false;
  chipSelect(true);
  if (tx_ln != 0)
  {
    SPDR = *data;
  }
  else
  {
    SPDR = 0xFF;
  }
}

void SPI_WriteReadContinious(uint8_t* dat, uint8_t tx_ln, uint8_t* r)
{
  cnt = 0;
  data = dat;
  tx_len = tx_ln;
  rx_len = 0;
  read = r;
  continious = true;
  SPI_TransferCompleted = false;
  chipSelect(true);
  if (tx_ln != 0)
  {
    SPDR = *data;
  }
  else
  {
    SPDR = 0xFF;
  }
}

void SPI_WriteReadContiniousNext(void)
{
  SPDR = 0xFF;
}

void SPI_WriteReadContiniousStop(void)
{
  continious = false;
  chipSelect(false);
}
