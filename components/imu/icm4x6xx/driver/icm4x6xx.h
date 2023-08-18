/*
 * ________________________________________________________________________________________________________
 * Copyright (c) 2015-2015 InvenSense Inc. All rights reserved.
 *
 * This software, related documentation and any modifications thereto (collectively Software is subject
 * to InvenSense and its licensors' intellectual property rights under U.S. and international copyright
 * and other intellectual property rights laws.
 *
 * InvenSense and its licensors retain all intellectual property and proprietary rights in and to the Software
 * and any use, reproduction, disclosure or distribution of the Software without an express license agreement
 * from InvenSense is strictly prohibited.
 *
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE SOFTWARE IS
 * PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * INVENSENSE BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
 * ________________________________________________________________________________________________________
 */
#ifndef _INV_ICM4X6XX_H_
#define _INV_ICM4X6XX_H_

#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

//#ifdef __cplusplus
//extern "C" {
//#endif // __cplusplus

#define FIFO_WM_MODE_EN                      1 //1:fifo mode 0:dri mode
#define SPI_MODE_EN                          1 //1:spi 0:i2c
#define INT_LATCH_EN                         0 //1:latch mode 0:pulse mode
#define INT_ACTIVE_HIGH_EN                   1 //1:active high 0:active low
#define SUPPORT_SELFTEST                     0
/* APEX Features */
#define SUPPORT_PEDOMETER                    0
#define SUPPORT_WOM                          0

#define SENSOR_DIRECTION                     0 //0~7 sensorConvert map index below
/* { { 1, 1, 1},    {0, 1, 2} },
   { { -1, 1, 1},   {1, 0, 2} },
   { { -1, -1, 1},  {0, 1, 2} },
   { { 1, -1, 1},   {1, 0, 2} },
   { { -1, 1, -1},  {0, 1, 2} },
   { { 1, 1, -1},   {1, 0, 2} },
   { { 1, -1, -1},  {0, 1, 2} },
   { { -1, -1, -1}, {1, 0, 2} }, */
#define SENSOR_LOG_LEVEL                     3 //INV_LOG_LEVEL_INFO
#define 21                      0
#define MAX_RECV_PACKET                      100

enum inv_log_level {
    INV_LOG_LEVEL_OFF     = 0,
    INV_LOG_LEVEL_ERROR,
    INV_LOG_LEVEL_WARNING,
    INV_LOG_LEVEL_INFO,
    INV_LOG_LEVEL_MAX
};

#if 0
/* customer board, need invoke system marco*/
#define INV_LOG
#else
/* smart motion board*/
#include "utils/Message.h"
#define INV_LOG           INV_MSG
#endif

typedef enum {
    ACC = 0,
    GYR,
    TEMP,
    #if SUPPORT_WOM
    WOM,
    #endif
    #if SUPPORT_PEDOMETER
    PEDO,
    #endif
    NUM_OF_SENSOR,
} SensorType_t;

struct accGyroData {
    uint8_t sensType;
    float x, y, z;
    uint64_t timeStamp;
};

struct accGyroDataPacket {
    uint8_t accOutSize;
    uint8_t gyroOutSize;
    uint64_t timeStamp;
    float temperature;
    struct accGyroData outBuf[MAX_RECV_PACKET];
    uint32_t magicNum;
};

int inv_icm4x6xx_initialize(void);
void inv_icm4x6xx_set_serif(int (*read)(void *, uint8_t, uint8_t *, uint32_t),
                            int (*write)(void *, uint8_t, uint8_t *, uint32_t));
void inv_icm4x6xx_set_delay(void (*delay_ms)(uint16_t), void (*delay_us)(uint16_t));
int inv_icm4x6xx_acc_enable();
int inv_icm4x6xx_gyro_enable();

int inv_icm4x6xx_acc_disable();
int inv_icm4x6xx_gyro_disable();

int inv_icm4x6xx_acc_set_rate(float odr_hz, uint16_t watermark);
int inv_icm4x6xx_gyro_set_rate(float odr_hz, uint16_t watermark);

int inv_icm4x6xx_get_rawdata(struct accGyroDataPacket *dataPacket);

#if SUPPORT_SELFTEST
int inv_icm4x6xx_acc_selftest(bool *result);
int inv_icm4x6xx_gyro_selftest(bool *result);
#endif

#if SUPPORT_PEDOMETER
int inv_icm4x6xx_pedometer_enable();
int inv_icm4x6xx_pedometer_disable();
int inv_icm4x6xx_pedometer_get_stepCnt(uint64_t *step_cnt);
#endif

#if SUPPORT_WOM
int inv_icm4x6xx_wom_enable();
int inv_icm4x6xx_wom_disable();
int inv_icm4x6xx_wom_get_event(bool *detect);
#endif

void inv_icm4x6xx_dumpRegs();

//#ifdef __cplusplus
//}
//#endif
#endif
