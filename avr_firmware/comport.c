#include "comport.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/setbaud.h>
#include <assert.h>
#include <stdlib.h>

/****************************************************************************
 * Private types/enumerations/variables                                     *
 ****************************************************************************/

static uint8_t tx_buffer[TX_BUFFER_SIZE];
static uint8_t tx_wr_index, tx_rd_index, tx_counter;
static uint8_t rx_buffer[RX_BUFFER_SIZE];
static uint8_t rx_wr_index, rx_rd_index, rx_counter;
static bool rx_buffer_overflow;

static LsmodPacket packet;
static ParserHandler parser_handler;
static bool packet_received, good_packet;
static uint8_t received_part_index;
static uint16_t received_data_index;

/****************************************************************************
 * Public types/enumerations/variables                                      *
 ****************************************************************************/

volatile bool ComportIsDataToParse, ComportNeedFeedback;

/****************************************************************************
 * Private functions                                                        *
 ****************************************************************************/

static void _putchar(uint8_t c)
{
  while (tx_counter == TX_BUFFER_SIZE);
  cli();
  if ((tx_counter != 0) || ((UCSR0A & (1 << UDRE0)) == 0))
  {
    tx_buffer[tx_wr_index++] = c;
    if (tx_wr_index == TX_BUFFER_SIZE)
    {
      tx_wr_index = 0;
    }
    ++tx_counter;
  }
  else
  {
    UDR0 = c;
  }
  sei();
}

static uint8_t _getchar(void)
{
  uint8_t data;
  while (rx_counter == 0);
  data = rx_buffer[rx_rd_index++];
  if (rx_rd_index == RX_BUFFER_SIZE)
  {
    rx_rd_index = 0;
  }
  cli();
  --rx_counter;
  sei();
  if (rx_counter == 0)
  {
    ComportIsDataToParse = false;
  }
  return data;
}

static void send()
{
  uint8_t i;
  uint8_t crc;
  
  crc = 0;
  _putchar(LSMOD_PACKET_HDR);
  crc += LSMOD_PACKET_HDR;
  _putchar(packet.to);
  crc += packet.to;
  _putchar(LSMOD_ADDR);
  crc += LSMOD_ADDR;
  _putchar(packet.cmd);
  crc += packet.cmd;
  for (i = 0; i < packet.len; ++i)
  {
    if ((LSMOD_PACKET_HDR == packet.data[i]) ||
        (LSMOD_PACKET_MSK == packet.data[i]) ||
        (LSMOD_PACKET_END == packet.data[i]))
    {     
      _putchar(LSMOD_PACKET_MSK);
      _putchar((uint8_t)(0xFF - packet.data[i]));
     }
    else
    {
      _putchar(packet.data[i]);
    }
    crc += packet.data[i];
  }
  if ((LSMOD_PACKET_HDR == crc) ||
      (LSMOD_PACKET_MSK == crc) ||
      (LSMOD_PACKET_END == crc))
  {     
    _putchar(LSMOD_PACKET_MSK);
    _putchar((uint8_t)(0xFF - crc));
  }
  else
  {
    _putchar(crc);
  }
  _putchar(LSMOD_PACKET_END);
}

/****************************************************************************
 * Interrupt handler functions                                              *
 ****************************************************************************/

ISR(USART_TX_vect)
{
  if (tx_counter)
  {
    --tx_counter;
    UDR0 = tx_buffer[tx_rd_index++];
    if (tx_rd_index == TX_BUFFER_SIZE)
    {
      tx_rd_index = 0;
    }
  }
}

ISR(USART_RX_vect)
{
  if ((UCSR0A & ((1 << FE0) | (1 << UPE0) | (1 << DOR0))) == 0)
  {
    rx_buffer[rx_wr_index++] = UDR0;
    if (rx_wr_index == RX_BUFFER_SIZE)
    {
      rx_wr_index = 0;
    }  
    if (++rx_counter == RX_BUFFER_SIZE)
    {
      rx_counter = 0;
      rx_buffer_overflow = true;
    }
    if (rx_counter > 0)
    {
      ComportIsDataToParse = true;
    }
    else
    {
      ComportIsDataToParse = false;
    }    
  }
}

/****************************************************************************
 * Public functions                                                         *
 ****************************************************************************/

void ComportSetup(ParserHandler handler)
{
  UBRR0H = UBRRH_VALUE;
  UBRR0L = UBRRL_VALUE;
#if USE_2X
  UCSR0A |= (1 << U2X0);
#else
  UCSR0A &= ~(1 << U2X0);
#endif
  UCSR0B = (1 << TXEN0) | (1 << RXEN0) | (1 << TXCIE0) | (1 << RXCIE0);
  UCSR0C = (1 << UCSZ00) | (1 << UCSZ01);
  parser_handler = handler;
  ComportIsDataToParse = false;
  packet_received = false;
  good_packet = false;
  ComportNeedFeedback = false;
  received_part_index = 0;
  received_data_index = 0;
}

