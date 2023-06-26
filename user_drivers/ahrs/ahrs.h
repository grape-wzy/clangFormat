/*******************************************************************************
* file    ahrs_if.h
* author  mackgim
* version 1.0.0
* date
* brief   combine accelerometer and gyroscope readings in order to obtain
accurate information about the inclination of your device relative
to the ground plane
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __AHRS_IF_H
#define __AHRS_IF_H

#include "stdint.h"

#define MFX_NUM_AXES    3
#define MFX_QNUM_AXES   4

typedef struct
{
	float mag[MFX_NUM_AXES];                 /* Calibrated mag [uT/50] */
	float acc[MFX_NUM_AXES];                 /* Acceleration in [g] */
	float gyro[MFX_NUM_AXES];                /* Angular rate [dps] */
} MFX_input_t;

typedef struct
{
	float rotation[MFX_NUM_AXES];            /* yaw, pitch and roll */
	float quaternion[MFX_QNUM_AXES];         /* quaternion */
	float gravity[MFX_NUM_AXES];             /* device frame gravity */
	float linear_acceleration[MFX_NUM_AXES]; /* device frame linear acceleration */
	float heading;                           /* heading */
	float headingErr;                        /* heading error in deg */
} MFX_output_t;


uint8_t ahrs_init(void);
void ahrs_register_cb(void* cb);

uint8_t ahrs_start(void);
uint8_t ahrs_stop(void);

void ahrs_calib(int32_t en);

void ahrs_isr(void);

#endif /*__AHRS_IF_H */

