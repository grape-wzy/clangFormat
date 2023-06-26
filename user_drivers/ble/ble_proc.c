/*******************************************************************************
* file    ble_proc.c
* author  mackgim
* version V1.0.1
* date    2021-09-21
* brief   蓝牙协议栈操作函数
*******************************************************************************/


/* Includes ------------------------------------------------------------------*/
#include "ble_proc.h"
#include "standard_lib.h"
#include "app_common.h"
#include "ble.h"
#include "app_entry.h"
#include "app_ble.h"
#include "ble_defs.h"
#include "utils.h"
#include "platform.h"

#pragma region 宏定义

#ifdef DEBUG
#if 1
#define bkpf(...) kprint(__VA_ARGS__)
#define bnpf(...) nprint(__VA_ARGS__)
#define btpf(...) tprint(__VA_ARGS__)
#else
#define bkpf(...)
#define bnpf(...)
#define btpf(...)
#endif
#else
#define bkpf(...)
#define bnpf(...)
#define btpf(...)
#endif

#pragma endregion

#pragma region 结构体定义
typedef struct {
	uint8_t Type;//1-cmd数据,2-image数据
	uint8_t BufferSize;
	uint8_t Buffer[BLE_GATT_CHAR_MAX_LENGTH];
}__BLE_RX_EVENT_TypeDef;
#pragma endregion

#pragma region 变量
#define CONN_HANDLE_NULL (0xFFFF)
//是否完成BLE初始化
static uint8_t sBleInit = false;
//回调函数
static __BLE_CALLBACK_TypeDef sBleCallBack = { NULL };
//蓝牙自身配置
static __BLE_LOCAL_CONFIG_TypeDef sBleLocalConfig = { 0 };
// 蓝牙连接变量
static __BLE_CONNECT_EVENT_TypeDef sConnEvent = { 0 };
//更新连接参数的过程变量
static __BLE_CONNECT_PARA_EVENT_TypeDef sConntParaEvt = { 0 };

#pragma endregion

#pragma region 函数

//蓝牙数据接收函数
void ble_rx_proc(void);

//app_ble 回调
static uint8_t ble_evt_rx_proc(void* pckt);
static uint8_t ble_init_services(void);

//蓝牙协议栈回调
static void gatt_attribute_modified_cb(uint16_t handle, uint8_t* att_data, uint8_t data_length);
static void gap_connected_cb(uint8_t addr[6], uint16_t handle);
static void gap_disconnected_cb(uint16_t handle);
#pragma endregion

#pragma region 功能
//初始化ble配置
void ble_init_clock(void)
{
	APPE_Init_Clock();
}

//初始化蓝牙协议栈
void ble_init(void)
{
	sConnEvent.ConnHandle = CONN_HANDLE_NULL;
	UTIL_SEQ_RegTask(1 << CFG_TASK_BLE_RX_PROC, UTIL_SEQ_RFU, ble_rx_proc);

	__APP_BLE_CALLBACK_TypeDef cb =
	{
		.user_evt_rx = ble_evt_rx_proc,
		.user_init = ble_init_services,
		.Config = &sBleLocalConfig,
	};

	APP_BLE_Register_CallBack(&cb);
	APPE_Init();

	bkpf("ok\r\n");

	sBleCallBack.disable_sleep(1);
}

uint8_t ble_is_init(void)
{
	return APPE_Get_CPU2_Status();
}


void ble_deinit(void)
{
	if (!sBleInit)
	{
		return;
	}
	sBleInit = false;
	if (sConnEvent.ConnHandle != CONN_HANDLE_NULL)
	{
		ble_disconnect();
		Clock_Wait(100);
	}

	APP_BLE_Adv_Stop();

	uint8_t ret = aci_hal_stack_reset();
	if (ret != BLE_STATUS_SUCCESS)
	{
		bkpf("failed to reset stack\r\n");
	}

	//if( (LL_PWR_IsActiveFlag_C1SB() == 0) )
	{
		bkpf("LL_PWR_IsActiveFlag_C1SB=%d \n\r", (int)LL_PWR_IsActiveFlag_C1SB());
	}
	//if( (LL_PWR_IsActiveFlag_C2SB() == 0) )
	{
		bkpf("LL_PWR_IsActiveFlag_C2SB=%d \n\r", (int)LL_PWR_IsActiveFlag_C2SB());
	}

	//if( (LL_PWR_GetPowerMode() == 0) )
	{
		bkpf("LL_PWR_GetPowerMode()=%d \n\r", (int)LL_PWR_GetPowerMode());
	}
	//if( (LL_C2_PWR_GetPowerMode() == 0) )
	{
		bkpf("LL_C2_PWR_GetPowerMode()=%d \n\r", (int)LL_C2_PWR_GetPowerMode());
	}

	if ((__HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET))
	{
		bkpf("PWR_FLAG_SB \n\r");
	}
	if ((__HAL_PWR_GET_FLAG(PWR_FLAG_C2SB) != RESET))
	{
		bkpf("PWR_FLAG_C2SB \n\r");
	}


	APPE_DeInit();

	kprint("ok\r\n");
}


