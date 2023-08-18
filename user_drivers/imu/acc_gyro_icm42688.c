/*******************************************************************************
* file     accgyro.c
* author   mackgim
* version  V1.0.0
* date
* brief ??
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "acc_gyro_if.h"
#include "lsm6ds3_reg.h"
#include "standard_lib.h"

#define MEMS_SUCCESS   (0)

#define BOOT_TIME      15  // ms
#define WAIT_TIME_A    100 // ms
#define WAIT_TIME_G_01 150 // ms
#define WAIT_TIME_G_02 50  // ms

static ACC_SCALE_E  acc_sensitivity  = ACC_SCALE_2g;
static GYRO_SCALE_E gyro_sensitivity = GYPO_SCALE_500dps;

static lsm6ds3_odr_xl_t acc_output_data_rate  = 0;
static lsm6ds3_odr_g_t  gyro_output_data_rate = 0;

static ACC_GYRO_IRQ_TypeDef acc_gyro_irq = NULL;

static stmdev_ctx_t    lsm6ds3_dev_ctx;
static lsm6ds3_xl_fs_t lsm6ds3_acc_scale;
static lsm6ds3_fs_g_t  lsm6ds3_gypo_scale;

static lsm6ds3_odr_xl_t acc_gyro_hal_get_acc_output_data_rate(uint16_t odr)
{
    lsm6ds3_odr_xl_t new_odr;

    new_odr = (odr <= 13)     ? LSM6DS3_XL_ODR_12Hz5
              : (odr <= 26)   ? LSM6DS3_XL_ODR_26Hz
              : (odr <= 52)   ? LSM6DS3_XL_ODR_52Hz
              : (odr <= 104)  ? LSM6DS3_XL_ODR_104Hz
              : (odr <= 208)  ? LSM6DS3_XL_ODR_208Hz
              : (odr <= 416)  ? LSM6DS3_XL_ODR_416Hz
              : (odr <= 833)  ? LSM6DS3_XL_ODR_833Hz
              : (odr <= 1660) ? LSM6DS3_XL_ODR_1k66Hz
              : (odr <= 3330) ? LSM6DS3_XL_ODR_3k33Hz
                              : LSM6DS3_XL_ODR_6k66Hz;

    return new_odr;
}

static lsm6ds3_odr_g_t acc_gyro_hal_get_gyro_output_data_rate(uint16_t odr)
{
    lsm6ds3_odr_g_t new_odr;

    new_odr = (odr <= 13)    ? LSM6DS3_GY_ODR_12Hz5
              : (odr <= 26)  ? LSM6DS3_GY_ODR_26Hz
              : (odr <= 52)  ? LSM6DS3_GY_ODR_52Hz
              : (odr <= 104) ? LSM6DS3_GY_ODR_104Hz
              : (odr <= 208) ? LSM6DS3_GY_ODR_208Hz
              : (odr <= 416) ? LSM6DS3_GY_ODR_416Hz
              : (odr <= 833) ? LSM6DS3_GY_ODR_833Hz
                             : LSM6DS3_GY_ODR_1k66Hz;

    return new_odr;
}

static uint8_t acc_gyro_hal_set_acc_full_scale(ACC_SCALE_E scale)
{
    lsm6ds3_acc_scale = (scale == ACC_SCALE_2g)   ? LSM6DS3_2g
                        : (scale == ACC_SCALE_4g) ? LSM6DS3_4g
                        : (scale == ACC_SCALE_8g) ? LSM6DS3_8g
                                                  : LSM6DS3_16g;

    kprint("[LSM6DS3]: acc full scale 0x%x\r\n", lsm6ds3_acc_scale);

    if (lsm6ds3_xl_full_scale_set(&lsm6ds3_dev_ctx, lsm6ds3_acc_scale) != MEMS_SUCCESS) {
        return STD_FAILED;
    }

    return STD_SUCCESS;
}

static uint8_t acc_gyro_hal_set_gyro_full_scale(GYRO_SCALE_E scale)
{
    lsm6ds3_gypo_scale = (scale == GYPO_SCALE_125dps)    ? LSM6DS3_125dps
                         : (scale == GYPO_SCALE_250dps)  ? LSM6DS3_250dps
                         : (scale == GYPO_SCALE_500dps)  ? LSM6DS3_500dps
                         : (scale == GYPO_SCALE_1000dps) ? LSM6DS3_1000dps
                                                         : LSM6DS3_2000dps;

    kprint("[LSM6DS3]: gyro full scale 0x%x\r\n", lsm6ds3_gypo_scale);

    if (lsm6ds3_gy_full_scale_set(&lsm6ds3_dev_ctx, lsm6ds3_gypo_scale) != MEMS_SUCCESS) {
        return STD_FAILED;
    }

    return STD_SUCCESS;
}

static uint8_t acc_gyro_hal_set_acc_power_mode(uint8_t high_performance_en)
{
    lsm6ds3_xl_hm_mode_t val = !(high_performance_en) ? LSM6DS3_XL_NORMAL
                                                      : LSM6DS3_XL_HIGH_PERFORMANCE;
    kprint("[LSM6DS3]: acc hm_mode = 0x%x\r\n", val);

    if (lsm6ds3_xl_power_mode_set(&lsm6ds3_dev_ctx, val) != MEMS_SUCCESS) {
        return STD_FAILED;
    }

    return STD_SUCCESS;
}

static uint8_t acc_gyro_hal_set_gyro_power_mode(uint8_t high_performance_en)
{
    lsm6ds3_g_hm_mode_t val = !(high_performance_en) ? LSM6DS3_GY_NORMAL
                                                     : LSM6DS3_GY_HIGH_PERFORMANCE;
    kprint("[LSM6DS3]: gyro new_lp = 0x%x\r\n", val);

    if (lsm6ds3_gy_power_mode_set(&lsm6ds3_dev_ctx, val) != MEMS_SUCCESS) {
        return STD_FAILED;
    }

    return STD_SUCCESS;
}

static uint8_t acc_gyro_hal_checkOut(void)
{
    uint8_t whoamI = 0, rst;

    for (uint8_t i = 0; i <= 10; i++) {
        if (lsm6ds3_device_id_get(&lsm6ds3_dev_ctx, &whoamI) != MEMS_SUCCESS) {
            continue;
        }

        if (whoamI != LSM6DS3_ID) {
            if (i == 10) {
                kprint("[LSM6DS3]: failed - %u ID = 0x%x , Read ID = 0x%x\r\n", i, LSM6DS3_ID, whoamI);
                return STD_FAILED;
            }
        } else {
            break;
        }
    }
    kprint("[LSM6DS3]: True - Read ID 0x%x (LSM6DS3 0x6a - LSM6DS3 0x69)\r\n", whoamI);

    /* Restore default configuration */
    lsm6ds3_reset_set(&lsm6ds3_dev_ctx, PROPERTY_ENABLE);

    for (uint8_t i = 0; i <= 10; i++) {
        if ((lsm6ds3_reset_get(&lsm6ds3_dev_ctx, &rst) == MEMS_SUCCESS) || !rst) {
            break;
        }

        if (i == 10) {
            kprint("[LSM6DS3]: failed - rst device\r\n");
            return STD_FAILED;
        }
    }

    kprint("[LSM6DS3]: True - ACC Read and Write\r\n");
    return STD_SUCCESS;
}

