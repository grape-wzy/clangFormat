/*******************************************************************************
* file     adis16505_if.c
* author   mackgim
* version  V1.0.0
* date
* brief ：
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/

#include "imu_if.h"
#include "adis16505_drv.h"
#include "standard_lib.h"



#define TIMEOUT_DURATION 100

static uint8_t adis16505_read_reg(__GYRO_ACC_HW_DRIVER_TypeDef* handle, uint8_t reg, uint16_t* data, uint16_t size);
static uint8_t adis16505_write_reg(__GYRO_ACC_HW_DRIVER_TypeDef* handle, uint8_t reg, uint16_t* data, uint16_t size);


uint8_t adis16505_read_reg(__GYRO_ACC_HW_DRIVER_TypeDef* handle, uint8_t reg, uint16_t* data, uint16_t size)
{
	uint16_t tTxBuffer = 0;// (((uint16_t)(reg & 0x7f)) << 8);
	uint16_t tRxBuffer = 0;
	for (uint16_t i = 0; i < (size + 1); i++)
	{
		tTxBuffer = (((uint16_t)((reg + 2 * i) & 0x7f)) << 8);
		if (tTxBuffer == 0)
		{
			tTxBuffer = 0x7200;
		}
		if (handle->transmit_receive((uint8_t*)&tTxBuffer, (uint8_t*)&tRxBuffer, 1, TIMEOUT_DURATION) != STD_SUCCESS)
		{
			return  STD_FAILED;
		}
		//kprint("i=%u, tx=0x%x,rx=0x%x\r\n", i, tTxBuffer, tRxBuffer);
		if (i > 0) // 丢弃第一个
		{
			data[i - 1] = tRxBuffer;
		}

		//延时才能正常读取
		for (uint32_t k = 0; k < 200; k++)
		{
			__NOP();
		}
	}

	return  STD_SUCCESS;
}

uint8_t adis16505_write_reg(__GYRO_ACC_HW_DRIVER_TypeDef* handle, uint8_t reg, uint16_t* data, uint16_t size)
{
	uint16_t tTxBuffer = (((uint16_t)(reg | 0x80)) << 8);
	uint16_t tRxBuffer = 0;
	for (uint16_t i = 0; i < size; i++)
	{
		//uint16_t add = (((uint16_t)((reg + 2 * i) | 0x80)) << 8) & 0xff00;
		uint16_t add = ((uint16_t)((reg + 2 * i) | 0x80));

		uint16_t temp = *(data + i);
		uint8_t lower = (uint8_t)(temp & 0xff);
		uint8_t upper = (uint8_t)((temp >> 8) & 0xff);
		tTxBuffer = (add << 8) + lower;

		if (handle->transmit_receive((uint8_t*)&tTxBuffer, (uint8_t*)&tRxBuffer, 1, TIMEOUT_DURATION) != STD_SUCCESS)
		{
			return  STD_FAILED;
		}

		for (uint32_t k = 0; k < 200; k++)
		{
			__NOP();
		}
		tTxBuffer = ((add + 1) << 8) + upper;

		if (handle->transmit_receive((uint8_t*)&tTxBuffer, (uint8_t*)&tRxBuffer, 1, TIMEOUT_DURATION) != STD_SUCCESS)
		{
			return  STD_FAILED;
		}
		for (uint32_t k = 0; k < 200; k++)
		{
			__NOP();
		}
	}

	return  STD_SUCCESS;
}

uint8_t  adis16505_reset(__GYRO_ACC_HW_DRIVER_TypeDef* handle)
{

	uint8_t ret = STD_FAILED;
	uint8_t aTxBuffer[32] = { 0 };
	uint8_t aRxBuffer[32] = { 0 };

	memset(&aTxBuffer, 0xff, sizeof(aTxBuffer));

	if (handle->transmit_receive((uint8_t*)aTxBuffer, (uint8_t*)aRxBuffer, sizeof(aTxBuffer), TIMEOUT_DURATION) == STD_SUCCESS)
	{
		ret = STD_SUCCESS;
	}

	Clock_Wait(200);
	return ret;
}

uint8_t  adis16505_check(__GYRO_ACC_HW_DRIVER_TypeDef* handle)
{
	uint16_t Old = 0, WNew = 0, RNew = 0;

	if (adis16505_read_reg(handle, ADIS16505_USER_SCR_1, &Old, 1) != STD_SUCCESS)
	{
		return STD_FAILED;
	}
	//kprint("UserScr1=0x%x\r\n", Old);
	WNew = 0x1234;
	if (adis16505_write_reg(handle, ADIS16505_USER_SCR_1, &WNew, 1) != STD_SUCCESS)
	{
		return STD_FAILED;
	}
	if (adis16505_read_reg(handle, ADIS16505_USER_SCR_1, &RNew, 1) != STD_SUCCESS)
	{
		return STD_FAILED;
	}

	if (WNew != RNew)
	{
		kprint("[a16505]: failed1, w - 0x%x-0x%x\r\n", WNew, RNew);
		return STD_FAILED;
	}

	WNew = 0xabcd;
	if (adis16505_write_reg(handle, ADIS16505_USER_SCR_1, &WNew, 1) != STD_SUCCESS)
	{
		return STD_FAILED;
	}

	if (adis16505_read_reg(handle, ADIS16505_USER_SCR_1, &RNew, 1) != STD_SUCCESS)
	{
		return STD_FAILED;
	}

	if (WNew != RNew)
	{
		kprint("[a16505]: failed2, w - 0x%x-0x%x\r\n", WNew, RNew);
		return STD_FAILED;
	}


	WNew = Old;
	if (adis16505_write_reg(handle, ADIS16505_USER_SCR_1, &WNew, 1) != STD_SUCCESS)
	{
		return STD_FAILED;
	}
	if (adis16505_read_reg(handle, ADIS16505_USER_SCR_1, &RNew, 1) != STD_SUCCESS)
	{
		return STD_FAILED;
	}

	if (WNew != RNew)
	{
		kprint("[a16505]: failed3, w - 0x%x-0x%x\r\n", WNew, RNew);
		return STD_FAILED;
	}

	if (WNew != RNew)
	{
		return STD_FAILED;
	}

#if 0
	__ADIS16505_BURST16_DATA_TypeDef sSensorData;
	if (adis16505_burst_read(handle, (uint16_t*)&sSensorData, sizeof(sSensorData) / 2) != STD_SUCCESS)
	{
		kprint("[a16505]: failed to burst read\r\n");
		return STD_FAILED;
	}

	kprint("[a16505]: DiagStat=0x%x \r\n", (unsigned int)sSensorData.DiagStat);
	kprint("[a16505]: rse=0x%x \r\n", (unsigned int)sSensorData.Res);
	kprint("[a16505]: g0=%d,g1=%d,g2=%d \r\n", sSensorData.Gyro[0], sSensorData.Gyro[1], sSensorData.Gyro[2]);
	kprint("[a16505]: a0=%d,a1=%d,a2=%d \r\n", sSensorData.Acc[0], sSensorData.Acc[1], sSensorData.Acc[2]);
	kprint("[a16505]: Temp=%d \r\n", sSensorData.Temp);
	kprint("[a16505]: DATA_CNTR=0x%x \r\n", sSensorData.DATA_CNTR);
	kprint("[a16505]: Checksum=0x%x \r\n", (unsigned int)sSensorData.Checksum);


	uint8_t* pt = (uint8_t*)&sSensorData;
	uint16_t sum = 0;
	for (uint8_t i = 0; i < (sizeof(sSensorData) - 4); i++)
	{
		sum += pt[i];
	}
	kprint("[a16505]: sum=0x%x \r\n", (unsigned int)sum);
#endif
	kprint("ok\r\n");
	return STD_SUCCESS;
}


uint8_t  adis16505_read_id(__GYRO_ACC_HW_DRIVER_TypeDef* handle, uint16_t* value)
{
	return adis16505_read_reg(handle, ADIS16505_PROD_ID, value, 1);
}

uint8_t adis16505_read_all_reg(__GYRO_ACC_HW_DRIVER_TypeDef* handle, __ADIS16505_MEMORY_MAP_TypeDef* mmt)
{

	adis16505_read_reg(handle, ADIS16505_XG_BIAS_LOW, (uint16_t*)&mmt->XGBias, sizeof(mmt->XGBias) / 2);
	adis16505_read_reg(handle, ADIS16505_YG_BIAS_LOW, (uint16_t*)&mmt->YGBias, sizeof(mmt->YGBias) / 2);
	adis16505_read_reg(handle, ADIS16505_ZG_BIAS_LOW, (uint16_t*)&mmt->ZGBias, sizeof(mmt->ZGBias) / 2);

	adis16505_read_reg(handle, ADIS16505_XA_BIAS_LOW, (uint16_t*)&mmt->XABias, sizeof(mmt->XABias) / 2);
	adis16505_read_reg(handle, ADIS16505_YA_BIAS_LOW, (uint16_t*)&mmt->YABias, sizeof(mmt->YABias) / 2);
	adis16505_read_reg(handle, ADIS16505_ZA_BIAS_LOW, (uint16_t*)&mmt->ZABias, sizeof(mmt->ZABias) / 2);


	adis16505_read_reg(handle, ADIS16505_FILT_CTRL, (uint16_t*)&mmt->FiltCtrl, sizeof(mmt->FiltCtrl) / 2);
	adis16505_read_reg(handle, ADIS16505_RANG_MDL, (uint16_t*)&mmt->RangMdl, sizeof(mmt->RangMdl) / 2);
	adis16505_read_reg(handle, ADIS16505_MSC_CTRL, (uint16_t*)&mmt->MscCtrl, sizeof(mmt->MscCtrl) / 2);

	adis16505_read_reg(handle, ADIS16505_UP_SCALE, (uint16_t*)&mmt->UpScale, sizeof(mmt->UpScale) / 2);
	adis16505_read_reg(handle, ADIS16505_DEC_RATE, (uint16_t*)&mmt->DecRate, sizeof(mmt->DecRate) / 2);

	adis16505_read_reg(handle, ADIS16505_FIRM_REV, (uint16_t*)&mmt->FirmRev, sizeof(mmt->FirmRev) / 2);
	adis16505_read_reg(handle, ADIS16505_FIRM_DM, (uint16_t*)&mmt->FirmDm, sizeof(mmt->FirmDm) / 2);

	adis16505_read_reg(handle, ADIS16505_FIRM_Y, (uint16_t*)&mmt->FirmY, sizeof(mmt->FirmY) / 2);
	adis16505_read_reg(handle, ADIS16505_PROD_ID, (uint16_t*)&mmt->ProdID, sizeof(mmt->ProdID) / 2);

	adis16505_read_reg(handle, ADIS16505_SERIAL_NUM, (uint16_t*)&mmt->SerialNum, sizeof(mmt->SerialNum) / 2);
	adis16505_read_reg(handle, ADIS16505_USER_SCR_1, (uint16_t*)&mmt->UserScr1, sizeof(mmt->UserScr1) / 2);
	adis16505_read_reg(handle, ADIS16505_USER_SCR_2, (uint16_t*)&mmt->UserScr2, sizeof(mmt->UserScr2) / 2);
	adis16505_read_reg(handle, ADIS16505_USER_SCR_3, (uint16_t*)&mmt->UserScr3, sizeof(mmt->UserScr3) / 2);

	adis16505_read_reg(handle, ADIS16505_TEMP_OUT, (uint16_t*)&mmt->TempOut, sizeof(mmt->TempOut) / 2);
	adis16505_read_reg(handle, ADIS16505_TIME_STAMP, (uint16_t*)&mmt->TimeStamp, sizeof(mmt->TimeStamp) / 2);
	adis16505_read_reg(handle, ADIS16505_DATA_CNTR, (uint16_t*)&mmt->DataCntr, sizeof(mmt->DataCntr) / 2);

	uint8_t ret = adis16505_read_reg(handle, ADIS16505_FLSHCNT_LOW, (uint16_t*)&mmt->FlashCnt, sizeof(mmt->FlashCnt) / 2);

#if 0
	//if (ret != STD_SUCCESS)
	//{
	//	kprint("failed to read\r\n");
	//	return ret;
	//}
	nprint("\r\n=======all reg begin======\r\n");

	nprint("DiagStat=%d\r\n", mmt->DiagStat);
	nprint("TempOut=%d\r\n", mmt->TempOut);
	nprint("TimeStamp=%d\r\n", mmt->TimeStamp);
	nprint("DataCntr=%d\r\n", mmt->DataCntr);

	nprint("XGBias=%d\r\n", (int)mmt->XGBias);
	nprint("YGBias=%d\r\n", (int)mmt->YGBias);
	nprint("ZGBias=%d\r\n", (int)mmt->ZGBias);
	nprint("XABias=%d\r\n", (int)mmt->XABias);
	nprint("YABias=%d\r\n", (int)mmt->YABias);
	nprint("ZABias=%d\r\n", (int)mmt->ZABias);

	nprint("R1=0x%x\r\n", (unsigned int)mmt->R1);

	nprint("FiltCtrl=0x%x\r\n", mmt->FiltCtrl);
	nprint("RangMdl=0x%x\r\n", mmt->RangMdl);
	nprint("MscCtrl=0x%x\r\n", mmt->MscCtrl);
	nprint("UpScale=0x%x\r\n", mmt->UpScale);

	nprint("DecRate=0x%x\r\n", mmt->DecRate);
	nprint("R2=0x%x\r\n", mmt->R2);

	nprint("GlobCmd=0x%x\r\n", mmt->GlobCmd);
	nprint("R3=0x%x\r\n", mmt->R3);

	nprint("FirmRev=0x%x\r\n", mmt->FirmRev);
	nprint("FirmDm=0x%x\r\n", mmt->FirmDm);

	nprint("FirmY=0x%x\r\n", mmt->FirmY);
	nprint("ProdID=0x%x\r\n", mmt->ProdID);

	nprint("SerialNum=0x%x\r\n", mmt->SerialNum);
	nprint("UserScr1=0x%x\r\n", mmt->UserScr1);

	nprint("UserScr2=0x%x\r\n", mmt->UserScr2);
	nprint("UserScr3=0x%x\r\n", mmt->UserScr3);

	nprint("FlashCnt=%u\r\n", (unsigned int)mmt->FlashCnt);
	nprint("=======all reg end======\r\n\r\n");
#endif
	return ret;
}

uint8_t adis16505_set_xa_bias_low(__GYRO_ACC_HW_DRIVER_TypeDef* handle, uint16_t newValue)
{
	uint8_t ret;
	uint16_t temp = 0;
	ret = adis16505_read_reg(handle, ADIS16505_XA_BIAS_LOW, &temp, 1);
	if (ret != STD_SUCCESS)
	{
		return ret;
	}
	if (temp == newValue)
	{
		kprint("it is the same\r\n");
	}
	else
	{

		temp = newValue;
		ret = adis16505_write_reg(handle, ADIS16505_XA_BIAS_LOW, &temp, 1);
		kprint("msc_ctrl=0x%x\r\n", temp);
	}
	return ret;

}

uint8_t adis16505_set_msc_ctrl(__GYRO_ACC_HW_DRIVER_TypeDef* handle, uint16_t newValue)
{
	uint8_t ret;
	uint16_t temp = 0;
	ret = adis16505_read_reg(handle, ADIS16505_MSC_CTRL, &temp, 1);
	if (ret != STD_SUCCESS)
	{
		return ret;
	}
	if (temp == newValue)
	{
		kprint("it is the same\r\n");
	}
	else
	{

		temp = newValue;
		ret = adis16505_write_reg(handle, ADIS16505_MSC_CTRL, &temp, 1);
		kprint("msc_ctrl=0x%x\r\n", temp);
	}
	return ret;

}

uint8_t adis16505_set_dec_rate(__GYRO_ACC_HW_DRIVER_TypeDef* handle, uint16_t newValue)
{
#define ADIS_MAX_FRE 2000
	uint8_t ret;
	uint16_t rate = newValue;

	if (rate > ADIS_MAX_FRE)
	{
		rate = ADIS_MAX_FRE;
	}
	rate = ADIS_MAX_FRE / rate - 1;
	uint16_t temp = 0;
	ret = adis16505_read_reg(handle, ADIS16505_DEC_RATE, &temp, 1);
	if (ret != STD_SUCCESS)
	{
		return ret;
	}
	if (temp == rate)
	{
		kprint("it is the same\r\n");
	}
	else
	{

		temp = rate;
		ret = adis16505_write_reg(handle, ADIS16505_DEC_RATE, &temp, 1);
		kprint("dec_rate=%u, fre=%u\r\n", temp, ADIS_MAX_FRE / (temp + 1));
	}
	return ret;

}

uint8_t adis1605_read_raw_data(__GYRO_ACC_HW_DRIVER_TypeDef* handle, uint8_t* value, uint16_t size)
{
	return handle->transmit_receive(value, value, size, TIMEOUT_DURATION);
}
/*******************************************************************************
END
*******************************************************************************/

