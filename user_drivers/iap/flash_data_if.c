/*******************************************************************************
* file    flash_data_if.c
* author  mackgim
* version 1.0.0
* date
* brief   FLASH 存储读取操作
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "flash_data_if.h" 
#include "platform.h"
#include "standard_lib.h"
#include "flash_driver.h"
#include "utils.h"

#ifdef DEBUG
#if 1
#define flkpt(...) kprint(__VA_ARGS__)
#define flnpt(...) nprint(__VA_ARGS__)
#else
#define flkpt(...)
#define flnpt(...) 
#endif
#else
#define flkpt(...)
#define flnpt(...) 
#endif

#define FLASH_DATA_MAX_BUFFER_SIZE  (1024)



//查找最后一个数据空间地址，和第一个空余空间地址
//STD_SUCCESS,找到数据
//STD_FAILED, 没有数据
//STD_ERROR, 严重故障，擦除flash处理
uint8_t flash_data_find_space(__FLASH_DATA_FIND_TOKEN_TypeDef* token)
{
	uint32_t begin = FLASH_SPACE_USER_BEGIN_ADDR;
	uint32_t end = FLASH_SPACE_USER_END_ADDR;
	__FLASH_DATA_FIND_TOKEN_TypeDef tktemp = { 0 };

	do
	{
		__FLASH_DATA_HEAD_TypeDef* headp = (__FLASH_DATA_HEAD_TypeDef*)begin;
		if ((headp->HeadCode != FLASH_DATA_FREE_TOKEN) && (headp->HeadCode != FLASH_DATA_HEAD_TOKEN))
		{
			flkpt("failed,head error,0x%x\r\n", (unsigned int)headp->HeadCode);
			return STD_FAILED;
		}

		if (headp->HeadCode == FLASH_DATA_HEAD_TOKEN)
		{
			tktemp.DataItems++;
			tktemp.DataAddr = begin;
			begin += headp->DataSize + sizeof(__FLASH_DATA_HEAD_TypeDef);
			//数据没有对齐
			if (!is_long_long_aligned((void const*)begin))
			{
				flkpt("failed, data(0x%x) is not align by 8\r\n", (unsigned int)begin);
				return STD_FAILED;
			}
			//数据超出范围
			if (begin > end)
			{
				flkpt("failed,size error,0x%x\r\n", (unsigned int)begin);
				return STD_FAILED;
			}
			//所有的都是数据
			if (begin == end)
			{
				tktemp.FreeAddr = FLASH_SPACE_USER_END_ADDR;
				tktemp.Percentage = 100;
				break;
			}
		}
		else if (headp->HeadCode == FLASH_DATA_FREE_TOKEN)
		{
			tktemp.FreeAddr = begin;
			tktemp.Percentage = (tktemp.FreeAddr - FLASH_SPACE_USER_BEGIN_ADDR) * 100 / (FLASH_SPACE_USER_END_ADDR - FLASH_SPACE_USER_BEGIN_ADDR);
			break;
		}

	} while (1);

	*token = tktemp;

	return STD_SUCCESS;
}

//buffer_size为字节数，必须为8的倍数
uint8_t flash_data_write(uint32_t* buffer, uint32_t buffer_size)
{

	if (buffer_size % 8)
	{
		flkpt("8 aligned error\r\n");
		return STD_FAILED;
	}

	__FLASH_DATA_FIND_TOKEN_TypeDef token;

	uint8_t ret;
	do
	{
		ret = flash_data_find_space(&token);

		//判断是否和要写入的内容一致，一致直接退出
		if (ret == STD_SUCCESS)
		{
			__FLASH_DATA_HEAD_TypeDef* headp = (__FLASH_DATA_HEAD_TypeDef*)token.DataAddr;
			uint32_t* datap = (uint32_t*)(token.DataAddr + sizeof(__FLASH_DATA_HEAD_TypeDef));
			//长度一样
			if (headp->DataSize == buffer_size)
			{
				//数据一样
				if (memcmp(buffer, datap, buffer_size) == 0)
				{
					flkpt("ok, same data\r\n");
					return STD_SUCCESS;
				}
			}
		}


		/* 擦除条件
		// 1、flash空间地址错误
		// 2、flash已经没有可用空间
		// 3、flash剩余空间小于或等于需要的数据空间，避免2条件的出现
		*/

		if ((ret == STD_FAILED) || (token.FreeAddr >= FLASH_SPACE_USER_END_ADDR) ||
			((token.FreeAddr + sizeof(__FLASH_DATA_HEAD_TypeDef) + buffer_size) >= FLASH_SPACE_USER_END_ADDR))
		{
			if (flash_data_erase() != STD_SUCCESS)
			{
				flkpt("failed to erase flash\r\n");
				return STD_FAILED;
			}
			flkpt("ok to erase flash\r\n");
		}
		else if (ret == STD_SUCCESS)
		{
			break;
		}
		else
		{
			flkpt("error!!!!\r\n");
		}
	} while (1);

	__FLASH_DATA_HEAD_TypeDef head = { 0 };

	head.HeadCode = FLASH_DATA_HEAD_TOKEN;
	head.DataSize = buffer_size;


	ret = flash_ll_write(token.FreeAddr, (uint32_t*)&head, sizeof(head));
	if (ret != STD_SUCCESS)
	{
		flkpt("failed to write head\r\n");
		return ret;
	}

	ret = flash_ll_write(token.FreeAddr + sizeof(head), buffer, buffer_size);
	if (ret != STD_SUCCESS)
	{
		flkpt("failed to write data\r\n");
		return ret;
	}

#ifdef DEBUG
	flash_data_find_space(&token);
	flkpt("DataAddr=0x%x, freeAddr=0x%x, items=%u, per=%u%%\r\n", (unsigned int)token.DataAddr,
		(unsigned int)token.FreeAddr, (unsigned int)token.DataItems, (unsigned int)token.Percentage);
#endif

	flkpt("ok\r\n");
	return STD_SUCCESS;
}

//获取最后一帧数据,如果没有返回STD_NODATA
uint8_t flash_data_read(uint32_t* buffer, uint32_t buffer_size)
{

	__FLASH_DATA_FIND_TOKEN_TypeDef token;

	uint8_t ret;
	ret = flash_data_find_space(&token);
	if (ret == STD_FAILED)
	{
		if (flash_data_erase() != STD_SUCCESS)
		{
			flkpt("failed to erase flash\r\n");
			return STD_FAILED;
		}
		flkpt("ok to erase flash\r\n");
		return STD_NODATA;
	}

	flkpt("Data=0x%x, free=0x%x, items=%u, per=%u%%\r\n", (unsigned int)token.DataAddr,
		(unsigned int)token.FreeAddr, (unsigned int)token.DataItems, (unsigned int)token.Percentage);

	if ((token.DataAddr == 0) && (token.DataItems == 0))
	{
		flkpt("no data\r\n");
		return STD_NODATA;
	}

	__FLASH_DATA_HEAD_TypeDef* headp = (__FLASH_DATA_HEAD_TypeDef*)token.DataAddr;

	uint32_t l = MIN(headp->DataSize, buffer_size);
	uint32_t* datap = (uint32_t*)(token.DataAddr + sizeof(__FLASH_DATA_HEAD_TypeDef));

	memcpy((uint8_t*)buffer, (uint8_t*)datap, l);

	return STD_SUCCESS;
}

uint8_t flash_data_erase(void)
{
	return flash_ll_erase(FLASH_SPACE_USER_BEGIN_ADDR, FLASH_SPACE_USER_END_ADDR);
}



/*******************************************************************************
END
*******************************************************************************/

