/*******************************************************************************
* file    user_global.h
* author  mackgim
* version 1.0.0
* date
* brief    全局变量；硬件所需的变量
*******************************************************************************/
#ifndef __USRE_GLOABL_H
#define __USRE_GLOABL_H

#ifdef __cplusplus
extern "C" {
#endif 

#include <stdint.h>
#include "cmsis_compiler.h"
#include "gcompiler.h"
#include "event_struct.h"
#include "com_struct.h"
#include "utils.h"
#include "gtimer.h"
#include "gpio_if.h"
#include "stm32_seq.h"
#include "app_conf.h"
#include "log.h"
	
#pragma region 全局故障

extern __MCU_CHECK_TypeDef sMcuCheck;
	/* Exported macros -----------------------------------------------------------*/
#define GLOBAL_FAULT_VALUE					(sMcuCheck.Error)
#define GLOBAL_FAULT_FLAG(flag)				(sMcuCheck.Error & flag)
#define GLOBAL_FAULT_FLAG_SET(flag)			(sMcuCheck.Error |= flag)
#define GLOBAL_FAULT_FLAG_CLEAR(flag)		(sMcuCheck.Error &= ~flag)
#define GLOBAL_FAULT_ALL_FLAG_CLEAR()		(sMcuCheck.Error = 0)

#define GLOBAL_FAULT_NO_KEY					((uint32_t)(0x01))			//传感器没有校验数据
#define GLOBAL_FAULT_IMU_INIT				((uint32_t)(0x02))			//传感器初始化失败
#define GLOBAL_FAULT_IMU_DATA_CHECK			((uint32_t)(0x04))			//传感器数据自检失败，连续10秒，即100帧数据出现这个故障提示
#define GLOBAL_FAULT_IMU_FULL				((uint32_t)(0x20000000))	//传感器缓存太多,不处理，记录
#define GLOBAL_FAULT_IMU_RESET				((uint32_t)(0x40000000))	//传感器失控，自己复位,不处理，记录
#define GLOBAL_FAULT_NO_DATA				((uint32_t)(0x80000000))	//最高位，表示接收器接收不到数据

#pragma endregion

#pragma region 设备码
#define DEVICE_TPYE_DEFAULT_CODE	(0x8020) //截骨设备
#pragma endregion

#pragma region  recorder flag	

#define RECORDER_PRODUCT_INDEX			(0) //产品信息
#define RECORDER_ALGO_INDEX				(1)	//算法信息 
#define RECORDER_ACCOUNT_INDEX			(2)	//账户信息
#define RECORDER_TIME_INDEX				(3)	//时间信息
#define RECORDER_BLE_MAC_INDEX			(4)	//蓝牙mac信息
#define RECORDER_SKT_CONFIG_INDEX		(5)	//截骨配置参数
	
#define RECORDER_ID_SET	  {\
			{ 0x1400, 0x8a30 },\
			{ 0x1400, 0x8a31 },\
			{ 0x1400, 0x8a32 },\
			{ 0x1400, 0x8a33 },\
			{ 0x1400, 0x8a34 },\
			{ 0x1400, 0x8a35 },\
			}

#pragma endregion	
	
#pragma region global variable

extern __FLASH_PRODUCT_DATA_TypeDef		gProductData;

extern __FLASH_SYSTEM_INFO_TypeDef		gSystemInfo;
#define   gAccountInfo					gSystemInfo.Account			

extern __FLASH_TIME_TypeDef				gFlashTime;

extern __FLASH_BLE_INFO_TypeDef			gFlashBleInfo;

extern __FLASH_ALGO_PARAMETER_TypeDef	gAlgoPara;

extern __FLASH_SKT_CONFIG_TypeDef		gSKTConfig;
#pragma endregion	

#pragma region 获取版本号

void get_version(uint8_t* version, uint8_t* other);

#pragma endregion

#pragma region 特定缓存,复位不会改变

/*(SRAM1_BASE) - (SRAM1_BASE+ 8) 缓存不跟随复位而更改*/

#define CFG_OTA_SRC_ADDR						(*(uint32_t*)(SRAM1_BASE+4))
#define CFG_OTA_DEST_ADDR						(*(uint32_t*)(SRAM1_BASE+8))

#define CFG_OTA_REBOOT_VAL_MSG						(*(uint8_t*)(SRAM1_BASE+0))
#define CFG_OTA_REBOOT_ON_CPU2_FW_CHECKED			(0x00)
#define CFG_OTA_REBOOT_ON_CPU2_START_FUS			(0x11)
#define CFG_OTA_REBOOT_ON_CPU2_DELETED_FW			(0x12)
#define CFG_OTA_REBOOT_ON_CPU2_WRITE_FW				(0x13)
#define CFG_OTA_REBOOT_ON_CPU2_UPGRADING			(0x14)
#define CFG_OTA_REBOOT_ON_CPU2_UPGRADED				(0x15)
#define CFG_OTA_REBOOT_ON_IDLE						(0xff)

void set_reboot_msg(uint8_t value);
uint8_t read_reboot_msg(void);
#pragma endregion


#ifdef __cplusplus
}
#endif

#endif /* __STM32_BLUENRG_BLE_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

