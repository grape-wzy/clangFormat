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
#include "motion_fx.h"
#include "platform.h"

#define AHRS_GYRO_SCALE           (2)             // GYPO_SCALE_500dps
#define AHRS_ACC_SCALE            (0)             // ACC_SCALE_2g
#define AHRS_ACC_GYRO_DATA_RATE   (400)           // 500Hz
#define AHRS_ALGO_DELTA_TIME      ((float)1.0 / AHRS_ACC_GYRO_DATA_RATE)
#define AHRS_ACC_GYRO_POWER_MODE  ((uint16_t)(1)) // high_performance, 0 disable, 1 enable

#define AHRS_DATA_REPORT_MIN_TIME (20)

#pragma region 函数

typedef struct {
    float    G[3];
    float    A[3];
    uint64_t time_point;
} __AHRS_INPUT_TypeDef;

typedef struct {
    uint8_t (*init)(void);
    uint8_t (*start)(void);
    uint8_t (*stop)(void);
    uint8_t (*run)(void *, void *, float);
    uint8_t (*calib)(int32_t);
} __AHRS_ALG_API_TypeDef;

typedef uint8_t (*AHRS_SEND_CALLBACK_TYPE)(uint8_t *, uint8_t);

#define IMU_INPUT_BUFF_LENGTH AHRS_ACC_GYRO_DATA_RATE

static uint32_t sInPtr = 0, sOutPtr = 0;

static __AHRS_INPUT_TypeDef sAhrsInput[IMU_INPUT_BUFF_LENGTH];

static __AHRS_ALG_API_TypeDef sAhrsAlgApi = {
    .init  = MotionFX_manager_init,
    .start = MotionFX_manager_start,
    .stop  = MotionFX_manager_stop,
    .run   = MotionFX_manager_run,
    .calib = MotionFX_manager_calib,
};

static MFX_output_t sAhrsOutput;
static uint8_t      sAhrsStartFlag = 0;
static uint8_t      sTSAhrsRunID;

#pragma endregion

#pragma region 函数

static AHRS_SEND_CALLBACK_TYPE send_callback;

void ahrs_register_cb(void *cb)
{
    send_callback = cb;
}

#pragma endregion

static void ahrs_run_ts_callback(void)
{
    UTIL_SEQ_SetTask(1 << CFG_TASK_AHRS_READ_ID, CFG_SCH_PRIO_0);
}

void ahrs_read_task(void)
{
    IMU_RESULT_TypeDef ahrs_result_buf;
    uint32_t           inp;

    if (sAhrsStartFlag == 0) return;

    if (STD_SUCCESS != imu_get_result(&ahrs_result_buf, 1))
        return;

    if ((ahrs_result_buf.acc[0] == 0) && (ahrs_result_buf.acc[1] == 0) && (ahrs_result_buf.acc[2] == 0)) {
        kprint("A is 0\r\n");
        return;
    }
    if ((ahrs_result_buf.gyro[0] == 0) && (ahrs_result_buf.gyro[1] == 0) && (ahrs_result_buf.gyro[2] == 0)) {
        kprint("G is 0\r\n");
        return;
    }

    inp = sInPtr;

    sAhrsInput[inp].time_point = Clock_Time();

    for (uint8_t i = 0; i < 3; i++) {
        sAhrsInput[inp].G[i] = (float)(ahrs_result_buf.gyro[i] / 1000.00);
        sAhrsInput[inp].A[i] = (float)(ahrs_result_buf.acc[i] / 1000.00);
    }

    inp++;
    if (inp == IMU_INPUT_BUFF_LENGTH) {
        inp = 0;
    }
    if (inp == sOutPtr) {
        kprint("read over\r\n");
    } else {
        sInPtr = inp;
    }

    /* start to process the imu data */
    UTIL_SEQ_SetTask(1 << CFG_TASK_AHRS_PROC_ID, CFG_PRIO_NBR_1);
}

