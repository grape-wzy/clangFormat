/*******************************************************************************
* file    saber_config.h
* author  mackgim
* version 1.0.0
* date
* brief   PNI对外接口
*******************************************************************************/

#ifndef __SABER_CONFIG_H
#define __SABER_CONFIG_H


#include "stdint.h"
#include "gcompiler.h"





#define SESSION_NAME_TEMPERATURE                0x0000
#define SESSION_NAME_RAW_ACC                    0x0400
#define SESSION_NAME_RAW_GYRO                   0x0401
#define SESSION_NAME_RAW_MAG                    0x0402
#define SESSION_NAME_CAL_ACC                    0x0800
#define SESSION_NAME_KAL_ACC                    0x0801
#define SESSION_NAME_LINEAR_ACC                 0x0802
#define SESSION_NAME_HEAVE_MOTION               0x0803
#define SESSION_NAME_DELTA_V_ACC                0x0804
#define SESSION_NAME_CAL_GYRO                   0x0C00
#define SESSION_NAME_KAL_GYRO                   0x0C01
#define SESSION_NAME_DELTA_Q_GYRO               0x0C02

#define SESSION_NAME_CAL_MAG                    0x1000
#define SESSION_NAME_KAL_MAG                    0x1001
#define SESSION_NAME_MAG_DEV                    0x1002
#define SESSION_NAME_MAG_DIS_PCT                0x1003

#define SESSION_NAME_CAL_BARO                   0x1400
#define SESSION_NAME_KAL_BARO                   0x1401

#define SESSION_NAME_GNSS_PVT                   0x2000
#define SESSION_NAME_GNSS_SATELITTE             0x2001

#define SESSION_NAME_GPS_DOP                    0x2400
#define SESSION_NAME_GPS_SOL                    0x2401
#define SESSION_NAME_GPS_TIME                   0x2402
#define SESSION_NAME_GPS_SV                     0x2403

#define SESSION_NAME_QUAT                       0x3000
#define SESSION_NAME_EULER                      0x3001
#define SESSION_NAME_ROTATION_M                 0x3002

#define SESSION_NAME_POSITION_ALTITUDE          0x3400
#define SESSION_NAME_POSITION_ECEF              0x3401
#define SESSION_NAME_POSITION_LATLON            0x3402
#define SESSION_NAME_POSITION_VELOCITY          0x3800
#define SESSION_NAME_POSITION_DISTANCE          0x3403


#define SESSION_NAME_PACKET_COUNTER             0x4000
#define SESSION_NAME_UTC_TIME                   0x4001
#define SESSION_NAME_OS_TIME                    0x4002
#define SESSION_NAME_SAMPLE_TIME_FINE           0x4003
#define SESSION_NAME_SAMPLE_TIME_COARSE         0x4004
#define SESSION_NAME_ITOW                       0x4005
#define SESSION_NAME_DELTA_T                    0x4006

#define SESSION_NAME_STATUS_WORD                0x4400
#define SESSION_NAME_RSSI                       0x4401
#define SESSION_NAME_CPU_USAGE                  0x4402



#define SET_SABER_DATA_CONFIG(name)            ((USE_##name)?((SESSION_NAME_##name)|0x8000):(SESSION_NAME_##name))




#define USE_TEMPERATURE                1
#define USE_RAW_ACC                    0//不能同时RAW和CAL
#define USE_RAW_GYRO                   0
#define USE_RAW_MAG                    0
#define USE_CAL_ACC                    1
#define USE_KAL_ACC					   0
#define USE_LINEAR_ACC                 0
#define USE_HEAVE_MOTION               0
#define USE_DELTA_V_ACC                0
#define USE_CAL_GYRO                   1
#define USE_KAL_GYRO                   0
#define USE_DELTA_Q_GYRO               0

#define USE_CAL_MAG                    0
#define USE_KAL_MAG                    0
#define USE_MAG_DEV                    0
#define USE_MAG_DIS_PCT                0

#define USE_CAL_BARO                   0
#define USE_KAL_BARO                   0

#define USE_GNSS_PVT                   0
#define USE_GNSS_SATELITTE             0

