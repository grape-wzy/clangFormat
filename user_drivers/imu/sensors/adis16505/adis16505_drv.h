 /*******************************************************************************
  * file     adis16505_drv.h
  * author   mackgim
  * version  V1.0.0
  * date     
  * brief ： 
  *******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __ADIS16505_DRV_H
#define __ADIS16505_DRV_H


#include "stdint.h"



#define ADIS16505_SET_REG(t,m,v)             ((t&(~m))|v)


#define ADIS16505_DIAG_STAT         ((uint8_t)0x02)
#define ADIS16505_X_GYRO_LOW        ((uint8_t)0x04)
#define ADIS16505_X_GYRO_OUT        ((uint8_t)0x06)
#define ADIS16505_Y_GYRO_LOW        ((uint8_t)0x08)
#define ADIS16505_Y_GYRO_OUT        ((uint8_t)0x0A)
#define ADIS16505_Z_GYRO_LOW        ((uint8_t)0x0C)
#define ADIS16505_Z_GYRO_OUT        ((uint8_t)0x0E)
#define ADIS16505_X_ACCL_LOW        ((uint8_t)0x10)
#define ADIS16505_X_ACCL_OUT        ((uint8_t)0x12)
#define ADIS16505_Y_ACCL_LOW        ((uint8_t)0x14)
#define ADIS16505_Y_ACCL_OUT        ((uint8_t)0x16)
#define ADIS16505_Z_ACCL_LOW        ((uint8_t)0x18)
#define ADIS16505_Z_ACCL_OUT        ((uint8_t)0x1A)
#define ADIS16505_TEMP_OUT          ((uint8_t)0x1C)
#define ADIS16505_TIME_STAMP        ((uint8_t)0x1E)
#define ADIS16505_DATA_CNTR         ((uint8_t)0x22)
#define ADIS16505_X_DELTANG_LOW     ((uint8_t)0x24)
#define ADIS16505_X_DELTANG_OUT     ((uint8_t)0x26)
#define ADIS16505_Y_DELTANG_LOW     ((uint8_t)0x28)
#define ADIS16505_Y_DELTANG_OUT     ((uint8_t)0x2A)
#define ADIS16505_Z_DELTANG_LOW     ((uint8_t)0x2C)
#define ADIS16505_Z_DELTANG_OUT     ((uint8_t)0x2E)
#define ADIS16505_X_DELTVEL_LOW     ((uint8_t)0x30)
#define ADIS16505_X_DELTVEL_OUT     ((uint8_t)0x32)
#define ADIS16505_Y_DELTVEL_LOW     ((uint8_t)0x34)
#define ADIS16505_Y_DELTVEL_OUT     ((uint8_t)0x36)
#define ADIS16505_Z_DELTVEL_LOW     ((uint8_t)0x38)
#define ADIS16505_Z_DELTVEL_OUT     ((uint8_t)0x3A)
#define ADIS16505_XG_BIAS_LOW       ((uint8_t)0x40)
#define ADIS16505_XG_BIAS_HIGH      ((uint8_t)0x42)
#define ADIS16505_YG_BIAS_LOW       ((uint8_t)0x44)
#define ADIS16505_YG_BIAS_HIGH      ((uint8_t)0x46)
#define ADIS16505_ZG_BIAS_LOW       ((uint8_t)0x48)
#define ADIS16505_ZG_BIAS_HIGH      ((uint8_t)0x4A)
#define ADIS16505_XA_BIAS_LOW       ((uint8_t)0x4C)
#define ADIS16505_XA_BIAS_HIGH      ((uint8_t)0x4E)
#define ADIS16505_YA_BIAS_LOW       ((uint8_t)0x50)
#define ADIS16505_YA_BIAS_HIGH      ((uint8_t)0x52)
#define ADIS16505_ZA_BIAS_LOW       ((uint8_t)0x54)
#define ADIS16505_ZA_BIAS_HIGH      ((uint8_t)0x56)
#define ADIS16505_FILT_CTRL         ((uint8_t)0x5C)
#define ADIS16505_RANG_MDL          ((uint8_t)0x5E)
#define ADIS16505_MSC_CTRL          ((uint8_t)0x60)
#define ADIS16505_UP_SCALE          ((uint8_t)0x62)
#define ADIS16505_DEC_RATE          ((uint8_t)0x64)
#define ADIS16505_NULL_CNFG         ((uint8_t)0x66)
#define ADIS16505_GLOB_CMD          ((uint8_t)0x68)
#define ADIS16505_FIRM_REV          ((uint8_t)0x6C)
#define ADIS16505_FIRM_DM           ((uint8_t)0x6E)
#define ADIS16505_FIRM_Y            ((uint8_t)0x70)
#define ADIS16505_PROD_ID           ((uint8_t)0x72)
#define ADIS16505_SERIAL_NUM        ((uint8_t)0x74)
#define ADIS16505_USER_SCR_1        ((uint8_t)0x76)
#define ADIS16505_USER_SCR_2        ((uint8_t)0x78)
#define ADIS16505_USER_SCR_3        ((uint8_t)0x7A)
#define ADIS16505_FLSHCNT_LOW       ((uint8_t)0x7C)
#define ADIS16505_FLSHCNT_HIGH      ((uint8_t)0x7E)


#define ADIS16505_ID				(0x4079)





/*******************************************************************************
* Register      : ADIS16505_MSC_CTRL
* Address       : 0x60
* Bit Group Name: BURST
* Permission    : WR
*******************************************************************************/
typedef enum
{
	__ADIS16505_MSC_CTRL_BURST16 = ((uint16_t)0x0), 
	__ADIS16505_MSC_CTRL_BURST32 = ((uint16_t)0x0200), 
} __ADIS16505_MSC_CTRL_BURST_t;

