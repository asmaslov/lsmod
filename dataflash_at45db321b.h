#ifndef __DATAFLASH_AT45DB321B_H_
#define __DATAFLASH_AT45DB321B_H_

#include "spi.h"

#include <inttypes.h>
#include <stdbool.h>

// Command Definition

// Read Commands
#define DB321_CONTINUOUS_ARRAY_READ  0xE8  // Continuous array read
#define DB321_PAGE_READ              0xD2  // Main memory page read
#define DB321_BUF1_READ              0xD4  // Buffer 1 read
#define DB321_BUF2_READ              0xD6  // Buffer 2 read
#define DB321_GET_STATUS             0xD7  // Get status register
#define DB321_GET_ID                 0x9F  // Get manufacturer and device ID

#define DB321_STATUS_READY           (1 << 7)
#define DB321_STATUS_COMPARE         (1 << 6)

// Program and Erase Commands
#define DB321_BUF1_WRITE             0x84  // Buffer 1 write
#define DB321_BUF2_WRITE             0x87  // Buffer 2 write
#define DB321_BUF1_PAGE_ERASE_PGM    0x83  // Buffer 1 to main memory page program with built-In erase
#define DB321_BUF2_PAGE_ERASE_PGM    0x86  // Buffer 2 to main memory page program with built-In erase
#define DB321_BUF1_PAGE_PGM          0x88  // Buffer 1 to main memory page program without built-In erase
#define DB321_BUF2_PAGE_PGM          0x89  // Buffer 2 to main memory page program without built-In erase
#define DB321_PAGE_ERASE             0x81  // Page Erase
#define DB321_BLOCK_ERASE            0x50  // Block Erase
#define DB321_PAGE_PGM_BUF1          0x82  // Main memory page through buffer 1
#define DB321_PAGE_PGM_BUF2          0x85  // Main memory page through buffer 2

// Additional Commands
#define DB321_PAGE_2_BUF1_TRF        0x53  // Main memory page to buffer 1 transfer
#define DB321_PAGE_2_BUF2_TRF        0x55  // Main memory page to buffer 2 transfer
#define DB321_PAGE_2_BUF1_CMP        0x60  // Main memory page to buffer 1 compare
#define DB321_PAGE_2_BUF2_CMP        0x61  // Main memory page to buffer 2 compare
#define DB321_AUTO_PAGE_PGM_BUF1     0x58  // Auto page rewrite through buffer 1
#define DB321_AUTO_PAGE_PGM_BUF2     0x59  // Auto page rewrite through buffer 2
 
// Timings

#define DB321_PAGE_ERASE_T_MS        10
#define DB321_BLOCK_ERASE_T_MS       12
#define DB321_PAGE_PGM_T_MS          14
#define DB321_PAGE_ERASE_PGM_T_MS    20
#define DB321_PAGE_2_BUF_TRF_T_US    250

// Size Definition
 
#define DB321_PAGE_SIZE              528
#define DB321_PAGE_NUM               8192 
#define DB321_PAGE_PER_BLOCK         8
#define DB321_BLOCK_SIZE             (DB321_PAGE_SIZE * DB321_PAGE_PER_BLOCK)  // 528 * 8 = 4224 bytes
#define DB321_BLOCK_PER_SECTOR       64
#define DB321_PAGE_PER_SECTOR        (DB321_PAGE_PER_BLOCK * DB321_BLOCK_PER_SECTOR)  // 8 * 64 = 512 pages
#define DB321_SECTOR_SIZE            (DB321_BLOCK_PER_SECTOR * DB321_BLOCK_SIZE)  // 64 * 4224 = 270336 bytes
#define DB321_SECTOR_0_SIZE           (DB321_PAGE_PER_BLOCK * DB321_PAGE_SIZE)  // 8 * 528 = 4224 bytes
#define DB321_SECTOR_1_SIZE          (DB321_BLOCK_PER_SECTOR * DB321_BLOCK_SIZE) // 64 * 4224 = 270336 bytes
#define DB321_SECTOR_NUMBER          17
#define DB321_SIZE                   (DB321_PAGE_NUM * DB321_PAGE_SIZE)  // 8192 * 528 = 4325376 bytes = 4224 Kbytes = 4 Mbytes + 128 Kbytes

#define DB321_BUFFER_SIZE            (8 + LSMOD_DATA_MAX_LEN / 2)

typedef void (*DataflashCommonCallback)(void);

bool DataflashInit(void);
bool DataflashRead(uint32_t src, uint8_t *dst, uint8_t size);
bool DataflashWrite(uint8_t *src, uint32_t dst, uint8_t size);

#endif // __DATAFLASH_AT45DB321B_H_
