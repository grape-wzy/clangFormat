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
#include "spi_a_hw_if.h"

#ifndef ENABLE_SKT_SENSOR

#define ROLE_MASTER 0
#define ROLE_REF 1
#define ROLE_SENSOR 5

#define SABER_ACC_GYRO_DATA_RATE (100)
#define SABER_OUTPUT_RATE (20)

#define SABER_INPUT_BUFF_LENGHT SABER_ACC_GYRO_DATA_RATE

static __GYRO_ACC_CONFIG_TypeDef sGyroAccConfig = {
	.BitDepth = 0,
	.DataRate = SABER_ACC_GYRO_DATA_RATE,
	.Mode = 0,
	.AccSensitivity = 1,
	.GyroSensitivity = 1,
};

static __GYRO_ACC_HW_DRIVER_TypeDef sGyroAccHwDrv = {
	.init = spi_a_hw_init,
	.deinit = spi_a_hw_deinit,
	.reset = spi_a_hw_reset,
	.pwr_ctrl = pwr_ctrl,
	.transmit_receive = spi_a_hw_transmit_receive,
	.get_ready = spi_a_hw_get_ready,
	.enable_irq = spi_a_hw_enable_irq,
	.generate_swi = spi_a_hw_generate_swi,
	.set_cs = spi_a_hw_set_cs,
};

static __GYRO_ACC_DEV_DRIVER_TypeDef sGyroAccDevDrv = {
	.init = saber_drv_init,
	.deinit = saber_drv_deinit,
	.restore = NULL,
	.enable = saber_drv_start,
	.disable = saber_drv_stop,
	.irq_handle = saber_drv_isr_handle,
	.read_raw = saber_drv_read_raw,
};

static __GYRO_ACC_HANDLE_TypeDef sIMUHandle = {
	.hw_drv = &sGyroAccHwDrv,
	.dev_drv = &sGyroAccDevDrv,
	.config = &sGyroAccConfig,
};

void skt_proc(void);

static uint8_t sSaberIsStarted = 0;
static uint8_t sDeviceRole = ROLE_MASTER; // 设备角色
static uint32_t sSaberIsrDataCount = 0;	  // 中断总数
static __SKT_CALLBACK_TypeDef sSktCB;

// 缓存变量
static uint32_t sInPtr = 0, sOutPtr = 0;
static __SABER_RESULT_TypeDef sSaberRaw[SABER_INPUT_BUFF_LENGHT] __ALIGNED(4);
static __SKT_REF_DEVICE_BLE_DATA_TypeDef sSktRefFrame;		 // BLE上传的referece帧
static __SKT_MASTER_DEVICE_BLE_DATA_TypeDef sSktMasterFrame; // BLE上传的master帧

uint8_t skt_init(uint8_t device, uint32_t key_addr)
{
	UTIL_SEQ_RegTask(1 << CFG_TASK_AHRS_REQ_ID, UTIL_SEQ_RFU, skt_proc);
	sSaberIsStarted = 0;
	sDeviceRole = device;
	return STD_SUCCESS;
}

void skt_register_cb(void *cb)
{
	sSktCB = *(__SKT_CALLBACK_TypeDef *)cb;
}

uint8_t skt_get_role(void)
{
	return sDeviceRole;
}

void stk_get_version(uint8_t *version)
{

	sprintf((char *)version, "YM ALG V1.0.0");
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
	return STD_SUCCESS;
}

uint8_t skt_stop(void)
{
	if (!sSaberIsStarted)
	{
		return STD_SUCCESS;
	}
	sSaberIsStarted = 0;
	sIMUHandle.dev_drv->disable(&sIMUHandle);
	sIMUHandle.dev_drv->deinit(&sIMUHandle);
	return STD_SUCCESS;
}

