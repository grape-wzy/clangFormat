/*******************************************************************************
 * file    itask.c
 * author  mackgim
 * version 1.0.0
 * date
 * brief   逻辑任务
 *******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "itask.h"
#include "pd_proc.h"
#include "pm_proc.h"
#include "user_drivers.h"

/***********************宏定义*************************************************/
#if DEBUG
#define tkkpf(...) kprint(__VA_ARGS__)
#else
#define tkkpf(...)
#endif

#pragma region 函数

static uint8_t itask_ble_connected();
static uint8_t itask_ble_disconnected();
static uint8_t itask_rx_proc(uint8_t *data, uint8_t size);
static uint8_t itask_data_proc(uint8_t *data, uint8_t size);
static uint8_t itask_image_proc(uint8_t *data, uint8_t size);

static uint8_t itask_set_alg_para(uint8_t *buffer, uint8_t buffersize);
static uint8_t itask_get_alg_para(uint8_t *input, uint8_t inputsize, uint8_t *out, uint8_t *outsize);

void itask_update_general_status(void);

static void itask_test_ble_speed_proc(void);

// 状态任务，当蓝牙连接之后，以2秒的频率运行
void itask_status_proc(void);
void itask_status_ts_callback(void);

// 系统时间管理，10分钟保持一次
void itask_time_keeper_proc(void);
void itask_time_keeper_ts_callback(void);

#ifdef DEBUG
void itaks_test_rx_evt(void);
#endif

#pragma endregion

#pragma region 变量

// 新定义: 1、休眠(新版改进，苏醒即休眠)，2、关机，3、休眠，4、复位，5、工作、6、校准，7、测试，8、固件升级
// 除了工作，其他时候尽可能休眠
static uint8_t sSystemStatus = SYSTEM_STATUS_NULL;
static uint8_t sLastSystemStatus = SYSTEM_STATUS_NULL;
// 有效设备校验码
static __MCU_MARK_TypeDef sMcuMark = {{0, 0}, 0};
// 状态任务
static uint8_t sTSItaskStatusID;
// 时间保存任务
static uint8_t sTSItaskTimeKeeperID;
// 设备是否正在老化
static uint8_t sDeviceAgeing = false;
// 速度测试
static __TESTSPEED_SETTING_TypeDef SpeedTestBuff; // = { 0, { 0x12345678, 0x9abcdef0, 0x02468ace, 0x13579bdf } };

#pragma endregion

#pragma region 基本功能

void itask_led_ctrl(void)
{
	// static uint8_t count = 0;
	// count++;
	// if (count == 20)
	{
		// count = 0;
		if (ble_is_connected())
		{
			led_ctrl(LED_MODE_G_BLINK, 200); // 绿灯闪烁
		}
		else if (sDeviceAgeing)
		{
			led_ctrl(LED_MODE_Y_G_BLINK, 200); // 蓝绿灯闪烁
		}
	}
}

void itask_init(void)
{

	ts_create(0, &(sTSItaskStatusID), TS_Repeated, itask_status_ts_callback);
	UTIL_SEQ_RegTask(1 << CFG_TASK_STATUS_ID, UTIL_SEQ_RFU, itask_status_proc);

	ts_create(0, &(sTSItaskTimeKeeperID), TS_Repeated, itask_time_keeper_ts_callback);
	UTIL_SEQ_RegTask(1 << CFG_TASK_TIME_KEEPER_ID, UTIL_SEQ_RFU, itask_time_keeper_proc);

	UTIL_SEQ_RegTask(1 << CFG_TASK_TEST_BLE_SPEED_ID, UTIL_SEQ_RFU, itask_test_ble_speed_proc);

#ifdef DEBUG
	log_register_rx_cb(itaks_test_rx_evt);
#endif

	// 注册4秒中断回调,中断为gtimer的周期中断
	__GTIMER_CALLBACK_TypeDef gtimer_cb = {
		.irq_cb = pwr_check_mcu_alive,
		.sleep_cb = pwr_stop_mcu_for_delay,
	};
	gtimer_register_callback(&gtimer_cb);

	__SKT_CALLBACK_TypeDef skt_cb = {
		.send = ble_update_data,
		.led_ctrl = itask_led_ctrl,
	};
	skt_register_cb(&skt_cb);

	// 读取skt的配置
	if (pd_read_skt_config() == STD_SUCCESS)
	{
		skt_set_config(gSKTConfig);
		kprint("have a skt config\r\n\r\n");
	}
	else
	{
		kprint("no skt config\r\n\r\n");
	}
	// 初始化skt
	uint8_t device = read_device_type();
	// 0-master, 1-reference
	skt_init(device, (uint32_t)&gAlgoPara.Value[0]);
	log_flush();

	// 注册BLE回调函数
	__BLE_CALLBACK_TypeDef sBleCallBack = {
		.connected = itask_ble_connected,
		.disconnected = itask_ble_disconnected,
		.rx_proc = itask_rx_proc,
		.data_proc = itask_data_proc,
		.image_proc = itask_image_proc,
		.disable_sleep = pwr_disable_sleep,
	};
	ble_register_callback(&sBleCallBack);
	// 在pd_init中已更新gFlashBleMac
	ble_set_local_id(device, (uint8_t *)&gFlashBleInfo.Mac[0]);
	ble_init();

	ts_start_ms(sTSItaskTimeKeeperID, 20 * 60 * 1000); // 20分钟
}

