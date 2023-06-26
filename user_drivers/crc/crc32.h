/*******************************************************************************
* file    CRC32.c
* author  mackgim
* version 1.0.0
* date
* brief   CRC驱动
*         CRC32和CRC-32/MPEG2区别
*         CRC32输入按byte反转，byte-wise inversion, 0x1A2B3C4D becomes 0x58D43CB2
*         CRC32输出反转，与0xffffffff进行xor运算
*
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __CRC32_H
#define __CRC32_H

#include "stdint.h"
#include "stdbool.h"

uint8_t crc32_init(void);
uint8_t crc32_deinit(void);
uint32_t crc32_calculate (uint32_t pBuffer[], uint32_t BufferLength);
uint32_t crc32_accumulate(uint32_t pBuffer[], uint32_t BufferLength, bool reset);

#endif /*__CRC32_H */