uint8_t skt_update_ref_data(uint8_t *buff, uint8_t buffsize)
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
	// 获取采样缓存
	uint32_t FifoSize = skt_get_buffersize();
	if (FifoSize == 0)
	{
		return;
	}

	uint32_t x = sSaberIsrDataCount;
	if ((x % (SABER_ACC_GYRO_DATA_RATE / SABER_OUTPUT_RATE) == 0))
	{
		if (sSktCB.led_ctrl != NULL)
		{
			sSktCB.led_ctrl();
		}

		if (sSktCB.send != NULL)
		{

			if (sDeviceRole == ROLE_REF)
			{

				sSktRefFrame.HeadSize = 18;
				sSktRefFrame.HeadType = 0x81;
				sSktRefFrame.DataType = ROLE_REF;
				sSktRefFrame.TimeStampms = rtc_get_stamp_ms();
				sSktRefFrame.MotionMode = 0;
				sSktRefFrame.MotionProcess = 0;
				sSktRefFrame.MotionStatus = 0;
				sSktRefFrame.MotionFreezPercent = 0;
				sSktRefFrame.MotionError = FifoSize;
				// sSktRefFrame.DebugStatus = 0;
				sSktRefFrame.DebugStatus = 0;
				sSktRefFrame.DataSize = 40;
				sSktRefFrame.Q[0] = sSaberRaw[sOutPtr].q[0];
				sSktRefFrame.Q[1] = sSaberRaw[sOutPtr].q[1];
				sSktRefFrame.Q[2] = sSaberRaw[sOutPtr].q[2];
				sSktRefFrame.Q[3] = sSaberRaw[sOutPtr].q[3];
				// sSktRefFrame.Q[0] = quat[3];
				// sSktRefFrame.Q[1] = quat[1];
				// sSktRefFrame.Q[2] = quat[2];
				// sSktRefFrame.Q[3] = quat[0];
				sSktRefFrame.G[0] = sSaberRaw[sOutPtr].Gyro[0];
				sSktRefFrame.G[1] = sSaberRaw[sOutPtr].Gyro[1];
				sSktRefFrame.G[2] = sSaberRaw[sOutPtr].Gyro[2];
				sSktRefFrame.A[0] = sSaberRaw[sOutPtr].Acc[0];
				sSktRefFrame.A[1] = sSaberRaw[sOutPtr].Acc[1];
				sSktRefFrame.A[2] = sSaberRaw[sOutPtr].Acc[2];
				sSktCB.send((uint8_t *)&sSktRefFrame, sizeof(sSktRefFrame));
				// nprint("Q0=%f,Q1=%f,Q2=%f,Q3=%f\r\n", sSktRefFrame.Q[0], sSktRefFrame.Q[1], sSktRefFrame.Q[2], sSktRefFrame.Q[3]);
			}
			else // master
			{

				sSktMasterFrame.HeadSize = 18;
				sSktMasterFrame.HeadType = 0x81;
				sSktMasterFrame.DataType = ROLE_MASTER;
				sSktMasterFrame.TimeStampms = rtc_get_stamp_ms();
				sSktMasterFrame.MotionMode = 0;
				sSktMasterFrame.MotionProcess = 0;
				sSktMasterFrame.MotionStatus = 0;

				sSktMasterFrame.DebugStatus = (uint16_t)FifoSize;
				sSktMasterFrame.DataSize = 64;
				sSktMasterFrame.Q[0] = sSaberRaw[sOutPtr].q[0];
				sSktMasterFrame.Q[1] = sSaberRaw[sOutPtr].q[1];
				sSktMasterFrame.Q[2] = sSaberRaw[sOutPtr].q[2];
				sSktMasterFrame.Q[3] = sSaberRaw[sOutPtr].q[3];
				sSktMasterFrame.AdjustVV = sSaberRaw[sOutPtr].e[0];
				sSktMasterFrame.AdjustFE = sSaberRaw[sOutPtr].e[1];
				sSktMasterFrame.NavigateVV = sSaberRaw[sOutPtr].e[2];
				sSktMasterFrame.NavigateFE = (float)sSaberRaw[sOutPtr].Temperature;
				// sSktMasterFrame.Q[0] = quat[3];
				// sSktMasterFrame.Q[1] = quat[1];
				// sSktMasterFrame.Q[2] = quat[2];
				// sSktMasterFrame.Q[3] = quat[0];
				sSktMasterFrame.G[0] = sSaberRaw[sOutPtr].Gyro[0];
				sSktMasterFrame.G[1] = sSaberRaw[sOutPtr].Gyro[1];
				sSktMasterFrame.G[2] = sSaberRaw[sOutPtr].Gyro[2];
				sSktMasterFrame.A[0] = sSaberRaw[sOutPtr].Acc[0];
				sSktMasterFrame.A[1] = sSaberRaw[sOutPtr].Acc[1];
				sSktMasterFrame.A[2] = sSaberRaw[sOutPtr].Acc[2];

				sSktCB.send((uint8_t *)&sSktMasterFrame, sizeof(sSktMasterFrame));
			}
		}

#ifdef DEBUG
		static uint8_t count1 = 0;
		count1++;
		if (count1 == SABER_INPUT_BUFF_LENGHT)
		{
			count1 = 0;

			if (sDeviceRole == ROLE_MASTER)
			{
				kprint("1-A0=%f,A1=%f,A2=%f\r\n", sSaberRaw[sOutPtr].Acc[0], sSaberRaw[sOutPtr].Acc[1], sSaberRaw[sOutPtr].Acc[2]);
				// nprint("other=%u, Prcessret=0x%x\r\n", (unsigned int)sSktRefFrame.Other, sSktRefFrame.MotionProcess);
				// nprint("Q0=%f,Q1=%f,Q2=%f,Q3=%f\r\n", sSktRefFrame.Q[0], sSktRefFrame.Q[1], sSktRefFrame.Q[2], sSktRefFrame.Q[3]);
				// kprint("G0=%f,G1=%f,G2=%f\r\n", sAhrsOutput.gravity[0], sAhrsOutput.gravity[1], sAhrsOutput.gravity[2]);
				// kprint("A0=%f,A1=%f,A2=%f\r\n", sAhrsOutput.linear_acceleration[0], sAhrsOutput.linear_acceleration[1], sAhrsOutput.linear_acceleration[2]);
				// kprint("id=%u,G0=%f,G1=%f,G2=%f\r\n", (unsigned int)sOutPtr, (float)(sSktInput[sOutPtr].G[0] / 25), (float)(sSktInput[sOutPtr].G[1] / 25), (float)(sSktInput[sOutPtr].G[2] / 25));
				// kprint("id=%u,A0=%f,A1=%f,A2=%f\r\n", (unsigned int)sOutPtr, (float)(sSktInput[sOutPtr].A[0] / 9806.65), (float)(sSktInput[sOutPtr].A[1] / 9806.65), (float)(sSktInput[sOutPtr].A[2] / 9806.65));

				// kprint("count=%u, Prcessret=0x%x\r\n", (unsigned int)sSktInput[sOutPtr].DataTick, Prcessret);
			}
			else
			{

				// tprint("Mode=0x%x, proc=0x%x, error=%d\r\n", sSktMasterFrame.MotionMode, sSktMasterFrame.MotionProcess, (int)sSktMasterFrame.MotionError);
			}
		}
#endif
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
	__SABER_OPCODE_TypeDef ret = (__SABER_OPCODE_TypeDef)sIMUHandle.dev_drv->irq_handle(&sIMUHandle);

	if ((!sSaberIsStarted) || (ret != SAVER_OPCODE_WAIT_FOR_DATA))
	{
		return;
	}

	sSaberIsrDataCount++;

	uint32_t inp = sInPtr;
	sIMUHandle.dev_drv->read_raw(&sIMUHandle, (uint8_t *)&sSaberRaw[inp], sizeof(sSaberRaw[inp]));
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

uint8_t skt_test(void)
{
	return saber_drv_test(&sIMUHandle);
}

#endif
/*******************************************************************************
END
*******************************************************************************/