#define USE_GPS_DOP                    0
#define USE_GPS_SOL                    0
#define USE_GPS_TIME                   0
#define USE_GPS_SV                     0

#define USE_QUAT                       1
#define USE_EULER                      1
#define USE_ROTATION_M                 0

#define USE_POSITION_ALTITUDE          0
#define USE_POSITION_ECEF              0
#define USE_POSITION_LATLON            0
#define USE_POSITION_VELOCITY          0
#define USE_POSITION_DISTANCE          0


#define USE_PACKET_COUNTER             0
#define USE_UTC_TIME                   0
#define USE_OS_TIME                    0
#define USE_SAMPLE_TIME_FINE           0
#define USE_SAMPLE_TIME_COARSE         0
#define USE_ITOW                       0
#define USE_DELTA_T                    0

#define USE_STATUS_WORD                0
#define USE_RSSI                       0
#define USE_CPU_USAGE                  0


typedef __I__packed struct {
	uint16_t session;
	uint8_t length;
}__I__PACKED __SABER_PACKET_SESSION_HEAD_TypeDef;



static const uint16_t packet_session[42] =
{
	SET_SABER_DATA_CONFIG(TEMPERATURE),
	SET_SABER_DATA_CONFIG(RAW_ACC),
	SET_SABER_DATA_CONFIG(RAW_GYRO),
	SET_SABER_DATA_CONFIG(RAW_MAG),
	SET_SABER_DATA_CONFIG(CAL_ACC),
	SET_SABER_DATA_CONFIG(KAL_ACC),
	SET_SABER_DATA_CONFIG(LINEAR_ACC),
	SET_SABER_DATA_CONFIG(HEAVE_MOTION),
	SET_SABER_DATA_CONFIG(DELTA_V_ACC),
	SET_SABER_DATA_CONFIG(CAL_GYRO),
	SET_SABER_DATA_CONFIG(KAL_GYRO),
	SET_SABER_DATA_CONFIG(DELTA_Q_GYRO),
	SET_SABER_DATA_CONFIG(CAL_MAG),
	SET_SABER_DATA_CONFIG(KAL_MAG),
	SET_SABER_DATA_CONFIG(MAG_DEV),
	SET_SABER_DATA_CONFIG(MAG_DIS_PCT),
	SET_SABER_DATA_CONFIG(CAL_BARO),
	SET_SABER_DATA_CONFIG(KAL_BARO),
	SET_SABER_DATA_CONFIG(GNSS_PVT),
	SET_SABER_DATA_CONFIG(GNSS_SATELITTE),
	SET_SABER_DATA_CONFIG(GPS_DOP),
	SET_SABER_DATA_CONFIG(GPS_SOL),
	SET_SABER_DATA_CONFIG(GPS_TIME),
	SET_SABER_DATA_CONFIG(GPS_SV),
	SET_SABER_DATA_CONFIG(QUAT),
	SET_SABER_DATA_CONFIG(EULER),
	SET_SABER_DATA_CONFIG(ROTATION_M),
	SET_SABER_DATA_CONFIG(POSITION_ALTITUDE),
	SET_SABER_DATA_CONFIG(POSITION_ECEF),
	SET_SABER_DATA_CONFIG(POSITION_LATLON),
	SET_SABER_DATA_CONFIG(POSITION_VELOCITY),
	SET_SABER_DATA_CONFIG(POSITION_DISTANCE),
	SET_SABER_DATA_CONFIG(PACKET_COUNTER),
	SET_SABER_DATA_CONFIG(UTC_TIME),
	SET_SABER_DATA_CONFIG(OS_TIME),
	SET_SABER_DATA_CONFIG(SAMPLE_TIME_FINE),
	SET_SABER_DATA_CONFIG(SAMPLE_TIME_COARSE),
	SET_SABER_DATA_CONFIG(ITOW),
	SET_SABER_DATA_CONFIG(DELTA_T),
	SET_SABER_DATA_CONFIG(STATUS_WORD),
	SET_SABER_DATA_CONFIG(RSSI),
	SET_SABER_DATA_CONFIG(CPU_USAGE),
};


#endif /*__PNI_IF_H */

