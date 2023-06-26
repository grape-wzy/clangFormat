/*******************************************************************************
* file     IAP.c
* author   mackgim
* version  V1.0.0
* date
* brief ：
*******************************************************************************/
#include "iap.h"
#include "firmware.h"
#include "standard_lib.h"
#include "platform.h"
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

void iap_init(void)
{

	uint32_t shadowAddr = flash_get_shadow_addr(0);

	__FIRMWARE_HEAD_TypeDef* headPtr = (__FIRMWARE_HEAD_TypeDef*)shadowAddr;

	flash_ram_init();

	//检测固件类型是否正确
	if ((headPtr->Type != DEVICE_FIRMWARE_TYPE) && (headPtr->Type != DEVICE_WB_BLE_STACK_FIRMWARE_TYPE))
	{
		fwkpf("quit!, neithor fw nor ble stack, this type=0x%x\r\n", (unsigned int)headPtr->Type);
		goto failed;
	}

	//获取固件地址
	uint32_t tFWAddr = shadowAddr + sizeof(__FIRMWARE_HEAD_TypeDef) + 4;

	//检测大小
	fwkpf("Code Size = %u\r\n", (unsigned int)headPtr->Size);
	uint32_t maxAddr = tFWAddr + (headPtr->Size + 7) / 8 * 8 + sizeof(__FIRMWARE_TAIL_TypeDef);
	if (maxAddr >= flash_get_shadow_end_addr())
	{
		fwkpf("size error,0x%x - max(0x%x)\r\n", (unsigned int)maxAddr, (unsigned int)flash_get_shadow_end_addr());
		goto failed;
	}

	//检查校验码
	__FIRMWARE_TAIL_TypeDef* tailPtr = (__FIRMWARE_TAIL_TypeDef*)(tFWAddr + (headPtr->Size + 7) / 8 * 8);
	uint32_t shadowCheckCode = crc32_calculate((uint32_t*)(tFWAddr), (headPtr->Size));
	if (shadowCheckCode != tailPtr->CheckCode)
	{
		fwkpf("crc error, firmware will be discarded,CRC(stored)=0x%x, CRC(calc)=0x%x\r\n", (unsigned int)tailPtr->CheckCode, (unsigned int)shadowCheckCode);
		flash_ram_erase_shadow_code();
		goto failed;
	}
	fwkpf("crc ok, CRC(stored)=0x%x, CRC(calc)=0x%x\r\n", (unsigned int)tailPtr->CheckCode, (unsigned int)shadowCheckCode);


	//如果是APP固件
	if (headPtr->Type == DEVICE_FIRMWARE_TYPE)
	{
		if ((headPtr->Size) > flash_get_running_max_size())
		{
			fwkpf("size bigger,0x%x - max(0x%x)\r\n", (unsigned int)(headPtr->Size), (unsigned int)flash_get_running_max_size());
			goto failed;
		}
	}
	//判断是蓝牙协议栈还是app固件, 进行分流处理
	//如果是蓝牙协议栈，退出，等待mcu2启动之后，继续更新蓝牙协议栈
	//非蓝牙协议栈，继续更新app固件
	else if (headPtr->Type == DEVICE_WB_BLE_STACK_FIRMWARE_TYPE)
	{
		//获取蓝牙协议栈配置地址
		__FIRMWARE_CONFIG_TypeDef* config = (__FIRMWARE_CONFIG_TypeDef*)((uint32_t)tailPtr + sizeof(__FIRMWARE_TAIL_TypeDef));
		fwkpf("ble stack, src=0x%x, desc=0x%x\r\n", (unsigned int)tFWAddr, (unsigned int)config->FWAddr);

		uint8_t error = 0;
		//判断stack地址
		if ((config->FWAddr < flash_get_ble_stack_addr()) || (config->FWAddr >= flash_get_end_addr()))
		{
			fwkpf("failed, stack addr, 0x%x(min=0x%x, max=0x%x)\r\n", (unsigned int)config->FWAddr, (unsigned int)flash_get_ble_stack_addr(), (unsigned int)flash_get_end_addr());
			error++;
		}

		//判断stack空间
		if (headPtr->Size > (flash_get_end_addr() - config->FWAddr))
		{
			fwkpf("size bigger than stack, %u-%u\r\n", (unsigned int)headPtr->Size, (unsigned int)(flash_get_end_addr() - config->FWAddr));
			error++;
		}

		if (error != 0)
		{
			set_reboot_msg(CFG_OTA_REBOOT_ON_CPU2_FW_CHECKED);
			flash_ram_erase_shadow_code();
			return;
		}


		uint32_t sftrst = 0, oblrst = 0;
		sftrst = LL_RCC_IsActiveFlag_SFTRST();
		oblrst = LL_RCC_IsActiveFlag_OBLRST();
		LL_RCC_ClearResetFlags();
		fwkpf("sftrst=%u, oblrst=%u\r\n", (unsigned int)sftrst, (unsigned int)oblrst);
		if (sftrst || oblrst)
		{
			//上电伊始是0x5f
			uint8_t msg = read_reboot_msg();
			fwkpf("reboot msg=0x%x\r\n", msg);

			//启动ble stack开始升级		
			if (msg == CFG_OTA_REBOOT_ON_CPU2_FW_CHECKED)
			{
				set_reboot_msg(CFG_OTA_REBOOT_ON_CPU2_START_FUS);
				fwkpf("then, update ble stack in other thread, when cpu2 start\r\n");
			}
			//fus删除之前的镜像后，把shadow区域的镜像复制到默认地址中
			else if (msg == CFG_OTA_REBOOT_ON_CPU2_DELETED_FW)
			{
				fwkpf("write stack, addr, start=0x%x, end=0x%x\r\n", (unsigned int)config->FWAddr, (unsigned int)(config->FWAddr + (headPtr->Size + 7) / 8 * 8));
				log_flush();
				flash_ram_write(config->FWAddr, (uint32_t*)tFWAddr, (headPtr->Size + 7) / 8 * 8);
				set_reboot_msg(CFG_OTA_REBOOT_ON_CPU2_WRITE_FW);
			}
			//fue升级stack完毕，删除shadow区镜像
			else if (msg == CFG_OTA_REBOOT_ON_CPU2_UPGRADED)
			{
				//set_reboot_msg(CFG_OTA_REBOOT_ON_IDLE);

				if (flash_ram_erase_shadow_code() == STD_SUCCESS)
				{
					fwkpf("ok to delecte shadow Code\r\n");
					set_reboot_msg(CFG_OTA_REBOOT_ON_CPU2_FW_CHECKED);
				}
				else
				{
					fwkpf("failed to delecte shadow Code\r\n");
				}
			}
		}
		else
		{
			set_reboot_msg(CFG_OTA_REBOOT_ON_CPU2_FW_CHECKED);
		}
		return;
	}

	//检测app的固件的堆栈是否正确
	fwkpf("shadowAddr(0x%x): stack addr=0x%x\r\n", (unsigned int)shadowAddr, (unsigned int)(*(volatile uint32_t*)(shadowAddr + sizeof(__FIRMWARE_HEAD_TypeDef) + 4)));
	if ((*(volatile uint32_t*)(tFWAddr)) != flash_get_mcu_stack_top_addr())
	{
		fwkpf("no firmware\r\n");
		goto failed;
	}


	//检测app的固件的身份ID是否正确
	if (flash_check_id(tFWAddr, headPtr->Size) == STD_SUCCESS)
	{
		fwkpf("Begin Program\r\n");
		led_blink_while_upgrading_fw();
		flash_ram_program(tFWAddr, headPtr->Size);
	}
	else
	{
		fwkpf("a wrong firmware , will erases and quit.\r\n");
		if (flash_ram_erase_shadow_code() == STD_SUCCESS)
		{
			fwkpf("ok to delecte shadow Code\r\n");
		}
		else
		{
			fwkpf("failed to delecte shadow Code\r\n");
		}
	}

	return;
failed:
	return;

}



