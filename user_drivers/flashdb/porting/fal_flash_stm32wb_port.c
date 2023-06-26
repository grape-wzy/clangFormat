/*
 * Copyright (c) 2020, Armink, <armink.ztl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <fal.h>
#include "platform.h"
#include "standard_lib.h"
#include "flash_driver.h"


 /*
 STM32F1会因容量不同而不同
	 小容量和中容量产品主存储块128KB以下，  每页1KB。
	 大容量和互联型产品主存储块256KB以上，  每页2KB。

 GD32   会因容量不同而不同
	 1. Low-density Products     Flash容量从 16KB到  32KB的产品
	 2. Medium-density Products  Flash容量从 64KB到 128KB的产品
		   全是1K
	 3. High-density Products    Flash容量从256KB到 512KB的产品
		   全是2K
	 4. XL-density Products      Flash容量从768KB到3072KB的产品
		   <512K 是2K
		   >512K 是4K

 雅特力
	 全是2K

 STM32F4
	 STM32F4的flash页尺寸不一样，低地址16KB，高地址32KB或128KB.

 WB55
	 全是4K
 */


static int init(void)
{
	/* do nothing now */
	return 1;
}

void feed_dog(void)
{

}

static int read(long offset, uint8_t* buf, size_t size)
{

	uint32_t addr = stm32_onchip_flash.addr + offset;

	if (((addr % 4) == 0) && ((uint32_t)buf % 4 == 0) && (size % 4 == 0))
	{
		uint32_t l = size / 4;
		uint32_t* pi = (uint32_t*)addr;
		uint32_t* po = (uint32_t*)buf;
		for (uint32_t i = 0; i < l; i++)
		{
			po[i] = pi[i];
		}
	}
	else
	{
		for (size_t i = 0; i < size; i++, addr++, buf++)
		{
			*buf = *(uint8_t*)addr;
		}
	}
	return size;
}


static int write(long offset, const uint8_t* buf, size_t size)
{
	uint32_t addr = stm32_onchip_flash.addr + offset;
	uint8_t ret = flash_ll_write(addr, buf, size);
	if (ret != STD_SUCCESS)
	{
		return -1;
	}
	return size;
}


static int erase(long offset, size_t size)
{
	uint32_t startaddr = stm32_onchip_flash.addr + offset;
	uint32_t endaddr = startaddr + size;
	uint8_t ret = flash_ll_erase(startaddr, endaddr);
	if (ret != STD_SUCCESS)
	{
		return -1;
	}
	return size;
}


/*
  "stm32_onchip" : Flash 设备的名字。
  0x08000000: 对 Flash 操作的起始地址。
  1024*1024：Flash 的总大小（1MB）。
  128*1024：Flash 块/扇区大小（因为 STM32F2 各块大小不均匀，所以擦除粒度为最大块的大小：128K）。
  {init, read, write, erase} ：Flash 的操作函数。 如果没有 init 初始化过程，第一个操作函数位置可以置空。
  8 : 设置写粒度，单位 bit， 0 表示未生效（默认值为 0 ），该成员是 fal 版本大于 0.4.0 的新增成员。各个 flash 写入粒度不尽相同，可通过该成员进行设置，以下列举几种常见 Flash 写粒度：
  nor flash:  1 bit
  stm32f2/f4: 8 bit
  stm32f1:    32 bit
  stm32l4:    64 bit
  stm32wb:    64 bit
 */

 //1.定义 flash 设备

const struct fal_flash_dev stm32_onchip_flash =
{
	.name = "stm32_onchip",
	.addr = WB55XE_FLASH_BEGIN_ADDR,
	.len = 512 * 1024,
	.blk_size = FLASH_PAGE_SIZE,
	.ops = {init, read, write, erase},
	.write_gran = 64
};


