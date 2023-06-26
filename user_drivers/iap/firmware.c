/*******************************************************************************
* file    firmware.c
* author  mackgim
* version 1.0.0
* date
* brief    固件升级驱动
*******************************************************************************/

#include "firmware.h"
#include "standard_lib.h"
#include "flash_driver.h"
#include "crc32.h"



#ifdef DEBUG
#if 1
#define fwkpf(...) kprint(__VA_ARGS__)
#define fwnpf(...) nprint(__VA_ARGS__)
#else
#define fwkpf(...)
#define fwnpf(...)
#endif
#else
#define fwkpf(...)
#define fwnpf(...)
#endif

#ifdef DEBUG
static uint64_t sFwStartTime = 0;
static uint64_t sFwStopTime = 0;
#endif


__FIRMWARE_HEAD_TypeDef sFwHead = { 0 };
__FIRMWARE_TAIL_TypeDef sFwTail = { 0 };
__FIRMWARE_DATA_TypeDef sFwData = { 0 };
__FIRMWARE_CONFIG_TypeDef sFwConfig = { 0 };

static uint64_t  sFwTimeout;
static uint32_t sFwTimeoutPreviousIndex = 0;

static  uint32_t sFwDataPreviousIndex = 0;
static  uint32_t sFwDataAddr = 0;

static  uint8_t sFwIsStarted = false;

static uint32_t FwDataSize = 0;

static uint8_t fw_proc_end(void);

void fw_clean(void)
{
	sFwIsStarted = false;
	memset((char*)&sFwHead, 0, sizeof(__FIRMWARE_HEAD_TypeDef));
	memset((char*)&sFwTail, 0, sizeof(__FIRMWARE_TAIL_TypeDef));
	memset((char*)&sFwData, 0, sizeof(__FIRMWARE_DATA_TypeDef));
	memset((char*)&sFwConfig, 0, sizeof(__FIRMWARE_CONFIG_TypeDef));
	sFwDataAddr = 0;
	sFwDataPreviousIndex = 0;
	sFwTimeoutPreviousIndex = 0;
	FwDataSize = 0;
#ifdef DEBUG  
	sFwStopTime = Clock_Time();
	fwkpf("time consuming=%u\r\n", (unsigned int)sFwStopTime - (unsigned int)sFwStartTime);
#endif
	fwkpf("done\r\n");
	set_reboot_msg(CFG_OTA_REBOOT_ON_CPU2_FW_CHECKED);//清除启动信号
}

uint8_t fw_start(uint8_t* buffer, uint32_t buffersize)
{

#ifdef DEBUG  
	sFwStartTime = Clock_Time();
#endif

	if (fw_is_ready(buffer, buffersize) != STD_SUCCESS)
	{
		fwkpf("failed, ready\r\n");
		goto failed;
	}
	memcpy(&sFwHead, buffer, buffersize);
	fwkpf("type=%u, size=%u, pack=%u\r\n", (unsigned int)sFwHead.Type, (unsigned int)sFwHead.Size, (unsigned int)sFwHead.NumOfPackage);

	//擦除flash
	uint8_t ret = flash_erase_shadow_code();
	if (ret != STD_SUCCESS)
	{
		fwkpf("failed to erase shadow, 0x%x\r\n", ret);
		goto failed;
	}

	//写入帧头
	sFwDataAddr = 0;
	uint32_t data[(sizeof(__FIRMWARE_HEAD_TypeDef) + 4) / 4] = { 0 };
	memcpy((char*)&data[0], (char*)&sFwHead, sizeof(__FIRMWARE_HEAD_TypeDef));
	if (flash_write_shadow_code(flash_get_shadow_addr(sFwDataAddr), (uint32_t*)&data[0], sizeof(__FIRMWARE_HEAD_TypeDef) + 4) != STD_SUCCESS)
	{
		fwkpf("failed, write\r\n");
		goto failed;
	}
	sFwDataAddr = sizeof(__FIRMWARE_HEAD_TypeDef) + 4;
	FwDataSize = 0;
	sFwDataPreviousIndex = sFwTimeoutPreviousIndex = sFwData.Index;
	sFwTimeout = Clock_Time() + CLOCK_SECOND * 10;
	fwkpf("ok\r\n");
	sFwIsStarted = true;
	return STD_SUCCESS;
failed:
	fw_clean();
	return STD_FAILED;
}

