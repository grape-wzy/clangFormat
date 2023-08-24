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
#include "imu_if.h"

typedef struct {
    uint16_t     output_data_rate;
    GYRO_SCALE_E gyro_scale;
    ACC_SCALE_E  acc_scale;
    uint8_t      high_performance; // 0 disable, 1 enable

    int32_t (*write_buffer)(void *handle, uint8_t reg, const uint8_t *data, uint32_t size);
    int32_t (*read_buffer)(void *handle, uint8_t reg, uint8_t *buff, uint32_t size);
    uint8_t (*irq_enable)(uint8_t state);

    void *handle;
} ACC_GYRO_CONFIG_TypeDef;

typedef uint8_t (*ACC_GYRO_IRQ_TypeDef)(uint8_t);

uint8_t acc_gyro_init(void *config);
uint8_t acc_gyro_deinit(void *config);

uint8_t acc_gyro_disable(void);
uint8_t acc_gyro_enable(void);

uint8_t acc_gyro_get_result(float acc[3], float gyro[3], uint32_t number);

uint8_t acc_gyro_get_tap_status(uint8_t *v);
uint8_t acc_gyro_set_tap_threshold(uint32_t value);

ACC_SCALE_E  acc_gyro_get_acc_sensitivity(void);
GYRO_SCALE_E acc_gyro_get_gyro_sensitivity(void);
#ifdef __cplusplus
}
#endif

#endif /*__ACCELEROMETER_H */
