/*******************************************************************************
* file    ahrs_if.c
* author  mackgim
* version 1.0.0
* date
* brief   combine accelerometer and gyroscope readings in order to obtain
accurate information about the inclination of your device relative
to the ground plane
*******************************************************************************/


#include "ahrs.h"
#include "imu_if.h"
#include "standard_lib.h"
#include "motion_fx_manager.h"



#define AHRS_GYRO_SENSITIVITY		((float)(25.0f)) // mdps/bit
#define AHRS_ACC_SENSITIVITY		((float)(2.45f)) //mg/bit

#define AHRS_ACC_GYRO_DATA_RATE		(500)
#define AHRS_ACC_GYRO_BIT_DEPTH		(16)
#define AHRS_ALGO_DELTA_TIME		((float)1/AHRS_ACC_GYRO_DATA_RATE)

#define AHRS_MAG_SENSITIVITY		((float)(1.5f))   //mguass/bit
#define AHRS_MAG_DATA_RATE			(50)

#pragma region 函数

typedef struct {
	float G[3];
	float A[3];
}__AHRS_INPUT_TypeDef;


typedef struct {
	int32_t G[3];
	int32_t A[3];
}__SENSOR_32BIT_RAW_DATA_TypeDef;

typedef struct {
	int16_t G[3];
	int16_t A[3];
}__SENSOR_16BIT_RAW_DATA_TypeDef;

typedef __I__packed struct
{
	uint8_t HeadSize;
	uint8_t HeadType;
	uint8_t DataType;
	uint64_t TimeStampms; //时间戳
	uint32_t ActionFlag;//动作标志位
	uint8_t Power;//
	uint8_t DataSize;// 字节数
	float Q[4];
	float Displacement;
	float G[3];
	float A[3];
} __I__PACKED __SKT_REF_DEVICE_BLE_DATA_TypeDef;

typedef struct {
	uint8_t(*init)   (void);
	uint8_t(*start)   (void);
	uint8_t(*stop)   (void);
	uint8_t(*run)(void*, void*, float);
	uint8_t(*calib)(int32_t);
}__AHRS_ALG_API_TypeDef;






static  __SKT_REF_DEVICE_BLE_DATA_TypeDef sSktRefFrame;
#define SKT_INPUT_BUFF_LENGHT	AHRS_ACC_GYRO_DATA_RATE
static uint32_t sInPtr = 0, sOutPtr = 0;;
static  __AHRS_INPUT_TypeDef sAhrsInput[SKT_INPUT_BUFF_LENGHT];
static __AHRS_ALG_API_TypeDef sAhrsAlgApi = {
	.init = MotionFX_manager_init,
	.start = MotionFX_manager_start,
	.stop = MotionFX_manager_stop,
	.run = MotionFX_manager_run,
	.calib = MotionFX_manager_calib,
};

static MFX_output_t sAhrsOutput;
static uint8_t sStartahrsFlag = 0;


#pragma endregion



#pragma region 函数


void ahrs_proc(void);

#pragma endregion



uint8_t ahrs_init(void)
{
	__GYRO_ACC_CONFIG_TypeDef acc_gyro_config = { AHRS_ACC_GYRO_BIT_DEPTH, AHRS_ACC_GYRO_DATA_RATE, AHRS_GYRO_SENSITIVITY, AHRS_ACC_SENSITIVITY };
	imu_init(&acc_gyro_config);
	sAhrsAlgApi.init();
	UTIL_SEQ_RegTask(1 << CFG_TASK_AHRS_REQ_ID, UTIL_SEQ_RFU, ahrs_proc);
	return STD_SUCCESS;
}


uint8_t ahrs_start(void)
{
	sInPtr = sOutPtr = 0;
	sAhrsAlgApi.start();
	imu_enable();
	kprint("start\r\n");
	sStartahrsFlag = 1;
	return STD_SUCCESS;
}

uint8_t ahrs_stop(void)
{
	sStartahrsFlag = 0;
	imu_disable();
	sAhrsAlgApi.stop();
	kprint("stop\r\n");
	return STD_SUCCESS;
}