uint8_t fw_proc(uint8_t* buffer, uint32_t buffersize)
{
	if (!sFwIsStarted)
	{
		fwkpf("not start\r\n");
		goto failed;
	}

	if (buffersize > sizeof(__FIRMWARE_DATA_TypeDef))
	{
		fwkpf("size error, %u!=%u\r\n", (unsigned int)buffersize, sizeof(__FIRMWARE_DATA_TypeDef));
		goto failed;
	}

	memcpy((uint8_t*)&sFwData, buffer, buffersize);
	uint32_t dataSize = buffersize - FIRMWARE_DATA_INDEX_SIZE;

	if ((sFwData.Index == 0) || (sFwData.Index > sFwHead.NumOfPackage))
	{
		fwkpf("index over,%u-(1,%u)\r\n", (unsigned int)sFwData.Index, (unsigned int)sFwHead.NumOfPackage);
		goto failed;
	}

	if ((sFwDataPreviousIndex + 1) != (sFwData.Index))
	{
		fwkpf("index error,%u-%u\r\n", (unsigned int)sFwDataPreviousIndex, (unsigned int)sFwData.Index);
		goto failed;
	}

	//写入数据
	{
		FwDataSize += dataSize;//统计总字节数
		uint32_t writeSize = 0;
		if (sFwData.Index == sFwHead.NumOfPackage) //last frame
		{
			if (FwDataSize != sFwHead.Size)
			{
				fwkpf("total size error,r=%u, d=%u\r\n", (unsigned int)FwDataSize, (unsigned int)sFwHead.Size);
				goto failed;
			}
			writeSize = dataSize;
			writeSize = (writeSize + 7) / 8 * 8;//保证64bit对齐

		}
		else
		{
			writeSize = dataSize;
		}
		if (flash_write_shadow_code(flash_get_shadow_addr(sFwDataAddr), (uint32_t*)sFwData.Data, writeSize) != STD_SUCCESS)
		{
			fwkpf("failed, write\r\n");
			goto failed;
		}


		{
#ifdef DEBUG1
			uint32_t* p = (uint32_t*)sFwData.Data;
			for (uint32_t i = 0; i < writeSize / 4; i++)
			{
				fwnpf("%08x ", p[i]);
			}
			fwnpf("\r\n");
#endif

		}
		sFwDataAddr += writeSize;
		sFwDataPreviousIndex = sFwData.Index;
	}

	return STD_SUCCESS;
failed:
	fw_clean();
	return STD_FAILED;
}

