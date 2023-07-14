/*******************************************************************************
 * file    main.c
 * author  mackgim
 * version 1.0.0
 * date
 * brief   主函数
 *******************************************************************************/

#include "main.h"
#include "platform.h"

#include "acc_gyro_if.h"

#include "motion_fx_manager.h"
#include "motion_fx.h"

#if 1
int main()
{

	platform_init();

    ahrs_init();

    ahrs_start();

    kprint("\r\n");
    kprint("----run----\r\n");
    kprint("\r\n");

	while (1)
	{
		itask_proc();
	}
}

#else

// #define MFX_STR_LENG 35
// #define STATE_SIZE   (size_t)(2560)
// #define ENABLE_6X    1

// static uint8_t motion_fx_state[STATE_SIZE];

// char lib_version[MFX_STR_LENG];

// void motion_fx_init(void)
// {
//     /*** Initialization ***/
//     MFX_knobs_t iKnobs;

//     /* Check if statically allocated memory size is sufficient
//         to store MotionFX algorithm state and resize if necessary */
//     if (STATE_SIZE < MotionFX_GetStateSize())
//         while (1) {}

//     /* Sensor Fusion API initialization function */
//     MotionFX_initialize((MFXState_t *)motion_fx_state);

//     /* Optional: Get version */
//     MotionFX_GetLibVersion(lib_version);

//     /* Modify knobs settings & set the knobs */
//     MotionFX_getKnobs(motion_fx_state, &iKnobs);

//     iKnobs.LMode                             = 1;
//     iKnobs.output_type                       = MFX_ENGINE_OUTPUT_NED;
//     iKnobs.modx                              = 1;
//     iKnobs.start_automatic_gbias_calculation = 1;

//     MotionFX_setKnobs(motion_fx_state, &iKnobs);

//     /* Enable 9-axis sensor fusion */
//     if (ENABLE_6X == 1) {
//         MotionFX_enable_6X(motion_fx_state, MFX_ENGINE_ENABLE);
//         MotionFX_enable_9X(motion_fx_state, MFX_ENGINE_DISABLE);
//     } else {
//         MotionFX_enable_6X(motion_fx_state, MFX_ENGINE_DISABLE);
//         MotionFX_enable_9X(motion_fx_state, MFX_ENGINE_ENABLE);
//     }
// }

// #define TIME_DELAY (3)

// char float_string[4][64];
int main(void)
{
    MFX_input_t      mfx_input;
    MFX_output_t     mfx_out;

    float delta_time = 0.005;

    uint8_t cacl_counter = 0;

    HAL_Init();

    ble_init_clock();

#if ENABLE_RCC_HSE_32M
    SystemClock_32M_Config();
#else
    SystemClock_64M_Config();
#endif

    crc32_init();

    gpio_init();

    // log_init();

    ahrs_init();

    mfx_input.acc[0]  = 0;
    mfx_input.acc[1]  = 0;
    mfx_input.acc[2]  = 0;
    mfx_input.gyro[0] = 0;
    mfx_input.gyro[1] = 0;
    mfx_input.gyro[2] = 0;

    while (1) {
        mfx_input.acc[0] += 0.01;
        mfx_input.acc[1] += 0.02;
        mfx_input.acc[2] += 0.03;
        mfx_input.gyro[0] += 0.04;
        mfx_input.gyro[1] += 0.05;
        mfx_input.gyro[2] += 0.1;
        MotionFX_manager_run(&mfx_input, &mfx_out, delta_time);

        // HAL_Delay(3);
    }

    return 0;
}
#endif