void itask_proc(void)
{
	UTIL_SEQ_Run(UTIL_SEQ_DEFAULT);
}

#pragma endregion

#pragma region 任务序列

void UTIL_SEQ_PreIdle(void)
{
	pm_proc_pre_idle(sSystemStatus);
}

void UTIL_SEQ_Idle(void)
{
	pm_proc_idle(sSystemStatus);
}

void UTIL_SEQ_PostIdle(void)
{
	pm_proc_post_idle(sSystemStatus);
}

/**
 * @brief  This function is called by the scheduler each time an event
 *         is pending.
 *
 * @param  evt_waited_bm : Event pending.
 * @retval None
 */
void UTIL_SEQ_EvtIdle(UTIL_SEQ_bm_t task_id_bm, UTIL_SEQ_bm_t evt_waited_bm)
{
	UTIL_SEQ_Run(UTIL_SEQ_DEFAULT);
}

#pragma endregion

#pragma region 系统时间周期存储

void itask_time_keeper_proc(void)
{
	pd_save_time();
}

void itask_time_keeper_ts_callback(void)
{
	// kprint("hello\r\n");
	UTIL_SEQ_SetTask(1 << CFG_TASK_TIME_KEEPER_ID, CFG_PRIO_NBR_4);
}
#pragma endregion

#pragma region 周期任务

void itask_status_ts_callback(void)
{
	UTIL_SEQ_SetTask(1 << CFG_TASK_STATUS_ID, CFG_PRIO_NBR_2);
}

void itask_status_proc(void)
{
	switch (sSystemStatus)
	{
	case SYSTEM_STATUS_NULL:
	case SYSTEM_STATUS_WORKING:
	{
		itask_update_general_status();
	}
	case SYSTEM_STATUS_UPDATING_IMAGE:
	{
		// 超时故障,如果蓝牙没有断开，迅速断开
		if (fw_timeout() == STD_TIMEOUT)
		{
			sSystemStatus = SYSTEM_STATUS_NULL;
			kprint("fw,timeout\r\n");

			if (ble_is_connected())
			{
				ble_disconnect();
			}
		}
	}
	break;
	default:
		break;
	}

	// case SYSTEM_STATUS_TESTING:
	//{
	//	itask_test_ble_speed_proc();
	// }
}
#pragma endregion

#pragma region 状态流程

// 更新日常状态
void itask_update_general_status(void)
{
	// 日常状态
	//__GENERAL_STATUS_TypeDef sGeneralStatus = { 0 };
	//__SYSTEM_TIME32_TypeDef t;
	// sys_get_time(&t);
	// sGeneralStatus.RunTime = t.RunTime;
	// sGeneralStatus.WorkTime = t.WorkTime;
	// sGeneralStatus.UpTime = t.UpTime;

	// sGeneralStatus.BleTransmitBytes = ble_get_send_bytes() + sizeof(sGeneralStatus) + 1;        //1为命令字的byte数
	// sGeneralStatus.Power = pwr_get_power_relative();       													 //pm_get_power_relative();

	// if (ble_update_status(STATUS_GENERAL_INDEX, (uint8_t*)&sGeneralStatus, sizeof(sGeneralStatus)) == STD_SUCCESS)
	//{
	// }

	__GENERAL_LONG_STATUS_TypeDef lstatus = {0};
	lstatus.Length = sizeof(lstatus) - 1;
	lstatus.DataType = STATUS_LONG_TYPE_0X02_INDEX;
	lstatus.BleTransmitBytes = ble_get_send_bytes() + sizeof(lstatus) + 1;
	lstatus.ErrorCode = GLOBAL_FAULT_VALUE;

	__SYSTEM_TIME32_TypeDef t;
	sys_get_time(&t);
	lstatus.RunTime = t.RunTime;
	lstatus.WorkTime = t.WorkTime;
	lstatus.UpTime = t.UpTime;

	lstatus.Power = pwr_get_power_relative();
	lstatus.Role = skt_get_role();

	if (ble_update_status(STATUS_LONG_GENERAL_INDEX, (uint8_t *)&lstatus, sizeof(lstatus)) == STD_SUCCESS)
	{
	}
}

// 更新系统状态
void itask_update_system_status(void)
{

	if (sLastSystemStatus != sSystemStatus)
	{
		if (ble_update_status(STATUS_SYSTEM_INDEX, (uint8_t *)&sSystemStatus, sizeof(sSystemStatus)) == STD_SUCCESS)
		{
			sLastSystemStatus = sSystemStatus;
		}
	}
}

