/*******************************************************************************
* file    record_if.h
* author  mackgim
* version 1.0.0
* date
* brief   record
*******************************************************************************/


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __RECORD_IF_H
#define __RECORD_IF_H

#ifdef __cplusplus
extern "C"
#endif

#include <stdint.h>



uint8_t recorder_init(void);
uint32_t recorder_stat(void);
uint8_t recorder_save(uint8_t index, void* buff, uint32_t size);
uint8_t recorder_read(uint8_t index, void* buff, uint32_t size);
uint8_t recorder_gc(void);

#endif /* __USERDRIVER_CONF_H*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
