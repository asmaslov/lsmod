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
static uint16_t txLen, rxLen;

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
static uint32_t dataBytes = 0;
static uint32_t dataBytesLeft = 0;
static uint32_t dataPageAddress = 0;

/****************************************************************************
 * Public types/enumerations/variables                                      *
 ****************************************************************************/

/****************************************************************************
 * Private functions                                                        *
 ****************************************************************************/

static void bufferInit(void)
{
  uint16_t i;

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
    if(dataBytesLeft >= DB321_PAGE_SIZE)
    {
      dataBytes = DB321_PAGE_SIZE;
      dataPageAddress += 1;
      dataBytesLeft -= DB321_PAGE_SIZE;
    }
    else
    {
      dataBytes = dataBytesLeft;
      dataBytesLeft = 0;
    }
    txLen = 8;
    rxLen = dataBytes;
    SPI_ReadWrite(buffer, txLen, rxLen, DataflashReadContinue);
  }
  else
  {
    DataflashOperationFinish();
  }
}

static void DataflashReadStart(uint32_t src, uint8_t* dst, uint32_t size)
{
  uint16_t dataBytesCount;
  uint16_t pageAddress;
  uint16_t byteAddress;
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
  uint16_t pageAddress;
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
  if(dataBytesLeft > 0)
  {
    if(dataBytesLeft >= DB321_PAGE_SIZE)
    {
      _delay_ms(DB321_PAGE_ERASE_PGM_T_MS);
      buffer[0] = DB321_PAGE_PGM_BUF1;
      buffer[1] = (uint8_t)((dataPageAddress >> 6) & 0x7F);
      buffer[2] = (uint8_t)((dataPageAddress << 2) & 0xFC);
      buffer[3] = 0x00;
      memcpy(&buffer[4], dataSource, DB321_PAGE_SIZE);
      txLen = 4;
      rxLen = DB321_PAGE_SIZE;
      dataPageAddress += 1;
      dataSource += DB321_PAGE_SIZE;
      dataBytesLeft -= DB321_PAGE_SIZE;
      SPI_ReadWrite(buffer, txLen, rxLen, DataflashWriteContinue);
    }
    else
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
  }
  else
  {
    _delay_ms(DB321_PAGE_ERASE_PGM_T_MS);
    DataflashOperationFinish();
  }
}

