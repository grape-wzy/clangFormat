/*******************************************************************************
 * file     imu_if.c
 * author   mackgim
 * version  V1.0.0
 * date
 * brief ：
 *******************************************************************************/
/* Includes ------------------------------------------------------------------*/
#include "imu_if.h"
#include "adis16505_dev.h"
#include "spi_if.h"
#include "standard_lib.h"

static __GYRO_ACC_CONFIG_TypeDef    sGyroAccConfig;
static __GYRO_ACC_HW_DRIVER_TypeDef sGyroAccHwDrv = {
    .init             = spi_a_hw_init,
    .deinit           = spi_a_hw_deinit,
    .reset            = spi_a_hw_reset,
    .pwr_ctrl         = pwr_ctrl,
    .transmit_receive = spi_a_hw_transmit_receive,
    .get_ready        = spi_a_hw_get_ready,
    .enable_irq       = spi_a_hw_enable_irq,
    .generate_swi     = spi_a_hw_generate_swi,
    .set_cs           = spi_a_hw_set_cs,
};

static __GYRO_ACC_DEV_DRIVER_TypeDef sGyroAccDevDrv = {
    .init     = adis16505_dev_init,
    .deinit   = adis16505_dev_deinit,
    .restore  = adis16505_dev_restore_config,
    .enable   = NULL,
    .disable  = NULL,
    .read_raw = adis16505_dev_read_raw
};

static __GYRO_ACC_HANDLE_TypeDef sIMUHandle = { &sGyroAccHwDrv, &sGyroAccDevDrv, &sGyroAccConfig };
typedef struct {
    uint8_t (*init)(void);
    uint8_t (*deinit)(void);
    void (*pwr_ctrl)(uint8_t);
    uint8_t (*reset)(uint8_t);
    uint8_t (*transmit_receive)(uint8_t *, uint8_t *, uint16_t, uint32_t);
    uint8_t (*get_ready)(void);
    uint8_t (*enable_irq)(uint8_t);
    uint8_t (*generate_swi)(void);
    uint8_t (*set_cs)(uint8_t);
    uint16_t aa   :1;
    uint8_t  bb   :1;
    uint8_t  leng :4;
} __GYRO_ACC_HW_DRIVER_TypeDef111;

#pragma region acc gyro interface

uint8_t imu_init(__GYRO_ACC_CONFIG_TypeDef *config)
{
    *(sIMUHandle.config) = *config;
    // spi_a_hw_register_cb(acc_gyro_get_32bit_raw);
    // spi_a_hw_register_cb(imu_get_raw);
    sIMUHandle.hw_drv->reset(0);
    // for (uint32_t i = 0; i < 100; i++)
    //{
    //	sIMUHandle.hw_drv->pwr_ctrl(1);
    //	for (uint32_t j = 0; j < 2000; j++)
    //	{
    //		__NOP();
    //	}
    //	sIMUHandle.hw_drv->pwr_ctrl(0);
    //	for (uint32_t j = 0; j < 500; j++)
    //	{
    //		__NOP();
    //	}
    // }
    // for (uint32_t i = 0; i < 100; i++)
    //{
    //	sIMUHandle.hw_drv->pwr_ctrl(1);
    //	for (uint32_t j = 0; j < 2000; j++)
    //	{
    //		__NOP();
    //	}
    //	sIMUHandle.hw_drv->pwr_ctrl(0);
    //	for (uint32_t j = 0; j < 500; j++)
    //	{
    //		__NOP();
    //	}
    // }
    Clock_Wait(2);
    kprint("power on\r\n");
    sIMUHandle.hw_drv->pwr_ctrl(1);
    Clock_Wait(10);
    sIMUHandle.hw_drv->reset(1);
    Clock_Wait(600);
    uint8_t ret = sIMUHandle.hw_drv->init();
    if (ret != STD_SUCCESS) {
        kprint("failed to init hw\r\n");
        return ret;
    }
    ret = sIMUHandle.dev_drv->init(&sIMUHandle);
    if (ret != STD_SUCCESS) {
        kprint("failed to init dev\r\n");
        return ret;
    }
    kprint("ok\r\n");
    // kprint("size=%u\r\n", BURST16_DATA_LENGTH);
    return ret;
}

uint8_t imu_deinit()
{
    uint16_t a, b, c, d,
        veryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryLongDescription, firstValue,
        SecondValueVeryVeryVeryVeryLong;
    a = 0;
    b = 2 + 3;
    c <<= 0;

    c = (a * 10000) + ((b)*100) + (d);

    if (a && ((b) || (c))) {
        return 0;
    }
    switch (a) {
    case 0: {
        veryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryLongDescription
            ? firstValue
            : SecondValueVeryVeryVeryVeryLong;
    } break;
    case 1:
        a = 0;
        break;
    default:
        break;
    }
    kprint("doing\r\n");
    sIMUHandle.dev_drv->deinit(&sIMUHandle);
    sIMUHandle.hw_drv->deinit();
    sIMUHandle.hw_drv->reset(0);
    sIMUHandle.hw_drv->pwr_ctrl(0);
    return STD_SUCCESS;
}

uint8_t imu_get_raw(uint8_t *buff, uint16_t size)
{
    return sIMUHandle.dev_drv->read_raw(&sIMUHandle, buff, size);
}

uint8_t imu_enable()
{
    if (sIMUHandle.dev_drv->enable != NULL) {
        return sIMUHandle.dev_drv->enable(&sIMUHandle);
    }
    // while (sIMUHandle.hw_drv->get_ready() == 0)
    // for(uint32_t i = 0; i < 2000; i++)
    //{
    //	acc_gyro_get_raw_result(NULL, 0);
    // }
    sIMUHandle.hw_drv->enable_irq(1);
    return STD_SUCCESS;
}

uint8_t imu_disable()
{
    sIMUHandle.hw_drv->enable_irq(0);
    if (sIMUHandle.dev_drv->disable != NULL) {
        return sIMUHandle.dev_drv->disable(&sIMUHandle);
    }
    return STD_SUCCESS;
}

uint8_t imu_generate_swi(void)
{
    sIMUHandle.hw_drv->generate_swi();
    return STD_SUCCESS;
}

uint8_t imu_restore_config(void)
{
    uint8_t ret = sIMUHandle.dev_drv->restore(&sIMUHandle);
    if (ret != STD_SUCCESS) {
        kprint("failed to restore\r\n");
    } else {
        kprint("ok\r\n");
    }
    return ret;
}

uint8_t imu_get_all_reg(void)
{
    return adis16505_dev_read_all_reg(&sIMUHandle);
}

uint8_t imu_get_ready(void)
{
    return sIMUHandle.hw_drv->get_ready();
}

#pragma endregion
/*******************************************************************************
END
*******************************************************************************/