static uint8_t acc_gyro_hal_init_tap()
{
    if (lsm6ds3_tap_detection_on_z_set(&lsm6ds3_dev_ctx, PROPERTY_ENABLE) != MEMS_SUCCESS) {
        goto clean;
    }

    if (lsm6ds3_tap_detection_on_y_set(&lsm6ds3_dev_ctx, PROPERTY_ENABLE) != MEMS_SUCCESS) {
        goto clean;
    }

    if (lsm6ds3_tap_detection_on_x_set(&lsm6ds3_dev_ctx, PROPERTY_ENABLE) != MEMS_SUCCESS) {
        goto clean;
    }

    if (lsm6ds3_int_notification_set(&lsm6ds3_dev_ctx, LSM6DS3_INT_LATCHED) != MEMS_SUCCESS) {
        goto clean;
    }

    // 1*FS_XL/2**5   500mg
    if (lsm6ds3_tap_threshold_set(&lsm6ds3_dev_ctx, 0x1) != MEMS_SUCCESS) {
        goto clean;
    }
    //2*8 / ODR_XL   2*8/100 = 160ms    3*8/832 = 28ms
    if (lsm6ds3_tap_shock_set(&lsm6ds3_dev_ctx, 0x1) != MEMS_SUCCESS) {
        goto clean;
    }

    //1*4 / ODR_XL   3*4/100 = 120ms  6*8/832 = 28ms
    if (lsm6ds3_tap_quiet_set(&lsm6ds3_dev_ctx, 0x3) != MEMS_SUCCESS) {
        goto clean;
    }

    kprint("[LSM6DS3]: true - Init tap\r\n");
    return STD_SUCCESS;

clean:
    kprint("[LSM6DS3]: failed - Init tap\r\n");
    return STD_FAILED;
}

