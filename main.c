#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#include "lsmod_config.h"
#include "comport.h"
#include "dataflash_at45db321b.h"
#include "adc.h"
#ifdef MMA7455L_USED
  #include "accel_mma7455l.h"
#endif
#ifdef ADXL330_USED
  #include "accel_adxl330.h"
#endif
#include "player.h"
#include "ledrgb.h"

#include "debug.h"

bool activated = false;
bool updateColor = false;
bool hit = false;
uint32_t sensorTimeCount = 0;
bool sensorTimeReach = false;
bool clash = false;
bool swing = false;
bool loadTrackActive = false;
uint8_t loadTrackIdx = 0;
uint32_t loadTrackPos = 0;
uint8_t loadTrackLen = 0;
uint32_t deltaTrackPos = 0;
uint32_t nexTrackPos = 0;
int8_t lsmodLen = 0;
uint32_t trueColor = 0;
uint16_t* rawVoltage = NULL;
uint8_t vcount = 0;
uint32_t voltageAccum = 0;
bool voltageMeasured = false;
uint8_t voltage = 0;
uint8_t voltageLevel = VOLTAGE_MAX;
uint32_t voltageTimeCount = 0;

void initBoard(void)
{
  PORTB = (1 << PB0);
  DDRB = 0x00;
  PORTC = 0x00;
  DDRC = 0x00;
  PORTD = (1 << PD4) | (1 << PD7);
  DDRD = (1 << DDD4) | (1 << DDD7);
}

#define BUTTON_PRESSED  (~PINB & (1 << PINB0))
#define SENSOR_ACTIVE   (PIND & (1 << PIND5))

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
                         abs(Adxl330_AccelReal.z),
                         voltage);
      #endif
      #if (!defined(ADXL330_USED) & !defined(MMA7455L_USED))
        ComportReplyStat(0, 0, 0, 0, 0, 0);
      #endif
        break;
      case LSMOD_CONTROL_COLOR:
        LedrgbColor = packet->data[0];
        LedrgbColor = LedrgbColor << 8;
        LedrgbColor += packet->data[1];
        LedrgbColor = LedrgbColor << 8;
        LedrgbColor += packet->data[2];
        updateColor = true;
        LedrgbSaveColor();
        ComportReplyAck(LSMOD_CONTROL_COLOR);
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

uint32_t reduce(uint32_t val, uint8_t rdc)
{
  uint8_t bytes[3];
  uint32_t res;
  
  bytes[0] = (uint8_t)val;
  bytes[0] /= rdc;
  bytes[1] = (uint8_t)(val >> 8);
  bytes[1] /= rdc;
  bytes[2] = (uint8_t)(val >> 16);
  bytes[2] /= rdc;
  res = 0;
  res += bytes[2];
  res <<= 8;
  res += bytes[1];
  res <<= 8;
  res += bytes[0];
  return res;
}

void readyVoltage(void)
{
  float vlt;
  
  if (vcount < VOLTAGE_ACCUMUL)
  {
    vcount++;
    voltageAccum += *rawVoltage;
  }
  else
  {
    if (!voltageMeasured)
    {
      vlt = voltageAccum / VOLTAGE_ACCUMUL;
      vlt *= VOLTAGE_MUL;
      vlt /= ADC_MAX_VALUE;
      voltage = (uint8_t)ceil(vlt);
      voltageMeasured = true;
      vcount = 0;
      voltageAccum = 0;
    }      
  }
}

void adjustBrightness(void)
{
  bool rduce;
  uint8_t rdc;
  bool update;
 
  update = false;
  rduce = false;
  if ((voltage < VOLTAGE_MAX) && (voltage >= VOLTAGE_AVERAGE) && (voltageLevel > VOLTAGE_HIGH))
  {
    voltageLevel = VOLTAGE_HIGH;
    rdc = REDUCE_HIGH;
    rduce = true;
    update = true;
  }
  if ((voltage < VOLTAGE_HIGH) && (voltage >= VOLTAGE_LOW) && (voltageLevel > VOLTAGE_AVERAGE))
  {
    voltageLevel = VOLTAGE_AVERAGE;
    rdc = REDUCE_AVERAGE;
    rduce = true;
    update = true;
  }
  if ((voltage < VOLTAGE_AVERAGE) && (voltage >= VOLTAGE_CRITICAL) && (voltageLevel > VOLTAGE_LOW))
  {
    voltageLevel = VOLTAGE_LOW;
    rdc = REDUCE_LOW;
    rduce = true;
    update = true;
  }
  if ((voltage < VOLTAGE_CRITICAL) && (voltageLevel > VOLTAGE_CRITICAL))
  {
    voltageLevel = VOLTAGE_CRITICAL;
    trueColor = 0;
    rduce = false;
    update = true;
  }
  if (rduce)
  {
    trueColor = reduce(LedrgbColor, rdc);
  }
  if (update)
  {
    updateColor = true;
  }
}

