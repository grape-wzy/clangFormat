#include "standard_lib.h"
#include "gtimer.h"

#include "drv_icm426xx_config.h"
#include "driver/Icm426xxDriver_HL.h"
#include "Helpers/Icm426xx/helperClockCalib.h"
#include "Invn/InvError.h"

#include "imu_dev.h"

/* structure allowing to handle clock calibration for ICM device timestamping */
static clk_calib_t clk_calib;

/* --------------------------------------------------------------------------------------
 *  Extern functions definition
 * -------------------------------------------------------------------------------------- */

/*
 * Icm426xx driver needs to get time in us. Let's give its implementation here.
 */
uint64_t inv_icm426xx_get_time_us(void)
{
    return TICKS_TO_US(Clock_Tick());
}

/*
 * Icm426xx driver needs a sleep feature from external device. Thus inv_icm426xx_sleep_us
 * is defined as extern symbol in driver. Let's give its implementation here.
 */
void inv_icm426xx_sleep_us(uint32_t us)
{
    Clock_Wait_us((uint64_t)us);
}

/*
 * Clock calibration module needs to disable IRQ. Thus inv_helper_disable_irq is
 * defined as extern symbol in clock calibration module. Let's give its implementation
 * here.
 */
void inv_helper_disable_irq(void)
{
}

/*
 * Clock calibration module needs to enable IRQ. Thus inv_helper_enable_irq is
 * defined as extern symbol in clock calibration module. Let's give its implementation
 * here.
 */
void inv_helper_enable_irq(void)
{
}

static int icm426xx_write_reg(struct inv_icm426xx_serif *serif, uint8_t reg, const uint8_t *buff, uint32_t len)
{
    uint8_t  ret, write_buff[32];
    uint16_t real_size;

    struct imu_device *icm_dev = serif->context;

    memset(write_buff, 0xFF, sizeof(write_buff));
    write_buff[0] = (reg & 0x7F);

    real_size = len + 1;
    if (real_size > sizeof(write_buff))
        real_size = sizeof(write_buff);

    memcpy(&write_buff[1], buff, real_size - 1);

    if (icm_dev->if_ops->transmit_receive != NULL) {
        ret = icm_dev->if_ops->transmit_receive(&write_buff[0], real_size, NULL, 0, 1000);
    }

    if (ret == STD_SUCCESS)
        return 0;

    return INV_ERROR_IO;
}

static int icm426xx_read_reg(struct inv_icm426xx_serif *serif, uint8_t reg, uint8_t *buff, uint32_t len)
{
    uint8_t ret, write_data;

    struct imu_device *icm_dev = serif->context;

    write_data = (reg | 0x80);

    if (icm_dev->if_ops->transmit_receive != NULL) {
        ret = icm_dev->if_ops->transmit_receive(&write_data, 1, buff, len, 1000);
    }

    if (ret == STD_SUCCESS)
        return 0;

    return INV_ERROR_IO;
}

static void icm426xx_sensor_event_cb(inv_icm426xx_sensor_event_t *event)
{
}

/**
 * \brief This function configures the device in order to output gyro and accelerometer.
 *
 * It initialyses clock calibration module (this will allow to extend the 16 bits
 * timestamp produced by Icm426xx to a 64 bits timestamp).
 * Then function sets full scale range and frequency for both accel and gyro and it
 * starts them in the requested power mode. (note that low-power mode is not available
 * for gyroscope in Icm426xx).
 *
 * \return 0 on success, negative value on error.
 */
static int ConfigureInvDevice(struct inv_icm426xx *icm426xx_block)
{
    int rc = 0;

#if ICM426XX_USE_CLK_IN
#if defined(ICM42633)
    /*
	 * ICM42633 is a triple interface device. To access CLKIN, AUX2 interface needs to be disabled.
	 * Use INV_ICM426XX_DUAL_INTERFACE mode. The following mode are also compatible:
	 *  - INV_ICM426XX_SINGLE_INTERFACE
	 *  - INV_ICM426XX_DUAL_INTERFACE_SPI4
	 */
    rc |= inv_icm426xx_interface_change_procedure(icm426xx_block, INV_ICM426XX_DUAL_INTERFACE);
#endif
    /* Enable CLKIN */
    if (inv_icm426xx_enable_clkin_rtc(icm426xx_block, 1) < 0) {
        INV_MSG(INV_MSG_LEVEL_ERROR, "Couldn't enable CLKIN");
        return -1;
    }
    clock_calibration_reset(icm426xx_block, &clk_calib);
    /* Ensure all coefficients are set to 1 as the clock will not drift */
    clk_calib.coef[INV_ICM426XX_PLL]    = 1.0f;
    clk_calib.coef[INV_ICM426XX_RC_OSC] = 1.0f;
    clk_calib.coef[INV_ICM426XX_WU_OSC] = 1.0f;
#else
    rc |= clock_calibration_init(icm426xx_block, &clk_calib);
#endif

#if ICM426XX_IS_HIGH_RES_MODE
    rc |= inv_icm426xx_enable_high_resolution_fifo(icm426xx_block);
#else
    rc |= inv_icm426xx_set_accel_fsr(icm426xx_block, ICM426XX_ACCEL_FSR);
    rc |= inv_icm426xx_set_gyro_fsr(icm426xx_block, ICM426XX_GYRO_FSR);
#endif

    rc |= inv_icm426xx_set_accel_frequency(icm426xx_block, ICM426XX_ACCEL_FREQ);
    rc |= inv_icm426xx_set_gyro_frequency(icm426xx_block, ICM426XX_GYRO_FREQ);

    return rc;
}