static uint8_t acc_gyro_hal_init(ACC_GYRO_CONFIG_TypeDef *config)
{
    /* Initialize mems driver interface */
    lsm6ds3_dev_ctx.write_reg = config->write_buffer;
    lsm6ds3_dev_ctx.read_reg  = config->read_buffer;
    lsm6ds3_dev_ctx.handle    = config->handle;

    acc_gyro_irq          = config->irq_enable;
    acc_sensitivity       = config->acc_scale;
    gyro_sensitivity      = config->gyro_scale;
    acc_output_data_rate  = acc_gyro_hal_get_acc_output_data_rate(config->output_data_rate);
    gyro_output_data_rate = acc_gyro_hal_get_gyro_output_data_rate(config->output_data_rate);

    kprint("\r\n");

    if (acc_gyro_hal_checkOut() != STD_SUCCESS) {
        goto clean;
    }

    /* Enable register address automatically incremented during a multiple byte
	access with a serial interface. */
    if (lsm6ds3_auto_increment_set(&lsm6ds3_dev_ctx, PROPERTY_ENABLE) != MEMS_SUCCESS) {
        goto clean;
    }

    /* Enable Block Data Update */
    if (lsm6ds3_block_data_update_set(&lsm6ds3_dev_ctx, PROPERTY_ENABLE) != MEMS_SUCCESS) {
        goto clean;
    }

    if ((acc_gyro_hal_set_acc_power_mode(config->high_performance)) != STD_SUCCESS) {
        goto clean;
    }

    if ((acc_gyro_hal_set_gyro_power_mode(config->high_performance)) != STD_SUCCESS) {
        goto clean;
    }

    if (acc_gyro_hal_set_acc_full_scale(config->acc_scale) != STD_SUCCESS) {
        goto clean;
    }

    if (acc_gyro_hal_set_gyro_full_scale(config->gyro_scale) != STD_SUCCESS) {
        goto clean;
    }

    /* Configure filtering chain(No aux interface) */
    /* Accelerometer - analog filter */
    if (lsm6ds3_xl_filter_analog_set(&lsm6ds3_dev_ctx, LSM6DS3_ANTI_ALIASING_400Hz) != MEMS_SUCCESS) {
        goto clean;
    }
    /* Accelerometer - LPF1 path ( LPF2 not used )*/
    // lsm6ds3_xl_lp1_bandwidth_set(&dev_ctx, LSM6DS3_XL_LP1_ODR_DIV_4);
    /* Accelerometer - LPF1 + LPF2 path */
    if (lsm6ds3_xl_lp2_bandwidth_set(&lsm6ds3_dev_ctx, LSM6DS3_XL_LP_ODR_DIV_100) != MEMS_SUCCESS) {
        goto clean;
    }
    /* Accelerometer - High Pass / Slope path */
    // lsm6ds3_xl_reference_mode_set(&lsm6ds3_dev_ctx, PROPERTY_DISABLE);
    // lsm6ds3_xl_hp_bandwidth_set(&lsm6ds3_dev_ctx, LSM6DS3_XL_HP_ODR_DIV_100);
    /* Gyroscope - filtering chain */
    if (lsm6ds3_gy_hp_bandwidth_set(&lsm6ds3_dev_ctx, LSM6DS3_HP_CUT_OFF_2Hz07) != MEMS_SUCCESS) {
        goto clean;
    }

    if (lsm6ds3_gy_data_rate_set(&lsm6ds3_dev_ctx, LSM6DS3_GY_ODR_OFF) != MEMS_SUCCESS) {
        goto clean;
    }

    if (lsm6ds3_xl_data_rate_set(&lsm6ds3_dev_ctx, LSM6DS3_XL_ODR_OFF) != MEMS_SUCCESS) {
        goto clean;
    }

    if (acc_gyro_hal_init_tap() != STD_SUCCESS) {
        goto clean;
    }

    //???G?????§Ø?
    lsm6ds3_int1_route_t int1_route_val;
    lsm6ds3_int2_route_t int2_route_val;

    memset(&int1_route_val, 0, sizeof(int1_route_val));
    memset(&int2_route_val, 0, sizeof(int2_route_val));
#ifdef ACC_GYRO_INT1
    int1_route_val.int1_single_tap = PROPERTY_ENABLE;
    int1_route_val.int1_double_tap = PROPERTY_ENABLE;
    int1_route_val.int1_drdy_g     = PROPERTY_ENABLE;
#elif defined(ACC_GYRO_INT2)
    int2_route_val.int2_single_tap = PROPERTY_ENABLE;
    int2_route_val.int2_double_tap = PROPERTY_ENABLE;
    int2_route_val.int2_drdy_g     = PROPERTY_ENABLE;
#endif

    if (lsm6ds3_pin_int1_route_set(&lsm6ds3_dev_ctx, &int1_route_val) != MEMS_SUCCESS) {
        goto clean;
    }
    if (lsm6ds3_pin_int2_route_set(&lsm6ds3_dev_ctx, &int2_route_val) != MEMS_SUCCESS) {
        goto clean;
    }

    kprint("[LSM6DS3]: True - Init\r\n");
    kprint("\r\n");
    return STD_SUCCESS;

clean:
    kprint("[LSM6DS3]: False - Init\r\n");
    kprint("\r\n");
    return STD_FAILED;
}