#pragma endregion

#pragma region 回调函数 - 蓝牙业务处理，包括与上位机的交互处理

// 蓝牙连接
uint8_t itask_ble_connected()
{
	led_ctrl(LED_MODE_Y_CTRL, 0); // 黄灯灭
	led_ctrl(LED_MODE_G_CTRL, 1); // 绿灯亮
	ts_start_ms(sTSItaskStatusID, 2000);
	return STD_SUCCESS;
}

// 蓝牙断开
uint8_t itask_ble_disconnected()
{
	led_ctrl(LED_MODE_G_CTRL, 0);  // 绿灯灭
	led_ctrl(LED_MODE_Y_BLINK, 0); // 黄灯闪烁
	// sSystemStatus = SYSTEM_STATUS_NULL;
	ts_stop(sTSItaskStatusID);
	return STD_SUCCESS;
}

// cmd数据交互,非中断，在正常的大循环中执行
uint8_t itask_rx_proc(uint8_t *data, uint8_t size)
{

	__BLE_COMMAND_EVENT_TypeDef *rx = (__BLE_COMMAND_EVENT_TypeDef *)data;
	{
		uint8_t rx_length = size - 2;
		tkkpf("rx=0x%x,seq=0x%x\r\n", rx->Cmd, rx->Sequence);
		switch (rx->Cmd)
		{
#pragma region 初始化指令
		case ET_GET_Device_Status:
		{
			__DEVICE_STATUS_TypeDef tDeviceInitStatus;
			tDeviceInitStatus.BootTimes = gFlashTime.BootTimes;
			tDeviceInitStatus.EverBootTimes = gFlashTime.EverBootTimes;
			tDeviceInitStatus.BorTimes = gFlashTime.BorTimes;
			tDeviceInitStatus.Power = pwr_get_power_relative();
			ble_reply_data((uint8_t *)&tDeviceInitStatus, sizeof(tDeviceInitStatus));
		}
		break;
		case ET_SET_Device_Mark:
		{
			if (rx_length == sizeof(sMcuMark))
			{
				memcpy(&sMcuMark, rx->Buffer, rx_length);
				uint64_t t = 0;
				memcpy((void *)&t, (const void *)&sMcuMark.TimeStamp[0], sizeof(sMcuMark.TimeStamp));
				rtc_set_unix_time_ms(t);
				kprint("stamp=%lu\r\n", (long unsigned int)t);

				uint8_t ret = pd_check_mcu_mark((void *)&sMcuMark);
				ble_done_complete_connt();
				ble_reply_command(STD_SUCCESS);
				if (ret == STD_SUCCESS)
				{
					tkkpf("ok, it is a allowed and checked client\r\n");
				}
				else
				{
					tkkpf("ok, it is a allowed client\r\n");
				}
			}
			else
			{
				kprint("it is not a mcu mark\r\n");
				ble_reply_command(STD_FAILED);
			}
		}
		break;
		case ET_GET_Device_Mark:
		{
			ble_reply_data((uint8_t *)&sMcuMark, sizeof(sMcuMark));
		}
		break;
#pragma endregion

#pragma region 时间类指令
		case ET_SET_TimeStamp_MS:
		{
			__MCU_TIMEMS_SYNC_TypeDef sync;
			if (rx_length == sizeof(sync))
			{
				ble_reply_command(STD_SUCCESS);
				memcpy(&sync, rx->Buffer, rx_length);
				if (sync.Index == 2)
				{
					uint64_t x = sync.TimeStampMS;
					rtc_set_unix_time_ms(x);
				}
			}
		}
		break;
		case ET_GET_Device_Time:
		{
			__DEVICE_TIME_TypeDef dt = {0};
			__SYSTEM_TIME32_TypeDef t;
			sys_get_time(&t);
			dt.UpTime = t.UpTime;
			dt.RunTime = t.RunTime;
			dt.WorkTime = t.WorkTime;
			ble_reply_data((uint8_t *)&dt, sizeof(dt));
		}
		break;
		case ET_CMD_RunTime_Clear:
		{
			if (!pd_device_is_checked())
			{
				ble_reply_command(STD_DO_NOTHING);
				break;
			}
			pd_clear_time();
			ble_reply_command(STD_SUCCESS);
			tkkpf("[task]: RunTime_Clear \r\n");
		}
		break;
#pragma endregion

#pragma region 版本查询
		case ET_GET_Version:
		{
			uint8_t buff[247] = {0};
			uint8_t alg_version[64] = {0};
			stk_get_version(alg_version);
			get_version(buff, alg_version);
			tkkpf("%s\r\n", buff);
			uint8_t len = strlen((const char *)buff);
			ble_reply_data((uint8_t *)&buff[0], len);
		}
		break;
#pragma endregion

#pragma region 名牌设置与查询
		case ET_SET_Nameplate:
		{
			if (!pd_device_is_checked())
			{
				ble_reply_command(STD_DO_NOTHING);
				break;
			}
			if (sizeof(gProductData.Nameplate) != rx_length)
			{
				kprint("set nameplate, size(%u) error\r\n", rx_length);
				ble_reply_command(STD_FAILED);
			}
			else
			{
				memcpy(&gProductData.Nameplate, rx->Buffer, rx_length);
				uint8_t ret = pd_save_product_info();
				ble_reply_command(ret);
			}
		}
		break;
		case ET_GET_Nameplate:
		{
			ble_reply_data((uint8_t *)&gProductData.Nameplate, sizeof(gProductData.Nameplate));
		}
		break;
#pragma endregion

#pragma region 注册信息
		case ET_GET_Reg_Info:
		{
			ble_reply_data((uint8_t *)&gAccountInfo, sizeof(gAccountInfo));
		}
		break;
		case ET_CMD_Register:
		{
			uint8_t regcmd = 1;
			if (pd_save_register_info(regcmd) != STD_SUCCESS)
			{
				ble_reply_command(STD_FAILED);
			}
			else
			{
				ble_reply_command(STD_SUCCESS);
			}
		}
		break;
		case ET_CMD_Unregister:
		{
			uint8_t regcmd = 0;
			if (pd_save_register_info(regcmd) != STD_SUCCESS)
			{
				ble_reply_command(STD_FAILED);
			}
			else
			{
				ble_reply_command(STD_SUCCESS);
			}
		}
		break;
		case ET_CMD_Clear_Reg:
		{
			if (!pd_device_is_checked())
			{
				ble_reply_command(STD_DO_NOTHING);
				break;
			}
			uint8_t regcmd = 2;
			if (pd_save_register_info(regcmd) != STD_SUCCESS)
			{
				ble_reply_command(STD_FAILED);
			}
			else
			{
				ble_reply_command(STD_SUCCESS);
			}
		}
		break;
#pragma endregion

#pragma region MCU控制指令

		case ET_CMD_Device_Reset:
		{
			tkkpf("reset\r\n");

			ble_reply_command(STD_SUCCESS);
			pwr_reset_mcu();
		}
		break;
		case ET_CMD_Device_PowerOff:
		{
			if (gAccountInfo.IsLogined == 0)
			{
				tkkpf("power off\r\n");

				ble_reply_command(STD_SUCCESS);
				pwr_shutdown_mcu();
			}
			else
			{
				ble_reply_command(STD_FAILED);
				tkkpf("deny, power off\r\n");
			}
		}
		break;

#pragma endregion

#pragma region 蓝牙配置相关指令
		case ET_SET_Device_Mac:
		{
			if (!pd_device_is_checked())
			{
				ble_reply_command(STD_DO_NOTHING);
				break;
			}

			if (rx_length != sizeof(gFlashBleInfo.Mac))
			{
				ble_reply_command(STD_FAILED);
				break;
			}
			uint8_t *pbuff = rx->Buffer;
			uint16_t DeviceType = (uint16_t)(((uint16_t)pbuff[7] << 8) & 0xff00) + (pbuff[6] & 0xff);
			if (DeviceType != DEVICE_TPYE_DEFAULT_CODE)
			{
				kprint("devcie type error, 0x8020-0x%x\r\n", DeviceType);
				ble_reply_command(STD_FAILED);
				break;
			}
			memcpy((void *)&gFlashBleInfo.Mac[0], rx->Buffer, rx_length);

			uint8_t ret = pd_save_ble_mac();

			ble_reply_command(ret);
		}
		break;
		case ET_CMD_Update_Ble_Mac:
		{
			if (!pd_device_is_checked())
			{
				ble_reply_command(STD_DO_NOTHING);
				break;
			}
			uint8_t ret = pd_generate_and_save_ble_mac();

			ble_reply_command(ret);
		}
		break;
#pragma endregion

#pragma region 查询基本信息
		case ET_GET_Error:
		{
			sMcuCheck.Check = (uint32_t)skt_is_check();
			ble_reply_data((uint8_t *)&sMcuCheck, sizeof(sMcuCheck));
		}
		break;
		case ET_GET_Device_RSSI: // 信号强度
		{
			int16_t DeviceRssi = 0;
			//				ble_get_rssi(&DeviceRssi);
			ble_reply_data((uint8_t *)&DeviceRssi, sizeof(DeviceRssi));
		}
		break;
		case ET_GET_Ble_Power_Level:
		{
			//			int8_t pl = ble_get_power_level();
			int8_t pl = 2;
			ble_reply_data((uint8_t *)&pl, sizeof(pl));
		}
		break;
#pragma endregion

#pragma region 获取flash数据
		case ET_GET_Flash_Data:
		{
			if (!pd_device_is_checked())
			{
				ble_reply_command(STD_DO_NOTHING);
				break;
			}
			__FLASH_DATA_READ_TypeDef fdr = {0};

			if (sizeof(fdr) != rx_length)
			{
				kprint("flash data, rx size(%u) error\r\n", rx_length);
				ble_reply_command(STD_FAILED);
				break;
			}
			memcpy(&fdr, rx->Buffer, rx_length);
			if ((fdr.Size > 224) || (fdr.Size == 0) || (fdr.Size & 0x3))
			{
				kprint("flash data, data size(%u) error\r\n", (unsigned int)fdr.Size);
				ble_reply_command(STD_FAILED);
				break;
			}
			// if ((fdr.Addr & 0x3))
			//{
			//	kprint("flash data, data Addr(%u) error\r\n", (unsigned int)fdr.Addr);
			//	ble_reply_command(STD_FAILED);
			//	break;
			// }
			if ((fdr.Addr + fdr.Size) >= pd_get_user_end_space())
			{
				kprint("error, flash addr(0x%x) overflow\r\n", (unsigned int)fdr.Addr);
				ble_reply_command(STD_FAILED);
				break;
			}
			ble_reply_data((uint8_t *)fdr.Addr, fdr.Size);
		}
		break;
#pragma endregion

#pragma region 算法参数操作
		case ET_SET_Alg_Setting:
		{
			if (!pd_device_is_checked())
			{
				ble_reply_command(STD_DO_NOTHING);
				break;
			}
			if (itask_set_alg_para(rx->Buffer, rx_length) == STD_SUCCESS)
			{
				ble_reply_command(STD_SUCCESS);
			}
			else
			{
				ble_reply_command(STD_FAILED);
			}
		}
		break;
		case ET_GET_Alg_Setting:
		{
			uint8_t buffer[255];
			uint8_t buffersize = 0;

			if (itask_get_alg_para(rx->Buffer, rx_length, buffer, &buffersize) == STD_SUCCESS)
			{
				ble_reply_data((uint8_t *)&buffer[0], buffersize);
			}
			else
			{
				ble_reply_command(STD_FAILED);
			}
		}
		break;
#pragma endregion

#pragma region SKT算法配置指令
		case ET_SET_SKT_Config:
		{
			__FLASH_SKT_CONFIG_TypeDef config;
			if (sizeof(config) != rx_length)
			{
				kprint("skt config error, %u\r\n", rx_length);
				ble_reply_command(STD_FAILED);
				break;
			}
			memcpy(&config, rx->Buffer, rx_length);
			uint8_t ret = skt_set_config(config);

			if (ret == STD_SUCCESS)
			{
				int32_t x = memcmp(&gSKTConfig, &config, sizeof(gSKTConfig));
				if (x != 0)
				{
					memcpy(&gSKTConfig, &config, sizeof(gSKTConfig));
					ret = pd_save_skt_config();
					kprint("save skt config\r\n");
				}
			}

			ble_reply_command(ret);
		}
		break;
		case ET_SET_SKT_Mode:
		{
			__SKT_MODE_TypeDef mode;
			if (sizeof(mode) != rx_length)
			{
				kprint("skt mode error, %u\r\n", rx_length);
				ble_reply_command(STD_FAILED);
				break;
			}
			memcpy(&mode, rx->Buffer, rx_length);
			uint8_t ret = skt_set_mode(mode);
			ble_reply_command(ret);
		}
		break;
		case ET_SET_SKT_Reg_Pos:
		{
			int32_t pos;
			if (sizeof(pos) != rx_length)
			{
				kprint("skt pos error, %u\r\n", rx_length);
				ble_reply_command(STD_FAILED);
				break;
			}
			memcpy(&pos, rx->Buffer, rx_length);
			uint8_t ret = skt_set_tibia_reg_pos(pos);
			ble_reply_command(ret);
		}
		break;
		case ET_SET_SKT_Navigation_Angle:
		{
			__SKT_NAVIGATION_ANGLE_TypeDef nav;
			if (sizeof(nav) != rx_length)
			{
				kprint("skt nav angle error, %u\r\n", rx_length);
				ble_reply_command(STD_FAILED);
				break;
			}
			memcpy(&nav, rx->Buffer, rx_length);
			uint8_t ret = skt_set_navigation_angle(nav);
			ble_reply_command(ret);
		}
		break;
#pragma endregion

#pragma region 工作指令

		case ET_CMD_Start:
		{
			if (sSystemStatus == SYSTEM_STATUS_NULL)
			{

				uint8_t ret = skt_start();
				ble_reply_command(ret);

				if (ret == STD_SUCCESS)
				{
					tkkpf("ok to start\r\n");
					sSystemStatus = SYSTEM_STATUS_WORKING;
				}
				else
				{
					tkkpf("failed to start\r\n");
				}
			}
			else if (sSystemStatus == SYSTEM_STATUS_WORKING)
			{
				tkkpf("started\r\n");
				ble_reply_command(STD_SUCCESS);
			}
			else
			{
				tkkpf("deny to start\r\n");
				ble_reply_command(STD_FAILED);
			}
		}
		break;
		case ET_CMD_Stop:
		{
			sDeviceAgeing = false;
			if (sSystemStatus == SYSTEM_STATUS_WORKING)
			{
				skt_stop();
				tkkpf("stop\r\n");
				sSystemStatus = SYSTEM_STATUS_NULL;
				ble_reply_command(STD_SUCCESS);

				if (ble_is_connected())
				{
					led_ctrl(LED_MODE_G_CTRL, 1);
				}
				else
				{
					led_ctrl(LED_MODE_G_CTRL, 0);
				}
			}
			else if (sSystemStatus == SYSTEM_STATUS_NULL)
			{
				tkkpf("stopped\r\n");
				ble_reply_command(STD_SUCCESS);
			}
			else
			{
				tkkpf("deny to stop - 0x%x\r\n", sSystemStatus);
				ble_reply_command(STD_FAILED);
			}
		}
		break;
		case ET_CMD_Device_Ageing:
		{
			tkkpf("enable ageing\r\n");
			sDeviceAgeing = true;
			ble_reply_command(STD_SUCCESS);
		}
		break;
#pragma endregion

#pragma region 测试类指令
		///////////////////////////////////test//////////////////////////////////
		case ET_TEST_Speed_Start_BLE_UpLoad:
		{
			if (!pd_device_is_checked())
			{
				ble_reply_command(STD_DO_NOTHING);
				break;
			}
			if ((sSystemStatus != SYSTEM_STATUS_NULL) || (rx_length != sizeof(SpeedTestBuff.DataSize)))
			{
				tkkpf("speed test: error\r\n");
				ble_reply_command(STD_FAILED);
				break;
			}
			memcpy(&SpeedTestBuff.DataSize, rx->Buffer, rx_length);
			if ((SpeedTestBuff.DataSize > BLE_GATT_CHAR_MAX_LENGTH) || (SpeedTestBuff.DataSize == 0))
			{
				tkkpf("speed test: error size = %u\r\n", (unsigned int)SpeedTestBuff.DataSize);
				ble_reply_command(STD_FAILED);
				break;
			}
			sSystemStatus = SYSTEM_STATUS_TESTING;
			SpeedTestBuff.ID = 1;
			tkkpf("Start Speed test, size=%u\r\n", (unsigned int)SpeedTestBuff.DataSize);
			ble_reply_command(STD_SUCCESS);
			UTIL_SEQ_SetTask(1 << CFG_TASK_TEST_BLE_SPEED_ID, CFG_PRIO_NBR_4);
		}
		break;
		case ET_TEST_Speed_Stop_BLE_UpLoad:
		{
			if (!pd_device_is_checked())
			{
				ble_reply_command(STD_DO_NOTHING);
				break;
			}
			if (sSystemStatus == SYSTEM_STATUS_TESTING)
			{
				sSystemStatus = SYSTEM_STATUS_NULL;
				tkkpf("Stop Speed test\r\n");
				ble_reply_command(STD_SUCCESS);
			}
			else
			{
				ble_reply_command(STD_FAILED);
			}
		}
		break;

#pragma endregion

#pragma region 固件升级类
		case ET_CMD_Firmware_Start:
		{
			if (!pd_device_is_checked())
			{
				ble_reply_command(STD_DO_NOTHING);
				break;
			}
			if (sSystemStatus == SYSTEM_STATUS_NULL)
			{
				if (fw_start(rx->Buffer, (uint32_t)rx_length) == STD_SUCCESS)
				{
					sSystemStatus = SYSTEM_STATUS_UPDATING_IMAGE;
					ble_reply_command(STD_SUCCESS);
				}
				else
				{
					ble_reply_command(STD_FAILED);
				}
			}
			else
			{
				ble_reply_command(STD_DENIED);
			}
		}
		break;
		case ET_CMD_Firmware_Stop:
		{
			if (!pd_device_is_checked())
			{
				ble_reply_command(STD_DO_NOTHING);
				break;
			}
			// if (DownLoad_Stop(sEvtRxFrame.buff) ==true)
			if (sSystemStatus == SYSTEM_STATUS_UPDATING_IMAGE)
			{
				if (fw_stop(rx->Buffer, (uint32_t)rx_length) == STD_SUCCESS)
				{
					ble_reply_command(STD_SUCCESS);
					sSystemStatus = SYSTEM_STATUS_NULL;
				}
				else
				{
					ble_reply_command(STD_FAILED);
				}
			}
			else
			{
				ble_reply_command(STD_DENIED);
			}
		}
		break;
		case ET_CMD_Firmware_IsReady:
		{
			if (!pd_device_is_checked())
			{
				ble_reply_command(STD_DO_NOTHING);
				break;
			}

			uint8_t ret = fw_is_ready(rx->Buffer, (uint32_t)rx_length);
			ble_reply_command(ret);
		}
		break;
		case ET_SET_Firmware_Config:
		{
			if (!pd_device_is_checked())
			{
				ble_reply_command(STD_DO_NOTHING);
				break;
			}
			uint8_t ret = fw_set_config(rx->Buffer, (uint32_t)rx_length);
			ble_reply_command(ret);
		}
		break;
#pragma endregion

#pragma region 默认
		default:
		{
			tkkpf("donot tell cmd - 0x%x\r\n", rx->Cmd);
			ble_reply_command(STD_DENIED);
		}
			return STD_DENIED;
#pragma endregion
		}
	}
	return STD_SUCCESS;
}

