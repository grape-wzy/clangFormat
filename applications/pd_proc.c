/*******************************************************************************
* file    pd_proc.c
* author  mackgim
* version 1.0.0
* date
* brief   产品信息
*******************************************************************************/
#include "pd_proc.h"
#include "platform.h"
#include "user_drivers.h"
#include "pm_proc.h"

#define ENABLE_PD_DEUBG 1

#ifdef DEBUG
#if ENABLE_PD_DEUBG
#define pdkpt(...) kprint(__VA_ARGS__)
#define pdnpt(...) nprint(__VA_ARGS__)
#else
#define pdkpt(...)
#define pdnpt(...)
#endif
#else
#define pdkpt(...)
#define pdnpt(...)
#endif


#pragma region 变量
//设备固定码
static const uint32_t gFixedCode[] = { 386178621 , 1426178053 , 3384928110 , 2746499889 , 1364761006, 2892065737 };
#pragma endregion

#pragma region 函数

uint8_t pd_get_mcu_uid(uint32_t* buff);
uint8_t pd_generate_serial(uint32_t* buff);
uint8_t pd_generate_ble_uid(uint8_t* mac);

#pragma endregion

#pragma region 功能

// 初始化产品信息
uint8_t pd_init(void)
{
	recorder_init();
	
#ifdef DEBUG
	uint32_t sn[3];
	pd_get_mcu_uid(sn);
	pdkpt("mcu id= %08lX%08lX%08lX\r\n", sn[2], sn[1], sn[0]);
#endif
#if 1

	//时间信息
	{
		uint8_t ret = pd_read_time();
		if (ret != STD_SUCCESS)
		{
			//清除数据
			memset((uint8_t*)&gFlashTime, 0, sizeof(gFlashTime));
			pdkpt("have no time data\r\n");
		}
		else
		{
			pdkpt("have time data\r\n");
		}
		gFlashTime.EverBootTimes++;
		gFlashTime.BootTimes++;
		sys_set_base_tick(gFlashTime.RunTime, gFlashTime.WorkTime); //同步起始时间
		
		pdkpt("save time data\r\n");
		pd_save_time();
	}

	//产品信息
	{
		uint8_t ret = pd_read_product_info();
		//如果flash中没有序列号，使用默认铭牌，并生成序列号
		if (ret != STD_SUCCESS)
		{
			//申请序列号,如果生成失败，不保存入flash中，重启后再重新生成
			if (pd_generate_serial((uint32_t*)&(gProductData.Serial)) == STD_SUCCESS)
			{
				pd_save_product_info();
				pdkpt("save product data\r\n");
			}
			else
			{
				pdkpt("failed to generate serial, and reset\r\n");
				pwr_reset_mcu();
			}
		}
		else
		{
			pdkpt("have a product data\r\n");
		}
	}

	//注册信息
	{
		uint8_t ret = pd_read_account_info();
		if (ret != STD_SUCCESS)
		{
			//初次上电默认系统状态为null,暂时没用
			gAccountInfo.SystemStatus = SYSTEM_STATUS_NULL;
			pdkpt("save account data\r\n");
			pd_save_account_info();
		}
		else
		{
			pdkpt("have a account data\r\n");
		}
	}

	//蓝牙地址
	{
		uint8_t ret = pd_read_ble_mac();
		if ((ret != STD_SUCCESS))
		{
			//当flash中没有mac，即刻生成新的mac，进行保存
			if (pd_generate_and_save_ble_mac() != STD_SUCCESS)
			{
				pdkpt("failed to update ble mac, and reset\r\n");
				pwr_reset_mcu();
			}
		}
		else
		{
			pdkpt("have a mac data\r\n");
		}
	}

	//算法参数配置
	{
		uint8_t ret = pd_read_algo_para();
		if ((ret != STD_SUCCESS))
		{
			pdkpt("have no algo data\r\n");
		}
		else
		{
			pdkpt("have a algo data\r\n");
		}
		//pdkpt("algo data=0x%x\r\n", *(unsigned int *)0x80530d0);
	}
#endif

#ifdef DEBUG
	pdkpt("[time]: boot=%u, run=%lu, work=%lu\r\n", (unsigned int)gFlashTime.BootTimes, (long unsigned int)gFlashTime.RunTime, (long unsigned int)gFlashTime.WorkTime);
	pdkpt("[account]: sys=%u, login=%u, login num=%u\r\n", gAccountInfo.SystemStatus, gAccountInfo.IsLogined, gAccountInfo.LoginNumber);
	pdnpt("[Serial]: ");
	for (uint32_t i = 0; i < sizeof(gProductData.Serial) / 4; i++)
	{
		pdnpt("%08x", (unsigned int)gProductData.Serial.Number[i]);
	}
	pdnpt("\r\n");
	pdkpt("ok,init\r\n\r\n");
#endif

	return STD_SUCCESS;
}