static uint8_t acc_gyro_hal_disable()
{
    if (acc_gyro_irq)
        acc_gyro_irq(false);

    if (lsm6ds3_gy_data_rate_set(&lsm6ds3_dev_ctx, LSM6DS3_GY_ODR_OFF) != MEMS_SUCCESS) {
        return STD_FAILED;
    }

    if (lsm6ds3_xl_data_rate_set(&lsm6ds3_dev_ctx, LSM6DS3_XL_ODR_OFF) != MEMS_SUCCESS) {
        return STD_FAILED;
    }

    return STD_SUCCESS;
}

static uint8_t acc_gyro_hal_deinit()
{
    return acc_gyro_hal_disable();
}

static uint8_t acc_gyro_hal_enable()
{
    if (lsm6ds3_gy_data_rate_set(&lsm6ds3_dev_ctx, gyro_output_data_rate) != MEMS_SUCCESS) {
        return STD_FAILED;
    }

    if (lsm6ds3_xl_data_rate_set(&lsm6ds3_dev_ctx, acc_output_data_rate) != MEMS_SUCCESS) {
        return STD_FAILED;
    }

    if (acc_gyro_irq)
        acc_gyro_irq(true);

    return STD_SUCCESS;
}

static uint8_t acc_gyro_hal_get_acc_mg(float acc[3], int16_t raw[3], lsm6ds3_xl_fs_t fs_xl)
{
    if (!acc)
        return STD_FAILED;

    for (uint8_t i = 0; i < 3; i++) {
        switch (fs_xl) {
        case LSM6DS3_2g:
            acc[i] = lsm6ds3_from_fs2g_to_mg(raw[i]);
            break;

        case LSM6DS3_4g:
            acc[i] = lsm6ds3_from_fs4g_to_mg(raw[i]);
            break;

        case LSM6DS3_8g:
            acc[i] = lsm6ds3_from_fs8g_to_mg(raw[i]);
            break;

        case LSM6DS3_16g:
            acc[i] = lsm6ds3_from_fs16g_to_mg(raw[i]);
            break;

        default:
            acc[0] = 0;
            acc[1] = 0;
            acc[2] = 0;
            return STD_FAILED;
        }
    }

    return STD_SUCCESS;
}

static uint8_t acc_gyro_hal_get_gyro_mdps(float gyro[3], int16_t raw[3], lsm6ds3_fs_g_t fs_g)
{
    if (!gyro)
        return STD_FAILED;

    for (uint8_t i = 0; i < 3; i++) {
        switch (fs_g) {
        case LSM6DS3_125dps:
            gyro[i] = lsm6ds3_from_fs125dps_to_mdps(raw[i]);
            break;

        case LSM6DS3_250dps:
            gyro[i] = lsm6ds3_from_fs250dps_to_mdps(raw[i]);
            break;

        case LSM6DS3_500dps:
            gyro[i] = lsm6ds3_from_fs500dps_to_mdps(raw[i]);
            break;

        case LSM6DS3_1000dps:
            gyro[i] = lsm6ds3_from_fs1000dps_to_mdps(raw[i]);
            break;

        case LSM6DS3_2000dps:
            gyro[i] = lsm6ds3_from_fs2000dps_to_mdps(raw[i]);
            break;

        default:
            gyro[0] = 0;
            gyro[1] = 0;
            gyro[2] = 0;
            return STD_FAILED;
        }
    }

    return STD_SUCCESS;
}

