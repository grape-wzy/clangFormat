/*******************************************************************************
* file    event_struct.c
* author  mackgim
* version 1.0.0
* date
* brief   event_struct.c
*******************************************************************************/

#ifndef __EVENT_STRUCT_H
#define __EVENT_STRUCT_H

/********** 头文件 ************************************************************/
#include <stdint.h>

/********** 宏定义 ***********************************************************/

//短指令,传输字节小于或等于20字节的BLE使用
#define STATUS_GENERAL_INDEX        1
#define STATUS_ERROR_INDEX          2
#define STATUS_SHUTDOWN_INDEX       3
#define STATUS_SYSTEM_INDEX         4
#define STATUS_GENERAL_2INDEX       5
#define STATUS_AUDIO_SYNC_INDEX     6
#define STATUS_KNEE_SAMPLE_INDEX    7
#define STATUS_TRAIN_STATUS_INDEX   8

//长指令状态, 传输字节超过20字节的BLE使用
#define STATUS_LONG_GENERAL_INDEX   0xff
#define STATUS_LONG_TYPE_0X01_INDEX 0x01 //TKA使用
#define STATUS_LONG_TYPE_0X02_INDEX 0x02 //SKT使用

#define BLE_FULL_MAC_PIN_SIZE       16   //单个mac的字节数
#define BLE_LOCAL_NAME_MAX_SIZE     20   //最大长度

typedef struct
{
    uint8_t  ID[BLE_FULL_MAC_PIN_SIZE];
    uint8_t  Name[BLE_LOCAL_NAME_MAX_SIZE];
    uint32_t PinCode;     //密钥
    uint16_t ServerType;  //设备类型
    uint8_t  ConnMacType; //蓝牙地址类型，公有或私有
} __BLE_LOCAL_CONFIG_TypeDef;

#define ET_RES_SUCCESS    (0x03)
#define ET_RES_FAILED     (0x04)
#define ET_RES_DENIED     (0x06)
#define ET_RES_DO_NOTHING (0x08)

#pragma region 命令通道指令集
#define EVENT_END_CMD                           ((uint8_t)0x0b)

// #define ET_GET_DeviceModel                      ((uint8_t)0x10)
// #define ET_GET_DeviceID1                        ((uint8_t)0x11)
// #define ET_GET_DeviceID2                        ((uint8_t)0x12)
// #define ET_GET_ProductDate                      ((uint8_t)0x13)

// #define ET_GET_HardwareVersion                  ((uint8_t)0x14)
// #define ET_GET_HardwareDate                     ((uint8_t)0x15)
// #define ET_GET_SoftVersion                      ((uint8_t)0x16)
// #define ET_GET_SoftDate                         ((uint8_t)0x17)
// #define ET_GET_Serial1                          ((uint8_t)0x18)
// #define ET_GET_Serial2                          ((uint8_t)0x19)

#define ET_GET_Error                            ((uint8_t)0x1a)
// #define ET_GET_Zero_Pressure_Left_Calibration   ((uint8_t)0x1b)
// #define ET_GET_Zero_Pressure_Right_Calibration  ((uint8_t)0x1c)
// #define ET_GET_Mould_Pressure_Left_Calibration  ((uint8_t)0x1d)
// #define ET_GET_Mould_Pressure_Right_Calibration ((uint8_t)0x1e)
// #define ET_GET_AccGyro_Calibration              ((uint8_t)0x1f)
// #define ET_GET_Magnet_Calibration               ((uint8_t)0x20)
// #define ET_GET_Current_Pressure_Left            ((uint8_t)0x21)
// #define ET_GET_Current_Pressure_Right           ((uint8_t)0x22)
// #define ET_GET_Current_AccGyro                  ((uint8_t)0x23)
// #define ET_GET_Current_Magnet                   ((uint8_t)0x24)
#define ET_GET_Power_Info                       ((uint8_t)0x25)
#define ET_GET_Device_RSSI                      ((uint8_t)0x26)
#define ET_GET_Device_Time                      ((uint8_t)0x27)
#define ET_GET_Device_Mark                      ((uint8_t)0x28)
#define ET_GET_Device_Status                    ((uint8_t)0x29)
// #define ET_GET_Device_Check                     ((uint8_t)0x2a)
#define ET_GET_Reg_Info                         ((uint8_t)0x2b)
#define ET_GET_Flash_Data                       ((uint8_t)0x2c)
#define ET_GET_Alg_Setting                      ((uint8_t)0x2d)
#define ET_GET_Sensor_Mode                      ((uint8_t)0x2e)
// #define ET_GET_All_Time_Record                  ((uint8_t)0x2f)

// #define ET_GET_Gbias                            ((uint8_t)0x30)
// #define ET_GET_Abias                            ((uint8_t)0x31)
// #define ET_GET_Mbias                            ((uint8_t)0x32)
#define ET_GET_Zero_Force                       ((uint8_t)0x33)
#define ET_GET_Mould_Force                      ((uint8_t)0x34)
#define ET_GET_Current_Force                    ((uint8_t)0x35)

