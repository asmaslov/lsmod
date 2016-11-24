#ifndef __COMPORT_H__
#define __COMPORT_H__

#include "lsmod_config.h"
#include "lsmod_protocol.h"

#include <inttypes.h>
#include <stdbool.h>

#define BAUD LSMOD_BAUDRATE

#define RX_BUFFER_SIZE  (LSMOD_SRV_LEN + LSMOD_DATA_MAX_LEN)
#define TX_BUFFER_SIZE  (LSMOD_SRV_LEN + LSMOD_STAT_MAX_LEN)

#define COMPORT_TIMEOUT_MS  100

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

#endif // __COMPORT_H__