//获取最大用户空间
uint32_t pd_get_user_end_space(void)
{
	pdkpt("user end space=0x%x\r\n", (unsigned int)(FLASH_SPACE_END_ADDR - ble_get_stack_size()));
	return  (FLASH_SPACE_END_ADDR - ble_get_stack_size());
	//return gFlashBleInfo.StackAddr;
	//return FLASH_SPACE_BLE_STACK_BEGIN_ADDR;
}

#pragma endregion

#pragma region 账户信息配置

//保存账户信息
uint8_t pd_save_account_info(void)
{
	pdkpt("logined=%u,number=%u,SystemStatus=%u\r\n", gAccountInfo.IsLogined, gAccountInfo.LoginNumber, gAccountInfo.SystemStatus);
	return recorder_save(RECORDER_ACCOUNT_INDEX, (uint8_t*)&gSystemInfo, sizeof(gSystemInfo));
}

//读取账户信息
uint8_t pd_read_account_info(void)
{
	return recorder_read(RECORDER_ACCOUNT_INDEX, (uint8_t*)&gSystemInfo, sizeof(gSystemInfo));
}

// 保存注册信息操作，为account info的子项
uint8_t pd_save_register_info(uint8_t cmd)
{

	// 0 移除注册, 1 注册 2 清除注册信息 3写入关机信息  4清除关机信息
	switch (cmd)
	{
	case 0: //解绑
	{
		gAccountInfo.IsLogined = 0;
	}
	break;
	case 1: //绑定
	{
		gAccountInfo.IsLogined = 1;
		gAccountInfo.LoginNumber++;
	}
	break;
	case 2: //清除注册消息
	{
		gAccountInfo.IsLogined = 0;
		gAccountInfo.LoginNumber = 0;
	}
	break;
	default:
	{
		return STD_FAILED;
	}
	}

	return pd_save_account_info();
}

//保存系统状态, 为account info的子项
uint8_t pd_save_system_status(uint8_t status)
{
	gAccountInfo.SystemStatus = status;
	return pd_save_account_info();
}

#pragma endregion

#pragma region 时间信息配置

//读取系统运行时间
uint8_t pd_read_time(void)
{
	uint8_t ret = recorder_read(RECORDER_TIME_INDEX, (uint8_t*)&gFlashTime, sizeof(gFlashTime));
	pdkpt("run=%lu, work=%lu, boot=%u, everboot=%u\r\n", (long unsigned int)gFlashTime.RunTime, (long unsigned int)gFlashTime.WorkTime, (unsigned int)gFlashTime.BootTimes, (unsigned int)gFlashTime.EverBootTimes);
	return ret;
}

//记录系统运行时间
uint8_t pd_save_time(void)
{
	/* 时间记录逻辑-单位为内部tick
	 * 1 - 即将复位的时候
	 * 2 - 即将关机的时候
	 * 3 - 定时保存
	 **/

	__SYSTEM_TIME64_TypeDef t;
	sys_get_tick(&t);
	gFlashTime.RunTime = t.RunTime;
	gFlashTime.WorkTime = t.WorkTime;
	pdkpt("run=%lu, work=%lu, boot=%u, everboot=%u, bor=%u\r\n",
		(long unsigned int)sys_tick_to_ms(gFlashTime.RunTime) / 1000,
		(long unsigned int)sys_tick_to_ms(gFlashTime.WorkTime) / 1000,
		(unsigned int)gFlashTime.BootTimes, (unsigned int)gFlashTime.EverBootTimes,
		(unsigned int)gFlashTime.BorTimes);
	return recorder_save(RECORDER_TIME_INDEX, (uint8_t*)&gFlashTime, sizeof(gFlashTime));
}

//清除系统运行时间
uint8_t pd_clear_time(void)
{
	gFlashTime.BootTimes = 0;
	gFlashTime.RunTime = 0;
	gFlashTime.WorkTime = 0;
	gFlashTime.BorTimes = 0;
	gFlashTime.Reserved = 0;
	sys_clear_time();
	return recorder_save(RECORDER_TIME_INDEX, (uint8_t*)&gFlashTime, sizeof(gFlashTime));
}

#pragma endregion

#pragma region 产品信息配置

//保存产品信息
uint8_t pd_save_product_info(void)
{
	return recorder_save(RECORDER_PRODUCT_INDEX, (uint8_t*)&gProductData, sizeof(gProductData));
}

