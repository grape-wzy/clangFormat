/**
 ******************************************************************************
 * @file    motion_fx_manager.c
 * @author  MEMS Software Solutions Team
 * @brief   This file contains Datalog Fusion interface functions
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed under Software License Agreement
 * SLA0077, (the "License"). You may not use this file except in compliance
 * with the License. You may obtain a copy of the License at:
 *
 *     www.st.com/content/st_com/en/search.html#q=SLA0077-t=keywords-page=1
 *
 *******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "standard_lib.h"
#include "motion_fx.h"

/* Extern variables ----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
#define STATE_SIZE       (2560)

//#define GBIAS_ACC_TH_SC                 (2.0f*0.000765f)
//#define GBIAS_GYRO_TH_SC                (2.0f*0.002f)
//#define GBIAS_MAG_TH_SC                 (2.0f*0.001500f)

#define GBIAS_ACC_TH_SC  (0.000115)
#define GBIAS_GYRO_TH_SC (0.000159)
#define GBIAS_MAG_TH_SC  (2.0f * 0.001500f)

#define DECIMATION       2U

/* Private variables ---------------------------------------------------------*/

static uint8_t motion_fx_state_buff[STATE_SIZE];

/* Private typedef -----------------------------------------------------------*/
/* Exported function prototypes ----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
/**
 * @brief  Initialize the MotionFX engine
 * @param  None
 * @retval None
 */
uint8_t MotionFX_manager_init(void)
{
    MFX_knobs_t iKnobs;

    size_t size = MotionFX_GetStateSize();
    kprint("size=%u\r\n", size);
    if (STATE_SIZE < size) {
        kprint("size error\r\n");
        return STD_FAILED;
    }

    MotionFX_initialize((MFXState_t *)motion_fx_state_buff);

    uint8_t version[64];
    MotionFX_GetLibVersion((char *)version);
    kprint("version=%s\r\n", (char *)version);

    MotionFX_getKnobs((MFXState_t *)motion_fx_state_buff, &iKnobs);

    kprint("ATime = %f\r\n", (float)iKnobs.ATime);
    kprint("MTime = %f\r\n", iKnobs.MTime);
    kprint("FrTime = %f\r\n", iKnobs.FrTime);
    kprint("LMode = %u\r\n", iKnobs.LMode);
    kprint("modx = %u\r\n", iKnobs.modx);
    kprint("acc_orientation = %s\r\n", iKnobs.acc_orientation);
    kprint("gyro_orientation = %s\r\n", iKnobs.gyro_orientation);
    kprint("mag_orientation = %s\r\n", iKnobs.mag_orientation);
    kprint("output_type = %u\r\n", iKnobs.output_type);
    kprint("start_automatic_gbias_calculation = %d\r\n", iKnobs.start_automatic_gbias_calculation);
    kprint("gbias_acc_th_sc = %f\r\n", iKnobs.gbias_acc_th_sc);
    kprint("gbias_gyro_th_sc = %f\r\n", iKnobs.gbias_gyro_th_sc);
    kprint("gbias_mag_th_sc = %f\r\n", iKnobs.gbias_mag_th_sc);

    // iKnobs.acc_orientation[0]  = 'e';
    // iKnobs.acc_orientation[1]  = 'n';
    // iKnobs.acc_orientation[2]  = 'u';
    // iKnobs.gyro_orientation[0] = 'e';
    // iKnobs.gyro_orientation[1] = 'n';
    // iKnobs.gyro_orientation[2] = 'u';

    // iKnobs.mag_orientation[0] = 'e';
    // iKnobs.mag_orientation[1] = 'n';
    // iKnobs.mag_orientation[2] = 'u';

    // iKnobs.gbias_acc_th_sc  = GBIAS_ACC_TH_SC;
    // iKnobs.gbias_gyro_th_sc = GBIAS_GYRO_TH_SC;
    // iKnobs.gbias_mag_th_sc  = GBIAS_MAG_TH_SC;

    iKnobs.output_type = MFX_ENGINE_OUTPUT_ENU;
    iKnobs.LMode       = 1;
    iKnobs.modx        = DECIMATION;

    MotionFX_setKnobs(motion_fx_state_buff, &iKnobs);

    MotionFX_enable_6X(motion_fx_state_buff, MFX_ENGINE_DISABLE);
    MotionFX_enable_9X(motion_fx_state_buff, MFX_ENGINE_DISABLE);
    return STD_SUCCESS;
}

#include "platform.h"
/**
 * @brief  Run Motion Sensor Data Fusion algorithm
 * @param  data_in  Structure containing input data
 * @param  data_out Structure containing output data
 * @param  delta_time Delta time
 * @retval None
 */
uint8_t MotionFX_manager_run(void *data_in, void *data_out, float delta_time)
{
    MotionFX_propagate(motion_fx_state_buff, (MFX_output_t *)data_out, (MFX_input_t *)data_in, &delta_time);

#if (DECIMATION > 1)
    static uint8_t cacl_counter = 0;
    cacl_counter++;
    if (cacl_counter >= DECIMATION) {
        cacl_counter = 0;
        MotionFX_update(motion_fx_state_buff, (MFX_output_t *)data_out, (MFX_input_t *)data_in, &delta_time, NULL);
    }
#else
    MotionFX_update(motion_fx_state_buff, (MFX_output_t *)data_out, (MFX_input_t *)data_in, &delta_time, NULL);
#endif

    return STD_SUCCESS;
}

/**
 * @brief  Start 6 axes MotionFX engine
 * @param  None
 * @retval None
 */
uint8_t MotionFX_manager_start(void)
{
    MotionFX_enable_6X(motion_fx_state_buff, MFX_ENGINE_ENABLE);
    return STD_SUCCESS;
}

/**
 * @brief  Stop 6 axes MotionFX engine
 * @param  None
 * @retval None
 */
uint8_t MotionFX_manager_stop(void)
{
    MotionFX_enable_6X(motion_fx_state_buff, MFX_ENGINE_DISABLE);
    return STD_SUCCESS;
}

uint8_t MotionFX_manager_calib(int32_t en)
{
    float       gbias[3];
    MFX_knobs_t iKnobs;

    MotionFX_getGbias(motion_fx_state_buff, gbias);

    kprint("bias, g0=%f, g1=%f, g2=%f.\r\n", gbias[0], gbias[1], gbias[2]);

    MotionFX_getKnobs(motion_fx_state_buff, &iKnobs);
    kprint("gbias_acc_th_sc = %f\r\n", iKnobs.gbias_acc_th_sc);
    kprint("gbias_gyro_th_sc = %f\r\n", iKnobs.gbias_gyro_th_sc);
    kprint("gbias_mag_th_sc = %f\r\n", iKnobs.gbias_mag_th_sc);

    iKnobs.start_automatic_gbias_calculation = en;
    MotionFX_setKnobs(motion_fx_state_buff, &iKnobs);

    return STD_SUCCESS;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
