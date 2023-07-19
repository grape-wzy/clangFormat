/*******************************************************************************
* file    ble_proc.c
* author  mackgim
* version V1.0.1
* date    2019-11-20
* brief   BLE接口定义
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __BLE_PROC_H
#define __BLE_PROC_H

#include <stdint.h>
#include "gcompiler.h"
/*******************结构定义****************************************************/
// 蓝牙故障统计信息
// ErrorTotalCount   ----------- 所有故障累计次数
// OverFlowCount----------- 蓝牙缓存溢出次数
// ErrorType     ----------- 故障类型BluenrgErrortype
// PairTotalNumber---------- 密钥验证不匹配次数
// Repetitions   ----------- 复位故障次数
// DisconntCount  ----------- 当前蓝牙故障累计次数
typedef struct {
	uint32_t ErrorTotalCount;
	uint32_t ErrorCycleCount;
	uint32_t OverFlowCount;
	uint32_t DisconntCount;
}__BLE_ERROR_TypeDef;

// BLE属性
// charUuid--------------------属性UUID
// charUuidType----------------属性UUID类型
// charValueLen----------------UUID值长度
// charProperties--------------属性性质
// secPermissions--------------权限
// gattEvtMask-----------------GATT事件掩码
// encryKeySize----------------秘钥大小
// isVariable------------------是否可视
typedef struct
{
	const uint8_t* charUuid;
	uint8_t charUuidType;
	uint8_t charValueLen;
	uint8_t charProperties;
	uint8_t secPermissions;
	uint8_t gattEvtMask;
	uint8_t encryKeySize;
	uint8_t isVariable;
} BLE_SERVICE_CHAR_TypeDef;

#define COPY_UUID_128(uuid_15, uuid_14, uuid_13, uuid_12, uuid_11, uuid_10, uuid_9, uuid_8, uuid_7, uuid_6, uuid_5, uuid_4, uuid_3, uuid_2, uuid_1, uuid_0) \
{uuid_0,uuid_1,uuid_2,uuid_3,uuid_4,uuid_5,uuid_6,uuid_7,uuid_8,uuid_9,uuid_10,uuid_11,uuid_12,uuid_13,uuid_14,uuid_15}

#define BLE_GATT_CHAR_MAX_LENGTH		CFG_BLE_MAX_ATT_MTU //ATT 数据最长长度



typedef struct {
	uint16_t min_interval;
	uint16_t max_interval;
	uint16_t latency;
	uint16_t timeout;
	uint16_t min_length;
	uint16_t max_length;
}__BLE_CONNECTION_PARAMETER_TypeDef;



//连接事件
typedef  struct
{
    //uint16_t ServerType;	//设备类型,
    uint16_t ConnHandle;    //连接句柄
    uint16_t ServiceHandle;	//服务根句柄
	uint16_t MTU;			//rx mtu
	uint32_t NotifyStatus;
	uint8_t DisconnWaitForCplt; //主动断开
	uint8_t ConnStatus;		//连接状态:0x00-未连接, 0x01-完整连接, 0x02-搜索到mac, 0x03-普通连接, 0x04-安全连接，0x05-认证连接, 0x10-尝试连接
	uint8_t TxStatus;		//0-tx缓冲未满，1-tx缓冲已满
	//uint8_t ConnMacType;	//mac类型，私有或共有
	//uint8_t LocalID[BLE_FULL_MAC_PIN_SIZE];	// 设备ID号
	//uint8_t LocalNameSize;//BLE名称长度
	//uint8_t LocalName[20];//BLE名称内容
	uint32_t SendBytes;
	//uint32_t PinCode;		//密钥
}  __BLE_CONNECT_EVENT_TypeDef;

//连接参数更新事件
// 0 没有配置连接参数， 1 当前为休眠参数， 2 当前为工作参数
typedef  struct
{
	uint8_t ParaMode;//当前连接参数
	uint8_t ParaGoal;//目标连接参数
	uint8_t ParaTemp;//连接参数的过程状态
	uint8_t Respond; //执行返回 // 1 -成功 ， 2-拒绝
	uint8_t Status;//执行状态  // 0 -未完成 ， 1-已完成
}  __BLE_CONNECT_PARA_EVENT_TypeDef;


typedef struct
{
	uint8_t  Cmd;// 命令字
	uint8_t  Sequence;// 序号
	uint8_t  Buffer[G_VARIABLE_SIZE];
} __BLE_COMMAND_EVENT_TypeDef;