uint8_t fw_stop(uint8_t* buffer, uint32_t buffersize)
{
	uint32_t ShadowCodeAddr = flash_get_shadow_addr(0);
	if (!sFwIsStarted)
	{
		fwkpf("not start\r\n");
		goto failed;
	}
	if (buffersize != sizeof(__FIRMWARE_TAIL_TypeDef))
	{
		fwkpf("error size,%u-%u\r\n", (unsigned int)buffersize, sizeof(__FIRMWARE_TAIL_TypeDef));
		goto failed;
	}

	memcpy((char*)&sFwTail, buffer, sizeof(__FIRMWARE_TAIL_TypeDef));
	fwnpf("\r\n");
	if (sFwTail.Type != sFwHead.Type)
	{
		fwkpf("error type,%u-%u\r\n", (unsigned int)sFwHead.Type, (unsigned int)sFwTail.Type);
		goto failed;
	}

	//检查最后的地址
	if (sFwDataAddr != (sizeof(__FIRMWARE_HEAD_TypeDef) + 4 + ((sFwHead.Size + 7) / 8 * 8)))
	{
		fwkpf("size wrong,%u-%u\r\n", (unsigned int)sFwDataAddr, (unsigned int)(sizeof(__FIRMWARE_HEAD_TypeDef) + sFwHead.Size));
		goto failed;
	}

	//校验数据
	{
		uint32_t crct = crc32_calculate((uint32_t*)(ShadowCodeAddr + sizeof(__FIRMWARE_HEAD_TypeDef) + 4), sFwHead.Size);
		if (crct != sFwTail.CheckCode)
		{
			fwkpf("error crc, 0x%x-0x%x\r\n", (unsigned int)crct, (unsigned int)sFwTail.CheckCode);
			goto failed;
		}

		fwkpf("crc ok, crc(calc)=0x%x, crc(send)=0x%x, size=%u\r\n", (unsigned int)crct, (unsigned int)sFwTail.CheckCode, (unsigned int)sFwHead.Size);

	}


#if 0 //打印整个shadow区的数据
	uint32_t* x = (uint32_t*)(flash_get_shadow_addr(0) + sizeof(__FIRMWARE_HEAD_TypeDef) + 4);

	for (uint32_t i = 0; i < (sFwHead.Size / 4); i++)
	{
		if ((i % 10) == 0)
		{
			nprint("\r\n");
		}
		nprint("0x%0.8x,", x[i]);
		if ((i % 50) == 0)
		{
			osDelay(50);
		}
	}
	nprint("\r\n");
#endif
	//保存Stop帧尾数据
	if (flash_write_shadow_code(flash_get_shadow_addr(sFwDataAddr), (uint32_t*)&sFwTail, sizeof(__FIRMWARE_TAIL_TypeDef)) != STD_SUCCESS)
	{
		fwkpf("failed, write stop info\r\n");
		goto failed;
	}
	sFwDataAddr += sizeof(sFwTail);

	//对数据做最后的处理
	if (fw_proc_end() != STD_SUCCESS)
	{
		fwkpf("failed to end proc\r\n");
		goto failed;
	}

	fwkpf("ok\r\n");
	fw_clean();
	return STD_SUCCESS;
failed:
	fwkpf("failed\r\n");
	fw_clean();
	return STD_FAILED;

}

uint8_t fw_set_config(uint8_t* buffer, uint32_t buffersize)
{

	//if (!is_word_aligned(buffer)) return STD_FAILED;
	__FIRMWARE_CONFIG_TypeDef* fct = (__FIRMWARE_CONFIG_TypeDef*)buffer;

	if ((fct->CfgLength * 4 + 4) != buffersize)
	{
		fwkpf("failed, size error, %u-%u\r\n", (unsigned int)(fct->CfgLength * 4 + 4), (unsigned int)buffersize);
		return STD_FAILED;
	}
	uint32_t type = fct->Type;

	switch (type)
	{
	case DEVICE_WB_BLE_STACK_FIRMWARE_TYPE:
	{
		uint32_t l = MIN(sizeof(sFwConfig), buffersize);
		memcpy(&sFwConfig, buffer, l);
		fwkpf("l=%u, count=%u, addr=0x%x\r\n", (unsigned int)l, (unsigned int)sFwConfig.CfgLength, (unsigned int)sFwConfig.FWAddr);
	}
	break;
	default:
	{
		return STD_FAILED;
	}
	break;
	}
	return STD_SUCCESS;
}

uint8_t fw_timeout(void)
{

	if (sFwHead.Type == 0)
	{
		return STD_DO_NOTHING;
	}

	if (!sFwIsStarted)
	{
		return STD_DO_NOTHING;
	}
	//10秒钟，序号没有增加，进行清理退出烧写流程
	if (sFwTimeoutPreviousIndex != sFwData.Index)
	{
		sFwTimeoutPreviousIndex = sFwData.Index;
		sFwTimeout = Clock_Time() + CLOCK_SECOND * 10;
	}
	else
	{
		if (Clock_Time() > sFwTimeout)
		{

			fwkpf("type=%u,package=%u,Index=%u\r\n", (unsigned int)sFwHead.Type, (unsigned int)sFwHead.NumOfPackage, (unsigned int)sFwData.Index);
			fw_clean();
			return STD_TIMEOUT;
		}
	}
	return STD_DO_NOTHING;
}