void iap_check(void)
{
	uint32_t error = flash_get_error_status();
	fwkpf("error status= 0x%x\r\n", (unsigned int)error);
}


uint8_t iap_write_ble_image(void)
{
	uint32_t shadowAddr = flash_get_shadow_addr(0);
	__FIRMWARE_HEAD_TypeDef* headPtr = (__FIRMWARE_HEAD_TypeDef*)shadowAddr;
	//获取固件地址
	uint32_t tFWAddr = shadowAddr + sizeof(__FIRMWARE_HEAD_TypeDef) + 4;
	__FIRMWARE_TAIL_TypeDef* tailPtr = (__FIRMWARE_TAIL_TypeDef*)(tFWAddr + (headPtr->Size + 7) / 8 * 8);
	__FIRMWARE_CONFIG_TypeDef* config = (__FIRMWARE_CONFIG_TypeDef*)((uint32_t)tailPtr + sizeof(__FIRMWARE_TAIL_TypeDef));

	fwkpf("end addr=0x%x\r\n", (unsigned int)(config->FWAddr + (headPtr->Size + 7) / 8 * 8));
	log_flush();
	return flash_ll_write(config->FWAddr, (uint32_t*)tFWAddr, (headPtr->Size + 7) / 8 * 8);
}
/*******************************************************************************
END
*******************************************************************************/


