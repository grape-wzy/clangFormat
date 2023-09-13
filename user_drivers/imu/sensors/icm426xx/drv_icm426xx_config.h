/*
 * @Author       : Zhaoyu.Wu
 * @Date         : 2023-09-13 15:17
 * @LastEditTime : 2023-09-13 15:24
 * @LastEditors  : Zhaoyu.Wu
 * @Description  : drv_icm426xx的配置文件
 * @FilePath     : d:/eMed/product/osteotomy_simple/firmware/user_drivers/imu/sensors/icm426xx/drv_icm426xx_config.h
 * If you have any questions, email to mr.wuzhaoyu@outlook.com.
 */
#ifndef __DRV_ICM426xx_CONFIG_H__
#define __DRV_ICM426xx_CONFIG_H__

/*
 * Define which ICM SENSOR model in targeted.
 * Possible values: ICM42600, ICM42602, ICM42605, ICM42622, ICM42631, ICM42633,
 *                  ICM42686V,ICM42688V,ICM42686P,ICM42688P,ICM42608, ICM40608.
 */
#define ICM42688P

/*
 * Clock In feature management
 * Enable Clock In feature only for ICM42622 and ICM42688x
 * Please set a hardware bridge between clock output (from MCU) and CLKIN pins (to ICM).
 */
#define ICM426XX_SUPPORT_CLK_IN 0

#if ((ICM426XX_USE_CLK_IN == 1) && (defined(ICM42622) || defined(ICM42688P) || defined(ICM42688V)))
#define ICM426XX_USE_CLK_IN 1
#else
#define ICM426XX_USE_CLK_IN 0
#endif

/*
 * Set power mode flag
 * Set this flag to run example in low-noise mode.
 * Reset this flag to run example in low-power mode.
 * Note : low-noise mode is not available with sensor data frequencies less than 12.5Hz.
 */
#define ICM426XX_IS_LOW_NOISE_MODE 1

/*
 * Accelerometer and gyroscope frequencies.
 * Recommended value: ICM426XX_GYRO_CONFIG0_ODR_1_KHZ and ICM426XX_ACCEL_CONFIG0_ODR_1_KHZ (1000 Hz)
 * Possible values (same for accel, replace "GYRO" with "ACCEL"):
 * - ICM426XX_GYRO_CONFIG0_ODR_1_KHZ  (1000 Hz)
 * - ICM426XX_GYRO_CONFIG0_ODR_500_HZ (500 Hz)
 * - ICM426XX_GYRO_CONFIG0_ODR_200_HZ (200 Hz)
 * - ICM426XX_GYRO_CONFIG0_ODR_100_HZ (100 Hz)
 * - ICM426XX_GYRO_CONFIG0_ODR_50_HZ (50 Hz)
 */
#define ICM426XX_GYRO_FREQ         ICM426XX_GYRO_CONFIG0_ODR_1_KHZ
#define ICM426XX_ACCEL_FREQ        ICM426XX_ACCEL_CONFIG0_ODR_1_KHZ

/*
 * Full scale range of accelerometer and gyroscope.
 * Possible values:
 * - see ICM426XX_ACCEL_CONFIG0_FS_SEL_t for accelerometer in file Icm426xxDefs.h..
 * - see ICM426XX_GYRO_CONFIG0_FS_SEL_t for gyroscope in file Icm426xxDefs.h.
 */
#define ICM426XX_ACCEL_FSR         ICM426XX_ACCEL_CONFIG0_FS_SEL_2g
#define ICM426XX_GYRO_FSR          ICM426XX_GYRO_CONFIG0_FS_SEL_250dps

/*
 * Select Fifo resolution Mode (default is low resolution mode)
 * Low resolution mode : 16 bits data format
 * High resolution mode : 20 bits data format
 * Warning: Enabling High Res mode will force FSR to 16g and 2000dps
 */
#define ICM426XX_IS_HIGH_RES_MODE  0

#endif /* __DRV_ICM426xx_CONFIG_H__ */