#define ET_GET_Motion_Sensitivity               ((uint8_t)0x36)
#define ET_GET_Motion_Rate                      ((uint8_t)0x37)
#define ET_GET_Ble_Power_Level                  ((uint8_t)0x38)
#define ET_GET_Device_Audio_Config              ((uint8_t)0x39)

#define ET_GET_Version                          ((uint8_t)0x40)
#define ET_GET_Nameplate                        ((uint8_t)0x41)

#define ET_SET_Device_Mark                      ((uint8_t)0x60)
#define ET_SET_Device_Mac                       ((uint8_t)0x61)
// #define ET_SET_DeviceModel                      ((uint8_t)0x61)
// #define ET_SET_DeviceID1                        ((uint8_t)0x62)
// #define ET_SET_DeviceID2                        ((uint8_t)0x63)
// #define ET_SET_ProductDate                      ((uint8_t)0x64)
#define ET_SET_Ble_Power_Level                  ((uint8_t)0x65)
#define ET_SET_TimeStamp_MS                     ((uint8_t)0x66)
#define ET_SET_SampleNumber                     ((uint8_t)0x67)
#define ET_SET_Work_Mode                        ((uint8_t)0x68)

// #define ET_SET_Gbias                            ((uint8_t)0x70)
// #define ET_SET_Abias                            ((uint8_t)0x71)
// #define ET_SET_Mbias                            ((uint8_t)0x72)
#define ET_SET_Zero_Force                       ((uint8_t)0x72)
#define ET_SET_Average_Times                    ((uint8_t)0x73)
#define ET_SET_Alg_Setting                      ((uint8_t)0x74)
#define ET_SET_Device_Audio_Config              ((uint8_t)0x75)
#define ET_SET_SKT_Config                       ((uint8_t)0x76)
#define ET_SET_SKT_Mode                         ((uint8_t)0x77)
#define ET_SET_SKT_Reg_Pos                      ((uint8_t)0x78)
#define ET_SET_SKT_Navigation_Angle             ((uint8_t)0x79)
#define ET_SET_Nameplate                        ((uint8_t)0x80)

//命令
#define ET_CMD_Start                            ((uint8_t)0xb0)
#define ET_CMD_Stop                             ((uint8_t)0xb1)
#define ET_CMD_Zero_Force_Calib                 ((uint8_t)0xb2)
#define ET_CMD_Mould_Force_Calib                ((uint8_t)0xb3)
#define ET_CMD_AccGyro_Calib                    ((uint8_t)0xb4)
#define ET_CMD_Magnet_Calib                     ((uint8_t)0xb5)
#define ET_CMD_FSS_Power_Off                    ((uint8_t)0xb6)
#define ET_CMD_FSS_Power_On                     ((uint8_t)0xb7)
#define ET_CMD_ACC_Power_Off                    ((uint8_t)0xb8)
#define ET_CMD_ACC_Power_On                     ((uint8_t)0xb9)
#define ET_CMD_Device_Ageing                    ((uint8_t)0xba)
#define ET_CMD_Device_PowerOff                  ((uint8_t)0xbb)
#define ET_CMD_Device_Reset                     ((uint8_t)0xbc)
#define ET_CMD_RunTime_Clear                    ((uint8_t)0xbd)
#define ET_CMD_Cancel_PowerOff                  ((uint8_t)0xbe)

#define ET_CMD_Motion_Start                     ((uint8_t)0xc0)
#define ET_CMD_Motion_Stop                      ((uint8_t)0xc1)
#define ET_CMD_Audio_Start                      ((uint8_t)0xc2)
#define ET_CMD_Audio_Stop                       ((uint8_t)0xc3)

#define ET_CMD_Register                         ((uint8_t)0xc5)
#define ET_CMD_Unregister                       ((uint8_t)0xc6)
#define ET_CMD_Clear_Reg                        ((uint8_t)0xc7)
#define ET_CMD_Func_Reset                       ((uint8_t)0xc8)
#define ET_CMD_Update_Ble_Mac                   ((uint8_t)0xc9)

// 固件烧写命令集
#define ET_CMD_Firmware_Start                   ((uint8_t)0xE0)
#define ET_CMD_Firmware_Stop                    ((uint8_t)0xE1)
#define ET_CMD_Firmware_IsReady                 ((uint8_t)0xE2)
#define ET_SET_Firmware_Config                  ((uint8_t)0xE3)

#define ET_TEST_Speed_Start_BLE_UpLoad          ((uint8_t)0xF0)
#define ET_TEST_Speed_Stop_BLE_UpLoad           ((uint8_t)0xF1)
#define ET_TEST_Speed_Start_BLE_Download        ((uint8_t)0xF2)
#define ET_TEST_Speed_Stop_BLE_Download         ((uint8_t)0xF3)

#pragma endregion

#pragma region 数据通道指令集

#define ET_DATA_Motion ((uint8_t)0x10)

#pragma endregion

#endif /*__EVENT_STRUCT_H */

/***************************** 结束 *****************************************************/
