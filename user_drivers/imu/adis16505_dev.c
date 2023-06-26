/*******************************************************************************
* file     imu_if.c
* author   mackgim
* version  V1.0.0
* date
* brief ：
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/


#include "imu_if.h"
#include "standard_lib.h"
#include "adis16505_drv.h"
#include "adis16505_dev.h"


uint8_t adis16505_dev_init(void* handle)
{
	__GYRO_ACC_HANDLE_TypeDef* gah = (__GYRO_ACC_HANDLE_TypeDef*)handle;
	uint8_t ret;

	uint16_t id = 0;
	ret = adis16505_read_id(gah->hw_drv, &id);
	if (ret != STD_SUCCESS)
	{
		return  STD_FAILED;
	}
	kprint("id=0x%x, real id=0x%x\r\n", id, ADIS16505_ID);

	ret = adis16505_check(gah->hw_drv);
	if (ret != STD_SUCCESS)
	{
		return  STD_FAILED;
	}
	log_flush();

#ifdef DEBUG
	__ADIS16505_MEMORY_MAP_TypeDef mmt = { 0 };
	ret = adis16505_read_all_reg(gah->hw_drv, &mmt);
	if (ret != STD_SUCCESS)
	{
		return  STD_FAILED;
	}

#endif
	ret = adis16505_set_dec_rate(gah->hw_drv, gah->config->DataRate);
	if (ret != STD_SUCCESS)
	{
		return  STD_FAILED;
	}
	Clock_Wait(2);
	__ADIS16505_MSC_CTRL_BURST_t burst = __ADIS16505_MSC_CTRL_BURST16;
	if (gah->config->BitDepth == 32)
	{
		burst = __ADIS16505_MSC_CTRL_BURST32;
	}

	uint16_t msc_ctrl = burst | __ADIS16505_MSC_CTRL_BURST_SEL_DISABLE
		| __ADIS16505_MSC_CTRL_LINEAR_ENABLE | __ADIS16505_MSC_CTRL_PERCUSSION_ENABLE
		| __ADIS16505_MSC_CTRL_SENS_BW_DISABLE | __ADIS16505_MSC_CTRL_SYNC_MODE_INTERNAL
		| __ADIS16505_MSC_CTRL_SYNC_POLARITY_FALLING | __ADIS16505_MSC_CTRL_DR_POLARITY_HIGH;
	ret = adis16505_set_msc_ctrl(gah->hw_drv, msc_ctrl);
	if (ret != STD_SUCCESS)
	{
		return  STD_FAILED;
	}
	Clock_Wait(2);

#ifdef DEBUG
	ret = adis16505_read_all_reg(gah->hw_drv, &mmt);
	if (ret != STD_SUCCESS)
	{
		return  STD_FAILED;
	}
#endif

	return ret;
}

uint8_t adis16505_dev_deinit(void* handle)
{
	return STD_SUCCESS;
}

uint8_t adis16505_dev_restore_config(void* handle)
{
	__GYRO_ACC_HANDLE_TypeDef* gah = (__GYRO_ACC_HANDLE_TypeDef*)handle;
	uint8_t ret;

	ret = adis16505_set_dec_rate(gah->hw_drv, gah->config->DataRate);
	Clock_Wait(2);
	if (ret != STD_SUCCESS)
	{
		return  STD_FAILED;
	}
	__ADIS16505_MSC_CTRL_BURST_t burst = __ADIS16505_MSC_CTRL_BURST16;
	if (gah->config->BitDepth == 32)
	{
		burst = __ADIS16505_MSC_CTRL_BURST32;
	}

	uint16_t msc_ctrl = burst | __ADIS16505_MSC_CTRL_BURST_SEL_DISABLE
		| __ADIS16505_MSC_CTRL_LINEAR_ENABLE | __ADIS16505_MSC_CTRL_PERCUSSION_ENABLE
		| __ADIS16505_MSC_CTRL_SENS_BW_DISABLE | __ADIS16505_MSC_CTRL_SYNC_MODE_INTERNAL
		| __ADIS16505_MSC_CTRL_SYNC_POLARITY_FALLING | __ADIS16505_MSC_CTRL_DR_POLARITY_HIGH;
	ret = adis16505_set_msc_ctrl(gah->hw_drv, msc_ctrl);
	Clock_Wait(2);
	if (ret != STD_SUCCESS)
	{
		return  STD_FAILED;
	}
	return ret;
}


uint8_t adis16505_dev_read_raw(void* handle, uint8_t* value, uint16_t size)
{
	__GYRO_ACC_HANDLE_TypeDef* gah = (__GYRO_ACC_HANDLE_TypeDef*)handle;
	return adis1605_read_raw_data(gah->hw_drv, value, size);//16 bit为基本单位, size 为16bit的个数
}


uint8_t adis16505_dev_read_all_reg(void* handle)
{
	__GYRO_ACC_HANDLE_TypeDef* gah = (__GYRO_ACC_HANDLE_TypeDef*)handle;
	__ADIS16505_MEMORY_MAP_TypeDef mmt = { 0 };
	return adis16505_read_all_reg(gah->hw_drv, &mmt);
}


//uint8_t imu_read_raw_result(uint16_t *value, uint16_t size)
//{
//	return adis1605_read_raw_data(&imu_hw_drv, value, size/2); //16 bit为基本单位
//}

/*******************************************************************************
END
*******************************************************************************/

