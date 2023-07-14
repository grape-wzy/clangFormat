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

uint8_t ahrs_init(void);
void ahrs_register_cb(void* cb);

uint8_t ahrs_start(void);
uint8_t ahrs_stop(void);

void ahrs_calib(int32_t en);

#endif /*__AHRS_IF_H */