/* process the imu data and report it */
void ahrs_proc_task(void)
{
    // LEDG_CTRL(1);
    MFX_input_t mfx_input_buf;

    static uint64_t cal_time = 0, report_time_point_recode = 0;
    static uint64_t current_time = 0;
    float           during_time  = 0.00;

    if (sAhrsStartFlag == 0) return;

    if (sInPtr != sOutPtr) {
        /* run motion fx */
        mfx_input_buf.acc[0] = sAhrsInput[sOutPtr].A[0];
        mfx_input_buf.acc[1] = sAhrsInput[sOutPtr].A[1];
        mfx_input_buf.acc[2] = sAhrsInput[sOutPtr].A[2];

        mfx_input_buf.gyro[0] = sAhrsInput[sOutPtr].G[0];
        mfx_input_buf.gyro[1] = sAhrsInput[sOutPtr].G[1];
        mfx_input_buf.gyro[2] = sAhrsInput[sOutPtr].G[2];

        during_time = (float)(sAhrsInput[sOutPtr].time_point - cal_time) / 1000.00;
        cal_time    = sAhrsInput[sOutPtr].time_point;

        sAhrsAlgApi.run(&mfx_input_buf, &sAhrsOutput, during_time);

        // if (send_callback != NULL)
        {
            if (sAhrsInput[sOutPtr].time_point > AHRS_DATA_REPORT_MIN_TIME + report_time_point_recode) {
                report_time_point_recode = sAhrsInput[sOutPtr].time_point;

                // sSktRefFrame.HeadSize = 15;
                // sSktRefFrame.HeadType = 0x81;
                // sSktRefFrame.DataType = 0x01;
                // sSktRefFrame.TimeStampms = rtc_get_stamp_ms();
                // sSktRefFrame.ActionFlag = 0;
                // sSktRefFrame.Power = 100;
                // sSktRefFrame.DataSize = 44;
                // sSktRefFrame.Q[0] =  sInPtr> sOutPtr ?sInPtr- sOutPtr: IMU_INPUT_BUFF_LENGTH - sOutPtr + sInPtr; //sAhrsOutput.quaternion[3]; //
                // sSktRefFrame.Q[1] = sAhrsOutput.quaternion[0];
                // sSktRefFrame.Q[2] = sAhrsOutput.quaternion[1];
                // sSktRefFrame.Q[3] = sAhrsOutput.quaternion[2];
                // sSktRefFrame.Displacement = 0;
                // sSktRefFrame.G[0] = sAhrsInput[sOutPtr].G[0];
                // sSktRefFrame.G[1] = sAhrsInput[sOutPtr].G[1];
                // sSktRefFrame.G[2] = sAhrsInput[sOutPtr].G[2];
                // sSktRefFrame.A[0] = sAhrsInput[sOutPtr].A[0];
                // sSktRefFrame.A[1] = sAhrsInput[sOutPtr].A[1];
                // sSktRefFrame.A[2] = sAhrsInput[sOutPtr].A[2];
                // send_callback((uint8_t*)&sSktRefFrame, sizeof(sSktRefFrame));

#ifdef DEBUG

                // nprint("%lf, %lf, %lf, %lf\r\n",
                //        sAhrsOutput.quaternion[0], // 四元数
                //        sAhrsOutput.quaternion[1],
                //        sAhrsOutput.quaternion[2],
                //        sAhrsOutput.quaternion[3]);

                //kprint("Q0=%f,Q1=%f,Q2=%f,Q3=%f\r\n", sAhrsOutput.quaternion[0], sAhrsOutput.quaternion[1], sAhrsOutput.quaternion[2], sAhrsOutput.quaternion[3]);
                //kprint("G0=%f,G1=%f,G2=%f\r\n", sAhrsOutput.gravity[0], sAhrsOutput.gravity[1], sAhrsOutput.gravity[2]);
                //kprint("A0=%f,A1=%f,A2=%f\r\n", sAhrsOutput.linear_acceleration[0], sAhrsOutput.linear_acceleration[1], sAhrsOutput.linear_acceleration[2]);
                //kprint("id=%u,G0=%f,G1=%f,G2=%f\r\n", (unsigned int)sOutPtr, (float)(sAhrsInput[sOutPtr].G[0] / 25), (float)(sAhrsInput[sOutPtr].G[1] / 25), (float)(sAhrsInput[sOutPtr].G[2] / 25));
                //kprint("id=%u,A0=%f,A1=%f,A2=%f\r\n", (unsigned int)sOutPtr, (float)(sAhrsInput[sOutPtr].A[0] / 9806.65), (float)(sAhrsInput[sOutPtr].A[1] / 9806.65), (float)(sAhrsInput[sOutPtr].A[2] / 9806.65));

#endif
            }
        }

        sOutPtr++;
        if (sOutPtr == IMU_INPUT_BUFF_LENGTH) {
            sOutPtr = 0;
        }
    }

    if (sInPtr != sOutPtr) {
        UTIL_SEQ_SetTask(1 << CFG_TASK_AHRS_PROC_ID, CFG_PRIO_NBR_1); /* restart current task */
    }
    // LEDG_CTRL(0);
}

uint8_t ahrs_init(void)
{
    __GYRO_ACC_CONFIG_TypeDef acc_gyro_config = { AHRS_ACC_GYRO_DATA_RATE, AHRS_GYRO_SCALE, AHRS_ACC_SCALE, AHRS_ACC_GYRO_POWER_MODE };

    imu_init(&acc_gyro_config);
    sAhrsAlgApi.init();

    ts_create(0, &(sTSAhrsRunID), TS_Repeated, ahrs_run_ts_callback);
    UTIL_SEQ_RegTask(1 << CFG_TASK_AHRS_READ_ID, UTIL_SEQ_RFU, ahrs_read_task);
    UTIL_SEQ_RegTask(1 << CFG_TASK_AHRS_PROC_ID, UTIL_SEQ_RFU, ahrs_proc_task);
    return STD_SUCCESS;
}

uint8_t ahrs_start(void)
{
    sInPtr = sOutPtr = 0;
    sAhrsAlgApi.start();
    imu_enable();
    kprint("start\r\n");
    sAhrsStartFlag = 1;
    ts_start_us(sTSAhrsRunID, (1000000 / AHRS_ACC_GYRO_DATA_RATE)); // 20分钟
    return STD_SUCCESS;
}

uint8_t ahrs_stop(void)
{
    sAhrsStartFlag = 0;
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