//复位
void ble_os_reset(void)
{

}

//注册蓝牙回调函数
void ble_register_callback(__BLE_CALLBACK_TypeDef* cb)
{
	sBleCallBack = *cb;
}

//设置蓝牙地址, include mac and name
void ble_set_local_id(uint8_t device, uint8_t input[])
{

#if DEBUG
	uint8_t* mac_ptr = (uint8_t*)input;
	nprint("[blue key]:");
	for (uint8_t i = 0; i < 16; i++)
	{
		nprint("%02x", mac_ptr[i]);
	}
	nprint("\r\n");
#endif

	// mac结构，4个32bit的数据
	//0-1： mac地址，前6个字节为蓝牙地址，第7、8个字节设备类型
	//2: pin码，已抛弃不用
	//3: 广播的数字名字
	memcpy(sBleLocalConfig.ID, (uint8_t*)input, sizeof(sBleLocalConfig.ID));
	sBleLocalConfig.PinCode = 0;
	memcpy(&sBleLocalConfig.PinCode, &input[8], 3);
	while (sBleLocalConfig.PinCode > 999999)
	{
		sBleLocalConfig.PinCode = sBleLocalConfig.PinCode >> 1;
	}

	//设置设备类型
	sBleLocalConfig.ServerType = (uint16_t)(((uint16_t)input[7] << 8) & 0xff00) + (input[6] & 0xff);

	//设置MAC地址类型
	sBleLocalConfig.ConnMacType = input[11];
#ifdef DEBUG
	bkpf("MAC=%02x%02x%02x%02x%02x%02x,", sBleLocalConfig.ID[5], sBleLocalConfig.ID[4],
		sBleLocalConfig.ID[3], sBleLocalConfig.ID[2],
		sBleLocalConfig.ID[1], sBleLocalConfig.ID[0]);
	bnpf("PIN=%u,", (unsigned int)sBleLocalConfig.PinCode);
	bnpf("DeviceType=0x%x,", (unsigned int)sBleLocalConfig.ServerType);
	const uint8_t DeivceType[][24] = { "PUBLIC", "RANDOM", "RESOLVABLE_PRIVATE", "NON_RESOLVABLE_PRIVATE" };
	bnpf("MacType=%s\r\n", DeivceType[sBleLocalConfig.ConnMacType]);

#endif  

	uint32_t num = 0;
	memcpy((uint8_t*)&num, (uint8_t*)input, sizeof(num));
	num = num % BLE_LOCAL_NAME_MAX_NUMBER;
	
	uint8_t buff[12];
	uint32_t l = uint_to_char(num, &buff[0]);

	uint8_t defaultname[BLE_LOCAL_NAME_MAX_SIZE] = BLE_LOCAL_NAEM_DEFAULT;
	defaultname[BLE_LOCAL_NAEM_NUMBER_INDEX - 1] = '_';
	if (device == 0)//master
	{
		defaultname[BLE_LOCAL_NAEM_NUMBER_INDEX - 2] = 'A';
	}
	else
	{
		defaultname[BLE_LOCAL_NAEM_NUMBER_INDEX - 2] = 'B';
	}
	memcpy(&defaultname[BLE_LOCAL_NAEM_NUMBER_INDEX], &buff[0], l);
	defaultname[BLE_LOCAL_NAEM_NUMBER_INDEX + l] = '\0';

	memcpy(&sBleLocalConfig.Name[0], &defaultname[0], BLE_LOCAL_NAME_MAX_SIZE);
	bkpf("%s\r\n", &sBleLocalConfig.Name[1]);

}


uint32_t ble_get_stack_size(void)
{
	return APPE_Get_Stack_Size();
}
//获取stack版本-数字版本
uint32_t ble_get_stack_numerical_version(void)
{
	return APPE_Get_Stack_Numerical_Version();
}

//获取fus版本-数字版本
uint32_t ble_get_fus_numerical_version(void)
{
	return APPE_Get_Fus_Numerical_Version();
}
//获取蓝牙协议栈所有版本-字符串版本
uint8_t ble_get_string_version(uint8_t* stack_hw, uint8_t* stack_fw, uint8_t* fus_fw)
{
	APPE_Get_String_Version(stack_hw, stack_fw, fus_fw);
	return STD_SUCCESS;
}

//获取当前的MTU配置
uint16_t ble_get_mtu(void)
{
	return sConnEvent.MTU;
}
#pragma endregion

#pragma region 添加蓝牙服务
/*-------------------------------服务设置UUID-------------------------------------------------*/
// 蓝牙句柄
// 膝关节垫片 - 0x30
// ECG - 0x40
// 康复训练 - 0x60
// 截骨系统 - 0x70

/***
974c2e70-3e83-465e-acde-6f92fe712134
974c2e71-3e83-465e-acde-6f92fe712134
974c2e72-3e83-465e-acde-6f92fe712134
974c2e73-3e83-465e-acde-6f92fe712134
974c2e74-3e83-465e-acde-6f92fe712134
974c2e75-3e83-465e-acde-6f92fe712134
*/


