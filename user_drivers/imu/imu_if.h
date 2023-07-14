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

typedef enum {
    ACC_SCALE_2g = 0,
    ACC_SCALE_4g,
    ACC_SCALE_8g,
    ACC_SCALE_16g,
} ACC_SCALE_E;

typedef enum {
    GYPO_SCALE_125dps = 0,
    GYPO_SCALE_250dps,
    GYPO_SCALE_500dps,
    GYPO_SCALE_1000dps,
    GYPO_SCALE_2000dps,
} GYRO_SCALE_E;

typedef struct
{
    uint16_t     output_data_rate;
    GYRO_SCALE_E gyro_scale;
    ACC_SCALE_E  acc_scale;
    uint8_t      high_performance; // 0 disable, 1 enable
} __GYRO_ACC_CONFIG_TypeDef;

typedef struct {
    float acc[3];
    float gyro[3];
} IMU_RESULT_TypeDef;

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
    uint8_t (*init)(void *config);
    uint8_t (*deinit)(void *);
    uint8_t (*restore)(void *);
    uint8_t (*enable)(void);
    uint8_t (*disable)(void);
    uint8_t (*read_raw)(void *handle, uint8_t *, uint16_t);
    uint8_t (*read_result)(float acc[3], float gyro[3], uint32_t number);
} __GYRO_ACC_DEV_DRIVER_TypeDef;

typedef struct
{
    __GYRO_ACC_HW_DRIVER_TypeDef  *hw_drv;
    __GYRO_ACC_DEV_DRIVER_TypeDef *dev_drv;
    __GYRO_ACC_CONFIG_TypeDef     *config;
} __GYRO_ACC_HANDLE_TypeDef;

uint8_t imu_init(__GYRO_ACC_CONFIG_TypeDef *config);
uint8_t imu_deinit(void);

uint8_t imu_disable(void);
uint8_t imu_enable(void);

uint8_t imu_get_raw(uint8_t *buff, uint16_t size);

uint8_t imu_get_result(IMU_RESULT_TypeDef *buff, uint32_t number);

uint8_t imu_get_all_reg(void);

uint8_t imu_generate_swi(void);

uint8_t imu_restore_config(void);

uint8_t imu_get_ready(void);
#endif /*__IMU_IF_H */
