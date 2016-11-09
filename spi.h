#ifndef __SPI_H
#define __SPI_H

#include <inttypes.h>
#include <stdbool.h>

#define SPI_FREQUENCY_HZ  10000000

typedef void (*SPIHandler)(void);

volatile bool SPI_TransferCompleted;

void SPI_Init(void); 
void SPI_ReadWrite(uint8_t* tx_dat, uint8_t* rx_dat, uint16_t tx_ln, uint16_t rx_ln, SPIHandler hdl);

#endif // __SPI_H
