#include "dataflash_at45db321b.h"
#include "spi.h"

#include <string.h>
#include <util/delay.h>
#include <assert.h>
#include <stdlib.h>

/****************************************************************************
 * Private types/enumerations/variables                                     *
 ****************************************************************************/

static uint8_t buffer[DB321_BUFFER_SIZE];

static bool dataflashInitialized = false;
static bool dataflashBusy = false;

/****************************************************************************
 * Public types/enumerations/variables                                      *
 ****************************************************************************/

/****************************************************************************
 * Private functions                                                        *
 ****************************************************************************/

static void bufferInit(void)
{
  uint8_t i;

  for (i = 0; i < DB321_BUFFER_SIZE; i++)
  {
    buffer[i] = 0x00;
  }
}

static void DataflashCheckID(void)
{
  bufferInit();
  buffer[0] = DB321_GET_ID;
  SPI_WriteRead(buffer, 1, 4);
  while (!SPI_TransferCompleted) {}
  if((buffer[1] == 0x1F) &&
     ((buffer[2] & 0xE0) == 0x20) &&
     (buffer[3] == 1)  &&
     (buffer[4] == 0))
  {
    dataflashInitialized = true;
  }
  else
  {
    dataflashInitialized = false;
  }
}

/****************************************************************************
 * Public functions                                                         *
 ****************************************************************************/

bool DataflashInit(void)
{
  SPI_Init();
  DataflashCheckID();
  return dataflashInitialized;
}

bool DataflashRead(uint32_t src, uint8_t *dst, uint8_t size)
{
  uint8_t dataBytesCount;
  uint32_t pageAddress;
  uint32_t byteAddress;
  
  assert(dataflashInitialized);
  if(dataflashBusy)
  {
    return false;
  }
  dataflashBusy = true;
  pageAddress = src / DB321_PAGE_SIZE;
  byteAddress = src % DB321_PAGE_SIZE;
  if((byteAddress + size) > DB321_PAGE_SIZE)
  {
    dataBytesCount = (DB321_PAGE_SIZE - byteAddress);
  }
  else
  {
    dataBytesCount = size;
  }
  bufferInit();
  buffer[0] = DB321_PAGE_READ;
  buffer[1] = (uint8_t)((pageAddress >> 6) & 0x7F);
  buffer[2] = (uint8_t)((pageAddress << 2) & 0xFC) | (uint8_t)((byteAddress >> 8) & 0x03);
  buffer[3] = (uint8_t)(byteAddress & 0xFF);
  buffer[4] = 0xFF;
  buffer[5] = 0xFF;
  buffer[6] = 0xFF;
  buffer[7] = 0xFF;
  SPI_WriteRead(buffer, 8, dataBytesCount);
  while (!SPI_TransferCompleted) {}
  memcpy(dst, &buffer[8], dataBytesCount);
  if(dataBytesCount < size)
  {
    pageAddress++;
    dst += dataBytesCount;
    dataBytesCount = size - dataBytesCount;
    bufferInit();
    buffer[0] = DB321_PAGE_READ;
    buffer[1] = (uint8_t)((pageAddress >> 6) & 0x7F);
    buffer[2] = (uint8_t)((pageAddress << 2) & 0xFC);
    buffer[3] = 0x00;
    buffer[4] = 0xFF;
    buffer[5] = 0xFF;
    buffer[6] = 0xFF;
    buffer[7] = 0xFF;
    SPI_WriteRead(buffer, 8, dataBytesCount);
    while (!SPI_TransferCompleted) {}
    memcpy(dst, &buffer[8], dataBytesCount);
  }
  return true;
}

