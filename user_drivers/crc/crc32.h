/*******************************************************************************
* file    CRC32.c
* author  mackgim
* version 1.0.0
* date
* brief   CRC����
*         CRC32��CRC-32/MPEG2����
*         CRC32���밴byte��ת��byte-wise inversion, 0x1A2B3C4D becomes 0x58D43CB2
*         CRC32�����ת����0xffffffff����xor����
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