// 数据通道，数据流，没有回复
uint8_t itask_data_proc(uint8_t *data, uint8_t size)
{
	__BLE_DATA_EVENT_TypeDef *rx = (__BLE_DATA_EVENT_TypeDef *)data;
	{
		uint8_t rx_length = size - 1;
		switch (rx->Cmd)
		{
#pragma region ref模块过来的数据

		case ET_DATA_Motion:
		{
			skt_update_ref_data(rx->Buffer, rx_length);
		}
		break;
#pragma endregion

#pragma region 默认
		default:
		{
			tkkpf("donot tell cmd - 0x%x\r\n", rx->Cmd);
		}
			return STD_DENIED;
#pragma endregion
		}
	}
	return STD_SUCCESS;
}

// 固件数据交互
uint8_t itask_image_proc(uint8_t *data, uint8_t size)
{
	//	tkkpf("size=%u\r\n", size);
	uint8_t ret = fw_proc(data, size);

	if (ret != STD_SUCCESS)
	{
		if (sSystemStatus == SYSTEM_STATUS_UPDATING_IMAGE)
		{
			sSystemStatus = SYSTEM_STATUS_NULL;
		}
		if (ble_is_connected())
		{
			ble_disconnect();
		}
	}
	return ret;
}
#pragma endregion

