#ifndef __SPI_H
#define __SPI_H

#include <inttypes.h>
#include <stdbool.h>

extern volatile bool SPI_TransferCompleted;

void SPI_Init(void);
void SPI_WriteRead(uint8_t* dat, uint8_t tx_ln, uint8_t rx_ln);
void SPI_WriteReadContinious(uint8_t* dat, uint8_t tx_ln, uint8_t* r);
void SPI_WriteReadContiniousNext(void);
void SPI_WriteReadContiniousStop(void);

#endif // __SPI_H
