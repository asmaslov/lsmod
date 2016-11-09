#include "dataflash_at45db321b.h"
#include "spi.h"

#include <string.h>
#include <util/delay.h>
#include <assert.h>
#include <stdlib.h>

/****************************************************************************
 * Private types/enumerations/variables                                     *
 ****************************************************************************/

static uint8_t Tx_Buf[DB321_BUFFER_SIZE];
static uint8_t Rx_Buf[DB321_BUFFER_SIZE];
static uint16_t Tx_Len, Rx_Len;

static DataflashCommonCallback DataflashOperationFinishCallback = NULL;

static bool dataflashInitialized = false;
static bool dataflashBusy = false;
static bool dataflashLastPageSaved = false;
static bool dataflashEraseManual = false;
static DataflashCommonCallback DataflashWriteLastWaitFunction = NULL;
static DataflashCommonCallback DataflashEraseLastManualWaitFunction = NULL;

static uint32_t dataSource = 0;
static uint32_t dataDest = 0;
static uint32_t dataBytes = 0;
static uint32_t dataBytesLeft = 0;
static uint32_t dataPageAddress = 0;

#if defined(DB321_INCLUDE_FLASH_TEST)
  #define TEST_SIZE 131072  // 128 KB
  #define TEST_START_ADDRESS 0x3FFFFF  // Flash tail
  static volatile uint32_t dataflashTestAddressShift = 0;
  static volatile bool dataflashTestInProgress = false;
  static DataflashCommonCallback DataflashTestAgainFunction = NULL;
#endif

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
    Tx_Buf[i] = 0x00;
    Rx_Buf[i] = 0x00;
  }
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
  Tx_Buf[0] = DB321_GET_ID;
  Tx_Len = 1;
  Rx_Len = 4;
  SPI_ReadWrite(Tx_Buf, Rx_Buf, Tx_Len, Rx_Len, NULL);
  while (!SPI_TransferCompleted) {}   
  if((Rx_Buf[1] == 0x1F) &&
     ((Rx_Buf[2] & 0xE0) == 0x20) &&
     (Rx_Buf[3] == 1)  &&
     (Rx_Buf[4] == 0))
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
  memcpy((uint8_t *)dataDest, &Rx_Buf[8], dataBytes);
  dataDest += dataBytes;
  if(dataBytesLeft > 0)
  {
    bufferInit();
    Tx_Buf[0] = DB321_PAGE_READ;
    Tx_Buf[1] = (uint8_t)((dataPageAddress >> 6) & 0x7F);
    Tx_Buf[2] = (uint8_t)((dataPageAddress << 2) & 0xFC);
    Tx_Buf[3] = 0x00;
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
    Tx_Len = 8;
    Rx_Len = dataBytes;
    SPI_ReadWrite(Tx_Buf, Rx_Buf, Tx_Len, Rx_Len, DataflashReadContinue);
  }
  else
  {
    DataflashOperationFinish();
  }
}

static void DataflashReadStart(uint32_t src, uint32_t dst, uint32_t size)
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
  Tx_Buf[0] = DB321_PAGE_READ;
  Tx_Buf[1] = (uint8_t)((pageAddress >> 6) & 0x7F);
  Tx_Buf[2] = (uint8_t)((pageAddress << 2) & 0xFC) | (uint8_t)((byteAddress >> 8) & 0x03);
  Tx_Buf[3] = (uint8_t)(byteAddress & 0xFF);
  Tx_Buf[4] = 0xFF;
  Tx_Buf[5] = 0xFF;
  Tx_Buf[6] = 0xFF;
  Tx_Buf[7] = 0xFF;
  dataDest = dst;
  dataBytes = dataBytesCount;
  dataPageAddress = pageAddress + 1;
  dataBytesLeft = size - dataBytesCount;
  Tx_Len = 8;
  Rx_Len = dataBytesCount;
  SPI_ReadWrite(Tx_Buf, Rx_Buf, Tx_Len, Rx_Len, DataflashReadContinue);
}

static void DataflashLoadBuffer(uint32_t dst, DataflashCommonCallback next)
{
  uint16_t pageAddress;
  pageAddress = dst / DB321_PAGE_SIZE;
  Tx_Buf[0] = DB321_PAGE_2_BUF1_TRF;
  Tx_Buf[1] = (uint8_t)((pageAddress >> 6) & 0x7F);
  Tx_Buf[2] = (uint8_t)((pageAddress << 2) & 0xFC);
  Tx_Buf[3] = 0x00;
  Tx_Len = 4;
  Rx_Len = 0;
  SPI_ReadWrite(Tx_Buf, Rx_Buf, Tx_Len, Rx_Len, next);
}