#pragma region 设置参数

static __LARGE_EVENT_Typedef sLargeEvent = LARGE_EVNT_DEFAULT();

uint8_t itask_set_alg_para(uint8_t *buffer, uint8_t buffersize)
{
	// 查询帧头
	uint8_t ret = sLargeEvent.is_head(&sLargeEvent, buffer, buffersize);
	// 如果是帧头的话，先导入数据
	if (ret == STD_SUCCESS)
	{
		sLargeEvent.DataP = &gAlgoPara.Value[0];
		sLargeEvent.DataSize = sizeof(gAlgoPara.Value);
		tkkpf("init\r\n");
	}

	ret = sLargeEvent.set(&sLargeEvent, buffer, buffersize);

	if (ret == STD_SUCCESS)
	{
		if (skt_check_keywork((uint32_t)&gAlgoPara.Value[0]) == STD_SUCCESS)
		{
			tkkpf("save alg para\r\n");
			pd_save_algo_para();
		}
		else
		{
			return STD_FAILED;
		}
	}
	else if (ret == STD_FAILED)
	{
		return STD_FAILED;
	}

	return STD_SUCCESS;
}

uint8_t itask_get_alg_para(uint8_t *input, uint8_t inputsize, uint8_t *out, uint8_t *outsize)
{
	uint8_t ret = sLargeEvent.is_head(&sLargeEvent, input, inputsize);
	// 如果是帧头的话，先导入数据
	if (ret == STD_SUCCESS)
	{
		pd_read_algo_para();
		sLargeEvent.DataP = &gAlgoPara.Value[0];
		sLargeEvent.DataSize = sizeof(gAlgoPara.Value);
		tkkpf("init\r\n");
	}

	ret = sLargeEvent.get(&sLargeEvent, input, inputsize, out, outsize);
	// 获取帧头
	if (ret == STD_FAILED)
	{
		return STD_FAILED;
	}
	return STD_SUCCESS;
}

