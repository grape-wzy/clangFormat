/*******************************************************************************
* file    CRC32.c
* author  mackgim
* version 1.0.0
* date
* brief   CRC驱动
*         CRC32和CRC-32/MPEG2区别
*         CRC32输入按byte反转，byte-wise inversion, 0x1A2B3C4D becomes 0x58D43CB2
*         CRC32输出反转，与0xffffffff进行xor运算
*
*******************************************************************************/

#include "crc32.h"
#include "Platform.h"
#include "standard_lib.h"



uint8_t crc32_check(void);

static CRC_HandleTypeDef   CrcHandle;


uint8_t crc32_init(void)
{
	CrcHandle.Instance = CRC;
	CrcHandle.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
	CrcHandle.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
	CrcHandle.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
	CrcHandle.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
	CrcHandle.InputDataFormat = CRC_INPUTDATA_FORMAT_WORDS;

	uint8_t ret = HAL_CRC_Init(&CrcHandle);
	if (ret != HAL_OK)
	{
		kprint("failed, 0x%x\r\n", ret);
		return STD_FAILED;
	}

	kprint("ok\r\n");
	return STD_SUCCESS;
}

uint8_t crc32_deinit(void)
{
	uint8_t ret = HAL_CRC_DeInit(&CrcHandle);
	if (ret != HAL_OK)
	{
		kprint("failed, 0x%x\r\n", ret);
		return STD_FAILED;
	}
	return STD_SUCCESS;
}

uint32_t crc32_calculate(uint32_t pBuffer[], uint32_t BufferLength)
{
	return HAL_CRC_Calculate(&CrcHandle, pBuffer, BufferLength / 4);
}

uint32_t crc32_accumulate(uint32_t pBuffer[], uint32_t BufferLength, bool reset)
{
	if (reset)
	{
		__HAL_CRC_DR_RESET(&CrcHandle);
	}

	return HAL_CRC_Accumulate(&CrcHandle, pBuffer, BufferLength / 4);
}

void HAL_CRC_MspInit(CRC_HandleTypeDef* hcrc)
{
	/* CRC Peripheral clock enable */
	__HAL_RCC_CRC_CLK_ENABLE();
}

/**
  * @brief CRC MSP De-Initialization
  *        This function freeze the hardware resources used in this example:
  *          - Disable the Peripheral's clock
  * @param hcrc: CRC handle pointer
  * @retval None
  */
void HAL_CRC_MspDeInit(CRC_HandleTypeDef* hcrc)
{
	/* Enable CRC reset state */
	__HAL_RCC_CRC_FORCE_RESET();

	/* Release CRC from reset state */
	__HAL_RCC_CRC_RELEASE_RESET();

	__HAL_RCC_CRC_CLK_DISABLE();
}











