#ifndef __IMU_IF_H
#define __IMU_IF_H

#include "stdint.h"

#define IMU_USE_ICM426XX

#ifdef IMU_USE_ICM426XX
#include "./sensors/icm426xx/drv_icm426xx.h"
#endif /* IMU_USE_ICM426XX */

typedef enum {
#ifdef IMU_USE_ICM426XX
    IMU_MODEL_ICM426xx,
#endif /* IMU_USE_ICM426XX */
#ifdef IMU_USE_LSM6DS3
    IMU_MODEL_LSM6DS3,
#endif /* IMU_USE_LSM6DS3 */
    IMU_MODEL_MAX,
} imu_sensor_t;

struct imu_device;

/*
 * Sensor device operation function, completed by the lower sensor adaptation layer for adaptation
 */
struct imu_sensor_ops {
    /**
     * @brief Initialize the sensor device.
     *
     * @param[in] dev is the sensor device object.
     *
     * @return according to the STD_Exx.
     */
    uint8_t (*init)(struct imu_device *dev);

    /**
     * @brief Enable the sensor device.
     *
     * @param[in] dev is the sensor device object.
     *
     * @return according to the STD_Exx.
     */
    uint8_t (*enable)(struct imu_device *dev);

    /**
     * @brief Disable the sensor device.
     *
     * @param[in] dev is the sensor device object.
     *
     * @return according to the STD_Exx.
     */
    uint8_t (*disable)(struct imu_device *dev);

    /**
     * @brief Get the data status of the sensor device.
     *
     * @param[in] dev is the sensor device object.
     *
     * @return 0 indicates that the sensor data is ready to be read.
     *         Otherwise, the sensor data is not ready.
     */
    uint8_t (*data_ready)(struct imu_device *dev);

    /**
     * @brief Reading a number of data from the sensor device.
     *
     * @param[in] dev is the sensor device object.
     *
     * @param[out] buffer is the address for receiving data.
     *
     * @param[in] buff_size is the size of the buffer.
     *
     * @retval The actual amount of data read in bytes.
     */
    uint32_t (*read_data)(struct imu_device *dev, void *buffer, uint32_t buff_size);

    /**
     * @brief Control of configure the IMU sensor.
     *
     * @param[in] dev is the sensor device object.
     *
     * @param[in] cmd is the command code.
     *
     * @param[in] size is the command arguments.
     *
     * @return according to the STD_Exx.
     */
    uint8_t (*control)(struct imu_device *dev, uint32_t cmd, void *config);
};

/*
 * Interface layer adaptation function, passed in by the user during sensor initialization
 */
struct imu_hw_interface_ops {
    // 接口初始化
    uint8_t (*init)(void);
    // 接口复位
    uint8_t (*deinit)(void);
    // 接口收发
    uint8_t (*transmit_receive)(uint8_t *send, uint16_t send_size,
                                uint8_t *recv, uint16_t recv_size,
                                uint32_t timeout);
};

/* typedef callback functions for pin control */
typedef uint8_t (*_pin_ctrl)(void *pin, void *arg);

/*
 * The IMU sensor device control block.
 */
struct imu_device {
    struct imu_hw_interface_ops *if_ops;

    struct imu_sensor_ops *ss_ops;

    _pin_ctrl ss_pin_ctrl;

    void *ss_data;
};

/**
 * @brief Initialize an IMU sensor.
 *
 * @param[in] sensor is the sensor device which will be initialized. See the imu_sensor_t.
 *
 * @param[in] if_ops is the hardware driver interface used by the sensor.
 *
 * @param[in] pin_ctrl [optional] is a possible pin control function that may be used.
 *          Called by the function: imu_dev_pin_ctrl.
 *
 * @return according to the STD_Exx.
 */
uint8_t imu_dev_init(imu_sensor_t sensor, struct imu_hw_interface_ops *if_ops, _pin_ctrl pin_ctrl);

/**
 * @brief Enable an IMU sensor.
 *
 * @param[in] sensor is the sensor device which will be used. See the imu_sensor_t.
 *
 * @return according to the STD_Exx.
 */
uint8_t imu_dev_enable(imu_sensor_t sensor);

/**
 * @brief Disable an IMU sensor.
 *
 * @param[in] sensor is the sensor device which will be used. See the imu_sensor_t.
 *
 * @return according to the STD_Exx.
 */
uint8_t imu_dev_disable(imu_sensor_t sensor);

/**
 * @brief Get the data status of the IMU sensor.
 *
 * @param[in] sensor is the sensor device which will be used. See the imu_sensor_t.
 *
 * @return 0 indicates that the sensor data is ready to be read.
 *         Otherwise, the sensor data is not ready.
 */
uint8_t imu_dev_data_ready(imu_sensor_t sensor);

/**
 * @brief Reading a number of data from IMU sensor.
 *
 * @param[in] sensor is the sensor device which will be used. See the imu_sensor_t.
 *
 * @param[out] buffer is the address for receiving data.
 *
 * @param[in] size is the size of the buffer.
 *
 * @retval The actual amount of data read in bytes.
 */
uint32_t imu_dev_read_data(imu_sensor_t sensor, void *buffer, uint32_t size);

/**
 * @brief Control of configure the IMU sensor.
 *
 * @param[in] sensor is the sensor device which will be used. See the imu_sensor_t.
 *
 * @param[in] cmd is the command code.
 *
 * @param[in] size is the command arguments.
 *
 * @return according to the STD_Exx.
 */
uint8_t imu_dev_control(imu_sensor_t sensor, uint32_t cmd, void *args);

/**
 * @brief Call the pin_ctrl function which setted by imu_dev_init.
 *
 * @param[in] sensor is the sensor device which will be used. See the imu_sensor_t.
 *
 * @param[in] pin is the pin parameter to be operated on.
 *
 * @param[in] arg  is the argument you want to set.
 *
 * @return according to the STD_Exx.
 */
uint8_t imu_dev_pin_ctrl(imu_sensor_t sensor, void *pin, void *arg);

/**
 * @brief Register the sensor operation function to the IMU sensor.
 *
 * @param[in] sensor is the sensor device which will be registered. See the imu_sensor_t.
 *
 * @param[in] ops is the the sensor operation function.
 *
 * @param[in] priv_data is private data of the sensor control block.
 *
 * @return according to the STD_Exx.
 */
uint8_t imu_dev_register(imu_sensor_t sensor, struct imu_sensor_ops *ops, void *priv_data);

#endif /*__IMU_IF_H */