void ahrs_calib(int32_t en)
{
	sAhrsAlgApi.calib(en);
	kprint("calib %d\r\n", (int)en);
}


typedef uint8_t(*AHRS_SEND_CALLBACK_TYPE) (uint8_t*, uint8_t);
static AHRS_SEND_CALLBACK_TYPE send_callback;
void ahrs_register_cb(void* cb)
{
	send_callback = cb;
}


void ahrs_proc(void)
{
	if (sStartahrsFlag == 0) return;
	if (sInPtr != sOutPtr)
	{
		MFX_input_t x;


		x.acc[0] = sAhrsInput[sOutPtr].A[0] / 9806.65;
		x.acc[1] = sAhrsInput[sOutPtr].A[1] / 9806.65;
		x.acc[2] = sAhrsInput[sOutPtr].A[2] / 9806.65;

		x.gyro[0] = sAhrsInput[sOutPtr].G[0] / 1000;
		x.gyro[1] = sAhrsInput[sOutPtr].G[1] / 1000;
		x.gyro[2] = sAhrsInput[sOutPtr].G[2] / 1000;


		sAhrsAlgApi.run(&x, &sAhrsOutput, AHRS_ALGO_DELTA_TIME);


		static uint8_t count = 0;
		count++;
		if (count == (AHRS_ACC_GYRO_DATA_RATE / 10))
		{
			count = 0;
			if (send_callback != NULL)
			{
				sSktRefFrame.HeadSize = 15;
				sSktRefFrame.HeadType = 0x81;
				sSktRefFrame.DataType = 0x01;
				sSktRefFrame.TimeStampms = rtc_get_stamp_ms();
				sSktRefFrame.ActionFlag = 0;
				sSktRefFrame.Power = 100;
				sSktRefFrame.DataSize = 44;
				sSktRefFrame.Q[0] =  sInPtr> sOutPtr ?sInPtr- sOutPtr: SKT_INPUT_BUFF_LENGHT - sOutPtr + sInPtr; //sAhrsOutput.quaternion[3]; //
				sSktRefFrame.Q[1] = sAhrsOutput.quaternion[0];
				sSktRefFrame.Q[2] = sAhrsOutput.quaternion[1];
				sSktRefFrame.Q[3] = sAhrsOutput.quaternion[2];
				sSktRefFrame.Displacement = 0;
				sSktRefFrame.G[0] = sAhrsInput[sOutPtr].G[0];
				sSktRefFrame.G[1] = sAhrsInput[sOutPtr].G[1];
				sSktRefFrame.G[2] = sAhrsInput[sOutPtr].G[2];
				sSktRefFrame.A[0] = sAhrsInput[sOutPtr].A[0];
				sSktRefFrame.A[1] = sAhrsInput[sOutPtr].A[1];
				sSktRefFrame.A[2] = sAhrsInput[sOutPtr].A[2];
				send_callback((uint8_t*)&sSktRefFrame, sizeof(sSktRefFrame));
			}

#ifdef DEBUG
			static uint8_t count1 = 0;
			count1++;
			if (count1 == 10)
			{
				count1 = 0;
				//kprint("Q0=%f,Q1=%f,Q2=%f,Q3=%f\r\n", sAhrsOutput.quaternion[0], sAhrsOutput.quaternion[1], sAhrsOutput.quaternion[2], sAhrsOutput.quaternion[3]);
				//kprint("G0=%f,G1=%f,G2=%f\r\n", sAhrsOutput.gravity[0], sAhrsOutput.gravity[1], sAhrsOutput.gravity[2]);
				//kprint("A0=%f,A1=%f,A2=%f\r\n", sAhrsOutput.linear_acceleration[0], sAhrsOutput.linear_acceleration[1], sAhrsOutput.linear_acceleration[2]);
				//kprint("id=%u,G0=%f,G1=%f,G2=%f\r\n", (unsigned int)sOutPtr, (float)(sAhrsInput[sOutPtr].G[0] / 25), (float)(sAhrsInput[sOutPtr].G[1] / 25), (float)(sAhrsInput[sOutPtr].G[2] / 25));
				//kprint("id=%u,A0=%f,A1=%f,A2=%f\r\n", (unsigned int)sOutPtr, (float)(sAhrsInput[sOutPtr].A[0] / 9806.65), (float)(sAhrsInput[sOutPtr].A[1] / 9806.65), (float)(sAhrsInput[sOutPtr].A[2] / 9806.65));
			}


#endif
		}

		sOutPtr++;
		if (sOutPtr == SKT_INPUT_BUFF_LENGHT)
		{
			sOutPtr = 0;
		}
	}

	if (sInPtr != sOutPtr)
	{
		UTIL_SEQ_SetTask(1 << CFG_TASK_AHRS_REQ_ID, CFG_PRIO_NBR_5);
	}

}