#pragma endregion

#pragma region 速度测试
void itask_test_ble_speed_proc(void)
{
	led_ctrl(LED_MODE_G_CTRL, 0); // 绿灯亮
	//	if (!ble_tx_is_idle())
	//	{
	//		return;
	//	}
	uint32_t id = SpeedTestBuff.ID;
	uint32_t tx[80];
	uint32_t l = SpeedTestBuff.DataSize / 4;
	for (uint32_t i = 0; i < l; i++)
	{
		tx[i] = id;
		id++;
	}

	uint8_t ret = ble_update_image((uint8_t *)&tx, SpeedTestBuff.DataSize);
	if (ret == STD_SUCCESS)
	{
		SpeedTestBuff.ID = id;
	}
	else if (ret == STD_BUSY)
	{
		ble_tx_full_wait();
	}

	if (SYSTEM_STATUS_TESTING == sSystemStatus)
	{
		UTIL_SEQ_SetTask(1 << CFG_TASK_TEST_BLE_SPEED_ID, CFG_PRIO_NBR_4);
	}
}
#pragma endregion

#pragma region 调试测试

void fault_test_by_div0(void);
void fault_test_by_unalign(void);

void fault_test_by_unalign(void)
{
	volatile int *SCB_CCR = (volatile int *)0xE000ED14; // SCB->CCR
	volatile int *p;
	volatile int value;

	*SCB_CCR |= (1 << 3); /* bit3: UNALIGN_TRP. */

	p = (int *)0x00;
	value = *p;
	printf("addr:0x%02X value:0x%08X\r\n", (int)p, value);

	p = (int *)0x04;
	value = *p;
	printf("addr:0x%02X value:0x%08X\r\n", (int)p, value);

	p = (int *)0x03;
	value = *p;
	printf("addr:0x%02X value:0x%08X\r\n", (int)p, value);
}

