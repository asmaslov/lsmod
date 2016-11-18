#include "dataflash_at45db321b.h"
#include "spi.h"

#include <string.h>
#include <util/delay.h>
#include <assert.h>
#include <stdlib.h>

#include "debug.h"

/****************************************************************************
 * Private types/enumerations/variables                                     *
 ****************************************************************************/

static uint8_t buffer[DB321_BUFFER_SIZE];
static uint8_t txLen, rxLen;

static DataflashCommonCallback DataflashOperationFinishCallback = NULL;

static bool dataflashInitialized = false;
static bool dataflashBusy = false;
static bool dataflashLastPageSaved = false;
static bool dataflashEraseManual = false;
static DataflashCommonCallback DataflashWriteLastWaitFunction = NULL;
static DataflashCommonCallback DataflashEraseLastManualWaitFunction = NULL;

static uint8_t* dataSource = NULL;
static uint8_t* dataDest = NULL;
static uint32_t dataflashDest = 0;
static uint8_t dataBytes = 0;
static uint8_t dataBytesLeft = 0;
static uint32_t dataPageAddress = 0;

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
  txLen = 0;
  rxLen = 0;
}

static void DataflashDummyCallback(void)
{
  
}

static void DataflashOperationFinish(void)
{
  dataflashBusy = false;
  DataflashOperationFinishCallback();
}

static void DataflashCheckID(void)
{
  buffer[0] = DB321_GET_ID;
  txLen = 1;
  rxLen = 4;
  SPI_ReadWrite(buffer, txLen, rxLen, NULL);
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

static void DataflashReadContinue(void)
{
  memcpy(dataDest, &buffer[8], dataBytes);
  dataDest += dataBytes;
  if(dataBytesLeft > 0)
  {
    bufferInit();
    buffer[0] = DB321_PAGE_READ;
    buffer[1] = (uint8_t)((dataPageAddress >> 6) & 0x7F);
    buffer[2] = (uint8_t)((dataPageAddress << 2) & 0xFC);
    buffer[3] = 0x00;
    dataBytes = dataBytesLeft;
    dataBytesLeft = 0;
    txLen = 8;
    rxLen = dataBytes;
    SPI_ReadWrite(buffer, txLen, rxLen, DataflashReadContinue);
  }
  else
  {
    DataflashOperationFinish();
  }
}

static void DataflashReadStart(uint32_t src, uint8_t* dst, uint8_t size)
{
  uint8_t dataBytesCount;
  uint32_t pageAddress;
  uint32_t byteAddress;
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
  dataDest = dst;
  dataBytes = dataBytesCount;
  dataPageAddress = pageAddress + 1;
  dataBytesLeft = size - dataBytesCount;
  txLen = 8;
  rxLen = dataBytesCount;
  SPI_ReadWrite(buffer, txLen, rxLen, DataflashReadContinue);
}

static void DataflashLoadBuffer(uint32_t dst, DataflashCommonCallback next)
{
  uint32_t pageAddress;
  pageAddress = dst / DB321_PAGE_SIZE;
  buffer[0] = DB321_PAGE_2_BUF1_TRF;
  buffer[1] = (uint8_t)((pageAddress >> 6) & 0x7F);
  buffer[2] = (uint8_t)((pageAddress << 2) & 0xFC);
  buffer[3] = 0x00;
  txLen = 4;
  rxLen = 0;
  SPI_ReadWrite(buffer, txLen, rxLen, next);
}

static void DataflashWriteContinue(void)
{
  debug(1);
  if(dataBytesLeft > 0)
  {
    if(dataflashLastPageSaved)
    {
      dataflashLastPageSaved = false;
      buffer[0] = DB321_PAGE_PGM_BUF1;
      buffer[1] = (uint8_t)((dataPageAddress >> 6) & 0x7F);
      buffer[2] = (uint8_t)((dataPageAddress << 2) & 0xFC);
      buffer[3] = 0x00;
      memcpy(&buffer[4], dataSource, dataBytesLeft);
      txLen = 4;
      rxLen = dataBytesLeft;
      dataBytesLeft = 0;
      SPI_ReadWrite(buffer, txLen, rxLen, DataflashWriteContinue);
    }
    else
    {
      _delay_ms(DB321_PAGE_ERASE_PGM_T_MS);
      DataflashLoadBuffer((dataPageAddress * DB321_PAGE_SIZE), DataflashWriteLastWaitFunction);
    }
  }
  else
  {
    _delay_ms(DB321_PAGE_ERASE_PGM_T_MS);
    DataflashOperationFinish();
  }
}

static void DataflashWriteStart(uint8_t* src, uint32_t dst, uint8_t size)
{
  uint8_t dataBytesCount;
  uint32_t pageAddress;
  uint32_t byteAddress;
  pageAddress = dst / DB321_PAGE_SIZE;
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
  dataPageAddress = pageAddress + 1;
  dataSource = src + dataBytesCount;
  dataBytesLeft = size - dataBytesCount;
  txLen = 4;
  rxLen = dataBytesCount;
  SPI_ReadWrite(buffer, txLen, rxLen, DataflashWriteContinue);
}

static void DataflashWriteLastWait(void)
{
  _delay_us(DB321_PAGE_2_BUF_TRF_T_US);
  dataflashLastPageSaved = true;
  DataflashWriteContinue();
}

static void DataflashWriteStartWait(void)
{
  _delay_us(DB321_PAGE_2_BUF_TRF_T_US);
  DataflashWriteStart(dataSource, dataflashDest, dataBytes);
}

/****************************************************************************
 * Public functions                                                         *
 ****************************************************************************/

bool DataflashInit(void)
{
  bufferInit();
  SPI_Init();
  DataflashCheckID();
  DataflashWriteLastWaitFunction = DataflashWriteLastWait;
  return dataflashInitialized;
}

bool DataflashRead(uint32_t src, uint8_t *dst, uint8_t size, DataflashCommonCallback callback)
{
  assert(dataflashInitialized);
  if(dataflashBusy)
  {
    return false;
  }
  dataflashBusy = true;
  if(callback)
  {
    DataflashOperationFinishCallback = callback;
  }
  else
  {
    DataflashOperationFinishCallback = DataflashDummyCallback;
  }
  DataflashReadStart(src, dst, size);
  return true;
}

bool DataflashWrite(uint8_t *src, uint32_t dst, uint8_t size, DataflashCommonCallback callback)
{
  assert(dataflashInitialized);
  if(dataflashBusy)
  {
    return false;
  }
  dataflashBusy = true;
  if(callback)
  {
    DataflashOperationFinishCallback = callback;
  }
  else
  {
    DataflashOperationFinishCallback = DataflashDummyCallback;
  }
  DataflashLoadBuffer(dst, DataflashWriteStartWait);
  return true;
}
