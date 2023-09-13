#include "imu_dev.h"
#include "standard_lib.h"

static struct imu_device imu_devs[IMU_MODEL_MAX];

uint8_t imu_dev_init(imu_sensor_t sensor, struct imu_hw_interface_ops *if_ops, _pin_ctrl pin_ctrl)
{
    uint8_t ret;

    if (sensor >= IMU_MODEL_MAX || IMU_MODEL_MAX == 0 || if_ops == NULL)
        return STD_ERROR;

    imu_devs[sensor].if_ops      = if_ops;
    imu_devs[sensor].ss_pin_ctrl = pin_ctrl;

    if (if_ops->init)
        if_ops->init();

#ifdef IMU_USE_ICM426XX
    extern uint8_t icm426xx_dev_init(imu_sensor_t sensor, struct imu_hw_interface_ops * if_ops);

    if (sensor == IMU_MODEL_ICM426xx) {
        ret = icm426xx_dev_init(sensor, if_ops);
        if (STD_SUCCESS != ret)
            return ret;

        if (imu_devs[sensor].ss_ops->init)
            return imu_devs[sensor].ss_ops->init(&imu_devs[sensor]);
    }
#endif /* IMU_USE_ICM426XX */
#ifdef IMU_USE_LSM6DS3
    extern uint8_t lsm6ds3_dev_init(imu_sensor_t sensor, struct imu_hw_interface_ops * if_ops);

    if (sensor == IMU_MODEL_LSM6DS3) {
        ret = lsm6ds3_dev_init(sensor, if_ops);
        if (STD_SUCCESS != ret)
            return ret;

        if (imu_devs[sensor].ss_ops->init)
            return imu_devs[sensor].ss_ops->init(&imu_devs[sensor]);
    }
#endif /* IMU_USE_LSM6DS3 */

    return STD_SUCCESS;
}

uint8_t imu_dev_enable(imu_sensor_t sensor)
{
    if (sensor >= IMU_MODEL_MAX || IMU_MODEL_MAX == 0)
        return STD_NO_INIT;

    if (imu_devs[sensor].if_ops && imu_devs[sensor].if_ops->init)
        imu_devs[sensor].if_ops->init();

    if (imu_devs[sensor].ss_ops->enable)
        return imu_devs[sensor].ss_ops->enable(&imu_devs[sensor]);

    return STD_SUCCESS;
}

uint8_t imu_dev_disable(imu_sensor_t sensor)
{
    if (sensor >= IMU_MODEL_MAX || IMU_MODEL_MAX == 0)
        return STD_NO_INIT;

    if (imu_devs[sensor].ss_ops->disable)
        imu_devs[sensor].ss_ops->disable(&imu_devs[sensor]);

    if (imu_devs[sensor].if_ops && imu_devs[sensor].if_ops->deinit)
        imu_devs[sensor].if_ops->deinit();

    return STD_SUCCESS;
}

uint8_t imu_dev_data_ready(imu_sensor_t sensor)
{
    if (sensor >= IMU_MODEL_MAX || IMU_MODEL_MAX == 0)
        return STD_NO_INIT;

    if (imu_devs[sensor].ss_ops->data_ready)
        return imu_devs[sensor].ss_ops->data_ready(&imu_devs[sensor]);

    return STD_SUCCESS;
}

uint32_t imu_dev_read_data(imu_sensor_t sensor, void *buffer, uint32_t size)
{
    if (sensor >= IMU_MODEL_MAX || IMU_MODEL_MAX == 0)
        return STD_NO_INIT;

    if (imu_devs[sensor].ss_ops->read_data)
        return imu_devs[sensor].ss_ops->read_data(&imu_devs[sensor], buffer, size);

    return 0;
}

uint8_t imu_dev_control(imu_sensor_t sensor, uint32_t cmd, void *args)
{
    if (sensor >= IMU_MODEL_MAX || IMU_MODEL_MAX == 0)
        return STD_NO_INIT;

    if (imu_devs[sensor].ss_ops->control)
        return imu_devs[sensor].ss_ops->control(&imu_devs[sensor], cmd, args);

    return STD_SUCCESS;
}

uint8_t imu_dev_pin_ctrl(imu_sensor_t sensor, void *pin, void *arg)
{
    if (sensor >= IMU_MODEL_MAX || IMU_MODEL_MAX == 0)
        return STD_NO_INIT;

    if (imu_devs[sensor].ss_pin_ctrl)
        return imu_devs[sensor].ss_pin_ctrl(pin, arg);

    return STD_SUCCESS;
}

uint8_t imu_dev_register(imu_sensor_t sensor, struct imu_sensor_ops *ops, void *priv_data)
{
    if (sensor >= IMU_MODEL_MAX || IMU_MODEL_MAX == 0 || ops == NULL)
        return STD_NO_INIT;

    imu_devs[sensor].ss_ops  = ops;
    imu_devs[sensor].ss_data = priv_data;

    return STD_SUCCESS;
}