void ComportDebug(char ch)
{
  _putchar(ch);
}

void ComportDebugString(char *str)
{
  while(*str != '\0')
  {
    _putchar(*str++);
  }
}

void ComportParse(void)
{
  uint8_t rec_byte, check_crc;
  uint16_t i, j;
  
  rec_byte = _getchar();
  if ((received_part_index == LSMOD_PACKET_DATA_INDEX) && (rec_byte == LSMOD_PACKET_END))
  {
    packet.end = rec_byte;
    packet.len = received_data_index;
    packet_received = true;
    received_part_index = LSMOD_PACKET_HEADER_INDEX;
  }
  if (received_part_index == LSMOD_PACKET_DATA_INDEX)
  {
    if (received_data_index < LSMOD_DATA_MAX_LEN)
    {
      packet.data[received_data_index++] = rec_byte;
    }
    else
    {
      received_part_index = 0;
    }
  }
  if (received_part_index == LSMOD_PACKET_CMD_INDEX)
  {
    packet.cmd = rec_byte;
    received_part_index = LSMOD_PACKET_DATA_INDEX;
  }
  if (received_part_index == LSMOD_PACKET_FROM_INDEX)
  {
    packet.from = rec_byte;
    received_part_index = LSMOD_PACKET_CMD_INDEX;
  }
  if (received_part_index == LSMOD_PACKET_TO_INDEX)
  {
    if (rec_byte == LSMOD_ADDR)
    {
      packet.to = rec_byte;
      received_part_index = LSMOD_PACKET_FROM_INDEX;
    }
    else
    {
      received_part_index = 0;
    }
  }
  if ((received_part_index == LSMOD_PACKET_HEADER_INDEX) && (rec_byte == LSMOD_PACKET_HDR) && (!packet_received))
  {
    packet.header = rec_byte;
    received_data_index = 0;
    received_part_index = LSMOD_PACKET_TO_INDEX;
  }
  if (packet_received)
  {
    packet_received = false;
    for (i = 0; i < packet.len; i++)
    {
      if (LSMOD_PACKET_MSK == packet.data[i])
      {
        assert(i < (packet.len - 1));
        packet.data[i] = 0xFF - packet.data[i + 1];
        for (j = (i + 1); j < (packet.len - 1); j++)
        {
          packet.data[j] = packet.data[j + 1];
        }
        packet.len--;
      }
    }
    packet.crc = packet.data[packet.len - 1];
    packet.len--;
    check_crc = 0;
    check_crc += packet.header;
    check_crc += packet.to;
    check_crc += packet.from;
    check_crc += packet.cmd;
    for (i = 0; i < packet.len; i++)
    {
      check_crc += packet.data[i];
    }
    if (check_crc == packet.crc)
    {
      good_packet = true;
    }
  }
  if (good_packet)
  {
    good_packet = false;
    ComportNeedFeedback = true;
    parser_handler(&packet);
  }
}

void ComportReplyError(uint8_t cmd)
{
  packet.to = packet.from;
  packet.cmd = LSMOD_REPLY_ERROR;
  packet.data[0] = cmd;
  packet.len = 1;
  send();
  ComportNeedFeedback = false;
}

void ComportReplyAck(uint8_t cmd)
{
  packet.to = packet.from;
  packet.cmd = LSMOD_REPLY_ACK;
  packet.data[0] = cmd;
  packet.len = 1;
  send();
  ComportNeedFeedback = false;
}

void ComportReplyLoaded(uint8_t bytes)
{
  packet.to = packet.from;
  packet.cmd = LSMOD_REPLY_LOADED;
  packet.data[0] = bytes;
  packet.len = 1;
  send();
  ComportNeedFeedback = false;
}

void ComportReplyStat(uint8_t axh, uint8_t axl, uint8_t ayh, uint8_t ayl, uint8_t azh, uint8_t azl, uint8_t vlt)
{
  packet.to = packet.from;
  packet.cmd = LSMOD_REPLY_STAT;
  packet.data[0] = axh;
  packet.data[1] = axl;
  packet.data[2] = ayh;
  packet.data[3] = ayl;
  packet.data[4] = azh;
  packet.data[5] = azl;
  packet.data[6] = vlt;
  packet.len = 7;
  send();
  ComportNeedFeedback = false;
}
