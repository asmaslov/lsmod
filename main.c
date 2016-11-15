#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <inttypes.h>
#include <stdbool.h>

#include "lsmod_config.h"
#include "comport.h"
#include "dataflash_at45db321b.h"
#include "accel_mma7455l.h"
#include "accel_adxl330.h"

#include "debug.h"

static void initBoard(void)
{
  PORTB = (1 << PB0);
  DDRB = 0x00;
  PORTC = 0x00;
  DDRC = 0x00;
  PORTD = (1 << PD4) | (1 << PD7);
  DDRD = (1 << DDD4) | (1 << DDD6) | (1 << DDD7);
  /*PCICR = (1 << PCIE0);
  PCMSK0 = (1 << PCINT0);*/
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
        ComportReplyAck(LSMOD_CONTROL_STAT);
        //TODO: ComportReplyStat(...);
        break;
      case LSMOD_CONTROL_LOAD:
        ComportReplyAck(LSMOD_CONTROL_LOAD);
        //TODO: Load wav into dataflash
        ComportReplyLoaded();
        break;
      case LSMOD_CONTROL_READ:
        ComportReplyAck(LSMOD_CONTROL_READ);
        //TODO: ComportReplyData(...);
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

ISR(PCINT0_vect)
{
  if (~PINB & (1 << PINB0))
  {
    led2Toggle();
    _delay_ms(200);
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
  //Mma7455l_Init()
  //Adxl330_Init();
  
  //wdt_enable(WDTO_500MS);
  while(1)
  {
    if (~PINB & (1 << PINB0))
    {
      led1Toggle();
      _delay_ms(200);
    }
    if (PIND & (1 << PIND5))
    {
      led2(true);
    }
    else
    {
      led2(false);
    } 
    /*if (ComportIsDataToParse & !ComportNeedFeedback)
    {
      ComportParse();
    }
    if (Mma7455l_MotionDetected)
    {
      Mma7455l_MotionDetected = false;
      // TODO: Play music
    }
    if (Adxl330_MotionDetected)
    {
      Adxl330_MotionDetected = false;
      // TODO: Play music
    */
    //wdt_reset();
  }
  return 0;
}
