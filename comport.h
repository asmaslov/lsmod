#ifndef __COMPORT_H__
#define __COMPORT_H__

#include "lsmod_config.h"
#include "lsmod_protocol.h"

#include <inttypes.h>
#include <stdbool.h>

#define BAUD LSMOD_BAUDRATE

#define RX_BUFFER_SIZE  16
#define TX_BUFFER_SIZE  16

typedef void (*ParserHandler)(void* args);

extern volatile bool ComportIsDataToParse, ComportNeedFeedback;

void ComportSetup(ParserHandler handler);

void ComportDebug(char ch);
void ComportDebugString(char *str);

void ComportParse(void);

void ComportReplyError(uint8_t cmd);
void ComportReplyAck(uint8_t cmd);
void ComportReplyLoaded(void);
void ComportReplyStat(uint8_t stat);
void ComportReplyData(uint8_t* data, uint16_t len);

#endif // __COMPORT_H__