static const uint8_t sServiceUUID[16] = COPY_UUID_128(0x97, 0x4c, 0x2e, 0x70, 0x3e, 0x83, 0x46, 0x5e, 0xac, 0xde, 0x6f, 0x92, 0xfe, 0x71, 0x21, 0x34);
static const uint8_t sCharaRxUUID[16] = COPY_UUID_128(0x97, 0x4c, 0x2e, 0x71, 0x3e, 0x83, 0x46, 0x5e, 0xac, 0xde, 0x6f, 0x92, 0xfe, 0x71, 0x21, 0x34);
static const uint8_t sCharaTxUUID[16] = COPY_UUID_128(0x97, 0x4c, 0x2e, 0x72, 0x3e, 0x83, 0x46, 0x5e, 0xac, 0xde, 0x6f, 0x92, 0xfe, 0x71, 0x21, 0x34);
static const uint8_t sCharaDataUUID[16] = COPY_UUID_128(0x97, 0x4c, 0x2e, 0x73, 0x3e, 0x83, 0x46, 0x5e, 0xac, 0xde, 0x6f, 0x92, 0xfe, 0x71, 0x21, 0x34);
static const uint8_t sCharaStatusUUID[16] = COPY_UUID_128(0x97, 0x4c, 0x2e, 0x74, 0x3e, 0x83, 0x46, 0x5e, 0xac, 0xde, 0x6f, 0x92, 0xfe, 0x71, 0x21, 0x34);
static const uint8_t sCharaImageUUID[16] = COPY_UUID_128(0x97, 0x4c, 0x2e, 0x75, 0x3e, 0x83, 0x46, 0x5e, 0xac, 0xde, 0x6f, 0x92, 0xfe, 0x71, 0x21, 0x34);


//const uint8_t SensorName[8][22]={"EventRX","EventTX","UpdatePressureLeft","UpdatePressureRight","UpdatePosture","UpdateRoutine","TestSpeed","TestAudio"};

#define DEVICE_SERVICES_CHAR_NUMBER (sizeof(sDeviceServiceChara)/sizeof(BLE_SERVICE_CHAR_TypeDef))

#define ENCRYPTION_KEY_SIZE 10
static const BLE_SERVICE_CHAR_TypeDef sDeviceServiceChara[] =
{
	//1
	{
	sCharaRxUUID,
	UUID_TYPE_128,
	BLE_GATT_CHAR_MAX_LENGTH,//数据长度
	CHAR_PROP_WRITE | CHAR_PROP_WRITE_WITHOUT_RESP,//Properties
	ATTR_PERMISSION_NONE,//Permissions
	GATT_NOTIFY_ATTRIBUTE_WRITE,//EvtMask
	ENCRYPTION_KEY_SIZE, //encryKey
	1//是否可变
	},
	//2
	{
	sCharaTxUUID,
	UUID_TYPE_128,
	BLE_GATT_CHAR_MAX_LENGTH,
	CHAR_PROP_NOTIFY,
	ATTR_PERMISSION_NONE,
	GATT_DONT_NOTIFY_EVENTS,
	ENCRYPTION_KEY_SIZE,
	1
	},
	//3
	{
	sCharaDataUUID,
	UUID_TYPE_128,
	BLE_GATT_CHAR_MAX_LENGTH,
	CHAR_PROP_NOTIFY | CHAR_PROP_WRITE | CHAR_PROP_WRITE_WITHOUT_RESP,
	ATTR_PERMISSION_NONE,
	GATT_NOTIFY_ATTRIBUTE_WRITE,
	ENCRYPTION_KEY_SIZE,
	1
	},
	//4
	{
	sCharaStatusUUID,
	UUID_TYPE_128,
	BLE_GATT_CHAR_MAX_LENGTH,
	CHAR_PROP_NOTIFY,
	ATTR_PERMISSION_NONE,
	GATT_DONT_NOTIFY_EVENTS,
	ENCRYPTION_KEY_SIZE,
	1
	},
	//5
	{
	sCharaImageUUID,
	UUID_TYPE_128,
	BLE_GATT_CHAR_MAX_LENGTH,
	CHAR_PROP_NOTIFY | CHAR_PROP_WRITE | CHAR_PROP_WRITE_WITHOUT_RESP,
	ATTR_PERMISSION_NONE,
	GATT_NOTIFY_ATTRIBUTE_WRITE,
	ENCRYPTION_KEY_SIZE,
	1
	}
};

