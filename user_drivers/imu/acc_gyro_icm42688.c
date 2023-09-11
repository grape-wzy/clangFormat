/*******************************************************************************
* file     accgyro.c
* author   mackgim
* version  V1.0.0
* date
* brief ??
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "acc_gyro_if.h"
#include "driver/Icm426xxDriver_HL.h"
#include "standard_lib.h"
#include "gtimer.h"

#define MEMS_SUCCESS   (0)

/* Icm426xx driver object */
static struct inv_icm426xx icm_driver;

static icm4x6xx_delay_ms(uint16_t ms)
{
    uint64_t m = (uint64_t)ms;
    Clock_Wait(m);
}

static icm4x6xx_delay_us(uint16_t us)
{
    uint64_t u = (uint64_t)us;
    Clock_Wait_us(u);
}

uint8_t acc_gyro_init(void *config)
{
    ACC_GYRO_CONFIG_TypeDef *cfg = config;

    inv_icm4x6xx_set_serif(cfg->read_buffer, cfg->write_buffer);

    inv_icm4x6xx_set_delay(icm4x6xx_delay_ms, icm4x6xx_delay_us);

    if (0 != inv_icm4x6xx_initialize())
        return STD_FAILED;

    inv_icm4x6xx_acc_set_rate(100, 2);  //200Hz, watermark 2.
    inv_icm4x6xx_gyro_set_rate(200, 4); //100Hz, watermark 2.

    return STD_SUCCESS;
}

uint8_t acc_gyro_deinit(void *config)
{
    inv_icm4x6xx_acc_disable();
    inv_icm4x6xx_gyro_disable();

    return STD_SUCCESS;
}

uint8_t acc_gyro_enable()
{
    inv_icm4x6xx_acc_enable();
    inv_icm4x6xx_gyro_enable();

    return STD_SUCCESS;
}

uint8_t acc_gyro_disable()
{
    inv_icm4x6xx_acc_disable();
    inv_icm4x6xx_gyro_disable();

    return STD_SUCCESS;
}

static struct accGyroDataPacket icm42688_data;

uint8_t acc_gyro_get_result(float acc[3], float gyro[3], uint32_t number)
{
    uint32_t retry = 0, num = 0;

    for (uint8_t i = 0; i < 3; i++) {
        acc[i]  = 0;
        gyro[i] = 0;
    }

    /* Read samples in polling mode (no int) */
    for (; retry < 100 && num < number; retry++) {
        if (inv_icm4x6xx_get_rawdata(&icm42688_data) != 0)
            continue;

        if (icm42688_data.accOutSize == 0 || icm42688_data.gyroOutSize == 0)
            continue;

        for (uint16_t count = 0; count < icm42688_data.accOutSize + icm42688_data.gyroOutSize; count++) {
            if (icm42688_data.outBuf[count].sensType == ACC) {
                acc[0] += icm42688_data.outBuf[count].x;
                acc[1] += icm42688_data.outBuf[count].y;
                acc[2] += icm42688_data.outBuf[count].z;
            }
            if (icm42688_data.outBuf[count].sensType == GYR) {
                gyro[0] += icm42688_data.outBuf[count].x;
                gyro[1] += icm42688_data.outBuf[count].y;
                gyro[2] += icm42688_data.outBuf[count].z;
            }
        }

        acc[0] = acc[0] / icm42688_data.accOutSize;
        acc[1] = acc[1] / icm42688_data.accOutSize;
        acc[2] = acc[2] / icm42688_data.accOutSize;

        gyro[0] = gyro[0] / icm42688_data.gyroOutSize;
        gyro[1] = gyro[1] / icm42688_data.gyroOutSize;
        gyro[2] = gyro[2] / icm42688_data.gyroOutSize;
        num++;
    }

    if (retry == 15 || num != number)
        return STD_FAILED;

    return STD_SUCCESS;
}

uint8_t acc_gyro_get_tap_status(uint8_t *value)
{
    return 0;
}

uint8_t acc_gyro_set_tap_threshold(uint32_t value)
{
    return 0;
}

ACC_SCALE_E acc_gyro_get_acc_sensitivity(void)
{
    return 0;
}
GYRO_SCALE_E acc_gyro_get_gyro_sensitivity(void)
{
        return 0;
}

/*******************************************************************************
END
*******************************************************************************/
