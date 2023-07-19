/*******************************************************************************
* file    large_event.h
* author  mackgim
* version 1.0.0
* date
* brief   ble 超过mtu长度的数据传输事件
*******************************************************************************/

#ifndef __LARGE_EVENT_H
#define __LARGE_EVENT_H

#include "stdint.h"
#include "gcompiler.h"

typedef enum {
	LARGE_EVENT_HEAD = 1,
	LARGE_EVNET_DATA = 2,
	LARGE_EVNET_TAIL = 3,
} __LARGE_EVENT_STEP_Typedef;


typedef __I__packed struct
{
	uint8_t  Cmd;
	uint8_t  Data[G_VARIABLE_SIZE];
}__I__PACKED __LARGE_EVENT_CMD_TypeDef;

typedef __I__packed struct {
	uint8_t  Index;
	uint8_t  Data[G_VARIABLE_SIZE];
}__I__PACKED __LARGE_EVENT_DATA_TypeDef;

typedef __I__packed struct {
	uint16_t  SizeOfBytes;
	uint16_t  MaxSizeOfPackage;
	uint16_t  NumOfPackage;
}__I__PACKED __LARGE_EVENT_HEAD_TypeDef;

typedef __I__packed struct {
	uint32_t  CheckCrc;
}__I__PACKED __LARGE_EVENT_TAIL_TypeDef;

typedef struct {
	uint8_t* InP;
	uint32_t InSize;
	uint8_t* OutP;
	uint32_t OutSize;
}__LARGE_EVENT_OBJECT_Typedef;


typedef struct {
	uint32_t  CheckCrc;
	uint16_t  SizeOfBytes;
	uint16_t  MaxSizeOfPackage;
	uint16_t  NumOfPackage;
	uint8_t   LastIndex;
	uint8_t	  Reserved;
	uint32_t  DataCount;
	uint8_t*  DataP;//数据指针
    uint32_t  DataSize; //数据大小
}__LARGE_EVENT_Typedef;

#define LARGE_EVNT_DEFAULT \
    {                      \
        0,                 \
            0,             \
            0,             \
            0,             \
            0,             \
            0,             \
            0,             \
            NULL,          \
            0              \
    }

uint8_t large_event_is_head(void* handle, uint8_t* buffer, uint8_t buffersize);
uint8_t large_event_set(void* handle, uint8_t* buffer, uint8_t buffersize);
uint8_t large_event_get(void* handle, uint8_t* buffer, uint8_t buffersize, uint8_t* outp, uint8_t* outsize);

#endif /* __LARGE_EVENT_H*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
