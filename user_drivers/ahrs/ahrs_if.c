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

#define AHRS_GYRO_SCALE           (2)   // GYPO_SCALE_500dps
#define AHRS_ACC_SCALE            (0)   // ACC_SCALE_2g
#define AHRS_ACC_GYRO_DATA_RATE   (104) // 500Hz
#define AHRS_ACC_GYRO_POWER_MODE  (1)   // high_performance, 0 disable, 1 enable

/* TODO: 当数据更新频率大于208的时候，陀螺仪静态校准几乎不可用了，打开动态校准后才能执行校准功能，但是校准几个数据后就误差数据就不再被更新了，
 * 当数据频率小于等于208的时候，可开启静态校准，且整个过程中，只要是处于静止状态时，校准则一直在继续，
 * 似乎与是否配置start_automatic_gbias_calculation无关，
 * 但数据频率低的时候，当快速旋转时，会造成角度的丢失；
 * 下一步测试高频率下，使用motionGC来执行误差校准功能是否可行(测试结果：MotionGC在高频率下和MotionFX 一样，无法执行校准)
 */

#define AHRS_DATA_REPORT_MIN_TIME (MS_TO_TICK(1000 / 40))

#define AHRS_BIAS_REPORT_MIN_TIME (MS_TO_TICK(200))

