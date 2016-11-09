#include "spi.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stddef.h>

/****************************************************************************
 * Private types/enumerations/variables                                     *
 ****************************************************************************/

static SPIHandler handler = NULL;
static uint8_t* tx_data = NULL;
static uint8_t* rx_data = NULL;
static uint16_t tx_len, rx_len, cnt;

/****************************************************************************
 * Public types/enumerations/variables                                      *
 ****************************************************************************/

/****************************************************************************
 * Private functions                                                        *
 ****************************************************************************/

static void chipSelect(bool select)
{
	if (select)
	{
    PORTB |= (1 << PB2);
	}
	else
	{
		PORTB &= ~(1 << PB2);
	}   
}

static void dummyHandler(void)
{
	
}

/****************************************************************************
 * Interrupt handler functions                                              *
 ****************************************************************************/

ISR(SPI_STC_vect)
{
	if (SPSR & (1 << WCOL))
	{
		*(rx_data + cnt) = SPDR;
	}
	if (cnt < tx_len)
	{
    cnt++;
    if (cnt < tx_len)
    {
	    SPDR = *(tx_data + cnt);
    }		
	}
	else
	{
    if (cnt < (tx_len + rx_len))
    {
	    *(rx_data + cnt) = SPDR;
			cnt++;
    }
	  else
    {
	    chipSelect(false);
	    SPI_TransferCompleted = true;
	    handler();
	    handler = dummyHandler;
	  }    
	}
}

/****************************************************************************
 * Public functions                                                         *
 ****************************************************************************/

void SPI_Init(void)
{
	uint32_t rate;
	
	DDRB |= (1 << DDB2) | (1 << DDB3) | (1 << DDB5);
  SPCR = (0 << CPOL) | (0 << CPHA) | (1 << SPE) | (1 << SPIE) | (1 << MSTR);
  rate = (F_CPU / SPI_FREQUENCY_HZ) / 2;
  
  // TODO: Calculate bits according to rate
  SPCR |= (0 << SPR0) | (0 << SPR1);
  SPSR = (1 << SPI2X);  // 10 Mbit (20 MHz clock)
  
  SPI_TransferCompleted = false;
  handler = dummyHandler;
}

void SPI_ReadWrite(uint8_t* tx_dat, uint8_t* rx_dat, uint16_t tx_ln, uint16_t rx_ln, SPIHandler hnd)
{
  cnt = 0;
  tx_data = tx_dat;
  rx_data = rx_dat;
  tx_len = tx_ln;
  rx_len = rx_ln;
  if (hnd)
  {
	  handler = hnd;
  }
  SPI_TransferCompleted = false;
  chipSelect(true);
  if (tx_ln != 0)
  {
	  SPDR = *tx_dat;
  }
}
