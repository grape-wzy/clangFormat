/*******************************************************************************
  * file     acc_gyro_if.h
  * author   mackgim
  * version  V1.0.0
  * date
  * brief ：
  *******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __ACC_GYRO_H
#define __ACC_GYRO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"

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

typedef struct {
    struct
    {
        float x;
        float y;
        float z;
    } acc, gyro;
} ACC_GYRO_FDATA_T;

typedef struct {
    uint16_t     output_data_rate;
    GYRO_SCALE_E gyro_scale;
    ACC_SCALE_E  acc_scale;
    uint8_t      high_performance; // 0 disable, 1 enable

    uint8_t (*write_buffer)(void *, uint8_t, const uint8_t *, uint16_t);
    uint8_t (*read_buffer)(void *, uint8_t, uint8_t *, uint16_t);
    uint8_t (*irq_enable)(uint8_t);

    void *handle;
} ACC_GYRO_CONFIG_TypeDef;

typedef struct {
    uint8_t (*init)(void);
    uint8_t (*deinit)(void);
    uint8_t (*transmit_receive)(uint8_t *, uint8_t *, uint16_t, uint32_t);
    uint8_t (*get_ready)(void);
    uint8_t (*set_cs)(uint8_t);
} __GYRO_ACC_HW_DRIVER_TypeDef;

typedef struct {
    uint8_t (*init)(void *);
    uint8_t (*deinit)(void *);
    uint8_t (*enable)(void *);
    uint8_t (*disable)(void *);
    uint8_t (*read_raw)(void *, int16_t *, uint16_t);
    uint8_t (*read_result)(void *, int16_t *, uint16_t, uint32_t);
    uint8_t (*get_tap)(void *, uint8_t *);
    uint8_t (*set_tap)(void *, uint32_t value);
} __GYRO_ACC_DEV_DRIVER_TypeDef;

typedef struct {
    ACC_GYRO_CONFIG_TypeDef       *config;
    __GYRO_ACC_HW_DRIVER_TypeDef  *hw_drv;
    __GYRO_ACC_DEV_DRIVER_TypeDef *dev_drv;
} __GYRO_ACC_HANDLE_TypeDef;

typedef uint8_t (*ACC_GYRO_IRQ_TypeDef)(uint8_t);

uint8_t acc_gyro_init(ACC_GYRO_CONFIG_TypeDef *config);
uint8_t acc_gyro_deinit(void);

uint8_t acc_gyro_disable(void);
uint8_t acc_gyro_enable(void);

uint8_t acc_gyro_get_result(ACC_GYRO_FDATA_T *buff, uint32_t number);

uint8_t acc_gyro_get_tap_status(uint8_t *v);
uint8_t acc_gyro_set_tap_threshold(uint32_t value);

ACC_SCALE_E  acc_gyro_get_acc_sensitivity(void);
GYRO_SCALE_E acc_gyro_get_gyro_sensitivity(void);
#ifdef __cplusplus
}
#endif

#endif /*__ACCELEROMETER_H */