uint8_t ble_init_services(void)
{
	tBleStatus ret;
	bnpf("\r\n");
	bkpf("chara number=%u\r\n", DEVICE_SERVICES_CHAR_NUMBER);
	Service_UUID_t sut;
	sut.Service_UUID_16 = ((uint16_t)sServiceUUID[13] >> 8) + sServiceUUID[12];
	memcpy((uint8_t*)&sut.Service_UUID_128[0], sServiceUUID, sizeof(sServiceUUID));
	ret = aci_gatt_add_service(UUID_TYPE_128, &sut, PRIMARY_SERVICE, 1 + DEVICE_SERVICES_CHAR_NUMBER * 3, &sConnEvent.ServiceHandle);
	if (ret != BLE_STATUS_SUCCESS)
	{
		bkpf("failed to add service, ret=0x%x\r\n", ret);
		goto failed;
	}

	bkpf("Service Handle=0x%04X\r\n", sConnEvent.ServiceHandle);

	BLE_SERVICE_CHAR_TypeDef bct;
	uint16_t subhandle;
	for (uint8_t i = 0; i < DEVICE_SERVICES_CHAR_NUMBER; i++)
	{
		bct = sDeviceServiceChara[i];
		Char_UUID_t cut;
		cut.Char_UUID_16 = ((uint16_t)bct.charUuid[13] >> 8) + bct.charUuid[12];
		memcpy((uint8_t*)&cut.Char_UUID_128[0], bct.charUuid, 16);
		ret = aci_gatt_add_char(sConnEvent.ServiceHandle, bct.charUuidType, &cut, bct.charValueLen, bct.charProperties, bct.secPermissions, bct.gattEvtMask, bct.encryKeySize, bct.isVariable, &subhandle);
		if (ret != BLE_STATUS_SUCCESS)
		{
			kprint("(%u)failed to add char, ret=0x%x\r\n", i, ret);
			goto failed;
		}

		kprint("add %u char, Handle=0x%04X\r\n", i, subhandle);
	}
	sBleInit = true;
	kprint("ok\r\n");
	sBleCallBack.disable_sleep(0);
	return STD_SUCCESS;
failed:
	sBleCallBack.disable_sleep(0);
	return STD_FAILED;
}
#pragma endregion

#pragma region 数据接收
static __BLE_RX_EVENT_TypeDef sRxEvt[BLE_RX_EVENT_QUEUE_LENGTH] = { 0 };
static uint8_t sRxInPtr = 0, sRxOutPtr = 0;
//判断是否有数据需要处理
uint8_t ble_rx_is_busy(void)
{
	return sRxInPtr != sRxOutPtr;
}

//使能接收处理任务
void ble_enable_rx_task(void)
{
	//bkpf("rx\r\n");
	UTIL_SEQ_SetTask(1 << CFG_TASK_BLE_RX_PROC, CFG_PRIO_NBR_1);
}

//接收数据流程
uint8_t ble_rx_data_in(uint8_t type, uint8_t* buffer, uint8_t buffersize)
{
	if (buffersize == 0)
	{
		bkpf("no data, check, type=%u\r\n", type);
		return STD_FAILED;
	}
	//bkkpf("0x%x\r\n", buffer[0]);
	uint8_t in, in_c, out;
	in = in_c = sRxInPtr;
	out = sRxOutPtr;

	//检测溢出，丢弃最新的
	in++;
	if (in == BLE_RX_EVENT_QUEUE_LENGTH)
	{
		in = 0;
	}
	if (in == out)
	{
		bnpf("full(%u)\r\n", type);
		return STD_FAILED;
	}

	memcpy(sRxEvt[in_c].Buffer, buffer, buffersize);
	sRxEvt[in_c].BufferSize = buffersize;
	sRxEvt[in_c].Type = type;
	sRxInPtr = in;

	ble_enable_rx_task();
	return STD_SUCCESS;

}

//数据处理流程
void ble_rx_proc(void)
{

	if (!ble_rx_is_busy())
	{
		kprint("do nothing\r\n");
		return;
	}

	//序号是连续的，需要顺序执行
	static uint8_t running = 0;
	if (running)
	{
		return;
	}
	running = true;

	uint8_t out = sRxOutPtr;
	switch (sRxEvt[out].Type)
	{
	case BLE_RX_CMD_TYPE:
	{
		sBleCallBack.rx_proc(sRxEvt[out].Buffer, sRxEvt[out].BufferSize);
	}
	break;
	case BLE_RX_DATA_TYPE:
	{
		sBleCallBack.data_proc(sRxEvt[out].Buffer, sRxEvt[out].BufferSize);
	}
	break;
	case BLE_RX_IMAGE_TYPE:
	{
		sBleCallBack.image_proc(sRxEvt[out].Buffer, sRxEvt[out].BufferSize);
	}
	break;
	}
	out++;
	if (out == BLE_RX_EVENT_QUEUE_LENGTH)
	{
		out = 0;
	}
	sRxOutPtr = out;

	if (ble_rx_is_busy())
	{
		ble_enable_rx_task();
	}
	running = false;
}
#pragma endregion

#pragma region 数据发送

//获取上传总字节数
uint32_t ble_get_send_bytes(void)
{
	return sConnEvent.SendBytes;
}

void ble_clear_send_bytes(void)
{
	sConnEvent.SendBytes = 0;
}

#pragma region 指令答复

