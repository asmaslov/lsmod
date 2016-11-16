#ifndef __LSMOD_PROTOCOL_H__
#define __LSMOD_PROTOCOL_H__

#define LSMOD_BAUDRATE  57600

#define LSMOD_PACKET_HDR  0xDA
#define LSMOD_PACKET_MSK  0xB0
#define LSMOD_PACKET_END  0xBA

#define LSMOD_DATA_MAX_LEN  128

#define LSMOD_CONTROL_PING  0x00
#define LSMOD_CONTROL_STAT  0x01
#define LSMOD_CONTROL_LOAD  0x02
#define LSMOD_CONTROL_READ  0x03

#define LSMOD_REPLY_ERROR  0x00
#define LSMOD_REPLY_ACK    0x01
#define LSMOD_REPLY_LOADED 0x02
#define LSMOD_REPLY_STAT   0x10
#define LSMOD_REPLY_DATA   0x11

typedef struct _LsmodPacket {
  unsigned char header;
  unsigned char to;
  unsigned char from;
  unsigned char cmd;
  unsigned char len;
  unsigned char data[LSMOD_DATA_MAX_LEN];
  unsigned char crc;
  unsigned char end;
} LsmodPacket;

#define LSMOD_PACKET_HEADER_INDEX  0
#define LSMOD_PACKET_TO_INDEX      1
#define LSMOD_PACKET_FROM_INDEX    2
#define LSMOD_PACKET_CMD_INDEX     3
#define LSMOD_PACKET_DATA_INDEX    4

#endif // __LSMOD_PROTOCOL_H__