typedef struct
{
	uint8_t  Cmd;// 命令字
	uint8_t  Buffer[G_VARIABLE_SIZE];
} __BLE_DATA_EVENT_TypeDef;


//蓝牙数据接收
//类型
#define BLE_RX_CMD_TYPE			0x1
#define BLE_RX_DATA_TYPE		0x2
#define BLE_RX_IMAGE_TYPE		0x3
//队列长度
#define BLE_RX_EVENT_QUEUE_LENGTH		20


//连接状态
#define BLE_CONNT_STATUS_IDLE					0x00 //未连接
#define BLE_CONNT_STATUS_GENERAL_CONNECTED		0x01 //完成蓝牙第一步连接
#define BLE_CONNT_STATUS_ENABLE_ALL_NOTIFY		0x02 //连接中，已使能蓝牙各属性的通知更新服务
#define BLE_CONNT_STATUS_COMPLETE_CONNECTED		0x03 //认证连接，正常工作的连接，此连接完成后，从机才可上传数据


//属性句柄
#define EVENT_RX_HANDLE							0x000D //client to server 上位机到传感器
#define EVENT_TX_HANDLE							0x000F //server to client 传感器到上位机
#define UPDATE_DATA_HANDLE						0x0012
#define UPDATE_STATUS_HANDLE					0x0015
#define DOWNLOAD_IMAGE_HANDLE					0x0018

//蓝牙名称，最大数值
#define BLE_LOCAL_NAME_MAX_NUMBER               (999999)

//通知使能宏定义
#define NOTIFY_TXEVENT_ENABLED                  0x1
#define NOTIFY_DATA_ENABLED						0x2
#define NOTIFY_STATUS_ENABLED					0x4
#define NOTIFY_IMAGE_ENABLED					0x8

#define NOTIFY_FLAG(flag)                       (sConnEvent.NotifyStatus & flag)
#define NOTIFY_FLAG_SET(flag)	(sConnEvent.NotifyStatus |= flag)
#define NOTIFY_FLAG_CLEAR(flag)                 (sConnEvent.NotifyStatus &= ~flag)
#define NOTIFY_FLAG_ALLCLEAR()	(sConnEvent.NotifyStatus = 0)



typedef struct {
	uint8_t(*connected)();
	uint8_t(*disconnected)();
	uint8_t(*rx_proc)(uint8_t*, uint8_t);
	uint8_t(*data_proc)(uint8_t*, uint8_t);
	uint8_t(*image_proc)(uint8_t*, uint8_t);
	void(*disable_sleep)(uint8_t);
}__BLE_CALLBACK_TypeDef;


void ble_init_clock(void);
void ble_init(void);
void ble_deinit(void);
uint8_t ble_is_init(void);
void ble_os_reset(void);


void ble_register_callback(__BLE_CALLBACK_TypeDef* cb);

void ble_set_local_id(uint8_t device, uint8_t input[]);

uint8_t ble_set_power_level(int8_t pl);
int8_t ble_get_power_level(void);

uint32_t ble_get_stack_size(void);
uint32_t ble_get_stack_numerical_version(void);
uint32_t ble_get_fus_numerical_version(void);
uint8_t ble_get_string_version(uint8_t* stack_hw, uint8_t* stack_fw, uint8_t* fus_fw);

uint8_t ble_get_rssi(int16_t* rssi);
uint16_t ble_get_mtu(void);

uint8_t ble_is_connected(void);
uint8_t ble_done_notify_enable(void);
uint8_t ble_done_complete_connt(void);
void ble_disconnect(void);

uint8_t ble_start_adv(void);

void ble_clear_send_bytes(void);
uint32_t ble_get_send_bytes(void);

//数据应答
uint8_t ble_reply_cmd(uint8_t cmd, uint8_t seq);
uint8_t ble_reply_buffer(uint8_t cmd, uint8_t seq, uint8_t *buffer, uint8_t buffersize);
//主动上传
uint8_t ble_update_status(uint8_t cmd, uint8_t* buffer, uint8_t buffersize);
uint8_t ble_update_data(uint8_t* buffer, uint8_t buffersize);
uint8_t ble_update_image(uint8_t* buffer, uint8_t buffersize);

#define ble_reply_command(c)  ble_reply_cmd(c,rx->Sequence)
#define ble_reply_data(b,s)  ble_reply_buffer(rx->Cmd,rx->Sequence,b,s)


void ble_tx_full_release(void);
void ble_tx_full_wait(void);

#endif /* __BLE_PROC_H*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
