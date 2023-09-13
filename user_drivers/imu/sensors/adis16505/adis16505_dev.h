 /*******************************************************************************
  * file     forcesensor.h
  * author   mackgim
  * version  V1.0.0
  * date     
  * brief ： 
  *******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __ADIS16505_DEV_H
#define __ADIS16505_DEV_H


#include "stdint.h"



uint8_t adis16505_dev_init(void *handle);
uint8_t adis16505_dev_deinit(void *handle);

uint8_t adis16505_dev_restore_config(void* handle);

uint8_t adis16505_dev_read_raw(void* handle, uint8_t* value, uint16_t size);

uint8_t adis16505_dev_read_all_reg(void* handle);
#endif /*__ADIS16505_DEV_H */