//lowlevel回复函数，在问答流程中使用
uint8_t ble_ll_reply(uint8_t* buffer, uint8_t buffersize)
{
	uint8_t ret;
	//在使能通知后，才可进行回复
	if (sConnEvent.ConnStatus < BLE_CONNT_STATUS_ENABLE_ALL_NOTIFY)
	{
		bkpf("failed, not connect\r\n");
		return STD_FAILED;
	}
	bkpf("tx=0x%x,seq=0x%x\r\n", buffer[0], buffer[1]);
	//设置延时时间,如果tx忙，等待1秒钟.
	uint64_t timeout = Clock_Time() + 1000;

	do {
		ret = aci_gatt_update_char_value(sConnEvent.ServiceHandle, EVENT_TX_HANDLE, 0, buffersize, (uint8_t*)buffer);

		if (ret == BLE_STATUS_SUCCESS)
		{
			sConnEvent.SendBytes += buffersize;
			return STD_SUCCESS;
		}
		else if (ret == BLE_STATUS_INSUFFICIENT_RESOURCES)
		{
			sConnEvent.TxStatus = 1;
			bkpf("tx full\r\n");
		}
		else
		{
#ifdef DEBUG
			static uint32_t count = 0;
			bkpf("hd=0x%x, rt=0x%x - %u \r\n", EVENT_TX_HANDLE, ret, (unsigned int)count++);
#endif      
			return STD_FAILED;
		}

		//如果发送缓存满，一直等待缓存被清空后，继续发送
		//除非蓝牙已断开或出故障
		while (1)
		{
			UTIL_SEQ_Run(UTIL_SEQ_DEFAULT);
			if (sConnEvent.ConnStatus < BLE_CONNT_STATUS_ENABLE_ALL_NOTIFY)
			{
				bkpf("disconnect\r\n");
				return STD_FAILED;
			}

			if (!sConnEvent.TxStatus)
			{
				break;
			}
			if (Clock_Time() > timeout)
			{
				bkpf("timeout\r\n");
				return STD_TIMEOUT;
			}
		}

	} while (1);
}

//回复命令字
uint8_t ble_reply_cmd(uint8_t cmd, uint8_t seq)
{
	uint8_t buff[2];
	buff[0] = cmd;
	buff[1] = seq;

	return ble_ll_reply((uint8_t*)&buff[0], 2);
}

//回复内容
uint8_t ble_reply_buffer(uint8_t cmd, uint8_t seq, uint8_t* buffer, uint8_t buffersize)
{

	uint8_t buff[256];
	buff[0] = cmd;
	buff[1] = seq;
	memcpy(&buff[2], buffer, buffersize);

	return ble_ll_reply((uint8_t*)&buff[0], 2 + buffersize);
}

#pragma endregion

#pragma region 数据主动上传
//low level 上传数据函数
uint8_t ble_ll_update(uint16_t attr_handle, uint8_t* buffer, uint8_t buffersize)
{
	uint8_t ret = BLE_STATUS_ERROR;

	if (sConnEvent.TxStatus)
	{
		//bknpf("A");
		return STD_BUSY;
	}
	//当连接时，没有完成完整连接，不可以上传数据
	if (sConnEvent.ConnStatus != BLE_CONNT_STATUS_COMPLETE_CONNECTED)
	{
		return STD_FAILED;
	}
	ret = aci_gatt_update_char_value(sConnEvent.ServiceHandle, attr_handle, 0, buffersize, (uint8_t*)buffer);

	if (ret == BLE_STATUS_SUCCESS)
	{
		sConnEvent.SendBytes += buffersize;

	}
	else if (ret == BLE_STATUS_INSUFFICIENT_RESOURCES)
	{
		sConnEvent.TxStatus = 1;
		btpf("I\r\n");
		return STD_BUSY;
	}
	//else if (ret == BLE_STATUS_TIMEOUT)//实测，超时，数据也发送出去了。估计蓝牙固件故障
	//{
	//	sConnEvent.SendBytes += buffersize;
	//	bkpf("timeout\r\n");
	//}
	else
	{

#ifdef DEBUG
		static uint32_t count = 0;
		bkpf("hd=0x%x,rt=0x%x-%u \r\n", attr_handle, ret, (unsigned int)count++);
#endif
		return STD_FAILED;
	}

	return STD_SUCCESS;
}
//上传状态
uint8_t ble_update_status(uint8_t cmd, uint8_t* buffer, uint8_t buffersize)
{
	uint8_t data[BLE_GATT_CHAR_MAX_LENGTH];
	uint8_t l = 1 + buffersize; //cmd + data 的长度
	data[0] = cmd;
	memcpy(&data[1], buffer, buffersize);
	return ble_ll_update(UPDATE_STATUS_HANDLE, (uint8_t*)&data[0], l);
}
//上传数据
uint8_t ble_update_data(uint8_t* buffer, uint8_t buffersize)
{
	return ble_ll_update(UPDATE_DATA_HANDLE, buffer, buffersize);
}
//上传镜像数据
uint8_t ble_update_image(uint8_t* buffer, uint8_t buffersize)
{
	return ble_ll_update(DOWNLOAD_IMAGE_HANDLE, buffer, buffersize);
}
#pragma endregion

#pragma endregion

#pragma region 连接操作


//连接判断， 1-连接，0-未连接，2-过程状态
uint8_t ble_is_connected(void)
{
	//状态，大部分时间应该处于认证连接或广播状态，其他状态都是临时状态，不应该长期存在
	//完整的连接
	if (sConnEvent.ConnStatus == BLE_CONNT_STATUS_COMPLETE_CONNECTED)
	{
		return 1;
	}
	//其他临时过程状态
	else
	{
		return 0;
	}
}

