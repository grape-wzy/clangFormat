 /*******************************************************************************
  * file     IAP.h
  * author   mackgim
  * version  V1.0.0
  * date     
  * brief £º 
  *******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __IAP_H
#define __IAP_H

#include "stdint.h"

void iap_init(void);
void iap_check(void);
uint8_t iap_write_ble_image(void);
#endif /* __IAP_H*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