//读取产品信息
uint8_t pd_read_product_info(void)
{
	return recorder_read(RECORDER_PRODUCT_INDEX, (uint8_t*)&gProductData, sizeof(gProductData));
}

#pragma endregion

#pragma region 蓝牙地址配置

//保存蓝牙地址
uint8_t pd_save_ble_mac(void)
{
	return recorder_save(RECORDER_BLE_MAC_INDEX, (uint8_t*)&gFlashBleInfo, sizeof(gFlashBleInfo));
}

//生成并保存蓝牙地址
uint8_t pd_generate_and_save_ble_mac(void)
{

	if (pd_generate_ble_uid((uint8_t*)&gFlashBleInfo.Mac[0]) != STD_SUCCESS)
	{
		pdkpt("failed to generate uid\r\n");
		return STD_FAILED;
	}

#if DEBUG1
	uint8_t* mac_ptr = (uint8_t*)&gFlashBleInfo;
	pdnpt("[blue mac]:");
	for (uint8_t i = 0; i < 16; i++)
	{
		pdnpt("%02x", mac_ptr[i]);
	}
	pdnpt("\r\n");
#endif

	pdkpt("generate ble mac\r\n");

	return pd_save_ble_mac();
}

//读取蓝牙地址
uint8_t pd_read_ble_mac(void)
{
	uint8_t ret = recorder_read(RECORDER_BLE_MAC_INDEX, (uint8_t*)&gFlashBleInfo, sizeof(gFlashBleInfo));

#if DEBUG1
	uint8_t* mac_ptr = (uint8_t*)&gFlashBleInfo;
	pdnpt("[read mac]:");
	for (uint8_t i = 0; i < 16; i++)
	{
		pdnpt("%02x", mac_ptr[i]);
	}
	pdnpt("\r\n");
#endif

	return ret;
}

#pragma endregion

#pragma region 算法配置

//gAlgoPara是临时缓存,直接读写操作，保存调用此函数
//把算法参数保存到flash中,
uint8_t pd_save_algo_para(void)
{
	return recorder_save(RECORDER_ALGO_INDEX, (uint8_t*)&gAlgoPara, sizeof(gAlgoPara));
}

//gAlgoPara是临时缓存,并且过大，无法一次设置，需要分段设置，出现非完整帧的情况
//gAlgoPara非完全可信，为确保安全，需从flash中再次更新到缓存中
//从flash中读取算法参数到缓存中，再进行操作
uint8_t pd_read_algo_para(void)
{
	return recorder_read(RECORDER_ALGO_INDEX, (uint8_t*)&gAlgoPara, sizeof(gAlgoPara));
}

#pragma endregion

#pragma region 截骨配置

//设置截骨配置参数
uint8_t pd_save_skt_config(void)
{
	return recorder_save(RECORDER_SKT_CONFIG_INDEX, (uint8_t*)&gSKTConfig, sizeof(gSKTConfig));
}

//读取截骨配置参数
uint8_t pd_read_skt_config(void)
{
	return recorder_read(RECORDER_SKT_CONFIG_INDEX, (uint8_t*)&gSKTConfig, sizeof(gSKTConfig));
}

#pragma endregion

#pragma region 序列号和设备码

// 生成完整序列号
uint8_t pd_generate_serial(uint32_t* buff)
{

	uint32_t serial[SERIAL_NUMBER_LENGTH] = { 0 };
	uint8_t index = pd_get_mcu_uid(&serial[0]);
	if (index >= SERIAL_NUMBER_LENGTH)
	{
		memcpy((uint8_t*)buff, (uint8_t*)&serial[0], sizeof(serial));
		return STD_SUCCESS;
	}
	uint8_t ret = rng_get_numbers((uint32_t*)&serial[index], (SERIAL_NUMBER_LENGTH - index));
	if (ret != STD_SUCCESS)
	{
		pdkpt("failed to get rand\r\n");
		return ret;
	}

	memcpy((uint8_t*)buff, (uint8_t*)&serial[0], sizeof(serial));
	return STD_SUCCESS;
}