//通过mcu校验码验证，设置蓝牙完成连接流程，可以上传数据
uint8_t ble_done_complete_connt(void)
{
	//已经使能所有属性的通知后，才使能notify中的数据上传
	if (sConnEvent.ConnStatus == BLE_CONNT_STATUS_ENABLE_ALL_NOTIFY)
	{
		sConnEvent.ConnStatus = BLE_CONNT_STATUS_COMPLETE_CONNECTED;
	}
	return STD_SUCCESS;
}

//完成全部的通知使能
uint8_t ble_done_notify_enable(void)
{
	if (NOTIFY_FLAG(NOTIFY_DATA_ENABLED) && \
		NOTIFY_FLAG(NOTIFY_STATUS_ENABLED) && \
		NOTIFY_FLAG(NOTIFY_IMAGE_ENABLED) && \
		NOTIFY_FLAG(NOTIFY_TXEVENT_ENABLED))
	{
		sConnEvent.ConnStatus = BLE_CONNT_STATUS_ENABLE_ALL_NOTIFY;
		bkpf("enable all notify\r\n");
	}
	return STD_SUCCESS;
}

//断开连接
void ble_disconnect(void)
{
	uint8_t ret;
	ret = aci_gap_terminate(sConnEvent.ConnHandle, BLE_STATUS_NOT_ALLOWED);
	if (ret)
	{
		bkpf("failed: 0x%02x\r\n", ret);
	}
	else
	{
		bkpf("ok\r\n");
	}

}

static uint8_t sStartAdvFlas = 0;
uint8_t ble_start_adv(void)
{
	if (sStartAdvFlas == 0)
	{
		sStartAdvFlas = 1;
		
	}

	return STD_SUCCESS;
}
#pragma endregion

#pragma region  协议栈发送缓存处理逻辑

void ble_tx_full_release(void)
{
	UTIL_SEQ_SetEvt(1 << CFG_IDLEEVT_BLE_TX_FULL_ID);
	return;
}

void ble_tx_full_wait(void)
{
	UTIL_SEQ_WaitEvt(1 << CFG_IDLEEVT_BLE_TX_FULL_ID);
	return;
}
#pragma endregion

#pragma region 蓝牙协议栈处理

