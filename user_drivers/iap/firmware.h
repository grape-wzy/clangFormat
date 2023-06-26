/*******************************************************************************
* file    firmware.h
* author  mackgim
* version 1.0.0
* date
* brief   FLASH 存储读取操作
*******************************************************************************/


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __FIRMWARE_H
#define __FIRMWARE_H

#include <stdint.h>




//#define RECEIVER_FIRMWARE_TYPE					1
#define DEVICE_FIRMWARE_TYPE					2
//#define RECEIVER_MS_BLE_STACK_FIRMWARE_TYPE		3
//#define DEVICE_MS_BLE_STACK_FIRMWARE_TYPE		4
//#define RECEIVER_WB_BLE_STACK_FIRMWARE_TYPE		5
#define DEVICE_WB_BLE_STACK_FIRMWARE_TYPE		6


#define FIRMWARE_DATA_DATA_SIZE  ((uint32_t)(256)) //data成员占用字节数
#define FIRMWARE_DATA_INDEX_SIZE  ((uint32_t)(4))	//index成员占用字节数
/**********固件升级******
* Type定义：
* 1--接收板固件
* 2--设备固件
* 3、接收器蓝牙bluenrg_ms固件
* 4、设备蓝牙bluenrg_ms固件
* 5、接收器蓝牙STM32WB固件
* 6、设备蓝牙STM32WB固件
*/
typedef struct {
	uint32_t Index;
	uint8_t  Data[FIRMWARE_DATA_DATA_SIZE];
}__FIRMWARE_DATA_TypeDef;


typedef struct {
	uint32_t  Type;
	uint32_t  Size;
	uint32_t  NumOfPackage;
}__FIRMWARE_HEAD_TypeDef;

typedef struct {
	uint32_t	Type;
	uint32_t	CheckCode;
}__FIRMWARE_TAIL_TypeDef;

typedef struct {
	uint32_t	CfgLength;//以下数据长度, word的长度
	uint32_t	Cmd;//命令字
	uint32_t	Type;//固件类型
	uint32_t	FWAddr;//写入地址， 蓝牙协议栈需要指定写入地址
	uint32_t	FWVersion;//蓝牙协议栈版本
	uint32_t	Reserved;//备用
}__FIRMWARE_CONFIG_TypeDef;


uint8_t fw_is_ready(uint8_t* buffer, uint32_t buffersize);
uint8_t fw_set_config(uint8_t* buffer, uint32_t buffersize);
uint8_t fw_start(uint8_t* buffer, uint32_t buffersize);
uint8_t fw_stop(uint8_t* buffer, uint32_t buffersize);
uint8_t fw_proc(uint8_t* buffer, uint32_t buffersize);
uint8_t fw_set_config(uint8_t* buffer, uint32_t buffersize);
uint8_t fw_timeout(void);
uint8_t fw_get_ble_stack_addr(void);

#endif /* __LEDDRIVER_H*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