void ahrs_isr(void)
{
	if (sStartahrsFlag == 0) return;

	//kprint("hahah\r\n");
	
	static __ADIS16505_BURST16_DATA_TypeDef raw;
	raw.TxCmd = 0x6800;
	uint8_t ret = imu_get_raw((uint8_t *) &raw, sizeof(raw)/2);
	if (ret != STD_SUCCESS)
	{
		kprint("error\r\n");
		return;
	}
	//nprint("%d,%d,%d\r\n", (int16_t)raw.Gyro[0], (int16_t)raw.Gyro[1], (int16_t)raw.Gyro[2]);
	//kprint("raw=0x%x\r\n", (unsigned int)&raw);
	//return;
#if 1
	uint8_t* ptr = (uint8_t*)&raw.DiagStat;
	uint16_t checksum = 0;
	for (uint32_t i = 0; i < (BURST16_DATA_LENGTH - 2 - 2); i++)//减去帧头，以及校验码
	{
		checksum += ptr[i];
}
	if ((checksum != raw.Checksum) ||( checksum == 0))
	{
		kprint("check error, %u-%u\r\n", checksum, raw.Checksum);
		return;
	}

	if (raw.DiagStat != 0)
	{
		kprint("0x%x\r\n", raw.DiagStat);
	}
	//static uint16_t lastchecksum = 0;
	//if (lastchecksum != checksum)
	//{
	//	nprint("c=%u\r\n", checksum);
	//	lastchecksum = checksum;
	//}
#endif

	uint32_t inp = sInPtr;
	for (uint8_t i = 0; i < 3; i++)
	{
#ifdef ENABLE_IMU_32BIT_RESOLUTION
		sAhrsInput[inp].G[i] = (float)((double)raw->G[i] * AHRS_GYRO_SENSITIVITY / 65536);
		sAhrsInput[inp].A[i] = (float)((double)raw->A[i] * AHRS_ACC_SENSITIVITY / 65536);
#else
		sAhrsInput[inp].G[i] = (float)((float)raw.G[i] * AHRS_GYRO_SENSITIVITY);
		sAhrsInput[inp].A[i] = (float)((float)raw.A[i] * AHRS_ACC_SENSITIVITY);
#endif
	}
	if ((sAhrsInput[inp].G[0] == 0) && (sAhrsInput[inp].G[1] == 0) && (sAhrsInput[inp].G[2] == 0))
	{
		kprint("G is 0, ck=%u, s=0x%x, a=%d, t=%d\r\n", checksum, raw.DiagStat, raw.A[0], raw.Temp);

	}
	if ((sAhrsInput[inp].A[0] == 0) && (sAhrsInput[inp].A[1] == 0) && (sAhrsInput[inp].A[2] == 0))
	{
		kprint("A is 0\r\n");

	}

	inp++;
	if (inp == SKT_INPUT_BUFF_LENGHT)
	{
		inp = 0;
	}
	if (inp == sOutPtr)
	{
		kprint("over\r\n");
	}
	else
	{
		sInPtr = inp;
	}
	UTIL_SEQ_SetTask(1 << CFG_TASK_AHRS_REQ_ID, CFG_PRIO_NBR_5);
}