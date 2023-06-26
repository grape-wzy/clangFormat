/******************************************************************************
 * file    
 * author  MCD Application Team
 * version V3.0.0
 * date    04/06/2009
 * brief   Status Monitoring and Fault Diagnosis
 ******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __CRC16_H
#define __CRC16_H

#include "stdint.h"

uint16_t crc16_ccitt(uint8_t* q, uint32_t len);
uint16_t crc16_for_band(uint8_t* q, uint32_t len);
#endif /*__CRC16_H */