typedef struct {
    float    G[3];
    float    A[3];
    uint64_t tick;
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

static uint8_t      sTSAhrsRunID = 0, sAhrsStartFlag = 0;
static uint32_t     sInPtr = 0, sOutPtr = 0;
static MFX_output_t sAhrsOutput; /* the sequence of quaternion is qX qY qZ and qW */

static __AHRS_INPUT_TypeDef sAhrsInput[IMU_INPUT_BUFF_LENGTH];

static __SKT_MASTER_DEVICE_BLE_DATA_TypeDef sSktMasterFrame; // BLE上传的master帧

static __AHRS_ALG_API_TypeDef sAhrsAlgApi = {
    .init  = MotionFX_manager_init,
    .start = MotionFX_manager_start,
    .stop  = MotionFX_manager_stop,
    .run   = MotionFX_manager_run,
    .calib = MotionFX_manager_calib,
};

static AHRS_SEND_CALLBACK_TYPE send_callback;

static void ahrs_read_task(void)
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

    sAhrsInput[inp].tick = Clock_Tick();

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
static void ahrs_proc_task(void)
{
    MFX_input_t mfx_input_buf;

    static uint64_t cal_tick = 0, report_time_point_recode = 0, report_time_point_recode1 = 0;
    uint32_t        current_tick = 0;

    float during_time = 0.00;
    float gbias[3];
    int   bias_update = 0;

    if (sAhrsStartFlag == 0) return;

    if (sInPtr != sOutPtr) {
        /* run motion fx */
        mfx_input_buf.acc[0] = sAhrsInput[sOutPtr].A[0];
        mfx_input_buf.acc[1] = sAhrsInput[sOutPtr].A[1];
        mfx_input_buf.acc[2] = sAhrsInput[sOutPtr].A[2];

        mfx_input_buf.gyro[0] = sAhrsInput[sOutPtr].G[0];
        mfx_input_buf.gyro[1] = sAhrsInput[sOutPtr].G[1];
        mfx_input_buf.gyro[2] = sAhrsInput[sOutPtr].G[2];

        during_time = (float)(sAhrsInput[sOutPtr].tick - cal_tick) / (float)(GTIMER_LPTIM_FREQ); //TICKS_TO_S
        cal_tick    = sAhrsInput[sOutPtr].tick;

        // if (learning != 0) {
        //     learning++;
        //     if (learning == 2) {
        //         ahrs_calib(1);
        //     } else if (learning == AHRS_ACC_GYRO_DATA_RATE * 15) {
        //         ahrs_calib(0);
        //         learning = 0;
        //     }
        // }

        sAhrsAlgApi.run(&mfx_input_buf, &sAhrsOutput, during_time);

        // if (sAhrsInput[sOutPtr].tick > AHRS_BIAS_REPORT_MIN_TIME + report_time_point_recode1) {
        //     gbias[0] = 0.0;
        //     gbias[1] = 0.0;
        //     gbias[2] = 0.0;

        //     report_time_point_recode1 = sAhrsInput[sOutPtr].tick;

        //     MotionFX_getGbias(motion_fx_state_buff, gbias);
        //     nprint("gyro: %f, %f, %f | bias: %f, %f, %f\r\n",
        //            mfx_input_buf.gyro[0], mfx_input_buf.gyro[1], mfx_input_buf.gyro[2],
        //            gbias[0], gbias[1], gbias[2]);
        // }

        // if (send_callback != NULL)
        {
            if (sAhrsInput[sOutPtr].tick > AHRS_DATA_REPORT_MIN_TIME + report_time_point_recode) {
                report_time_point_recode = sAhrsInput[sOutPtr].tick;

                sSktMasterFrame.HeadSize      = 18;
                sSktMasterFrame.HeadType      = 0x81;
                sSktMasterFrame.DataType      = 0;
                sSktMasterFrame.TimeStampms   = TICKS_TO_MS(sAhrsInput[sOutPtr].tick);
                sSktMasterFrame.MotionMode    = 0;
                sSktMasterFrame.MotionProcess = 0;
                sSktMasterFrame.MotionStatus  = 0;

                sSktMasterFrame.DebugStatus = 1;
                sSktMasterFrame.DataSize    = 64;
                sSktMasterFrame.Q[0]        = sAhrsOutput.quaternion[3];
                sSktMasterFrame.Q[1]        = sAhrsOutput.quaternion[0];
                sSktMasterFrame.Q[2]        = sAhrsOutput.quaternion[1];
                sSktMasterFrame.Q[3]        = sAhrsOutput.quaternion[2];
                sSktMasterFrame.AdjustVV    = 0;
                sSktMasterFrame.AdjustFE    = -45.0;
                sSktMasterFrame.NavigateVV  = 0;
                sSktMasterFrame.NavigateFE  = -45.0;

                sSktMasterFrame.G[0] = sAhrsInput[sOutPtr].G[0];
                sSktMasterFrame.G[1] = sAhrsInput[sOutPtr].G[1];
                sSktMasterFrame.G[2] = sAhrsInput[sOutPtr].G[2];
                sSktMasterFrame.A[0] = sAhrsInput[sOutPtr].A[0];
                sSktMasterFrame.A[1] = sAhrsInput[sOutPtr].A[1];
                sSktMasterFrame.A[2] = sAhrsInput[sOutPtr].A[2];

                // send_callback((uint8_t *)&sSktMasterFrame, sizeof(sSktMasterFrame));

#ifdef DEBUG
                do {
                    // break;
                    // if ((sAhrsOutput.quaternion[0] + 1.5 > 3.0) || (sAhrsOutput.quaternion[0] + 1.5 < 0.1) ||
                    //     (sAhrsOutput.quaternion[1] + 1.5 > 3.0) || (sAhrsOutput.quaternion[1] + 1.5 < 0.1) ||
                    //     (sAhrsOutput.quaternion[2] + 1.5 > 3.0) || (sAhrsOutput.quaternion[2] + 1.5 < 0.1) ||
                    //     (sAhrsOutput.quaternion[3] + 1.5 > 3.0) || (sAhrsOutput.quaternion[3] + 1.5 < 0.1))

                    //     break;

                    current_tick = TICKS_TO_MS((uint32_t)Clock_Tick());
                    // nprint("%lu,", current_tick);

                    // nprint("%lf,%lf,%lf,%lf,",
                    //        sAhrsOutput.quaternion[0], // 四元数
                    //        sAhrsOutput.quaternion[1],
                    //        sAhrsOutput.quaternion[2],
                    //        sAhrsOutput.quaternion[3]);

                    nprint("%lu,%lf,%lf,%lf\r\n", current_tick,
                           sAhrsOutput.rotation[0],  // yaw-z
                           sAhrsOutput.rotation[1],  //pitch-x
                           sAhrsOutput.rotation[2]); // roll-y
                    // report_time_point_recode1++;

                    // nprint("%lf/%lf/%lf, ",
                    //        mfx_input_buf.acc[0],                                 // yaw-z
                    //        mfx_input_buf.acc[1],                                 //pitch-x
                    //        mfx_input_buf.acc[2]);                                // roll-y
                    // nprint("%lf, %lf, %lf, ",
                    //        mfx_input_buf.gyro[0],                                // yaw-z
                    //        mfx_input_buf.gyro[1],                                //pitch-x
                    //        mfx_input_buf.gyro[2]);                               // roll-y

                    // nprint("%lf, %lf, %lf, %d\r\n",
                    //        mgc_output_buf.GyroBiasX,                             // yaw-z
                    //        mgc_output_buf.GyroBiasY,                             //pitch-x
                    //        mgc_output_buf.GyroBiasZ, report_time_point_recode1); // roll-y

                    // nprint("%lf, %lf, %lf, %lf, %lf, %lf\r\n",
                    //        sAhrsOutput.linear_acceleration[0],
                    //        sAhrsOutput.linear_acceleration[1],
                    //        sAhrsOutput.linear_acceleration[2],
                    //        sAhrsOutput.gravity[0],
                    //        sAhrsOutput.gravity[1],
                    //        sAhrsOutput.gravity[2]); // 欧拉角

                } while (0);
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
}

static void ahrs_run_ts_callback(void)
{
    UTIL_SEQ_SetTask(1 << CFG_TASK_AHRS_READ_ID, CFG_SCH_PRIO_0);
    // ahrs_read_task();
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
    ts_start_ms(sTSAhrsRunID, 1000 / 100);
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

void ahrs_register_cb(void *cb)
{
    send_callback = cb;
}