void fault_test_by_div0(void)
{
	volatile int *SCB_CCR = (volatile int *)0xE000ED14; // SCB->CCR
	int x, y, z;

	*SCB_CCR |= (1 << 4); /* bit4: DIV_0_TRP. */

	x = 10;
	y = 0;
	z = x / y;
	printf("z:%d\n", z);
}

void itask_test_proc(void)
{
	uint8_t code = log_get();
	switch (code)
	{
	case '1':
	{
		kprint("XXX\r\n");
		uint32_t x[115];
		uint8_t ret = rng_get_numbers((uint32_t *)&x[0], 115);
		if (ret != STD_SUCCESS)
		{
			kprint("failed to get rand\r\n");
		}
		else
		{
			kprint("ok to get rand\r\n");
		}
		for (uint8_t i = 0; i < 5; i++)
		{
			nprint("x[%u]=0x%x, ", i, (unsigned int)x[i]);
		}
		nprint("\r\n");
	}
	break;
	case '2':
	{
		kprint("25666\r\n");
		fault_test_by_unalign();
		// skt_test();
	}
	break;
	case '3':
	{
		kprint("3\r\n");
	}
	break;
	case '4':
	{

		kprint("4\r\n");
		// ahrs_calib(0);
		// uint64_t* ap = (uint64_t*)FLASH_SPACE_SHADOW_END_ADDR;
		// uint64_t* bp = (uint64_t*)(FLASH_SPACE_SHADOW_END_ADDR + 8);
		// uint8_t ret = STD_SUCCESS;

		// uint64_t a = 0x1111222233334445;
		// for (uint32_t i = 0; i < (FLASH_PAGE_SIZE / 8); i++)
		//{
		//	a++;
		//	ret = flash_ll_write(FLASH_SPACE_SHADOW_END_ADDR + FLASH_PAGE_SIZE - 8 * (i + 1), (uint8_t*)&a, sizeof(a));
		//	if (ret != STD_SUCCESS)
		//	{
		//		kprint("write, ret=0x%x\r\n", ret);
		//		break;
		//	}
		// }

		// kprint("1,a=0x%lx, b=0x%lx\r\n", (long unsigned int) * ap, (long unsigned int) * bp);
		// uint64_t a = 0x1111222233334445;
		// ret = flash_ll_write(FLASH_SPACE_SHADOW_END_ADDR + 8, (uint8_t*)&a, sizeof(a));
		// kprint("write, ret=0x%x\r\n", ret);
		// uint64_t b = 0x5555666677778889;
		// ret = flash_ll_write(FLASH_SPACE_SHADOW_END_ADDR, (uint8_t*)&b, sizeof(b));
		// kprint("write, ret=0x%x\r\n", ret);
		// kprint("2, a=0x%lx, b=0x%lx\r\n", (long unsigned int) * ap, (long unsigned int) * bp);
	}
	break;
	case '5':
	{
		kprint("5\r\n");

		// uint64_t a = 0xffffffffffff4445;
		// flash_ll_write(FLASH_SPACE_SHADOW_END_ADDR, (uint8_t*)&a, sizeof(a));

		// uint64_t b = 0xffffffff12344445;
		// flash_ll_write(FLASH_SPACE_SHADOW_END_ADDR, (uint8_t*)&b, sizeof(b));

		// kprint("end\r\n");
		// imu_get_all_reg();
	}
	break;
	case '6':
	{
		recorder_stat();
	}
	break;
	case '7':
	{
		for (uint32_t i = 0; i < 1000; i++)
		{
			uint8_t ret = pd_save_time();
			if (ret != STD_SUCCESS)
			{
				kprint("faile, ret=0x%x,i=%d\r\n", ret, (unsigned int)i);
				break;
			}
			log_flush();
		}
		kprint("test end\r\n");
	}
	break;
	case '8':
	{
		recorder_gc();
	}
	break;
	}
}

void itaks_test_rx_evt(void)
{
#if DEBUG
	static uint8_t teskTask = 0;
	if (teskTask == 0)
	{
		teskTask = 1;
		UTIL_SEQ_RegTask(1 << CFG_TASK_TEST_PROC, UTIL_SEQ_RFU, itask_test_proc);
	}
	UTIL_SEQ_SetTask(1 << CFG_TASK_TEST_PROC, CFG_PRIO_NBR_3);
#endif
}

#pragma endregion

/*******************************************************************************
END
*******************************************************************************/
