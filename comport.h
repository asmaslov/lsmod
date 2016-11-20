#ifndef __COMPORT_H__
#define __COMPORT_H__

#include "lsmod_config.h"
#include "lsmod_protocol.h"

#include <inttypes.h>
#include <stdbool.h>

#define BAUD LSMOD_BAUDRATE

#define RX_BUFFER_SIZE  (LSMOD_DATA_SRV_LEN + LSMOD_DATA_MAX_LEN)
#define TX_BUFFER_SIZE  (LSMOD_DATA_SRV_LEN + LSMOD_DATA_MAX_LEN)

typedef void (*ParserHandler)(void* args);

extern volatile bool ComportIsDataToParse, ComportNeedFeedback;

void ComportSetup(ParserHandler handler);

void ComportDebug(char ch);
void ComportDebugString(char *str);

void ComportParse(void);

void ComportReplyError(uint8_t cmd);
void ComportReplyAck(uint8_t cmd);
void ComportReplyLoaded(uint8_t bytes);
void ComportReplyStat(uint8_t axh, uint8_t axl, uint8_t ayh, uint8_t ayl, uint8_t azh, uint8_t azl);
void ComportReplyData(uint8_t* data, uint8_t len);

#endif // __COMPORT_H__