int main(void)
{
  cli();
  initBoard();
  ComportSetup(commandHandler);
  PlayerInit();
  ADC_Init();
  sei();
  PlayerLoadMem();
  if (DataflashInit())
  {
    deblink(1);
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
  LedrgbInit();
  LedrgbLoadColor();
  trueColor = LedrgbColor;
  rawVoltage = ADC_ChannelSetup(VOLTAGE_CHAN, readyVoltage);
  while(1)
  {
    if (ComportIsDataToParse && !ComportNeedFeedback && !activated && !PlayerActive)
    {
      ComportParse();
    }
    voltageTimeCount++;
    if (voltageTimeCount > VOLTAGE_DELAY_TICKS)
    {
      voltageTimeCount = 0;
      if (voltageMeasured)
      {
        adjustBrightness();
        voltageMeasured = false;
      }    
    }
    if (BUTTON_PRESSED)
    {
      if (!activated)
      {
        led2(true);
        deltaTrackPos = PlayerTracksLen[TRACK_TURNON] / LEDRGB_TOTAL_LEN;
        nexTrackPos = deltaTrackPos;
        lsmodLen = 1;
        PlayerStart(TRACK_TURNON);
        while (PlayerActive)
        {
          while ((PlayerTrackPos < nexTrackPos) && PlayerActive);
          LedrgbSet(trueColor, lsmodLen);
          lsmodLen++;
          nexTrackPos += deltaTrackPos;
        }
        LedrgbOn(trueColor);
      #ifdef MMA7455L_USED
        Mma7455l_MotionDetected = false;
      #endif
      #ifdef ADXL330_USED
        Adxl330_HitDetected = false;
        Adxl330_MotionDetected = false;
      #endif
        activated = true;
      }
      else
      {
        activated = false;
        deltaTrackPos = PlayerTracksLen[TRACK_TURNOFF] / LEDRGB_TOTAL_LEN;
        nexTrackPos = deltaTrackPos;
        lsmodLen = LEDRGB_TOTAL_LEN;
        PlayerStart(TRACK_TURNOFF);
        while (PlayerActive)
        {
          while ((PlayerTrackPos < nexTrackPos) && PlayerActive);
          LedrgbSet(trueColor, lsmodLen);
          lsmodLen--;
          nexTrackPos += deltaTrackPos;
        }      
        LedrgbOff();
        led2(false);
      }
    }
    if (activated)
    {
      if (updateColor)
      {
        updateColor = false;
        LedrgbOn(trueColor);
      }
      if (hit && !PlayerActive)
      {
        hit = false;
      }
      if (swing && !PlayerActive)
      {
        swing = false;
      }
    #ifndef CLASH_DISABLE
      if (SENSOR_ACTIVE && !sensorTimeReach)
      {
        sensorTimeCount++;
        if (sensorTimeCount > SENSOR_DELAY_TICKS)
        {
          sensorTimeReach = true;
          sensorTimeCount = 0;
        }
      }
      if (!SENSOR_ACTIVE)
      {
        sensorTimeReach = false;
        sensorTimeCount = 0;
      }
      if (!hit)
      {
        if (sensorTimeReach && !clash)
        {
          clash = true;
          led1(true);
          PlayerStart(TRACK_CLASH);
        }
        if (clash && sensorTimeReach && !PlayerActive)
        {
          PlayerStart(TRACK_CLASH);
        }
        if (clash && !sensorTimeReach)
        {
          clash = false;
          led1(false);
          PlayerStop();        
        }
      }
    #endif
    #ifdef MMA7455L_USED
      if (Mma7455l_MotionDetected)
      {
        Mma7455l_MotionDetected = false;
        // TODO: Play music
      }
    #endif
    #ifdef ADXL330_USED
      if (Adxl330_HitDetected)
      {
        Adxl330_HitDetected = false;
        if (!hit)
        {
          hit = true;
          PlayerStart(TRACK_HIT);
        }
      }
      if (Adxl330_MotionDetected)
      {
        Adxl330_MotionDetected = false;
        if (!swing && !hit && !clash)
        {
          swing = true;
          PlayerStart(TRACK_SWING);
        }
      }
    #endif
      if (!PlayerActive && !hit && !clash)
      {
        PlayerStart(TRACK_HUM);
      }
    }
  }
  return 0;
}