uint8_t ble_evt_rx_proc(void* pckt)
{
	hci_event_pckt* event_pckt;
	//evt_blue_aci* blue_evt;
	//uint8_t index;

	event_pckt = (hci_event_pckt*)((hci_uart_pckt*)pckt)->data;
	//bkpf("pckt=0x%x\r\n", event_pckt->evt);
	switch (event_pckt->evt)
	{
	case EVT_CONN_COMPLETE:
	{
		bkpf("EVT_CONN_COMPLETE\r\n");
	}
	break;
	case HCI_HARDWARE_ERROR_EVT_CODE:
	{
		bkpf("hw error\r\n");
		//ble_os_reset();
	}
	break;
	case HCI_DISCONNECTION_COMPLETE_EVT_CODE:// HCI_DISCONNECTION_COMPLETE_EVT_CODE EVT_DISCONN_COMPLETE
	{

		hci_disconnection_complete_event_rp0* evt = (void*)event_pckt->data;

#ifdef DEBUG
		bkpf("[disconnect]: handle=0x%x, status=%u, reason=0x%x  \r\n", evt->Connection_Handle, evt->Status, evt->Reason);
#endif

		if (sConnEvent.DisconnWaitForCplt)
		{
			sConnEvent.DisconnWaitForCplt = 0;
		}

		gap_disconnected_cb(evt->Connection_Handle);
	}
	break;
#if 0
	case HCI_COMMAND_COMPLETE_EVT_CODE:
	{
		evt_cmd_complete* evt = (void*)event_pckt->data;

		uint16_t ogf;     /**< Opcode Group Field */
		uint16_t ocf;     /**< Opcode Command Field */
		//cmd_opcode_pack(ogf, ocf);
		ocf = cmd_opcode_ocf(evt->opcode);
		ogf = cmd_opcode_ogf(evt->opcode);
		bkpf("cmd complete - opcode=0x%x-0x%x, ncmd=0x%x\r\n", ogf, ocf, evt->ncmd);
	}
	break;
#endif
	case HCI_LE_META_EVT_CODE://HCI_LE_META_EVT_CODE EVT_LE_META_EVENT
	{
		evt_le_meta_event* evt = (void*)event_pckt->data;

		switch (evt->subevent)
		{
		case HCI_LE_CONNECTION_COMPLETE_SUBEVT_CODE:
		{

			hci_le_connection_complete_event_rp0* cc = (void*)evt->data;

			bkpf("conn complete, status= %u, interval=%u, latency=%u, timeout=%u\r\n", cc->Status, cc->Conn_Interval, cc->Conn_Latency, cc->Supervision_Timeout);
			if (cc->Status == BLE_STATUS_SUCCESS)
			{
				gap_connected_cb(cc->Peer_Address, cc->Connection_Handle);
			}
			else
			{
				bkpf("conn complete, other status = %u\r\n", cc->Status);

			}
		}
		break;
		case HCI_LE_CONNECTION_UPDATE_COMPLETE_SUBEVT_CODE:
		{
			sConntParaEvt.Status = 1;
			//sLeConnCplt = 1;
			hci_le_connection_update_complete_event_rp0* cc = (void*)evt->data;
			if (cc->Status == 0)//成功
			{
				sConntParaEvt.Respond = 1;
			}
			else//失败
			{
				sConntParaEvt.Respond = 2;
			}
#ifdef DEBUG

			bkpf("[l2cap_clpt]: status= %u, interval=%u, latency=%u, timeout=%u\r\n", cc->Status, cc->Conn_Interval, cc->Conn_Latency, cc->Supervision_Timeout);
#endif
		}
		break;
		case HCI_LE_PHY_UPDATE_COMPLETE_SUBEVT_CODE:
		{
			hci_le_phy_update_complete_event_rp0* puc = (void*)evt->data;
			if (puc->Status == 0)
			{
				bkpf("phy update, ok\r\n");
			}
			else
			{
				bkpf("phy update, failed (0x%x)\r\n", puc->Status);
			}
			uint8_t TX_PHY, RX_PHY;
			tBleStatus ret = hci_le_read_phy(puc->Connection_Handle, &TX_PHY, &RX_PHY);
			if (ret == BLE_STATUS_SUCCESS)
			{
				bkpf("PHY Param: TX= %d, RX= %d\r\n", TX_PHY, RX_PHY);
			}
			else
			{
				bkpf("failed to read phy param\r\n");
			}
		}
		break;
		default:
			bkpf("[BLE]: META_EVENT = 0x%x\r\n", evt->subevent);
			break;
		}
	}
	break;
	case HCI_ENCRYPTION_CHANGE_EVT_CODE:// HCI_ENCRYPTION_CHANGE_EVT_CODE EVT_ENCRYPT_CHANGE
	{
#ifdef DEBUG       
		hci_encryption_change_event_rp0* evt = (void*)event_pckt->data;
		bkpf("[encrypt]: handle=0x%x, status=0x%x, encrypt=0x%x\r\n", evt->Connection_Handle, evt->Status, evt->Encryption_Enabled);

#endif
	}
	break;
	case HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE:
	{
		evt_blue_aci* blue_evt = (void*)event_pckt->data;
		switch (blue_evt->ecode)
		{
		case ACI_HAL_END_OF_RADIO_ACTIVITY_VSEVT_CODE:
		{
			bkpf("radio activity end\r\n");
			ble_os_reset();
		}
		break;
		case ACI_GAP_PROC_COMPLETE_VSEVT_CODE:
		{

			aci_gap_proc_complete_event_rp0* pr = (void*)blue_evt->data;
#if DEBUG
			if (pr->Status != 0)
			{
				bkpf("[gap_procedure_complete]: code=%02x, status=%02x, l=%u\r\n", pr->Procedure_Code, pr->Status, pr->Data_Length);
				//break;
			}
#endif
			switch (pr->Procedure_Code)
			{
			case GAP_GENERAL_DISCOVERY_PROC:
			case GAP_GENERAL_CONNECTION_ESTABLISHMENT_PROC:
			{
				bkpf("discovery end, status=%02x, l=%u\r\n", pr->Status, pr->Data_Length);
			}
			break;
			case GAP_DIRECT_CONNECTION_ESTABLISHMENT_PROC:
			{
				bkpf("gap direct establish connection \r\n");
			}
			break;
			default:
			{
				bkpf("Can't tell gap procedure code(0x%x)\r\n", pr->Procedure_Code);
			}
			break;

			}
		}
		break;
		case ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE:
		{
			aci_gatt_attribute_modified_event_rp0* rp = (aci_gatt_attribute_modified_event_rp0*)blue_evt->data;
			gatt_attribute_modified_cb(rp->Attr_Handle, rp->Attr_Data, rp->Attr_Data_Length);
		}
		break;
		case ACI_GATT_PROC_TIMEOUT_VSEVT_CODE:
		{
			bkpf("[gatt_procedure]: time out\r\n");
		}
		break;
		case ACI_ATT_EXCHANGE_MTU_RESP_VSEVT_CODE: //ACI_GATT_PROC_COMPLETE_EVENT
		{
			aci_att_exchange_mtu_resp_event_rp0* evt = (aci_att_exchange_mtu_resp_event_rp0*)blue_evt->data;
			//bkpf("att exchange: handle=0x%x,mtu=%u\r\n", evt->Connection_Handle, evt->Server_RX_MTU);
			bkpf("mtu=%u\r\n", evt->Server_RX_MTU);
			sConnEvent.MTU = evt->Server_RX_MTU;
		}
		break;
#if 0
		case ACI_GATT_PROC_COMPLETE_VSEVT_CODE:
		{
			aci_gatt_proc_complete_event_rp0* pr = (aci_gatt_proc_complete_event_rp0*)blue_evt->data;
			//ble_read_notify_return((uint8_t *)pr);
			sGattEvt.Complete = 1;
			sGattEvt.ErrorCode = pr->Error_Code;

			if (sGattEvt.PrintMode == 0)
			{
				kprint("[gatt clpt]: handle = 0x%x, error = 0x%x\r\n", pr->Connection_Handle, pr->Error_Code);
			}
			else
			{
				//bkpf("[ble]:gatt complete\r\n");
				sGattEvt.PrintMode = 0;
			}
		}
		break;
#endif
		case ACI_GATT_ERROR_RESP_VSEVT_CODE:
		{
			aci_gatt_error_resp_event_rp0* pr = (aci_gatt_error_resp_event_rp0*)blue_evt->data;
			bkpf("[gatt error resp]: handle=0x%x, attr=0x%x, opcode=0x%x, error=0x%x\r\n", pr->Connection_Handle, pr->Attribute_Handle, pr->Req_Opcode, pr->Error_Code);
		}
		break;
		case ACI_GATT_TX_POOL_AVAILABLE_VSEVT_CODE:
		{
			/* New BlueNRG-MS FW 7.1c stack event: it allows to notify when at least 2 GATT TX buffers are available.
			This avoids continuous polling until BLE_STATUS_INSUFFICIENT_RESOURCES is returned. */
			sConnEvent.TxStatus = 0;
			ble_tx_full_release();
		}
		break;
		default:
		{
			bkpf("[EVT_VENDOR]: don't tell VENDOR EVT_CODE 0x%x\r\n", blue_evt->ecode);
		}
		break;
		}
	}
	break;
	default:
	{
		bkpf("don't tell event_pckt 0x%x\r\n", event_pckt->evt);
	}
	break;
	}

	return STD_SUCCESS;
}

