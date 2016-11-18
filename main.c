#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

#include "lsmod_config.h"
#include "comport.h"
#include "dataflash_at45db321b.h"
#ifdef MMA7455L_USED
  #include "accel_mma7455l.h"
#endif
#ifdef ADXL330_USED
  #include "accel_adxl330.h"
#endif

#include "debug.h"

uint32_t EEMEM tracksAddr[MAX_TRACKS];
uint32_t EEMEM tracksLen[MAX_TRACKS];

static bool loadActivated = false;
static uint8_t trackIdx = 0;
static uint32_t trackPos = 0;
static uint8_t len = 0;

static void initBoard(void)
{
  PORTB = (1 << PB0);
  DDRB = 0x00;
  PORTC = 0x00;
  DDRC = 0x00;
  PORTD = (1 << PD4) | (1 << PD7);
  DDRD = (1 << DDD4) | (1 << DDD6) | (1 << DDD7);
}

static void led1(bool on)
{
  if (on == 0)
  {
    PORTD |= (1 << PD4);
  }
  else
  {
    PORTD &= ~(1 << PD4);
  }
}

static void led1Toggle(void)
{
  led1(PORTD & (1 << PD4));
}

static void led2(bool on)
{
  if (on == 0)
  {
    PORTD |= (1 << PD7);
  }
  else
  {
    PORTD &= ~(1 << PD7);
  }
}

static void led2Toggle(void)
{
  led2(PORTD & (1 << PD7));
}

static void replyLoaded(void)
{
  ComportReplyLoaded(len);
}

static void commandHandler(void* args)
{
  LsmodPacket* packet = (LsmodPacket*)args;
  if (packet->to == LSMOD_ADDR)
  {
    switch (packet->cmd) {
      case LSMOD_CONTROL_PING:
        ComportReplyAck(LSMOD_CONTROL_PING);
        break;
      case LSMOD_CONTROL_STAT:
        ComportReplyStat((abs(Adxl330_AccelReal.x) >> 8) | ((Adxl330_AccelReal.x < 0) ? (1 << 7) : 0),
                         abs(Adxl330_AccelReal.x),
                         (abs(Adxl330_AccelReal.y) >> 8) | ((Adxl330_AccelReal.y < 0) ? (1 << 7) : 0),
                         abs(Adxl330_AccelReal.y),
                         (abs(Adxl330_AccelReal.z) >> 8) | ((Adxl330_AccelReal.z < 0) ? (1 << 7) : 0),
                         abs(Adxl330_AccelReal.z));
        break;
      case LSMOD_CONTROL_LOAD_BEGIN:
        if (packet->data[0] == trackIdx)
        {
          eeprom_write_dword(tracksAddr[trackIdx], 0);
          eeprom_write_dword(tracksLen[trackIdx], 0);
          trackPos = 0;
          loadActivated = true;
          ComportReplyAck(LSMOD_CONTROL_LOAD_BEGIN);
        }
        else
        {
          ComportReplyError(LSMOD_CONTROL_LOAD_BEGIN);
        }
        break;
      case LSMOD_CONTROL_LOAD:
        if (loadActivated)
        {
          len = packet->len;
          //DataflashWrite(packet->data, (tracksAddr[trackIdx] + trackPos), packet->len, replyLoaded);
          _delay_ms(100);
          ComportReplyLoaded(packet->len);
          trackPos += packet->len;
          led1Toggle();
        }
        else
        {
          ComportReplyError(LSMOD_CONTROL_LOAD);
        }          
        break;
      case LSMOD_CONTROL_LOAD_END:
        if ((packet->data[0] == trackIdx) && loadActivated)
        {
          eeprom_write_dword(tracksLen[trackIdx], (trackPos + 1));
          loadActivated = false;
          ComportReplyAck(LSMOD_CONTROL_LOAD_END);
          trackIdx++;
          led1(false);
        }
        else
        {
          ComportReplyError(LSMOD_CONTROL_LOAD_END);
        }
        break;
      case LSMOD_CONTROL_READ:
        ComportReplyAck(LSMOD_CONTROL_READ);
        //TODO: ComportReplyData(...) Read and send wav from dataflash
        break;
      default:
        ComportReplyError(packet->cmd);
    }      
  }
  else
  {
    ComportReplyError(packet->cmd);
  }
}

int main(void)
{
  cli();
  wdt_disable();
  initBoard();
  ComportSetup(commandHandler);
  sei();
  if (DataflashInit())
  {
    deblink(3);
  }
#ifdef MMA7455L_USED
  if (Mma7455l_Init())
  {
    deblink(2);
  }
#endif
#ifdef ADXL330_USED
  Adxl330_Init();
#endif
#ifdef WATCHDOG_USED
  wdt_enable(WDTO_500MS);
#endif
  while(1)
  {
    if (~PINB & (1 << PINB0))
    {
      // TODO: Do something
    }
    if (ComportIsDataToParse & !ComportNeedFeedback)
    {
      ComportParse();
    }
  #ifdef MMA7455L_USED
    if (Mma7455l_MotionDetected)
    {
      Mma7455l_MotionDetected = false;
      // TODO: Play music
    }
  #endif
  #ifdef ADXL330_USED
    if (Adxl330_MotionDetected)
    {
      Adxl330_MotionDetected = false;
      //led2Toggle();
      // TODO: Play music
    }
    if (Adxl330_HitDetected)
    {
      Adxl330_HitDetected = false;
      //led1Toggle();
      // TODO: Play music
    }
  #endif    
  #ifdef WATCHDOG_USED
    wdt_reset();
  #endif
  }
  return 0;
}
