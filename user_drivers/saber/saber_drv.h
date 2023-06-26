 /*******************************************************************************
  * file     saber_drv.h
  * author   mackgim
  * version  V1.0.0
  * date     
  * brief ： 
  *******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __SABER_DRV_H
#define __SABER_DRV_H


#include "stdint.h"



typedef struct {
	uint16_t BitDepth; // 32 or 16
	uint16_t DataRate;
	uint8_t Mode;
	float GyroSensitivity;
	float AccSensitivity;
}__GYRO_ACC_CONFIG_TypeDef;


typedef struct {
	uint8_t(*init)   (void);
	uint8_t(*deinit)   (void);
	void(*pwr_ctrl)   (uint8_t);
	uint8_t(*reset)   (uint8_t);
	uint8_t(*transmit_receive)(uint8_t*, uint8_t*, uint16_t, uint32_t);
	uint8_t(*get_ready)   (void);
	uint8_t(*enable_irq)   (uint8_t);
	uint8_t(*generate_swi)   (void);
	uint8_t(*set_cs)   (uint8_t);
}__GYRO_ACC_HW_DRIVER_TypeDef;


typedef struct {
	uint8_t(*init)   (void*);
	uint8_t(*deinit)   (void*);
	uint8_t(*restore)   (void*);
	uint8_t(*enable)   (void*);
	uint8_t(*disable)   (void*);
	uint8_t(*irq_handle)   (void*);
	uint8_t(*read_raw)(void*, uint8_t*, uint16_t);
}__GYRO_ACC_DEV_DRIVER_TypeDef;


typedef struct {
	__GYRO_ACC_HW_DRIVER_TypeDef* hw_drv;
	__GYRO_ACC_DEV_DRIVER_TypeDef* dev_drv;
	__GYRO_ACC_CONFIG_TypeDef* config;
}__GYRO_ACC_HANDLE_TypeDef;


//返回结果
typedef struct {
	float Temperature;
	float Acc[3];
	float Gyro[3];
	float Mag[3];
	float q[4];
	float e[3]; //旋转方向为内旋ZXY，外旋YXZ, 0-Y,1-X,2-Z
}__SABER_RESULT_TypeDef;

//中断返回数据操作
typedef enum {
	SAVER_OPCODE_NULL = 0,
	SAVER_OPCODE_WAIT_FOR_RSP = 1,
	SAVER_OPCODE_WAIT_FOR_DATA = 2,
} __SABER_OPCODE_TypeDef;

//fifo状态
typedef struct {
	uint32_t response_unread;
	uint32_t measure_unread;
}__SABER_FIFO_TypeDef;

#define SABER_I2C_ADDR					0xAE

#define SABER_MEASURE_CHANNEL			0xA1
#define SABER_MESSAGE_CHANNEL			0xA2
#define SABER_FIFO_CHANNEL				0xA3
#define SABER_RESPONSE_CHANNEL			0xA4


uint8_t saber_drv_init(void* handle);
uint8_t saber_drv_deinit(void* handle);
uint8_t saber_drv_start(void* handle);
uint8_t saber_drv_stop(void* handle);
uint8_t saber_drv_read_raw(void* handle, uint8_t* value, uint16_t size);
uint8_t saber_drv_isr_handle(void* handle);
uint8_t saber_drv_test(void* handle);
#endif /*__SABER_DRV_H */

