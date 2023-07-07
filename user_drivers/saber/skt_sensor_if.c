/*******************************************************************************
* file     saber_if.c
* author   mackgim
* version  V1.0.0
* date
* brief ：
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/

#include "standard_lib.h"
#include "skt_if.h"
#include "saber_drv.h"
#include "saber_config.h"
#include "spi_if.h"


#ifdef ENABLE_SKT_SENSOR


//返回结果
typedef __I__packed struct
{
	uint8_t HeadSize;
	uint8_t HeadType;
	uint8_t DataType;
	uint8_t DataSize;// 字节数
	float Value[60];
} __I__PACKED __SKT_DEVICE_SENSOR_BLE_DATA_TypeDef;


#define SABER_ACC_GYRO_DATA_RATE			(400)
#define SABER_OUTPUT_RATE					((1000 * SABER_ACC_GYRO_DATA_RATE)/(1000 * 10))// (40)



#define SABER_INPUT_BUFF_LENGHT				10


static __GYRO_ACC_CONFIG_TypeDef sGyroAccConfig = {
	.BitDepth = 0,
	.DataRate = SABER_ACC_GYRO_DATA_RATE,
	.Mode = 2,
	.AccSensitivity = 1,
	.GyroSensitivity = 1,
};
static __GYRO_ACC_HW_DRIVER_TypeDef sGyroAccHwDrv =
{
		.init = spi_a_hw_init,
		.deinit = spi_a_hw_deinit,
		.reset = spi_a_hw_reset,
		.pwr_ctrl = pwr_ctrl,
		.transmit_receive = spi_a_hw_transmit_receive,
		.get_ready = spi_a_hw_get_ready,
		.enable_irq = spi_a_hw_enable_irq,
		.generate_swi = spi_a_hw_generate_swi,
		.set_cs = spi_a_hw_set_cs
};
static __GYRO_ACC_DEV_DRIVER_TypeDef sGyroAccDevDrv
= {
	.init = saber_drv_init,
	.deinit = saber_drv_deinit,
	.restore = NULL,
	.enable = saber_drv_start,
	.disable = saber_drv_stop,
	.irq_handle = saber_drv_isr_handle,
	.read_raw = saber_drv_read_raw,
};

static __GYRO_ACC_HANDLE_TypeDef sIMUHandle =
{
	.hw_drv = &sGyroAccHwDrv,
	.dev_drv = &sGyroAccDevDrv,
	.config = &sGyroAccConfig,
};

void skt_proc(void);

static uint8_t sSaberIsStarted = 0;
static uint32_t sSaberIsrDataCount = 0;//中断总数
static __SKT_CALLBACK_TypeDef sSktCB;

//缓存变量
static uint32_t sInPtr = 0, sOutPtr = 0;
static  __SABER_RESULT_TypeDef sSaberRaw  __ALIGNED(4);
static __SKT_DEVICE_SENSOR_BLE_DATA_TypeDef sSktSensorFrame[SABER_INPUT_BUFF_LENGHT];


uint8_t skt_init(uint8_t device, uint32_t key_addr)
{
	UTIL_SEQ_RegTask(1 << CFG_TASK_AHRS_REQ_ID, UTIL_SEQ_RFU, skt_proc);
	sSaberIsStarted = 0;
	return STD_SUCCESS;
}


void skt_register_cb(void* cb)
{
	sSktCB = *(__SKT_CALLBACK_TypeDef*)cb;
}

uint8_t skt_get_role(void)
{
	return 1;
}

void stk_get_version(uint8_t* version)
{

	sprintf((char*)version, "YM ALG V1.0.0");
}

uint8_t skt_set_config(__FLASH_SKT_CONFIG_TypeDef config)
{
	return STD_SUCCESS;
}

uint8_t skt_is_check(void)
{
	return true;
}

uint8_t skt_check_keywork(uint32_t key_addr)
{
	return true;
}

uint8_t skt_set_mode(__SKT_MODE_TypeDef work_mode)
{
	return STD_SUCCESS;
}

uint8_t skt_set_tibia_reg_pos(int32_t pos)
{
	return STD_SUCCESS;
}

uint8_t skt_set_navigation_angle(__SKT_NAVIGATION_ANGLE_TypeDef nav)
{
	return STD_SUCCESS;
}

uint8_t skt_start(void)
{
	if (sSaberIsStarted)
	{
		return STD_SUCCESS;
	}


	uint8_t ret = sIMUHandle.dev_drv->init(&sIMUHandle);
	if (ret != STD_SUCCESS)
	{
		kprint("failed to init\r\n");
		return ret;
	}
	ret = sIMUHandle.dev_drv->enable(&sIMUHandle);
	if (ret != STD_SUCCESS)
	{
		kprint("failed to start\r\n");
		return ret;
	}
	sSaberIsStarted = 1;
	sSaberIsrDataCount = 0;
	return STD_SUCCESS;
}


uint8_t skt_stop(void)
{
	if (!sSaberIsStarted)
	{
		return STD_SUCCESS;
	}
	sSaberIsStarted = 0;
	sSaberIsrDataCount = 0;
	sIMUHandle.dev_drv->disable(&sIMUHandle);
	sIMUHandle.dev_drv->deinit(&sIMUHandle);
	return STD_SUCCESS;
}

uint8_t skt_update_ref_data(uint8_t* buff, uint8_t buffsize)
{
	return STD_SUCCESS;
}

uint32_t skt_get_buffersize(void)
{
	ATOMIC_SECTION_BEGIN();
	uint32_t in = sInPtr;
	uint32_t out = sOutPtr;
	uint32_t size = in < out ? (SABER_INPUT_BUFF_LENGHT - out + in) : (in - out);
	ATOMIC_SECTION_END();
	return size;
}

void skt_proc(void)
{

	if (sInPtr == sOutPtr)
	{
		return;
	}

	uint32_t outp = sOutPtr;

	sSktSensorFrame[outp].HeadSize = 2;
	sSktSensorFrame[outp].HeadType = 0x81;
	sSktSensorFrame[outp].DataType = 0x05;
	sSktSensorFrame[outp].DataSize = 240;


	static uint32_t count = 0;

	count++;
	if (count == SABER_OUTPUT_RATE)
	{
		//kprint("\r\n");
		count = 0;
		if (sSktCB.led_ctrl != NULL)
		{
			sSktCB.led_ctrl();
		}
	}


	if (sSktCB.send != NULL)
	{
		uint8_t ret = sSktCB.send((uint8_t*)&sSktSensorFrame[outp], sizeof(sSktSensorFrame[outp]));
		if (ret == STD_SUCCESS)
		{

		}
	}

	sOutPtr++;
	if (sOutPtr == SABER_INPUT_BUFF_LENGHT)
	{
		sOutPtr = 0;
	}


	if (sInPtr != sOutPtr)
	{
		UTIL_SEQ_SetTask(1 << CFG_TASK_AHRS_REQ_ID, CFG_PRIO_NBR_3);
	}
}

void skt_irq_handle(void)
{

	do
	{

		__SABER_OPCODE_TypeDef ret = (__SABER_OPCODE_TypeDef)sIMUHandle.dev_drv->irq_handle(&sIMUHandle);

		if ((!sSaberIsStarted) || (ret != SAVER_OPCODE_WAIT_FOR_DATA))
		{
			return;
		}

		if (ret == SAVER_OPCODE_NULL)
		{
			return;
		}


		if ((sSaberIsrDataCount + 6) > 60)
		{
			kprint("count is too big, %u\r\n", (unsigned int)(sSaberIsrDataCount + 6));
			return;
		}

		uint8_t res = sIMUHandle.dev_drv->read_raw(&sIMUHandle, (uint8_t*)&sSaberRaw, sizeof(sSaberRaw));
		if (res != STD_SUCCESS)
		{
			return;
		}
		uint32_t inp = sInPtr;
		sSktSensorFrame[inp].Value[sSaberIsrDataCount++] = sSaberRaw.Gyro[0];
		sSktSensorFrame[inp].Value[sSaberIsrDataCount++] = sSaberRaw.Gyro[1];
		sSktSensorFrame[inp].Value[sSaberIsrDataCount++] = sSaberRaw.Gyro[2];
		sSktSensorFrame[inp].Value[sSaberIsrDataCount++] = sSaberRaw.Acc[0];
		sSktSensorFrame[inp].Value[sSaberIsrDataCount++] = sSaberRaw.Acc[1];
		sSktSensorFrame[inp].Value[sSaberIsrDataCount++] = sSaberRaw.Acc[2];

		if (sSaberIsrDataCount == 60)
		{
			sSaberIsrDataCount = 0;

			inp++;
			if (inp == SABER_INPUT_BUFF_LENGHT)
			{
				inp = 0;
			}
			if (inp == sOutPtr)
			{
				nprint("o\r\n");
			}
			else
			{
				sInPtr = inp;
			}

			UTIL_SEQ_SetTask(1 << CFG_TASK_AHRS_REQ_ID, CFG_PRIO_NBR_3);
		}
	} while (1);
}




uint8_t skt_test(void)
{
	return saber_drv_test(&sIMUHandle);
}


#endif

/*******************************************************************************
END
*******************************************************************************/