#define   __ADIS16505_MSC_CTRL_BURST_MASK    ((uint16_t)0x0200)

typedef enum
{
	__ADIS16505_MSC_CTRL_BURST_SEL_DISABLE = ((uint16_t)0x0),
	__ADIS16505_MSC_CTRL_BURST_SEL_ENABLE = ((uint16_t)0x100),
} __ADIS16505_MSC_CTRL_BURST_SEL_t;

#define   __ADIS16505_MSC_CTRL_BURST_SEL_MASK    ((uint16_t)0x100)

typedef enum
{
	__ADIS16505_MSC_CTRL_LINEAR_DISABLE = ((uint16_t)0x0), 
	__ADIS16505_MSC_CTRL_LINEAR_ENABLE = ((uint16_t)0x80), 
} __ADIS16505_MSC_CTRL_LINEAR_t;

#define   __ADIS16505_MSC_CTRL_LINEAR_MASK    ((uint16_t)0x80)

typedef enum
{
	__ADIS16505_MSC_CTRL_PERCUSSION_DISABLE = ((uint16_t)0x0),
	__ADIS16505_MSC_CTRL_PERCUSSION_ENABLE = ((uint16_t)0x40), 
} __ADIS16505_MSC_CTRL_PERCUSSION_t;

#define   __ADIS16505_MSC_CTRL_PERCUSSION_MASK    ((uint16_t)0x40)

typedef enum
{
	__ADIS16505_MSC_CTRL_SENS_BW_DISABLE = ((uint16_t)0x0),
	__ADIS16505_MSC_CTRL_SENS_BW_ENABLE = ((uint16_t)0x10),
} __ADIS16505_MSC_CTRL_SENS_BW_t;

#define   __ADIS16505_MSC_CTRL_SENS_BW_MASK    ((uint16_t)0x10)


typedef enum
{
	__ADIS16505_MSC_CTRL_SYNC_MODE_INTERNAL = ((uint16_t)0x0),
	__ADIS16505_MSC_CTRL_SYNC_MODE_INPUT = ((uint16_t)0x04),
	__ADIS16505_MSC_CTRL_SYNC_MODE_SCALE = ((uint16_t)0x08),
	__ADIS16505_MSC_CTRL_SYNC_MODE_OUTPUT = ((uint16_t)0x0C),

} __ADIS16505_MSC_CTRL_SYNC_MODE_t;

#define   __ADIS16505_MSC_CTRL_SYNC_MODE_MASK    ((uint16_t)0x0C)

typedef enum
{
	__ADIS16505_MSC_CTRL_SYNC_POLARITY_FALLING = ((uint16_t)0x0),
	__ADIS16505_MSC_CTRL_SYNC_POLARITY_RISING = ((uint16_t)0x02),

} __ADIS16505_MSC_CTRL_SYNC_POLARITY_t;

#define   __ADIS16505_MSC_CTRL_SYNC_POLARITY_MASK    ((uint16_t)0x02)


typedef enum
{
	__ADIS16505_MSC_CTRL_DR_POLARITY_LOW = ((uint16_t)0x0),
	__ADIS16505_MSC_CTRL_DR_POLARITY_HIGH = ((uint16_t)0x01),

} __ADIS16505_MSC_CTRL_DR_POLARITY_t;

#define   __ADIS16505_MSC_CTRL_DR_POLARITY_MASK    ((uint16_t)0x01)

uint8_t adis16505_set_xa_bias_low(__GYRO_ACC_HW_DRIVER_TypeDef* handle, uint16_t newValue);

uint8_t adis16505_set_msc_ctrl(__GYRO_ACC_HW_DRIVER_TypeDef* handle, uint16_t newValue);

uint8_t adis16505_set_dec_rate(__GYRO_ACC_HW_DRIVER_TypeDef* handle, uint16_t newValue);



typedef struct {

	uint16_t  DiagStat;
	uint16_t  TempOut;

	uint16_t  TimeStamp;
	uint16_t  DataCntr;

	int32_t XGBias;
	int32_t YGBias;
	int32_t ZGBias;

	int32_t XABias;
	int32_t YABias;
	int32_t ZABias;

	uint32_t R1;

	uint16_t FiltCtrl;
	uint16_t RangMdl;

	uint16_t MscCtrl;
	uint16_t UpScale;

	uint16_t DecRate;
	uint16_t R2;

	uint16_t GlobCmd;
	uint16_t R3;

	uint16_t FirmRev;
	uint16_t FirmDm;

	uint16_t FirmY;
	uint16_t ProdID;

	uint16_t SerialNum;
	uint16_t UserScr1;

	uint16_t UserScr2;
	uint16_t UserScr3;

	uint32_t FlashCnt;

}__ADIS16505_MEMORY_MAP_TypeDef;


#define  ADIS16505_MEMORY_MAP_BEGIN   ADIS16505_XG_BIAS_LOW 



uint8_t  adis16505_read_id(__GYRO_ACC_HW_DRIVER_TypeDef *handle, uint16_t *value);

uint8_t adis16505_read_all_reg(__GYRO_ACC_HW_DRIVER_TypeDef* handle, __ADIS16505_MEMORY_MAP_TypeDef *mmt);



uint8_t  adis16505_check(__GYRO_ACC_HW_DRIVER_TypeDef *handle);

uint8_t adis1605_read_raw_data(__GYRO_ACC_HW_DRIVER_TypeDef *handle, uint8_t *value, uint16_t size);
#endif /*__ADIS16505_DRV_H */

