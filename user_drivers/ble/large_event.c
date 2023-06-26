/*******************************************************************************
* file    large_event.c
* author  mackgim
* version 1.0.0
* date
* brief   ble 超过mtu长度的数据传输事件
*******************************************************************************/

#include "large_event.h"
#include "standard_lib.h"
#include "ble_proc.h"
#include "crc32.h"

#ifdef DEBUG
#if 0
#define bknpf(...) nprint(__VA_ARGS__)
#define bkkpf(...) kprint(__VA_ARGS__)
#else
#define bknpf(...)
#define bkkpf(...)
#endif
#else
#define bknpf(...)
#define bkkpf(...)
#endif

// 最大一次不能操作65535
// 
//计算发送数据包的数据大小，不包含包头
uint32_t large_calc_pack_size(void)
{
	uint32_t packgeSize = 0;
	//根据mtu决定每包数据的大小
	uint32_t mtuSize = (uint32_t)ble_get_mtu();

	packgeSize = mtuSize - 3 - 2 - 2;//减去ATT包头3, 结构帧头2，数据帧头2
	packgeSize = packgeSize / 8 * 8;//保证64bit对齐
	return packgeSize;
}

uint8_t large_event_is_head(void* handle, uint8_t* buffer, uint8_t buffersize)
{
	__LARGE_EVENT_CMD_TypeDef* lec = (__LARGE_EVENT_CMD_TypeDef*)buffer;
	switch ((__LARGE_EVENT_STEP_Typedef)(lec->Cmd))
	{
	case LARGE_EVENT_HEAD:
		return STD_SUCCESS;
	default:
		break;
	}

	return STD_FAILED;
}

uint8_t large_event_set(void* handle, uint8_t* buffer, uint8_t buffersize)
{
	__LARGE_EVENT_CMD_TypeDef* lec = (__LARGE_EVENT_CMD_TypeDef*)buffer;
	__LARGE_EVENT_Typedef* hd = (__LARGE_EVENT_Typedef*)handle;

	if (hd->DataSize == 0)
	{
		return STD_FAILED;
	}

	switch ((__LARGE_EVENT_STEP_Typedef)(lec->Cmd))
	{
	case LARGE_EVENT_HEAD:
	{
		__LARGE_EVENT_HEAD_TypeDef* leh = (__LARGE_EVENT_HEAD_TypeDef*)lec->Data;
		hd->SizeOfBytes = leh->SizeOfBytes;
		hd->NumOfPackage = leh->NumOfPackage;
		hd->MaxSizeOfPackage = leh->MaxSizeOfPackage;
		hd->CheckCrc = 0;
		hd->DataCount = 0;
		hd->LastIndex = 0;

		if (hd->SizeOfBytes > hd->DataSize)
		{
			bkkpf("size error, %u-%u\r\n", hd->SizeOfBytes, (unsigned int)hd->DataSize);
			goto failed;
		}

		if (hd->SizeOfBytes % 4 != 0)
		{
			bkkpf("size(%u) error, not multiples of 4\r\n", hd->SizeOfBytes);
			goto failed;
		}
		bkkpf("ok, head, size=%u, package=%u, number=%u\r\n", hd->SizeOfBytes, hd->MaxSizeOfPackage, hd->NumOfPackage);
	}
	break;
	case LARGE_EVNET_TAIL:
	{
		__LARGE_EVENT_TAIL_TypeDef* let = (__LARGE_EVENT_TAIL_TypeDef*)lec->Data;
		hd->CheckCrc = let->CheckCrc;
		if (hd->DataCount != hd->SizeOfBytes)
		{
			bkkpf("total size error, %u-%u\r\n", (unsigned int)hd->DataCount, hd->SizeOfBytes);
			goto failed;
		}
		if (hd->LastIndex != hd->NumOfPackage)
		{
			bkkpf("index error 1, %u-%u\r\n", hd->NumOfPackage, hd->LastIndex);
			goto failed;
		}

		uint32_t AlgCrc = check_bcc(hd->DataP, hd->SizeOfBytes);
		if (AlgCrc != hd->CheckCrc)
		{
			bkkpf("crc error\r\n");
			goto failed;
		}
		hd->DataSize = 0;
		bkkpf("ok, tail\r\n");
	}
	return STD_SUCCESS;
	case LARGE_EVNET_DATA:
	{
		__LARGE_EVENT_DATA_TypeDef* led = (__LARGE_EVENT_DATA_TypeDef*)lec->Data;
		uint8_t rxlen = buffersize - 2;//减去所有其他的命令字
		if ((led->Index - 1) != hd->LastIndex)
		{
			bkkpf("index error 2, %u-%u\r\n", (led->Index - 1), hd->LastIndex);
			goto failed;
		}

		if (led->Index > hd->NumOfPackage)
		{
			bkkpf("index error 3, %u-%u\r\n", led->Index, hd->NumOfPackage);
			goto failed;
		}

		if (hd->DataCount > hd->SizeOfBytes)
		{
			bkkpf("size error 1, %u-%u\r\n", hd->SizeOfBytes, (unsigned int)hd->DataCount);
			goto failed;
		}

		memcpy(&hd->DataP[hd->DataCount], &led->Data[0], rxlen);
		hd->DataCount += rxlen;
		hd->LastIndex = led->Index;
	}
	break;
	}
	return STD_NEXT;
failed:
	return STD_FAILED;
}


