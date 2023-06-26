/******************************************************************************
* file    comstruct.c
* author  mackgim
* version V1.0.0
* date    08/01/2016
* brief   data struct about communication
******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __COMSTRUCT_H
#define __COMSTRUCT_H

#ifdef __cplusplus
extern "C"
{
#endif 

#include <stdint.h>

#ifndef __CC_ARM
#ifndef G_VARIABLE_SIZE
#define G_VARIABLE_SIZE 
#endif
#else
#ifndef G_VARIABLE_SIZE
#define G_VARIABLE_SIZE 1
#endif
#endif


#pragma region 传感器相关

	typedef struct {
		uint32_t Type;
		uint32_t all_bytes_size;
	}__SENSOR_MODE_TypeDef;

	typedef struct
	{
		uint64_t Number;
		uint8_t Value[512];
	} __FLASH_ALGO_PARAMETER_TypeDef;

	typedef struct
	{
		int32_t WhichLeg;
		float Axis3[3];
	} __FLASH_SKT_CONFIG_TypeDef;

	typedef __I__packed struct
	{
		uint8_t HeadSize;
		uint8_t HeadType;
		uint8_t DataType;
		uint64_t TimeStampms; //时间戳
		uint8_t MotionMode;
		uint8_t MotionProcess;
		uint8_t MotionStatus;
		uint8_t MotionFreezPercent;
		int16_t MotionError;
		uint16_t DebugStatus;
		uint8_t DataSize;// 字节数
		float Q[4];
		float G[3];
		float A[3];
	} __I__PACKED __SKT_REF_DEVICE_BLE_DATA_TypeDef;

	typedef __I__packed struct
	{
		uint8_t HeadSize;
		uint8_t HeadType;
		uint8_t DataType;
		uint64_t TimeStampms; //时间戳
		uint8_t MotionMode;
		uint8_t MotionProcess;
		uint8_t MotionStatus;
		uint8_t MotionFreezPercent;
		int16_t MotionError;
		uint16_t DebugStatus;
		uint8_t DataSize;// 字节数
		float Q[4];
		float G[3];
		float A[3];
		float AdjustVV; //(-1 < VV < 1) 
		float AdjustFE; //(-50 < FE < -40)
		float NavigateVV;
		float NavigateFE;
		float RelativeVV;
		float RelativeFE;
	} __I__PACKED __SKT_MASTER_DEVICE_BLE_DATA_TypeDef;
#pragma endregion


#pragma region 速度测试相关
	typedef struct {
		uint32_t        ID;
		uint32_t        DataSize;
	}__TESTSPEED_SETTING_TypeDef;

#pragma endregion


#pragma region 设备结构体

	typedef  struct {
		uint32_t Check;
		volatile uint32_t Error;
	} __MCU_CHECK_TypeDef;

	typedef __I__packed struct {
		uint32_t TimeStamp[2];
		uint32_t Mark;
	}__I__PACKED __MCU_MARK_TypeDef;

	typedef __I__packed struct  {
		uint32_t Index;
		uint64_t TimeStampMS;
	}__I__PACKED __MCU_TIMEMS_SYNC_TypeDef;

	typedef __I__packed struct {
		uint32_t	EverBootTimes;//曾经开机次数
		uint32_t	BootTimes;//开机次数
		uint32_t	BorTimes; //失电重启次数
		uint8_t		Power; //电量
		uint8_t		Reserved[3];
	}__I__PACKED __DEVICE_STATUS_TypeDef;


	typedef __I__packed struct {
		uint32_t RunTime;//总运行时间, 单位为s
		uint32_t WorkTime; //工作时间,  单位为s
		uint32_t UpTime; //当次开机后，运行时间,  单位为s
		uint8_t Reserved[4];
	}__I__PACKED __DEVICE_TIME_TypeDef;


	//短状态包，针对20字节的蓝牙传输
	typedef __I__packed struct {
		uint32_t RunTime;
		uint32_t WorkTime;
		uint32_t UpTime;
		uint32_t BleTransmitBytes;
		uint8_t Power;
	}__I__PACKED __GENERAL_STATUS_TypeDef;

	//长状态包，针对字节数长于20字节的蓝牙传输
	typedef __I__packed struct {
		uint8_t Length;//有长度和类型决定以下内容，不同设备有不同内容
		uint8_t DataType;
		uint32_t BleTransmitBytes;
		uint32_t ErrorCode;
		uint32_t RunTime;
		uint32_t WorkTime;
		uint32_t UpTime;
		uint8_t Power;
		uint8_t Role;
	}__I__PACKED __GENERAL_LONG_STATUS_TypeDef;

#pragma endregion

#pragma region flash数据结构,保证double对齐，备以后扩展

#define SERIAL_NUMBER_LENGTH 3
	typedef struct {
		uint32_t Number[3];
	}__DEVICE_SERIAL_TypeDef;

	typedef struct {
		uint8_t DeviceModel[16];  //"KH001"
		uint8_t DeviceID[20]; //"KH001-01-000001"
		uint8_t ProductDate[8]; //"20130927"
	}__DEVICE_NAMEPLATE_TypeDef;

	typedef struct
	{
		__DEVICE_SERIAL_TypeDef Serial; //12
		//__DEVICE_NAMEPLATE_TypeDef Nameplate;//44
		uint32_t Reserved;
		uint8_t Nameplate[128];// 此处替代__DEVICE_NAMEPLATE_TypeDef， 
	} __FLASH_PRODUCT_DATA_TypeDef;

	typedef struct {
		uint8_t  SystemStatus; //保留
		uint8_t  IsLogined; //设备是否被注册
		uint16_t LoginNumber;
	}__ACCOUNT_INFO_TypeDef;

	typedef struct {
		__ACCOUNT_INFO_TypeDef Account;
		uint32_t Reserved;//预留,凑64bit
	}__FLASH_SYSTEM_INFO_TypeDef;


	typedef struct
	{
		uint32_t EverBootTimes;
		uint32_t BootTimes;//开机次数
		uint64_t RunTime;//总运行时间, 内部单位, tick
		uint64_t WorkTime;//工作时间, 内部单位, tick
		uint32_t BorTimes;//电量低，而造成的复位
		uint32_t Reserved;//保留
	} __FLASH_TIME_TypeDef;
	
	typedef struct
	{
		uint8_t Mac[16];  //蓝牙Mac地址
	} __FLASH_BLE_INFO_TypeDef;


	typedef struct
	{
		uint32_t Addr;
		uint32_t Size;
	} __FLASH_DATA_READ_TypeDef;

#pragma endregion


#pragma region 命令

#define         CMD_HELLO  														0x0001
#define         CMD_ERROR  														0x0002
#define         CMD_SUCCESS														0x0003
#define         CMD_FAILED														0x0004
#define         CMD_BUSY														0x0005
#define         CMD_DENIED 														0x0006
#define         CMD_PING 														0x0007
#define         CMD_DO_NOTHING						                      		0x0008

	//
#define 		Get_Device_Nameplate											0x0100
#define 		Get_Device_Version												0x0101
#define			Get_Device_Error												0x0102
#define 		Get_Zero_Pressure_Calibration									0x0103
#define 		Get_Mould_Pressure_Calibration									0x0104
#define 		Get_AccGyro_Calibration											0x0105
#define			Get_Current_Pressure											0x0106
#define 		Get_Current_AccGyro												0x0107
#define 		Get_Power_Info													0x0108
#define         Get_RSSI	  													0x0109
#define         Get_DeviceTime                                                  0x010A
#define         Get_BT_Connected                                                0x010B
#define         Get_Device_Status               								0x010C
#define         Get_Error_From_NFC                                              0x010D
#define         Get_Acceptor_Version               								0x010E
#define         Get_Current_Magnet               								0x010F
#define         Get_Device_Calibration											0x0110
#define         Get_All_Mac_Pin                                                 0x0111
#define 		Get_Magnet_Calibration											0x0112
#define         Get_Device_Serial                                               0x0113 
#define         Get_Acceptor_Serial                                             0x0114

#define 		Get_Data_From_NFC												0x0115
#define 		Get_Product_From_NFC											0x0116
#define         Get_SecretKey_From_NFC                                          0x0117

#define         Get_Device_NRG_Version                                          0x0118
#define         Get_Acceptor_NRG_Version                                        0x0119

#define         Get_StartUpProcess_From_NFC                                     0x011A
#define         Get_Register_Info                                               0x011B
#define         Get_Device_BT_Error_Statistics                                  0x011C
#define         Get_Acceptor_BT_Error_Statistics                                0x011D
#define         Get_Alg_Setting                                                 0x011E
#define         Get_Sensor_Mode                                                 0x011F


#define 		Get_Gyro_Bias													0x0120
#define 		Get_Acc_Bias													0x0121
#define 		Get_Magnet_Bias													0x0122

#define         Get_Standby_Current                                             0x0123        
#define         Get_Sleep_Current                                               0x0124
#define         Get_Working_Current                                             0x0125
#define         Get_Short_Current                                               0x0126
#define         Get_All_Voltage                                                 0x0127
#define         Get_All_Time_Record                                             0x0128
#define         Get_UID_From_NFC												0x0129
#define 		Get_Pcb_Nameplate_By_Nfc										0x012a
#define 		Get_Button_Status												0x012b
#define 		Get_Device_Nameplate_By_Nfc										0x012c

// 
#define 		Set_Device_Mark 			    			                    0x0200
#define 		Set_Device_Nameplate						                    0x0201
#define 		Set_Average_Times                                               0x0202
#define 		Set_NFC_Current_PassWord                                        0x0203
#define 		Set_NFC_New_PassWord                                            0x0204
#define 		Set_NFC_ADD_Pointer                                             0x0205
#define         Set_Acceptor_Channel                                            0x0206
#define         Set_Add_BT_Mac_Pin                                              0x0207
#define         Set_Delete_BT_Mac_Pin                                           0x0208
#define         Set_Device_Sleep_Time                                           0x0209
#define         Set_Current_Mac_Pin                                             0x020a
#define         Set_Add_All_Mac_Pin                                             0x020b
#define         Set_Alg_Setting                                                 0x020c
#define         Set_Alg_Setting_Bytes                                           0x020d
#define         Set_Log_Level													0x020e
#define         Set_iNemo_Mode													0x020f
#define 		Set_Pcb_Nameplate_By_Nfc										0x0210
#define 		Set_Force_Index													0x0211
#define 		Set_Date_Time													0x0212
#define 		Set_Default_Year												0x0213
#define 		Set_Device_Nameplate_By_Nfc										0x0214

#define         Set_Tap_Threshold												0x0220
#define         Set_Audio_Frq													0x0221
#define         Set_ADC_Working_Time											0x0222
#define         Set_Play_Audio_Volume											0x0223
#define         Set_Led_Status													0x0224
//
#define 		Cmd_Knee_Start													0x0300
#define 		Cmd_Knee_Stop													0x0301
#define			Cmd_Zero_Pressure_Calibration									0x0302
#define			Cmd_Mould_Pressure_Calibration									0x0303
#define			Cmd_AccGyro_Calibration 										0x0304
#define         Cmd_FSS_Power_Off                                               0x0305
#define         Cmd_FSS_Power_On												0x0306
#define         Cmd_ACC_Power_Off												0x0307
#define         Cmd_ACC_Power_On		 										0x0308
#define         Cmd_Acceptor_BT_Reset											0x0309
#define         Cmd_Device_PowerOff 											0x030A
#define         Cmd_Device_Reset 												0x030B
#define         Cmd_RunTime_Clear                                               0x030C
#define         Cmd_Device_Sleep                                                0x030D
#define         Cmd_Audio_Start													0x030E
#define         Cmd_Audio_Stop													0x030F
#define         Cmd_Acceptor_Reset												0x0310
#define         Cmd_Device_Wakeup												0x0311


#define			Cmd_Magnet_Calibration 											0x0313
#define			Cmd_Reconnect													0x0314
#define			Cmd_Cancel_PowerOff 											0x0315
#define			Cmd_Check_NFC_Exist 											0x0316

#define			Cmd_Close_Standby_Relay 										0x0317
#define			Cmd_Close_Sleep_Relay 											0x0318
#define			Cmd_Close_Working_Relay 										0x0319
#define			Cmd_Close_Short_Relay 											0x031a
#define			Cmd_Open_All_Relay 												0x031b
#define			Cmd_Reset_Device_By_IO 											0x031c
#define			Cmd_Turn_On_NFC 												0x031d
#define			Cmd_Turn_Off_NFC  												0x031e
#define			Cmd_Calibrate_ADC 												0x031f


#define         Cmd_Audio_Pcm_Start												0x0320
#define         Cmd_Audio_Pcm_Stop												0x0321
#define			Cmd_Turn_On_EX_Power 											0x0322
#define			Cmd_Turn_Off_EX_Power 											0x0323



#define         Cmd_StatusUpdate_Enable                                         0x0340
#define         Cmd_StatusUpdate_Disable                                        0x0341
#define         Cmd_Register_Device                                             0x0342
#define         Cmd_Unregister_Device                                           0x0343
#define         Cmd_Clear_Register_Info                                         0x0344
#define         Cmd_Enable_Compare_DeviceID                                     0x0345
#define         Cmd_Disable_Compare_DeviceID                                    0x0346
#define         Cmd_Clear_Device_BT_Error_Statistics                            0x0347
#define         Cmd_Clear_Acceptor_BT_Error_Statistics                          0x0348
#define         Cmd_Log_Enable                                                  0x0349
#define         Cmd_Log_Disable                                                 0x034a

#define			Cmd_Connect 													0x0380
#define			Cmd_Discnnect 													0x0381
#define			Cmd_Start_Calibrate_Magnet 										0x0382
#define			Cmd_Stop_Calibrate_Magnet 										0x0383
#define			Cmd_Start_Calibrate_Magnet_After_Run 							0x0384
#define			Cmd_Stop_Calibrate_Magnet_After_Run 							0x0385
#define			Cmd_Direct_Connect 												0x0386


#define         Notice_DeviceStatus                                             0x0400
#define         Notice_AudioTransmit                                            0x0401
#define         Notice_Log                                                      0x0410 


#define         Cmd_Start_Upload_Firmware                                       0x0600
#define         Cmd_Stop_Upload_Firmware                                        0x0601
#define         Cmd_Upload_Firmware_Data                                        0x0602
#define         Cmd_Start_Upload_Hex_File										0x0603
#define         Cmd_Stop_Upload_Hex_File										0x0604
#define         Cmd_Upload_Hex_File_Data										0x0605
#define         Cmd_Audio_Play													0x0606
#define         Cmd_Audio_Pause													0x0607
#define         Cmd_Sine_Play													0x0608
#define         Cmd_Sine_Pause													0x0609
#define         Cmd_Erase_Hex_File										        0x060a




#define         Test_Speed_Start_COM_UpLoad                                     0x0680
#define         Test_Speed_Stop_COM_UpLoad                                      0x0681
#define         Test_Speed_Start_COM_Download                                   0x0682
#define         Test_Speed_Stop_COM_Download                                    0x0683
#define 		Test_Speed_Start_BLE_UpLoad										0x0684
#define 		Test_Speed_Stop_BLE_UpLoad										0x0685
#define 		Test_Speed_Start_BLE_Download									0x0686
#define 		Test_Speed_Stop_BLE_Download									0x0687

#define         Cmd_Start_Sample                                                0x0688
#define         Cmd_Stop_Sample                                                 0x0689


#define         Test_Speed_COM_Download_Data                                    0x0690

#define         Test_Speed_Get_COM_Download_Count                               0x06a0
#define         Test_Speed_Get_COM_Status                                       0x06a1
#define         Test_Speed_Get_BLE_Status               						0x06a2

#define         Cmd_Shake_Hand_Down       										0x0a00


#pragma endregion 

#pragma region 状态

#define SYSTEM_STATUS_NULL								1
#define SYSTEM_STATUS_POWEROFF							2
#define SYSTEM_STATUS_SLEEP								3
#define SYSTEM_STATUS_RESET								4
#define SYSTEM_STATUS_WORKING							5
#define SYSTEM_STATUS_TESTING							6
#define SYSTEM_STATUS_UPDATING_IMAGE					7

#pragma endregion

#ifdef __cplusplus
}
#endif

#endif /*__COM_STRUCT_H */