#include "platform.h"
static uint8_t acc_gyro_hal_get_result(float acc[3], float gyro[3], uint32_t number)
{
    int16_t  raw_data[3];
    uint16_t retry = 0, num = 0;
    lsm6ds3_status_reg_t reg_status;

    /* Read samples in polling mode (no int) */
    for (; retry < 15 && num < number; retry++) {
        /* Read output only if new value is available */
        if ((lsm6ds3_status_reg_get(&lsm6ds3_dev_ctx, &reg_status) != MEMS_SUCCESS) ||
            !(reg_status.xlda) || !(reg_status.gda))
            continue;
        /* Read magnetic field data */
        memset(raw_data, 0x00, 3 * sizeof(int16_t));
        if (lsm6ds3_acceleration_raw_get(&lsm6ds3_dev_ctx, raw_data) != MEMS_SUCCESS) {
            continue;
        }

        acc_gyro_hal_get_acc_mg(acc, raw_data, lsm6ds3_acc_scale);

        /* Read magnetic field data */
        memset(raw_data, 0x00, 3 * sizeof(int16_t));

        if (lsm6ds3_angular_rate_raw_get(&lsm6ds3_dev_ctx, raw_data) != MEMS_SUCCESS) {
            continue;
        }

        acc_gyro_hal_get_gyro_mdps(gyro, raw_data, lsm6ds3_gypo_scale);

        retry = 0;
        num++;

#if 0
        int16_t data_raw_temperature;

        if (reg.status_reg.tda) {
            /* Read temperature data */
            memset(&data_raw_temperature, 0x00, sizeof(int16_t));
            lsm6ds3_temperature_raw_get(&dev_ctx, &data_raw_temperature);
            temperature_degC = lsm6ds3_from_lsb_to_celsius(
                data_raw_temperature);
            sprintf((char *)tx_buffer, "Temperature [degC]:%6.2f\r\n",
                    temperature_degC);
            tx_com(tx_buffer, strlen((char const *)tx_buffer));
        }
#endif
    }

    if (retry == 15 || num != number)
        return STD_FAILED;

    return STD_SUCCESS;
}

static uint8_t acc_gyro_hal_get_tap_status(lsm6ds3_tap_src_t *value)
{
    lsm6ds3_all_src_t sources;

    if (!lsm6ds3_all_sources_get(&lsm6ds3_dev_ctx, &sources))
        return STD_FAILED;

    *value = sources.tap_src;

    return STD_SUCCESS;
}

static uint8_t acc_gyro_hal_set_tap_threshold(uint32_t value)
{
    // 9*FS_XL/2**5   562.5mg  when FS_XL = 2G

    if (value > 31) {
        return STD_FAILED;
    }
    uint8_t threshold = (uint8_t)value & 0xff;
    //9 * FS_XL / 2 * *5   562.5mg

    if (lsm6ds3_tap_threshold_set(&lsm6ds3_dev_ctx, threshold) != MEMS_SUCCESS) {
        return STD_FAILED;
    }

    kprint("[acc]:set tag threhold %fmg\r\n", (float)threshold * 16 * 1000 / 32);
    return STD_SUCCESS;
}

#pragma region incall

#pragma endregion

#pragma region acc gyro interface

uint8_t acc_gyro_init(void *config)
{
    ACC_GYRO_CONFIG_TypeDef *cfg = config;

    return acc_gyro_hal_init(cfg);
}

uint8_t acc_gyro_deinit(void *config)
{
    return acc_gyro_hal_deinit();
}

uint8_t acc_gyro_enable()
{
    return acc_gyro_hal_enable();
}

uint8_t acc_gyro_disable()
{
    return acc_gyro_hal_disable();
}

uint8_t acc_gyro_get_result(float acc[3], float gyro[3], uint32_t number)
{
    return acc_gyro_hal_get_result(acc, gyro, number);
}

uint8_t acc_gyro_get_tap_status(uint8_t *value)
{
    return acc_gyro_hal_get_tap_status((lsm6ds3_tap_src_t *)value);
}

uint8_t acc_gyro_set_tap_threshold(uint32_t value)
{
    return acc_gyro_hal_set_tap_threshold(value);
}

ACC_SCALE_E acc_gyro_get_acc_sensitivity(void)
{
    return acc_sensitivity;
}
GYRO_SCALE_E acc_gyro_get_gyro_sensitivity(void)
{
    return gyro_sensitivity;
}

#pragma endregion

/*******************************************************************************
END
*******************************************************************************/