static uint8_t icm426xx_ops_init(struct imu_device *dev)
{
    struct inv_icm426xx_serif icm426xx_serif = {
        .configure  = NULL,
        .read_reg   = icm426xx_read_reg,
        .write_reg  = icm426xx_write_reg,
        .max_read   = 0xFFFF,
        .max_write  = 31,
        .serif_type = ICM426XX_UI_SPI4,
        .context    = dev, /*! point to imu_device */
    };

    int     rc = 0;
    uint8_t who_am_i;

    if (!dev || !dev->ss_data)
        return STD_INVALID_ARG;

    struct inv_icm426xx *icm_ctrl_blk = (struct inv_icm426xx *)dev->ss_data;

    rc |= inv_icm426xx_init(icm_ctrl_blk, &icm426xx_serif, icm426xx_sensor_event_cb);
    if (rc != INV_ERROR_SUCCESS) {
        nprint("!!! ERROR : failed to initialize Icm426xx.");
        return STD_ERROR;
    }

    rc = inv_icm426xx_get_who_am_i(icm_ctrl_blk, &who_am_i);
    if (rc != INV_ERROR_SUCCESS) {
        nprint("!!! ERROR : failed to read Icm426xx whoami value.");
        return STD_ERROR;
    }

    if (who_am_i != ICM_WHOAMI) {
        nprint("!!! ERROR :  bad WHOAMI value. Got 0x%02x (expected: 0x%02x)",
               who_am_i, ICM_WHOAMI);
        return STD_ERROR;
    }

    rc |= ConfigureInvDevice(icm_ctrl_blk);
    if (rc != INV_ERROR_SUCCESS) {
        nprint("!!! ERROR : failed to configure Icm426xx.");
        return STD_ERROR;
    }

    return STD_SUCCESS;
}

static uint8_t icm426xx_ops_enable(struct imu_device *dev)
{
    int rc = 0;

    if (!dev || !dev->ss_data)
        return STD_INVALID_ARG;

    struct inv_icm426xx *icm_ctrl_blk = (struct inv_icm426xx *)dev->ss_data;

#if ICM426XX_IS_LOW_NOISE_MODE
    rc |= inv_icm426xx_enable_accel_low_noise_mode(icm_ctrl_blk);
#else
    rc |= inv_icm426xx_enable_accel_low_power_mode(icm_ctrl_blk);
#endif

    rc |= inv_icm426xx_enable_gyro_low_noise_mode(icm_ctrl_blk);

    if (rc != INV_ERROR_SUCCESS) {
        nprint("!!! ERROR : failed to enable Icm426xx.");
        return STD_ERROR;
    }

    return STD_SUCCESS;
}

static uint8_t icm426xx_ops_disable(struct imu_device *dev)
{
    int rc = 0;

    if (!dev || !dev->ss_data)
        return STD_INVALID_ARG;

    struct inv_icm426xx *icm_ctrl_blk = (struct inv_icm426xx *)dev->ss_data;

    rc |= inv_icm426xx_disable_accel(icm_ctrl_blk);
    rc |= inv_icm426xx_disable_gyro(icm_ctrl_blk);

    if (rc != INV_ERROR_SUCCESS) {
        nprint("!!! ERROR : failed to enable Icm426xx.");
        return STD_ERROR;
    }

    return STD_SUCCESS;
}

/* Icm426xx driver object */
static struct inv_icm426xx icm426xx_driver_block;

static struct imu_sensor_ops icm426xx_ops = {
    .init       = icm426xx_ops_init,
    .enable     = icm426xx_ops_enable,
    .disable    = icm426xx_ops_disable,
    .data_ready = NULL,
    .read_data  = NULL,
    .control    = NULL,
};

uint8_t icm426xx_dev_init(imu_sensor_t sensor, struct imu_hw_interface_ops *if_ops)
{
    return imu_dev_register(sensor, &icm426xx_ops, &icm426xx_driver_block);
}