uint8_t large_event_get(void* handle, uint8_t* buffer, uint8_t buffersize, uint8_t* outp, uint8_t* outsize)
{
	

	__LARGE_EVENT_CMD_TypeDef* lec = (__LARGE_EVENT_CMD_TypeDef*)buffer;
	__LARGE_EVENT_Typedef* hd = (__LARGE_EVENT_Typedef*)handle;
	
	if (hd->DataSize == 0)
	{
		return STD_FAILED;
	}

	__LARGE_EVENT_CMD_TypeDef* lecout = (__LARGE_EVENT_CMD_TypeDef*)outp;
	switch ((__LARGE_EVENT_STEP_Typedef)(lec->Cmd))
	{
	case LARGE_EVENT_HEAD:
	{
		//返回帧头
		lecout->Cmd = LARGE_EVENT_HEAD;
		__LARGE_EVENT_HEAD_TypeDef* head = (__LARGE_EVENT_HEAD_TypeDef*)lecout->Data;
		hd->SizeOfBytes = head->SizeOfBytes = hd->DataSize;
		hd->MaxSizeOfPackage = head->MaxSizeOfPackage = large_calc_pack_size();
		hd->CheckCrc = 0;
		hd->DataCount = 0;
		hd->LastIndex = 0;

		uint8_t xmod = hd->SizeOfBytes % hd->MaxSizeOfPackage;
		if (xmod == 0)
			hd->NumOfPackage = head->NumOfPackage = hd->SizeOfBytes / hd->MaxSizeOfPackage;
		else
			hd->NumOfPackage = head->NumOfPackage = hd->SizeOfBytes / hd->MaxSizeOfPackage + 1;

		*outsize = sizeof(__LARGE_EVENT_HEAD_TypeDef) + 1;
		bkkpf("ok, head, size=%u, package=%u, number=%u\r\n", hd->SizeOfBytes, hd->MaxSizeOfPackage, hd->NumOfPackage);
	}
	return STD_SUCCESS;
	case LARGE_EVNET_TAIL:
	{
		lecout->Cmd = LARGE_EVNET_TAIL;
		__LARGE_EVENT_TAIL_TypeDef* tail = (__LARGE_EVENT_TAIL_TypeDef*)lecout->Data;
		tail->CheckCrc = check_bcc(hd->DataP, hd->SizeOfBytes);
		*outsize = sizeof(__LARGE_EVENT_TAIL_TypeDef) + 1;
		hd->DataSize = 0;
		bkkpf("ok, tail\r\n");
	}
	return STD_SUCCESS;
	case LARGE_EVNET_DATA:
	{
		__LARGE_EVENT_DATA_TypeDef* led = (__LARGE_EVENT_DATA_TypeDef*)lec->Data;
		//序号从1开始
		if ((led->Index - 1) != hd->LastIndex)
		{
			bkkpf("index error, %u-%u\r\n", (led->Index - 1), hd->LastIndex);
			goto failed;
		}
		if (led->Index > hd->NumOfPackage)
		{
			bkkpf("index error 2, %u-%u\r\n", led->Index, hd->NumOfPackage);
			goto failed;
		}

		if (hd->DataCount > hd->SizeOfBytes)
		{
			bkkpf("size error 1, %u-%u\r\n", (unsigned int)hd->DataCount, hd->SizeOfBytes);
			goto failed;
		}

		lecout->Cmd = LARGE_EVNET_DATA;
		__LARGE_EVENT_DATA_TypeDef* ptr = (__LARGE_EVENT_DATA_TypeDef*)lecout->Data;
		ptr->Index = hd->LastIndex = led->Index;

		uint8_t rxlen;
		//帧序列从1开始计数，查看是否最后一帧，最后一帧长度与其他帧不一致
		//计算传输长度
		if (hd->NumOfPackage == led->Index)
		{
			uint8_t xmod = hd->SizeOfBytes % hd->MaxSizeOfPackage;
			if (xmod == 0)
			{
				rxlen = hd->MaxSizeOfPackage;
			}
			else
			{
				rxlen = xmod;
			}
		}
		else
		{
			rxlen = hd->MaxSizeOfPackage;
		}

		memcpy(&ptr->Data[0], &hd->DataP[hd->DataCount], rxlen);
		*outsize = rxlen + 2;
		hd->DataCount += rxlen;
	}
	break;

	}
	return STD_NEXT;
failed:
	return STD_FAILED;
}