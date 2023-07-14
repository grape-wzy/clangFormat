/*******************************************************************************
 * file     imu_if.c
 * author   mackgim
 * version  V1.0.0
 * date
 * brief ：
 *******************************************************************************/
/* Includes ------------------------------------------------------------------*/
#include "imu_if.h"
#include "acc_gyro_if.h"
#include "spi_a_hw_if.h"
#include "standard_lib.h"

#define TIMEOUT_DURATION (1000)

static __GYRO_ACC_CONFIG_TypeDef    sGyroAccConfig;
static __GYRO_ACC_HW_DRIVER_TypeDef sGyroAccHwDrv = {
    .init             = spi_a_hw_init,
    .deinit           = spi_a_hw_deinit,
    .reset            = spi_a_hw_reset,
    .pwr_ctrl         = NULL,
    .transmit_receive = spi_a_hw_transmit_receive,
    .get_ready        = spi_a_hw_get_ready,
    .enable_irq       = spi_a_hw_enable_irq,
    .generate_swi     = spi_a_hw_generate_swi,
    .set_cs           = spi_a_hw_set_cs,
};

static __GYRO_ACC_DEV_DRIVER_TypeDef sGyroAccDevDrv = {
    .init        = acc_gyro_init,
    .deinit      = acc_gyro_deinit,
    .read_result = acc_gyro_get_result,
    .enable      = acc_gyro_enable,
    .disable     = acc_gyro_disable,
    .read_raw    = NULL,
    .restore     = NULL,
};

static __GYRO_ACC_HANDLE_TypeDef sIMUHandle = { &sGyroAccHwDrv, &sGyroAccDevDrv, &sGyroAccConfig };

#pragma region acc gyro interface

static int32_t acc_write_buffer(void *handle, uint8_t reg, const uint8_t *data, uint16_t size)
{
    uint8_t  ret, write_buff[32], read_buff[32];
    uint16_t real_size;

    __GYRO_ACC_HANDLE_TypeDef *imu_handle = (__GYRO_ACC_HANDLE_TypeDef *)handle;

    memset(write_buff, 0xFF, sizeof(write_buff));
    write_buff[0] = (reg & 0x7F);

    real_size = size + 1;
    if (real_size > sizeof(write_buff))
        real_size = sizeof(write_buff);

    memcpy(&write_buff[1], data, real_size - 1);

    if (imu_handle->hw_drv->transmit_receive != NULL) {
        ret = imu_handle->hw_drv->transmit_receive(&write_buff[0], &read_buff[0], real_size, TIMEOUT_DURATION);
    }

    if (ret == STD_SUCCESS)
        return 0;

    return ret;
}

static int32_t acc_read_buffer(void *handle, uint8_t reg, uint8_t *buff, uint16_t size)
{
    uint8_t ret, write_buff[256];

    __GYRO_ACC_HANDLE_TypeDef *imu_handle = (__GYRO_ACC_HANDLE_TypeDef *)handle;

    memset(write_buff, 0xFF, sizeof(write_buff));
    write_buff[0] = (reg | 0x80);

    if (imu_handle->hw_drv->transmit_receive != NULL) {
        ret = imu_handle->hw_drv->transmit_receive(write_buff, write_buff, size + 1, TIMEOUT_DURATION);
        memcpy(buff, &write_buff[1], size);
    }

    if (ret == STD_SUCCESS)
        return 0;

    return ret;
}

static ACC_GYRO_CONFIG_TypeDef acc_config = {
    .write_buffer = acc_write_buffer,
    .read_buffer  = acc_read_buffer,
    .irq_enable   = NULL,
    .handle       = &sIMUHandle,
};

uint8_t imu_init(__GYRO_ACC_CONFIG_TypeDef *config)
{
    uint8_t ret          = STD_SUCCESS;
    *(sIMUHandle.config) = *config;

    acc_config.acc_scale        = config->acc_scale;
    acc_config.gyro_scale       = config->gyro_scale;
    acc_config.high_performance = config->high_performance;
    acc_config.output_data_rate = config->output_data_rate;

    kprint("power on\r\n");

    if (sIMUHandle.hw_drv->pwr_ctrl != NULL) {
        sIMUHandle.hw_drv->pwr_ctrl(1);
    }

    if (sIMUHandle.hw_drv->init != NULL) {
        ret = sIMUHandle.hw_drv->init();
    }
    if (ret != STD_SUCCESS) {
        kprint("failed to init hw\r\n");
        return ret;
    }

    if (sIMUHandle.dev_drv->init != NULL) {
        ret = sIMUHandle.dev_drv->init(&acc_config);
    }

    if (ret != STD_SUCCESS) {
        kprint("failed to init dev\r\n");
        return ret;
    }
    kprint("ok\r\n");

    return ret;
}

uint8_t imu_deinit()
{
    kprint("doing\r\n");
    if (sIMUHandle.dev_drv->deinit != NULL) {
        sIMUHandle.dev_drv->deinit(&sIMUHandle);
    }
    if (sIMUHandle.hw_drv->deinit != NULL) {
        sIMUHandle.hw_drv->deinit();
    }
    if (sIMUHandle.hw_drv->reset != NULL) {
        sIMUHandle.hw_drv->reset(0);
    }
    if (sIMUHandle.hw_drv->pwr_ctrl != NULL) {
        sIMUHandle.hw_drv->pwr_ctrl(0);
    }
    return STD_SUCCESS;
}

uint8_t imu_get_raw(uint8_t *buff, uint16_t size)
{
    return sIMUHandle.dev_drv->read_raw(&sIMUHandle, buff, size);
}

uint8_t imu_get_result(IMU_RESULT_TypeDef *buff, uint32_t number)
{
    return sIMUHandle.dev_drv->read_result(buff->acc, buff->gyro, number);
}

uint8_t imu_enable()
{
    if (sIMUHandle.dev_drv->enable != NULL) {
        return sIMUHandle.dev_drv->enable();
    }
    if (sIMUHandle.hw_drv->enable_irq != NULL) {
        sIMUHandle.hw_drv->enable_irq(1);
    }

    return STD_SUCCESS;
}

uint8_t imu_disable()
{
    if (sIMUHandle.hw_drv->enable_irq != NULL) {
        sIMUHandle.hw_drv->enable_irq(0);
    }

    if (sIMUHandle.dev_drv->disable != NULL) {
        return sIMUHandle.dev_drv->disable();
    }
    return STD_SUCCESS;
}

uint8_t imu_generate_swi(void)
{
    if (sIMUHandle.hw_drv->generate_swi != NULL) {
        sIMUHandle.hw_drv->generate_swi();
    }
    return STD_SUCCESS;
}

uint8_t imu_restore_config(void)
{
    uint8_t ret = STD_SUCCESS;

    if (sIMUHandle.dev_drv->restore != NULL) {
        ret = sIMUHandle.dev_drv->restore(&sIMUHandle);
    }
    if (ret != STD_SUCCESS) {
        kprint("failed to restore\r\n");
    } else {
        kprint("ok\r\n");
    }
    return ret;
}

uint8_t imu_get_all_reg(void)
{
    // return adis16505_dev_read_all_reg(&sIMUHandle);
    return 0;
}

uint8_t imu_get_ready(void)
{
    uint8_t ret = STD_SUCCESS;
    if (sIMUHandle.hw_drv->get_ready != sIMUHandle.hw_drv->get_ready) {
        ret = sIMUHandle.hw_drv->get_ready();
    }
    return ret;
}

#pragma endregion
/*******************************************************************************
END
*******************************************************************************/
