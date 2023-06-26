/**
 ******************************************************************************
 * @file    motion_fx_manager.h
 * @author  MEMS Software Solutions Team
 * @brief   This file contains definitions for the motion_fx_manager.c file
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef MOTION_FX_MANAGER_H
#define MOTION_FX_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"



/* Extern variables ----------------------------------------------------------*/
/* Exported Macros -----------------------------------------------------------*/
/* Exported Types ------------------------------------------------------------*/
/* Imported Variables --------------------------------------------------------*/
/* Exported Functions Prototypes ---------------------------------------------*/
uint8_t MotionFX_manager_init(void);
uint8_t MotionFX_manager_run(void* data_in, void* data_out, float delta_time);
uint8_t MotionFX_manager_start(void);
uint8_t MotionFX_manager_stop(void);
uint8_t MotionFX_manager_calib(int32_t en);

#ifdef __cplusplus
}
#endif

#endif /* MOTION_FX_MANAGER_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