#pragma endregion

#pragma region 蓝牙回调
//数据接收事件
void gatt_attribute_modified_cb(uint16_t handle, uint8_t* att_data, uint8_t data_length)
{

	switch (handle)
	{
#pragma region 使能通知
	case EVENT_TX_HANDLE + 2:
	{
		btpf("1 Enable Tx Event notify - 0x%x\r\n", handle);
		NOTIFY_FLAG_SET(NOTIFY_TXEVENT_ENABLED);
		ble_done_notify_enable();
	}
	break;
	case UPDATE_DATA_HANDLE + 2:
	{
		btpf("2 Enable Data notify - 0x%x\r\n", handle);
		NOTIFY_FLAG_SET(NOTIFY_DATA_ENABLED);
		ble_done_notify_enable();
	}
	break;
	case UPDATE_STATUS_HANDLE + 2:
	{
		btpf("3 Enable Status notify - 0x%x\r\n", handle);
		NOTIFY_FLAG_SET(NOTIFY_STATUS_ENABLED);
		ble_done_notify_enable();
	}
	break;
	case DOWNLOAD_IMAGE_HANDLE + 2:
	{
		btpf("4 Enable BurnImage notify - 0x%x\r\n", handle);
		NOTIFY_FLAG_SET(NOTIFY_IMAGE_ENABLED);
		ble_done_notify_enable();
	}
	break;

#pragma endregion

#pragma region cmd数据接收
	case EVENT_RX_HANDLE + 1:
	{
		ble_rx_data_in(BLE_RX_CMD_TYPE, att_data, data_length);
	}
	break;
	case UPDATE_DATA_HANDLE + 1:
	{
		ble_rx_data_in(BLE_RX_DATA_TYPE, att_data, data_length);
	}
	break;
	case DOWNLOAD_IMAGE_HANDLE + 1:
	{
		ble_rx_data_in(BLE_RX_IMAGE_TYPE, att_data, data_length);
#if DEBUG
		bnpf("y");
		static uint16_t count = 0;
		if (60 == count++)
		{
			count = 0;
			bnpf("\r\n");
		}
#endif
	}
	break;
#pragma endregion

#pragma region 其他
	default:
	{
		bkpf("handle=0x%x, size=%u -- ", handle, data_length);
		for (uint8_t i = 0; i < data_length; i++)
		{
			bnpf("0x%x, ", att_data[i]);
		}
		bnpf("\r\n");
	}
	break;
#pragma endregion
	}

}

//连接事件
void gap_connected_cb(uint8_t addr[6], uint16_t handle)
{
	sConnEvent.ConnHandle = handle;
	bkpf("connected, handle=0x%x, id=%02x%02x%02x%02x%02x%02x\r\n", handle, addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
	sConnEvent.ConnStatus = BLE_CONNT_STATUS_GENERAL_CONNECTED;
	if (sBleCallBack.connected != NULL)
	{
		sBleCallBack.connected();
	}
}

//断开事件
void gap_disconnected_cb(uint16_t handle)
{
	sConnEvent.ConnStatus = BLE_CONNT_STATUS_IDLE;
	sConnEvent.TxStatus = 0;
	sConnEvent.MTU = 0;
	sConnEvent.SendBytes = 0;
	if (sBleCallBack.disconnected != NULL)
	{
		sBleCallBack.disconnected();
	}
	ble_tx_full_release();
	sConnEvent.ConnHandle = 0xffff;
	APP_BLE_Adv_Start();

}




#pragma endregion





/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