static void DataflashWriteContinue(void)
{
  if(dataBytesLeft > 0)
  {
    if(dataBytesLeft >= DB321_PAGE_SIZE)
    {
      _delay_ms(DB321_PAGE_ERASE_PGM_T_MS);
      Tx_Buf[0] = DB321_PAGE_PGM_BUF1;
      Tx_Buf[1] = (uint8_t)((dataPageAddress >> 6) & 0x7F);
      Tx_Buf[2] = (uint8_t)((dataPageAddress << 2) & 0xFC);
      Tx_Buf[3] = 0x00;
      memcpy(&Tx_Buf[4], (uint8_t *)dataSource, DB321_PAGE_SIZE);
      Tx_Len = 4;
      Rx_Len = DB321_PAGE_SIZE;
      dataPageAddress += 1;
      dataSource += DB321_PAGE_SIZE;
      dataBytesLeft -= DB321_PAGE_SIZE;
      SPI_ReadWrite(Tx_Buf, Rx_Buf, Tx_Len, Rx_Len, DataflashWriteContinue);
    }
    else
    {
      if(dataflashLastPageSaved)
      {
        dataflashLastPageSaved = false;
        Tx_Buf[0] = DB321_PAGE_PGM_BUF1;
        Tx_Buf[1] = (uint8_t)((dataPageAddress >> 6) & 0x7F);
        Tx_Buf[2] = (uint8_t)((dataPageAddress << 2) & 0xFC);
        Tx_Buf[3] = 0x00;
        memcpy(&Tx_Buf[4], (uint8_t *)dataSource, dataBytesLeft);
        Tx_Len = 4;
        Rx_Len = dataBytesLeft;
        dataBytesLeft = 0;
        SPI_ReadWrite(Tx_Buf, Rx_Buf, Tx_Len, Rx_Len, DataflashWriteContinue);
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

static void DataflashWriteStart(uint32_t src, uint32_t dst, uint32_t size)
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
  Tx_Buf[0] = DB321_PAGE_PGM_BUF1;
  Tx_Buf[1] = (uint8_t)((pageAddress >> 6) & 0x7F);
  Tx_Buf[2] = (uint8_t)((pageAddress << 2) & 0xFC) | (uint8_t)((byteAddress >> 8) & 0x03);
  Tx_Buf[3] = (uint8_t)(byteAddress & 0xFF);
  memcpy(&Tx_Buf[4], (uint8_t *)src, dataBytesCount);  
  dataPageAddress = pageAddress + 1;
  dataSource = src + dataBytesCount;
  dataBytesLeft = size - dataBytesCount;
  Tx_Len = 4;
  Rx_Len = dataBytesCount;
  SPI_ReadWrite(Tx_Buf, Rx_Buf, Tx_Len, Rx_Len, DataflashWriteContinue);
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
  DataflashWriteStart(dataSource, dataDest, dataBytes);
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
      Tx_Buf[0] = DB321_PAGE_ERASE;
      Tx_Buf[1] = (uint8_t)((dataPageAddress >> 6) & 0x7F);
      Tx_Buf[2] = (uint8_t)((dataPageAddress << 2) & 0xFC);
      Tx_Buf[3] = 0x00;
      Tx_Len = 4;
      Rx_Len = DB321_PAGE_SIZE;
      dataPageAddress += 1;
      dataBytesLeft -= DB321_PAGE_SIZE;
      SPI_ReadWrite(Tx_Buf, Rx_Buf, Tx_Len, Rx_Len, DataflashEraseContinue);
    }
    else
    {
      if(dataflashLastPageSaved)
      {
        dataflashLastPageSaved = false;
        Tx_Buf[0] = DB321_PAGE_PGM_BUF1;
        Tx_Buf[1] = (uint8_t)((dataPageAddress >> 6) & 0x7F);
        Tx_Buf[2] = (uint8_t)((dataPageAddress << 2) & 0xFC);
        Tx_Buf[3] = 0x00;
        for(i = 0; i < dataBytesLeft; i++)
        {
          Tx_Buf[4 + i] = 0xFF;
        }
        Tx_Len = 4;
        Rx_Len = dataBytesLeft;
        dataBytesLeft = 0;
        dataflashEraseManual = true;
        SPI_ReadWrite(Tx_Buf, Rx_Buf, Tx_Len, Rx_Len, DataflashWriteContinue);
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
  Tx_Buf[0] = DB321_PAGE_ERASE;
  Tx_Buf[1] = (uint8_t)((pageAddress >> 6) & 0x7F);
  Tx_Buf[2] = (uint8_t)((pageAddress << 2) & 0xFC);
  Tx_Buf[3] = 0x00;
  Tx_Len = 4;
  Rx_Len = 0;
  dataPageAddress = pageAddress + 1;
  dataBytesLeft = size - DB321_PAGE_SIZE;
  SPI_ReadWrite(Tx_Buf, Rx_Buf, Tx_Len, Rx_Len, DataflashEraseContinue);
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
  Tx_Buf[0] = DB321_PAGE_PGM_BUF1;
  Tx_Buf[1] = (uint8_t)((pageAddress >> 6) & 0x7F);
  Tx_Buf[2] = (uint8_t)((pageAddress << 2) & 0xFC) | (uint8_t)((byteAddress >> 8) & 0x03);
  Tx_Buf[3] = (uint8_t)(byteAddress & 0xFF);
  for(i = 0; i < dataBytesCount; i++)
  {
    Tx_Buf[4 + i] = 0xFF;
  }
  dataPageAddress = pageAddress + 1;
  dataBytesLeft = size - dataBytesCount;  
  Tx_Len = 4;
  Rx_Len = dataBytesCount;
  dataflashEraseManual = true;
  SPI_ReadWrite(Tx_Buf, Rx_Buf, Tx_Len, Rx_Len, DataflashEraseContinue);
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
  DataflashEraseStartManual(dataDest, dataBytes);
}

/****************************************************************************
 * Public functions                                                         *
 ****************************************************************************/

void DataflashInit(void)
{
  bufferInit();
  SPI_Init();
  DataflashCheckID();
  DataflashWriteLastWaitFunction = DataflashWriteLastWait;
  DataflashEraseLastManualWaitFunction = DataflashEraseLastManualWait; 
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
    dataDest = dst;
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
  DataflashReadStart(src, (uint32_t)dst, size);
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
    dataSource = (uint32_t)src;
    dataDest = dst;
    dataBytes = size;
    DataflashLoadBuffer(dst, DataflashWriteStartWait);
  }
  else
  {
    DataflashWriteStart((uint32_t)src, dst, size);
  } 
  return true;
}
