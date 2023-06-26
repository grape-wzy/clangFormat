/*******************************************************************************
* file    user_global.c
* author  mackgim
* version 1.0.0
* date
* brief   全局变量；硬件所需的变量
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/

#include "user_global.h" 
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "platform.h"
#include "ble_proc.h"
#include "skt_if.h"

#pragma region product information 

#define HARDWARE_VERSION_NUMBER				"SKTHWV1.0.0"
#define HARDWARE_VERSION_DATE				"20210420"
#define SOFTWARE_VERSION_NUMBER_HEAD		"SKTTXV1.3.6-"
#define SOFTWARE_VERSION_DATE				__DATE__ 

#ifdef DEBUG
#define SOFTWARE_VERSION_NUMBER		SOFTWARE_VERSION_NUMBER_HEAD "B"
#else
//#define SOFTWARE_VERSION_NUMBER		SOFTWARE_VERSION_NUMBER_HEAD "R"
#define SOFTWARE_VERSION_NUMBER		SOFTWARE_VERSION_NUMBER_HEAD "T"
#endif 

#define JSON_VERSION_FORMAT  "{\"Version\":\
{\"SN\":\"%s\",\
\"HVN\":\"%s\",\
\"HD\":\"%s\",\
\"SVN\":\"%s\",\
\"SD\":\"%s\",\
\"SHVN\":\"%s\",\
\"SSVN\":\"%s\",\
\"Other\":\"%s\"}\
}"

void get_compile_date_base(uint16_t* Year, uint16_t* Month, uint16_t* Day)
{
	const char* pMonth[] = { "Jan", "Feb", "M ar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	const char Date[12] = __DATE__;//取编译时间
	uint16_t i;
	for (i = 0; i < 12; i++)
	{
		if (memcmp(Date, pMonth[i], 3) == 0)
		{
			*Month = i + 1;
			break;
		}
	}
	*Year = (uint16_t)atoi(Date + 7); //Date[9]为２位年份，Date[7]为完整年份
	*Day = (uint8_t)atoi(Date + 4);
}

void get_compile_date(uint8_t* buff)
{
	uint16_t  Year = 0, Month = 0, Day = 0;
	get_compile_date_base(&Year, &Month, &Day);//取编译时间
	sprintf((char*)buff, "%04d%02d%02d", Year, Month, Day);//任意格式化
}


void get_version(uint8_t* version, uint8_t* other)
{
	//序列号
	uint8_t Serial[64];
	sprintf((char*)Serial,
		"%08x%08x%08x",
		(unsigned int)gProductData.Serial.Number[2],
		(unsigned int)gProductData.Serial.Number[1],
		(unsigned int)gProductData.Serial.Number[0]);


	//蓝牙版本
	uint8_t stack_hw[64];
	uint8_t stack_fw[64];
	uint8_t fus_fw[64];
	ble_get_string_version(&stack_hw[0], &stack_fw[0], &fus_fw[0]);


	//其他信息
	uint8_t rest[128];

	if (strlen((char*)other) == 0)
	{
		sprintf((char*)rest, "%s", fus_fw);
	}
	else
	{
		sprintf((char*)rest, "%s, %s", fus_fw, other);
	}


	uint8_t softdate[64];
	get_compile_date(softdate);
	sprintf((char*)version, JSON_VERSION_FORMAT,
		Serial,
		HARDWARE_VERSION_NUMBER,
		HARDWARE_VERSION_DATE,
		SOFTWARE_VERSION_NUMBER,
		softdate,
		stack_hw,
		stack_fw,
		rest
	);
}
#pragma endregion

#pragma region global variable

// 设备故障信息
__MCU_CHECK_TypeDef sMcuCheck  = { 0 };

//在flash中保存的时间信息
__FLASH_TIME_TypeDef gFlashTime __ALIGNED(8) = { 0 };

//注册信息
__FLASH_SYSTEM_INFO_TypeDef gSystemInfo __ALIGNED(8) = { 0 };

//在flash中保存的产品信息
__FLASH_PRODUCT_DATA_TypeDef gProductData __ALIGNED(8) = { 0 };

//蓝牙地址
__FLASH_BLE_INFO_TypeDef gFlashBleInfo __ALIGNED(8) = {
	{0xae, 0x33, 0x79, 0x34,
	0x08, 0xcf,(uint8_t)(DEVICE_TPYE_DEFAULT_CODE & 0xff), (uint8_t)((DEVICE_TPYE_DEFAULT_CODE >> 8) & 0xff),
	0x00, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x00} };

//算法参数
__FLASH_ALGO_PARAMETER_TypeDef gAlgoPara __ALIGNED(8) = { 0 };

//截骨配置
__FLASH_SKT_CONFIG_TypeDef gSKTConfig __ALIGNED(8) = {0};
#pragma endregion

#pragma region 读取指定缓存

void set_reboot_msg(uint8_t value)
{
	CFG_OTA_REBOOT_VAL_MSG = value;
}

uint8_t read_reboot_msg(void)
{
	return CFG_OTA_REBOOT_VAL_MSG;
}

#pragma endregion

/*******************************************************************************
END
*******************************************************************************/

