/*******************************************************************************
 * file     imu_if.h
 * author   mackgim
 * version  V1.0.0
 * date
 * brief ：
 *******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __IMU_IF_H
#define __IMU_IF_H

#include "stdint.h"
// #include "gcompiler.h"

#define A     \
    int aaaa; \
    int b;    \
    int dddddddddd;

typedef struct
{
    uint16_t BitDepth; // 0 high, 1 low
    uint16_t DataRate;
    float    GyroSensitivity;
    float    AccSensitivity;
} __GYRO_ACC_CONFIG_TypeDef;

typedef struct
{
    uint8_t (*init)(void);
    uint8_t (*deinit)(void);
    void (*pwr_ctrl)(uint8_t);
    uint8_t (*reset)(uint8_t);
    uint8_t (*transmit_receive)(uint8_t *, uint8_t *, uint16_t, uint32_t);
    uint8_t (*get_ready)(void);
    uint8_t (*enable_irq)(uint8_t);
    uint8_t (*generate_swi)(void);
    uint8_t (*set_cs)(uint8_t);
} __GYRO_ACC_HW_DRIVER_TypeDef;

typedef struct
{
    uint8_t (*init)(void *);
    uint8_t (*deinit)(void *);
    uint8_t (*restore)(void *);
    uint8_t (*enable)(void *);
    uint8_t (*disable)(void *);
    uint8_t (*read_raw)(void *, uint8_t *, uint16_t);
} __GYRO_ACC_DEV_DRIVER_TypeDef;

typedef struct
{
    __GYRO_ACC_HW_DRIVER_TypeDef  *hw_drv;
    __GYRO_ACC_DEV_DRIVER_TypeDef *dev_drv;
    __GYRO_ACC_CONFIG_TypeDef     *config;
} __GYRO_ACC_HANDLE_TypeDef;

#define BURST16_DATA_LENGTH (sizeof(__ADIS16505_BURST16_DATA_TypeDef))
typedef struct
{
    uint16_t TxCmd;
    uint16_t DiagStat;
    int16_t  G[3];
    int16_t  A[3];
    int16_t  Temp;
    uint16_t DataCntr;
    uint16_t Checksum;
} __ADIS16505_BURST16_DATA_TypeDef;

#define BURST32_DATA_LENGTH (sizeof(__ADIS16505_BURST32_DATA_TypeDef) - 2)
typedef struct
{
    uint16_t Reserved;
    uint16_t DiagStat;
    int32_t  Gyro[3];
    int32_t  Acc[3];
    int16_t  Temp;
    uint16_t DataCntr;
    uint16_t Checksum;
    uint16_t Reserved1;
} __ADIS16505_BURST32_DATA_TypeDef;

uint8_t imu_init(__GYRO_ACC_CONFIG_TypeDef *config);
uint8_t imu_deinit(void);

uint8_t imu_disable(void);
uint8_t imu_enable(void);

uint8_t imu_get_raw(uint8_t *buff, uint16_t size);

uint8_t imu_get_all_reg(void);

uint8_t imu_generate_swi(void);

uint8_t imu_restore_config(void);

uint8_t imu_get_ready(void);
#endif /*__IMU_IF_H */