static void DataflashWriteStart(uint8_t* src, uint32_t dst, uint32_t size)
{
  uint16_t dataBytesCount;
  uint16_t pageAddress;
  uint16_t byteAddress;
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

static void DataflashEraseContinue(void)
{
  uint16_t i;
  if(dataBytesLeft > 0)
  {
    if(dataBytesLeft >= DB321_PAGE_SIZE)
    {
      if(dataflashEraseManual)
      {
        dataflashEraseManual = false;
        _delay_ms(DB321_PAGE_ERASE_PGM_T_MS);
      }
      else
      {
        _delay_ms(DB321_PAGE_ERASE_T_MS);
      }
      buffer[0] = DB321_PAGE_ERASE;
      buffer[1] = (uint8_t)((dataPageAddress >> 6) & 0x7F);
      buffer[2] = (uint8_t)((dataPageAddress << 2) & 0xFC);
      buffer[3] = 0x00;
      txLen = 4;
      rxLen = DB321_PAGE_SIZE;
      dataPageAddress += 1;
      dataBytesLeft -= DB321_PAGE_SIZE;
      SPI_ReadWrite(buffer, txLen, rxLen, DataflashEraseContinue);
    }
    else
    {
      if(dataflashLastPageSaved)
      {
        dataflashLastPageSaved = false;
        buffer[0] = DB321_PAGE_PGM_BUF1;
        buffer[1] = (uint8_t)((dataPageAddress >> 6) & 0x7F);
        buffer[2] = (uint8_t)((dataPageAddress << 2) & 0xFC);
        buffer[3] = 0x00;
        for(i = 0; i < dataBytesLeft; i++)
        {
          buffer[4 + i] = 0xFF;
        }
        txLen = 4;
        rxLen = dataBytesLeft;
        dataBytesLeft = 0;
        dataflashEraseManual = true;
        SPI_ReadWrite(buffer, txLen, rxLen, DataflashWriteContinue);
      }
      else
      {
        if(dataflashEraseManual)
        {
          dataflashEraseManual = false;
          _delay_ms(DB321_PAGE_ERASE_PGM_T_MS);
        }
        else
        {
          _delay_ms(DB321_PAGE_ERASE_T_MS);
        }
        DataflashLoadBuffer((dataPageAddress * DB321_PAGE_SIZE), DataflashEraseLastManualWaitFunction);
      }
    }
  }
  else
  {
    if(dataflashEraseManual)
    {
      dataflashEraseManual = false;
      _delay_ms(DB321_PAGE_ERASE_PGM_T_MS);
    }
    else
    {
      _delay_ms(DB321_PAGE_ERASE_T_MS);
    }
    DataflashOperationFinish();
  }
}

static void DataflashEraseStart(uint32_t dst, uint32_t size)
{
  uint16_t pageAddress;
  pageAddress = dst / DB321_PAGE_SIZE;
  buffer[0] = DB321_PAGE_ERASE;
  buffer[1] = (uint8_t)((pageAddress >> 6) & 0x7F);
  buffer[2] = (uint8_t)((pageAddress << 2) & 0xFC);
  buffer[3] = 0x00;
  txLen = 4;
  rxLen = 0;
  dataPageAddress = pageAddress + 1;
  dataBytesLeft = size - DB321_PAGE_SIZE;
  SPI_ReadWrite(buffer, txLen, rxLen, DataflashEraseContinue);
}

static void DataflashEraseStartManual(uint32_t dst, uint32_t size)
{
  uint16_t dataBytesCount;
  uint16_t pageAddress;
  uint16_t byteAddress;
  uint16_t i;
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
  for(i = 0; i < dataBytesCount; i++)
  {
    buffer[4 + i] = 0xFF;
  }
  dataPageAddress = pageAddress + 1;
  dataBytesLeft = size - dataBytesCount;  
  txLen = 4;
  rxLen = dataBytesCount;
  dataflashEraseManual = true;
  SPI_ReadWrite(buffer, txLen, rxLen, DataflashEraseContinue);
}

static void DataflashEraseLastManualWait(void)
{
  _delay_us(DB321_PAGE_2_BUF_TRF_T_US);
  dataflashLastPageSaved = true;
  DataflashEraseContinue();
}

static void DataflashEraseStartManualWait(void)
{
  _delay_us(DB321_PAGE_2_BUF_TRF_T_US);
  DataflashEraseStartManual(dataflashDest, dataBytes);
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
  DataflashEraseLastManualWaitFunction = DataflashEraseLastManualWait; 
  return dataflashInitialized;
}

bool DataflashErase(uint32_t dst, uint32_t size, DataflashCommonCallback callback)
{
  uint16_t byteAddress;
  assert(dataflashInitialized);
  assert((dst + size) <= DB321_SIZE);
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
  byteAddress = dst % DB321_PAGE_SIZE;
  if((byteAddress != 0) || ((size - byteAddress) < DB321_PAGE_SIZE))
  {
    dataflashDest = dst;
    dataBytes = size;
    DataflashLoadBuffer(dst, DataflashEraseStartManualWait);
  }
  else
  {
    DataflashEraseStart(dst, size);
  }
  return true;
}

bool DataflashRead(uint32_t src, uint8_t *dst, uint32_t size, DataflashCommonCallback callback)
{
  assert(dataflashInitialized);
  assert((src + size) <= DB321_SIZE);
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

bool DataflashWrite(uint8_t *src, uint32_t dst, uint32_t size, DataflashCommonCallback callback)
{
  uint16_t byteAddress;
  assert(dataflashInitialized);
  assert((dst + size) <= DB321_SIZE);
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
  byteAddress = dst % DB321_PAGE_SIZE;
  if((byteAddress != 0) || ((size - byteAddress) < DB321_PAGE_SIZE))
  {
    dataSource = src;
    dataflashDest = dst;
    dataBytes = size;
    DataflashLoadBuffer(dst, DataflashWriteStartWait);
  }
  else
  {
    DataflashWriteStart(src, dst, size);
  } 
  return true;
}