const uint8_t aDataBufferByte[] = {
	  0x00,0x00,0x10,0x21, 0x20,0x42,0x30,0x63, 0x40,0x84,0x50,0xa5, 0x60,0xc6,0x70,0xe7, 0x91,0x29,0xa1,0x4a, 0xb1,0x6b,0xc1,0x8c,
  0xd1,0xad,0xe1,0xce, 0xf1,0xef,0x12,0x31, 0x32,0x73,0x22,0x52, 0x52,0xb5,0x42,0x94, 0x72,0xf7,0x62,0xd6, 0x93,0x39,0x83,0x18,
  0xa3,0x5a,0xd3,0xbd, 0xc3,0x9c,0xf3,0xff, 0xe3,0xde,0x24,0x62, 0x34,0x43,0x04,0x20, 0x64,0xe6,0x74,0xc7, 0x44,0xa4,0x54,0x85,
  0xa5,0x6a,0xb5,0x4b, 0x85,0x28,0x95,0x09, 0xf5,0xcf,0xc5,0xac, 0xd5,0x8d,0x36,0x53, 0x26,0x72,0x16,0x11, 0x06,0x30,0x76,0xd7,
  0x56,0x95,0x46,0xb4, 0xb7,0x5b,0xa7,0x7a, 0x97,0x19,0x87,0x38, 0xf7,0xdf,0xe7,0xfe, 0xc7,0xbc,0x48,0xc4, 0x58,0xe5,0x68,0x86,
  0x78,0xa7,0x08,0x40, 0x18,0x61,0x28,0x02, 0xc9,0xcc,0xd9,0xed, 0xe9,0x8e,0xf9,0xaf, 0x89,0x48,0x99,0x69, 0xa9,0x0a,0xb9,0x2b,
  0x4a,0xd4,0x7a,0xb7, 0x6a,0x96,0x1a,0x71, 0x0a,0x50,0x3a,0x33, 0x2a,0x12,0xdb,0xfd, 0xfb,0xbf,0xeb,0x9e, 0x9b,0x79,0x8b,0x58,
  0xbb,0x3b,0xab,0x1a, 0x6c,0xa6,0x7c,0x87, 0x5c,0xc5,0x2c,0x22, 0x3c,0x03,0x0c,0x60, 0x1c,0x41,0xed,0xae, 0xfd,0x8f,0xcd,0xec,
  0xad,0x2a,0xbd,0x0b, 0x8d,0x68,0x9d,0x49, 0x7e,0x97,0x6e,0xb6, 0x5e,0xd5,0x4e,0xf4, 0x2e,0x32,0x1e,0x51, 0x0e,0x70,0xff,0x9f,
  0xef,0xbe,0xdf,0xdd, 0xcf,0xfc,0xbf,0x1b, 0x9f,0x59,0x8f,0x78, 0x91,0x88,0x81,0xa9, 0xb1,0xca,0xa1,0xeb, 0xd1,0x0c,0xc1,0x2d,
  0xe1,0x6f,0x10,0x80, 0x00,0xa1,0x30,0xc2, 0x20,0xe3,0x50,0x04, 0x40,0x25,0x70,0x46, 0x83,0xb9,0x93,0x98, 0xa3,0xfb,0xb3,0xda,
  0xc3,0x3d,0xd3,0x1c, 0xe3,0x7f,0xf3,0x5e, 0x12,0x90,0x22,0xf3, 0x32,0xd2,0x42,0x35, 0x52,0x14,0x62,0x77, 0x72,0x56,0xb5,0xea,
  0x95,0xa8,0x85,0x89, 0xf5,0x6e,0xe5,0x4f, 0xd5,0x2c,0xc5,0x0d, 0x34,0xe2,0x24,0xc3, 0x04,0x81,0x74,0x66, 0x64,0x47,0x54,0x24,
  0x44,0x05,0xa7,0xdb, 0xb7,0xfa,0x87,0x99, 0xe7,0x5f,0xf7,0x7e, 0xc7,0x1d,0xd7,0x3c, 0x26,0xd3,0x36,0xf2, 0x06,0x91,0x16,0xb0,
  0x76,0x76,0x46,0x15, 0x56,0x34,0xd9,0x4c, 0xc9,0x6d,0xf9,0x0e, 0xe9,0x2f,0x99,0xc8, 0xb9,0x8a,0xa9,0xab, 0x58,0x44,0x48,0x65,
  0x78,0x06,0x68,0x27, 0x18,0xc0,0x08,0xe1, 0x28,0xa3,0xcb,0x7d, 0xdb,0x5c,0xeb,0x3f, 0xfb,0x1e,0x8b,0xf9, 0x9b,0xd8,0xab,0xbb,
  0x4a,0x75,0x5a,0x54, 0x6a,0x37,0x7a,0x16, 0x0a,0xf1,0x1a,0xd0, 0x2a,0xb3,0x3a,0x92, 0xed,0x0f,0xdd,0x6c, 0xcd,0x4d,0xbd,0xaa,
  0xad,0x8b,0x9d,0xe8, 0x8d,0xc9,0x7c,0x26, 0x5c,0x64,0x4c,0x45, 0x3c,0xa2,0x2c,0x83, 0x1c,0xe0,0x0c,0xc1, 0xef,0x1f,0xff,0x3e,
  0xdf,0x7c,0xaf,0x9b, 0xbf,0xba,0x8f,0xd9, 0x9f,0xf8,0x6e,0x17, 0x7e,0x36,0x4e,0x55, 0x2e,0x93,0x3e,0xb2, 0x0e,0xd1,0x1e,0xf0
};