//生成蓝牙 mac
uint8_t pd_generate_ble_uid(uint8_t* mac)
{
	uint8_t hash[16];
	uint8_t temp[256] __ALIGNED(8);
	uint16_t tempsize = 0;
	//ble mac 由序列号、固定密钥、随机长度的随机数据，计算hash产生
	//pdkpt("tempsize=%u\r\n", tempsize);

	memcpy(&temp[tempsize], &gProductData.Serial, sizeof(gProductData.Serial));
	tempsize += sizeof(gProductData.Serial);
	//pdkpt("tempsize=%u\r\n", tempsize);
	memcpy(&temp[tempsize], &gFixedCode[0], sizeof(gFixedCode));
	tempsize += sizeof(gFixedCode);
	//pdkpt("tempsize=%u\r\n", tempsize);

	uint32_t l = 0;
	uint8_t ret = rng_get_numbers((uint32_t*)&l, 1);
	if (ret != STD_SUCCESS)
	{
		pdkpt("failed to get rng\r\n");
		return ret;
	}
	//for (uint32_t i = 0; i < tempsize; i++)
	//{
	//	l += temp[i];
	//}
	l = 2 + l % 20; //在2-20个之间

	ret = rng_get_numbers((uint32_t*)&temp[tempsize], l); //tempsize必为4的倍数
	if (ret != STD_SUCCESS)
	{
		pdkpt("failed to get rng\r\n");
		return ret;
	}
	tempsize += (l * 4);

	pdkpt("get %u rng number(x4), and total number have %u bytes\r\n", (unsigned int)l, (unsigned int)tempsize);
#ifdef DEBUG
	uint32_t* dat = (uint32_t*)&temp[tempsize - l * 4];
	pdnpt("rng=");
	for (uint32_t i = 0; i < l; i++)
	{
		pdnpt("0x%x, ", (unsigned int)dat[i]);

	}
	pdnpt("\r\n");
#endif


	md5_get_hash_value((uint8_t*)temp, tempsize, hash);

	//设备地址，6个字节，0-5字节
	//
	//设备类型，2个字节，6-7字节
	//
	//设备pin码， 3个字节，8-10字节
	//
	//设备地址类型，1个字节，11字节，定义如下
	//#define PUBLIC_ADDR                 (0)
	//#define RANDOM_ADDR                 (1)
	//#define STATIC_RANDOM_ADDR          (1)
	//#define RESOLVABLE_PRIVATE_ADDR     (2)
	//#define NON_RESOLVABLE_PRIVATE_ADDR (3)
	//
	//数字名称，3个字节,12-14字节
	//备用，1个字节,15

	//设备类型
	hash[5] = hash[6] | 0xC0;
	hash[6] = (uint8_t)(DEVICE_TPYE_DEFAULT_CODE & 0xff);		 //0x02;
	hash[7] = (uint8_t)((DEVICE_TPYE_DEFAULT_CODE >> 8) & 0xff); //0x80;
	hash[8] = 0;
	hash[9] = 0;
	hash[10] = 0;
	hash[11] = 1; //RANDOM_ADDR

	//根据MAC前四字节生成蓝牙名称
	uint32_t num = 0;
	memcpy((uint8_t*)&num, (uint8_t*)hash, sizeof(num));
	num = num % BLE_LOCAL_NAME_MAX_NUMBER;
	uint8_t* nump = (uint8_t*)&num;

	hash[12] = nump[0];
	hash[13] = nump[1];
	hash[14] = nump[2];
	hash[15] = 0;

	memcpy(mac, hash, sizeof(hash));
	return STD_SUCCESS;
}

// 获取mcu UID, 返回uint32_t的长度
// lot number [95-40] -- 产品批号, 
//Wafer number[39-32] -- 晶圆批号, 
//X and Y coordinates[31-0] -- 坐标
uint8_t pd_get_mcu_uid(uint32_t* buff)
{
	buff[0] = HAL_GetUIDw0();
	buff[1] = HAL_GetUIDw1();
	buff[2] = HAL_GetUIDw2();
	return 3;
}

#pragma endregion

#pragma region 校验设备


static uint8_t sDeviceIsChecked = false;
// 确认对方是否是正确设备
uint8_t pd_check_mcu_mark(void* mark)
{
	__MCU_MARK_TypeDef* mm = (__MCU_MARK_TypeDef*)mark;
	uint32_t buff[sizeof(gFixedCode) / 4 + 2] = { 0 };
	memcpy((uint8_t*)buff, (uint8_t*)gFixedCode, sizeof(gFixedCode));
	buff[sizeof(gFixedCode) / 4] = mm->TimeStamp[0];
	buff[sizeof(gFixedCode) / 4 + 1] = mm->TimeStamp[1];
	uint32_t crcc = crc32_calculate(buff, sizeof(buff));
	if (mm->Mark == crcc)
	{
		sDeviceIsChecked = true;
		return STD_SUCCESS;
	}
	else
	{
		sDeviceIsChecked = false;
		return STD_FAILED;
	}
}

uint8_t pd_device_is_checked(void)
{
	pdkpt("checked=%u\r\n", sDeviceIsChecked);
	return sDeviceIsChecked;
}

void pd_clear_device_check(void)
{
	sDeviceIsChecked = false;
}

#pragma endregion

/*************************End****************************************************/