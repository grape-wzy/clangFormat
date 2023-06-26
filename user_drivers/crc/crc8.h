/******************************************************************************
 * file    
 * author  MCD Application Team
 * version V3.0.0
 * date    04/06/2009
 * brief   Status Monitoring and Fault Diagnosis
 ******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __CRC8_H
#define __CRC8_H

#include "stdint.h"

uint8_t crc8_calculate(uint8_t* q, uint32_t len);
uint8_t crc8_calculate2(uint8_t* q, uint32_t len);
#endif /*__CRC16_H */