bool DataflashWrite(uint8_t *src, uint32_t dst, uint8_t size)
{
  uint8_t dataBytesCount;
  uint32_t pageAddress;
  uint32_t byteAddress;
  
  assert(dataflashInitialized);
  if(dataflashBusy)
  {
    return false;
  }
  dataflashBusy = true;
  pageAddress = dst / DB321_PAGE_SIZE;
  buffer[0] = DB321_PAGE_2_BUF1_TRF;
  buffer[1] = (uint8_t)((pageAddress >> 6) & 0x7F);
  buffer[2] = (uint8_t)((pageAddress << 2) & 0xFC);
  buffer[3] = 0x00;
  SPI_WriteRead(buffer, 4, 0);
  while (!SPI_TransferCompleted) {}
  _delay_us(DB321_PAGE_2_BUF_TRF_T_US);
  byteAddress = dst % DB321_PAGE_SIZE;
  if((byteAddress + size) > DB321_PAGE_SIZE)
  {
    dataBytesCount = (DB321_PAGE_SIZE - byteAddress);
  }
  else
  {
    dataBytesCount = size;
  }
  buffer[0] = DB321_PAGE_PGM_BUF1;
  buffer[1] = (uint8_t)((pageAddress >> 6) & 0x7F);
  buffer[2] = (uint8_t)((pageAddress << 2) & 0xFC) | (uint8_t)((byteAddress >> 8) & 0x03);
  buffer[3] = (uint8_t)(byteAddress & 0xFF);
  memcpy(&buffer[4], src, dataBytesCount);
  SPI_WriteRead(buffer, 4 + dataBytesCount, 0);
  while (!SPI_TransferCompleted) {}
  _delay_ms(DB321_PAGE_ERASE_PGM_T_MS);
  if(dataBytesCount < size)
  {
    pageAddress++;
    src += dataBytesCount;
    dataBytesCount = size - dataBytesCount;
    buffer[0] = DB321_PAGE_2_BUF1_TRF;
    buffer[1] = (uint8_t)((pageAddress >> 6) & 0x7F);
    buffer[2] = (uint8_t)((pageAddress << 2) & 0xFC);
    buffer[3] = 0x00;
    SPI_WriteRead(buffer, 4, 0);
    while (!SPI_TransferCompleted) {}
    _delay_us(DB321_PAGE_2_BUF_TRF_T_US);
    buffer[0] = DB321_PAGE_PGM_BUF1;
    buffer[1] = (uint8_t)((pageAddress >> 6) & 0x7F);
    buffer[2] = (uint8_t)((pageAddress << 2) & 0xFC);
    buffer[3] = 0x00;
    memcpy(&buffer[4], src, dataBytesCount);
    SPI_WriteRead(buffer, 4 + dataBytesCount, 0);
    while (!SPI_TransferCompleted) {}
    _delay_ms(DB321_PAGE_ERASE_PGM_T_MS);
  }  
  dataflashBusy = false;
  return true;
}

bool DataflashReadContinious(uint32_t src, uint8_t *dst)
{
  uint32_t pageAddress;
  uint32_t byteAddress;
  
  assert(dataflashInitialized);
  if(dataflashBusy)
  {
    return false;
  }
  dataflashBusy = true;
  pageAddress = src / DB321_PAGE_SIZE;
  byteAddress = src % DB321_PAGE_SIZE;
  bufferInit();
  buffer[0] = DB321_CONTINUOUS_ARRAY_READ;
  buffer[1] = (uint8_t)((pageAddress >> 6) & 0x7F);
  buffer[2] = (uint8_t)((pageAddress << 2) & 0xFC) | (uint8_t)((byteAddress >> 8) & 0x03);
  buffer[3] = (uint8_t)(byteAddress & 0xFF);
  buffer[4] = 0xFF;
  buffer[5] = 0xFF;
  buffer[6] = 0xFF;
  buffer[7] = 0xFF;
  SPI_WriteReadContinious(buffer, 8, dst);
  while (!SPI_TransferCompleted) {}
  return true;
}

void DataflashReadContiniousNext(void)
{
  SPI_WriteReadContiniousNext();
}

void DataflashReadContiniousStop(void)
{
  SPI_WriteReadContiniousStop();
  dataflashBusy = false;
}
