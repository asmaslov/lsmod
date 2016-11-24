#include <avr/io.h>
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
#include "player.h"

#include "debug.h"

bool loadTrackActive = false;
uint8_t loadTrackIdx = 0;
uint32_t loadTrackPos = 0;
uint8_t loadTrackLen = 0;

void initBoard(void)
{
  PORTB = (1 << PB0);
  DDRB = 0x00;
  PORTC = 0x00;
  DDRC = 0x00;
  PORTD = (1 << PD4) | (1 << PD7);
  DDRD = (1 << DDD4) | (1 << DDD6) | (1 << DDD7);
}

void led1(bool on)
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

void led1Toggle(void)
{
  led1(PORTD & (1 << PD4));
}

void led2(bool on)
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

void led2Toggle(void)
{
  led2(PORTD & (1 << PD7));
}

void commandHandler(void* args)
{
  LsmodPacket* packet = (LsmodPacket*)args;
  if (packet->to == LSMOD_ADDR)
  {
    switch (packet->cmd) {
      case LSMOD_CONTROL_PING:
        ComportReplyAck(LSMOD_CONTROL_PING);
        break;
      case LSMOD_CONTROL_STAT:
      #ifdef ADXL330_USED
        ComportReplyStat((abs(Adxl330_AccelReal.x) >> 8) | ((Adxl330_AccelReal.x < 0) ? (1 << 7) : 0),
                         abs(Adxl330_AccelReal.x),
                         (abs(Adxl330_AccelReal.y) >> 8) | ((Adxl330_AccelReal.y < 0) ? (1 << 7) : 0),
                         abs(Adxl330_AccelReal.y),
                         (abs(Adxl330_AccelReal.z) >> 8) | ((Adxl330_AccelReal.z < 0) ? (1 << 7) : 0),
                         abs(Adxl330_AccelReal.z));
      #endif
      #if (!defined(ADXL330_USED) & !defined(MMA7455L_USED))
        ComportReplyStat(0, 0, 0, 0, 0, 0);
      #endif
        break;
      case LSMOD_CONTROL_LOAD_BEGIN:
        if (packet->data[0] == loadTrackIdx)
        {
          if (loadTrackIdx == 0)
          {
            PlayerTracksAddr[loadTrackIdx] = 0;
          }
          else
          {
            PlayerTracksAddr[loadTrackIdx] = PlayerTracksAddr[loadTrackIdx - 1] + PlayerTracksLen[loadTrackIdx - 1];
          }
          PlayerTracksLen[loadTrackIdx] = 0;
          loadTrackPos = 0;
          loadTrackActive = true;
          ComportReplyAck(LSMOD_CONTROL_LOAD_BEGIN);
        }
        else
        {
          ComportReplyError(LSMOD_CONTROL_LOAD_BEGIN);
        }
        break;
      case LSMOD_CONTROL_LOAD:
        if (loadTrackActive)
        {
          loadTrackPos = packet->data[0];
          loadTrackPos = loadTrackPos << 8;
          loadTrackPos += packet->data[1];
          loadTrackPos = loadTrackPos << 8;
          loadTrackPos += packet->data[2];
          loadTrackPos = loadTrackPos << 8;
          loadTrackPos += packet->data[3];
          loadTrackLen = packet->len - LSMOD_DATA_IDX_LEN;
          if (DataflashWrite(&packet->data[LSMOD_DATA_IDX_LEN], (PlayerTracksAddr[loadTrackIdx] + loadTrackPos), loadTrackLen))
          {
            led2Toggle();
            ComportReplyLoaded(loadTrackLen);
          }
          else
          {
            loadTrackActive = false;
            ComportReplyError(LSMOD_CONTROL_LOAD);
          }
        }
        else
        {
          ComportReplyError(LSMOD_CONTROL_LOAD);
        }
        break;
      case LSMOD_CONTROL_LOAD_END:
        if ((packet->data[0] == loadTrackIdx) && loadTrackActive)
        {
          PlayerTracksLen[loadTrackIdx] = loadTrackPos + loadTrackLen;
          loadTrackActive = false;
          ComportReplyAck(LSMOD_CONTROL_LOAD_END);
          loadTrackIdx++;
          if (loadTrackIdx == PLAYER_MAX_TRACKS)
          {
            loadTrackIdx = 0;
            _delay_ms(COMPORT_TIMEOUT_MS);
            //TODO: Bug is not in eeprom function, written values are wrong!
            PlayerSaveMem();
            led2(false);
          }
          else
          {
            led2(true);
          }
        }
        else
        {
          ComportReplyError(LSMOD_CONTROL_LOAD_END);
        }
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
  PlayerInit();
  sei();
  PlayerLoadMem();
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
    if ((~PINB & (1 << PINB0)) && !PlayerActive)
    {
      PlayerStart(0);
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