const uint32_t aDataBuffer[] =
{
  0x00001021, 0x20423063, 0x408450a5, 0x60c670e7, 0x9129a14a, 0xb16bc18c,
  0xd1ade1ce, 0xf1ef1231, 0x32732252, 0x52b54294, 0x72f762d6, 0x93398318,
  0xa35ad3bd, 0xc39cf3ff, 0xe3de2462, 0x34430420, 0x64e674c7, 0x44a45485,
  0xa56ab54b, 0x85289509, 0xf5cfc5ac, 0xd58d3653, 0x26721611, 0x063076d7,
  0x569546b4, 0xb75ba77a, 0x97198738, 0xf7dfe7fe, 0xc7bc48c4, 0x58e56886,
  0x78a70840, 0x18612802, 0xc9ccd9ed, 0xe98ef9af, 0x89489969, 0xa90ab92b,
  0x4ad47ab7, 0x6a961a71, 0x0a503a33, 0x2a12dbfd, 0xfbbfeb9e, 0x9b798b58,
  0xbb3bab1a, 0x6ca67c87, 0x5cc52c22, 0x3c030c60, 0x1c41edae, 0xfd8fcdec,
  0xad2abd0b, 0x8d689d49, 0x7e976eb6, 0x5ed54ef4, 0x2e321e51, 0x0e70ff9f,
  0xefbedfdd, 0xcffcbf1b, 0x9f598f78, 0x918881a9, 0xb1caa1eb, 0xd10cc12d,
  0xe16f1080, 0x00a130c2, 0x20e35004, 0x40257046, 0x83b99398, 0xa3fbb3da,
  0xc33dd31c, 0xe37ff35e, 0x129022f3, 0x32d24235, 0x52146277, 0x7256b5ea,
  0x95a88589, 0xf56ee54f, 0xd52cc50d, 0x34e224c3, 0x04817466, 0x64475424,
  0x4405a7db, 0xb7fa8799, 0xe75ff77e, 0xc71dd73c, 0x26d336f2, 0x069116b0,
  0x76764615, 0x5634d94c, 0xc96df90e, 0xe92f99c8, 0xb98aa9ab, 0x58444865,
  0x78066827, 0x18c008e1, 0x28a3cb7d, 0xdb5ceb3f, 0xfb1e8bf9, 0x9bd8abbb,
  0x4a755a54, 0x6a377a16, 0x0af11ad0, 0x2ab33a92, 0xed0fdd6c, 0xcd4dbdaa,
  0xad8b9de8, 0x8dc97c26, 0x5c644c45, 0x3ca22c83, 0x1ce00cc1, 0xef1fff3e,
  0xdf7caf9b, 0xbfba8fd9, 0x9ff86e17, 0x7e364e55, 0x2e933eb2, 0x0ed11ef0
};


const uint8_t aDataBuffer1[] =
{ 0x08, 0x34, 0x79, 0x8F, 0x3C, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x00, 0xA6, 0x33, 0x79, 0x34, 0x08, 0x8F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xBC, 0x0C, 0x64, 0x00, 0xE4, 0x06, 0xBC, 0x0C, 0xB1, 0xFF, 0xB1, 0xFF, 0x70, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB0, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
;



/* Expected CRC Value */
uint32_t uwExpectedCRCValue = 0x379E9F06;


uint8_t crc32_check(void)
{
	uint32_t crc32;
	//crc32 = crc32_calculate((uint32_t *)aDataBuffer1,sizeof(aDataBuffer1));
	//kprint("size = %u, crc = 0x%x\r\n",sizeof(aDataBuffer1),(unsigned int)crc32);
	crc32 = crc32_calculate((uint32_t*)aDataBuffer, sizeof(aDataBuffer));
	if (crc32 == uwExpectedCRCValue)
	{
		kprint("CRC: OK! R(0x%x)-C(0x%x)\r\n", (unsigned int)uwExpectedCRCValue, (unsigned int)crc32);
		return true;
	}
	else
	{
		kprint("CRC: No! R(0x%x)-C(0x%x)\r\n", (unsigned int)uwExpectedCRCValue, (unsigned int)crc32);
		kprint("xor:0x%x\r\n", (unsigned int)(crc32 ^ 0xffffffff));
		return false;
	}

}
