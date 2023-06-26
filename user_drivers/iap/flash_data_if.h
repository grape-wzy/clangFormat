/*******************************************************************************
* file    flash_data_if.h
* author  mackgim
* version 1.0.0
* date
* brief   FLASH 存储读取操作
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __FLASH_DATA_IF_H
#define __FLASH_DATA_IF_H

#include "stdint.h"


#define FLASH_DATA_HEAD_TOKEN	(0x3b98f3a6)
#define FLASH_DATA_FREE_TOKEN	(0xffffffff)



typedef struct {
	uint32_t HeadCode;
	uint16_t DataSize;
	uint16_t Reseved;
}__FLASH_DATA_HEAD_TypeDef;

typedef struct {
	uint32_t  DataAddr;//最后一个数据的地址
	uint32_t  FreeAddr;//可用空间第一个地址
	uint32_t  DataItems;//数据条目
	uint32_t  Percentage; //已用空间百分比
}__FLASH_DATA_FIND_TOKEN_TypeDef;





uint8_t flash_data_write(uint32_t* buffer, uint32_t buffer_size);
uint8_t flash_data_read(uint32_t* buffer, uint32_t buffer_size);

uint8_t flash_data_erase(void);

#endif /*__EE_IF_H */

