#ifndef __LSMOD_PROTOCOL_H__
#define __LSMOD_PROTOCOL_H__

#define LSMOD_BAUDRATE  115200

#define LSMOD_PACKET_HDR  0xDA
#define LSMOD_PACKET_MSK  0xB0
#define LSMOD_PACKET_END  0xBA

#define LSMOD_SRV_LEN         6
#define LSMOD_DATA_IDX_LEN    4
#define LSMOD_DATA_MAX_LEN  262
#define LSMOD_STAT_MAX_LEN    6

#define LSMOD_CONTROL_PING        0x00
#define LSMOD_CONTROL_STAT        0x01
#define LSMOD_CONTROL_COLOR       0x02
#define LSMOD_CONTROL_LOAD_BEGIN  0x10
#define LSMOD_CONTROL_LOAD        0x11
#define LSMOD_CONTROL_LOAD_END    0x12

#define LSMOD_REPLY_ERROR   0x00
#define LSMOD_REPLY_ACK     0x01
#define LSMOD_REPLY_LOADED  0x02
#define LSMOD_REPLY_STAT    0x03

typedef struct {
  unsigned char header;
  unsigned char to;
  unsigned char from;
  unsigned char cmd;
  unsigned char len;  // This byte is not transmitted
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
