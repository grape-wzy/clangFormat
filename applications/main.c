/*******************************************************************************
 * file    main.c
 * author  mackgim
 * version 1.0.0
 * date
 * brief   主函数
 *******************************************************************************/

#include "main.h"
#include "platform.h"
#include "spi_if.h"

#include "acc_gyro_if.h"

#include "driver_cfg.h"

#include "motion_fx.h"

#if 0
int main()
{

	platform_init();

	kprint("\r\n");
	kprint("----run----\r\n");
	kprint("\r\n");

	while (1)
	{
		itask_proc();
	}
}

#else

#define MFX_STR_LENG 35
#define STATE_SIZE   (size_t)(2560)
#define ENABLE_6X    1

static uint8_t motion_fx_state[STATE_SIZE];

char lib_version[MFX_STR_LENG];

void motion_fx_init(void)
{
    /*** Initialization ***/
    MFX_knobs_t iKnobs;

    /* Check if statically allocated memory size is sufficient
        to store MotionFX algorithm state and resize if necessary */
    if (STATE_SIZE < MotionFX_GetStateSize())
        while (1) {}

    /* Sensor Fusion API initialization function */
    MotionFX_initialize((MFXState_t *)motion_fx_state);

    /* Optional: Get version */
    MotionFX_GetLibVersion(lib_version);

    /* Modify knobs settings & set the knobs */
    MotionFX_getKnobs(motion_fx_state, &iKnobs);

    iKnobs.LMode                             = 1;
    iKnobs.output_type                       = MFX_ENGINE_OUTPUT_NED;
    iKnobs.modx                              = 1;
    iKnobs.start_automatic_gbias_calculation = 1;

    MotionFX_setKnobs(motion_fx_state, &iKnobs);

    /* Enable 9-axis sensor fusion */
    if (ENABLE_6X == 1) {
        MotionFX_enable_6X(motion_fx_state, MFX_ENGINE_ENABLE);
        MotionFX_enable_9X(motion_fx_state, MFX_ENGINE_DISABLE);
    } else {
        MotionFX_enable_6X(motion_fx_state, MFX_ENGINE_DISABLE);
        MotionFX_enable_9X(motion_fx_state, MFX_ENGINE_ENABLE);
    }
}

extern void MX_DMA_Init(void);
extern void MX_CRC_Init(void);

#define TIME_DELAY (3)

char float_string[4][64];
int main(void)
{
    ACC_GYRO_FDATA_T imu_data[2];
    MFX_input_t      mfx_input;
    MFX_output_t     mfx_out;

    float delta_time = 0.005;

    uint64_t tick_last, tick_now;

    uint8_t time_counter = 0, cacl_counter = 0;

    HAL_Init();

    ble_init_clock();

#if ENABLE_RCC_HSE_32M
    SystemClock_32M_Config();
#else
    SystemClock_64M_Config();
#endif

    ym_hw_pin_init();

    MX_DMA_Init();

    MX_CRC_Init();

    log_init();

    gtimer_init();

    spi1_hw_init();

    acc_gpro_init();

    acc_gyro_enable();

    motion_fx_init();

    tick_now  = Clock_Time();
    tick_last = tick_now;

    while (1) {
        acc_gyro_get_result(imu_data, 1);

        tick_now   = Clock_Time();
        delta_time = (float)((float)(tick_now - tick_last) / 1000);
        tick_last  = tick_now;

        mfx_input.acc[0] = imu_data->acc.x / 1000;
        mfx_input.acc[1] = imu_data->acc.y / 1000;
        mfx_input.acc[2] = imu_data->acc.z / 1000;

        mfx_input.gyro[0] = imu_data->gyro.x / 1000;
        mfx_input.gyro[1] = imu_data->gyro.y / 1000;
        mfx_input.gyro[2] = imu_data->gyro.z / 1000;

        if (cacl_counter++ < 2) {
            MotionFX_propagate(motion_fx_state, &mfx_out, &mfx_input, &delta_time);
        } else {
            cacl_counter = 0;
            MotionFX_update(motion_fx_state, &mfx_out, &mfx_input, &delta_time, 0);
        }

        time_counter++;
        if (time_counter * TIME_DELAY >= 20) {
            // nprint("%lf, %lf, %lf, ",
            //        mfx_out.gravity[0], //静态加速度
            //        mfx_out.gravity[1],
            //        mfx_out.gravity[2]);

            nprint("%lf, %lf, %lf, ",
                   mfx_input.gyro[0], // 陀螺仪原始数据
                   mfx_input.gyro[1],
                   mfx_input.gyro[2]);

            nprint("%lf, %lf, %lf, ",
                   mfx_out.linear_acceleration[0], // 动态加速度
                   mfx_out.linear_acceleration[1],
                   mfx_out.linear_acceleration[2]);

            nprint("%lf, %lf, %lf, %lf\r\n",
                   mfx_out.quaternion[0], // 四元数
                   mfx_out.quaternion[1],
                   mfx_out.quaternion[2],
                   mfx_out.quaternion[3]);

            // nprint("\r\n");
            time_counter = 0;
        }

        HAL_Delay(TIME_DELAY);
    }

    return 0;
}
#endif