uint8_t fw_is_ready(uint8_t* buffer, uint32_t buffersize)
{

	if (buffersize != sizeof(__FIRMWARE_HEAD_TypeDef))
	{
		fwkpf("error size, %u-%u\r\n", (unsigned int)buffersize, sizeof(__FIRMWARE_HEAD_TypeDef));
		goto failed;
	}

	__FIRMWARE_HEAD_TypeDef headTemp;
	memcpy((char*)&headTemp, buffer, sizeof(__FIRMWARE_HEAD_TypeDef));

	//判断固件类型
	if ((headTemp.Type != DEVICE_FIRMWARE_TYPE) && (headTemp.Type != DEVICE_WB_BLE_STACK_FIRMWARE_TYPE))
	{
		fwkpf("type error, 0x%x\r\n", (unsigned int)headTemp.Type);
		goto failed;
	}

	//判断shadow空间
	if (headTemp.Size > flash_get_shadow_max_size())
	{
		fwkpf("size bigger than shadow, %u-%u\r\n", (unsigned int)headTemp.Size, (unsigned int)flash_get_shadow_max_size());
		goto failed;
	}

	//如果是app固件，判断running空间
	if (headTemp.Type == DEVICE_FIRMWARE_TYPE)
	{
		if (headTemp.Size > flash_get_running_max_size())
		{
			fwkpf("size bigger than running, %u-%u\r\n", (unsigned int)headTemp.Size, (unsigned int)flash_get_running_max_size());
			goto failed;
		}
	}
	//如果是stack固件,判断stack地址以及stack空间
	else
	{
		//判断stack地址
		if ((sFwConfig.FWAddr < flash_get_ble_stack_addr()) || (sFwConfig.FWAddr >= flash_get_end_addr()))
		{
			fwkpf("failed, stack addr, 0x%x(min=0x%x, max=0x%x)\r\n", (unsigned int)sFwConfig.FWAddr, (unsigned int)flash_get_ble_stack_addr(), (unsigned int)flash_get_end_addr());
			return STD_FAILED;
		}

		//判断stack空间
		if (headTemp.Size > (flash_get_end_addr() - sFwConfig.FWAddr))
		{
			fwkpf("size bigger than stack, %u-%u\r\n", (unsigned int)headTemp.Size, (unsigned int)(flash_get_end_addr() - sFwConfig.FWAddr));
			goto failed;
		}
	}
	
	//判断包大小
	uint32_t num = headTemp.Size / headTemp.NumOfPackage;
	if (num > FIRMWARE_DATA_DATA_SIZE)
	{
		fwkpf("size per data error, %u-%u\r\n", (unsigned int)num, (unsigned int)FIRMWARE_DATA_DATA_SIZE);
		goto failed;
	}
	fwkpf("ok\r\n");
	return STD_SUCCESS;
failed:
	return STD_FAILED;
}

uint8_t fw_proc_end(void)
{
	switch (sFwHead.Type)
	{
	case DEVICE_FIRMWARE_TYPE:
		return STD_SUCCESS;
	case DEVICE_WB_BLE_STACK_FIRMWARE_TYPE:
	{
		//保存config数据
		if (flash_write_shadow_code(flash_get_shadow_addr(sFwDataAddr), (uint32_t*)&sFwConfig, sizeof(sFwConfig)) != STD_SUCCESS)
		{
			fwkpf("failed, write config info\r\n");
			return STD_FAILED;
		}
		sFwDataAddr += sizeof(sFwConfig);
	}
	break;
	}
	return STD_SUCCESS;
}

//如果是协议栈，返回蓝牙
uint8_t fw_get_ble_stack_addr(void)
{
	uint32_t shadowAddr = flash_get_shadow_addr(0);
	__FIRMWARE_HEAD_TypeDef* headPtr = (__FIRMWARE_HEAD_TypeDef*)shadowAddr;
	if (headPtr->Type != DEVICE_WB_BLE_STACK_FIRMWARE_TYPE)
	{
		return 0;
	}

	uint32_t tFWAddr = shadowAddr + sizeof(__FIRMWARE_HEAD_TypeDef) + 4;
	__FIRMWARE_TAIL_TypeDef* tailPtr = (__FIRMWARE_TAIL_TypeDef*)(tFWAddr + (headPtr->Size + 7) / 8 * 8);
	__FIRMWARE_CONFIG_TypeDef* config = (__FIRMWARE_CONFIG_TypeDef*)((uint32_t)tailPtr + sizeof(__FIRMWARE_TAIL_TypeDef));

	return config->FWAddr;
}


