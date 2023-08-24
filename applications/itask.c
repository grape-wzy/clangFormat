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

// 状态任务
static uint8_t sTSItaskStatusID;
// 时间保存任务
static uint8_t sTSItaskTimeKeeperID;
// 速度测试
static __TESTSPEED_SETTING_TypeDef SpeedTestBuff;

static void itask_time_keeper_ts_callback(void)
{
    /* task callback: void itask_time_keeper_proc(void) */
    UTIL_SEQ_SetTask(1 << CFG_TASK_TIME_KEEPER_ID, CFG_PRIO_NBR_4);
}

/* 系统时间管理，10分钟保持一次 */
static void itask_time_keeper_proc(void)
{
    pd_save_time();
}

/* 更新日常状态 */
static void itask_update_general_status(void)
{
    __GENERAL_LONG_STATUS_TypeDef lstatus = { 0 };
    lstatus.Length                        = sizeof(lstatus) - 1;
    lstatus.DataType                      = STATUS_LONG_TYPE_0X02_INDEX;
    lstatus.BleTransmitBytes              = ble_get_send_bytes() + sizeof(lstatus) + 1;
    lstatus.ErrorCode                     = GLOBAL_FAULT_VALUE;

    __SYSTEM_TIME32_TypeDef t;
    sys_get_time(&t);
    lstatus.RunTime  = t.RunTime;
    lstatus.WorkTime = t.WorkTime;
    lstatus.UpTime   = t.UpTime;

    lstatus.Power = pwr_get_power_relative();
    lstatus.Role  = skt_get_role();

    ble_update_status(STATUS_LONG_GENERAL_INDEX, (uint8_t *)&lstatus, sizeof(lstatus));
}

static void itask_status_ts_callback(void)
{
    /* task callback: void itask_status_proc(void) */
    UTIL_SEQ_SetTask(1 << CFG_TASK_STATUS_ID, CFG_PRIO_NBR_2);
}

// 状态任务，当蓝牙连接之后，以2秒的频率运行
static void itask_status_proc(void)
{
    uint8_t status = itask_get_system_status();
    switch (status) {
    case SYSTEM_STATUS_NULL:

    case SYSTEM_STATUS_WORKING: {
        itask_update_general_status();
    }

    case SYSTEM_STATUS_UPDATING_IMAGE: {
        // 超时故障,如果蓝牙没有断开，迅速断开
        if (fw_timeout() == STD_TIMEOUT) {
            itask_set_system_status(SYSTEM_STATUS_NULL);
            kprint("fw, timeout\r\n");

            if (ble_is_connected()) {
                ble_disconnect();
            }
        }
    } break;

    default:
        break;
    }
}

static void itask_test_ble_speed_proc(void)
{
    led_ctrl(LED_MODE_G_CTRL, 0); // 绿灯亮
    //	if (!ble_tx_is_idle())
    //	{
    //		return;
    //	}
    uint32_t id = SpeedTestBuff.ID;
    uint32_t tx[80];
    uint32_t l = SpeedTestBuff.DataSize / 4;
    for (uint32_t i = 0; i < l; i++) {
        tx[i] = id;
        id++;
    }

    uint8_t ret = ble_update_image((uint8_t *)&tx, SpeedTestBuff.DataSize);
    if (ret == STD_SUCCESS) {
        SpeedTestBuff.ID = id;
    } else if (ret == STD_BUSY) {
        ble_tx_full_wait();
    }

    if (SYSTEM_STATUS_TESTING == itask_get_system_status()) {
        UTIL_SEQ_SetTask(1 << CFG_TASK_TEST_BLE_SPEED_ID, CFG_PRIO_NBR_4);
    }
}

__WEAK uint8_t cmd_handle_proc(uint8_t command, uint8_t sequence, uint8_t *buffer, uint8_t buff_len)
{
    (void)command;
    (void)buffer;
    (void)buff_len;

    ble_reply_cmd(ET_RES_FAILED, sequence);
    kprint("do not implement the cmd_handle_proc\r\n");
    return STD_FAILED;
}

void itask_init(void)
{
    ts_create(0, &(sTSItaskStatusID), TS_Repeated, itask_status_ts_callback);
    UTIL_SEQ_RegTask(1 << CFG_TASK_STATUS_ID, UTIL_SEQ_RFU, itask_status_proc);

    ts_create(0, &(sTSItaskTimeKeeperID), TS_Repeated, itask_time_keeper_ts_callback);
    UTIL_SEQ_RegTask(1 << CFG_TASK_TIME_KEEPER_ID, UTIL_SEQ_RFU, itask_time_keeper_proc);

    UTIL_SEQ_RegTask(1 << CFG_TASK_TEST_BLE_SPEED_ID, UTIL_SEQ_RFU, itask_test_ble_speed_proc);

    ts_start_ms(sTSItaskTimeKeeperID, 20 * 60 * 1000); // 20分钟
}

void itask_proc(void)
{
    UTIL_SEQ_Run(UTIL_SEQ_DEFAULT);
}

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
    // itask_set_system_status(SYSTEM_STATUS_NULL);
    ts_stop(sTSItaskStatusID);
    return STD_SUCCESS;
}

// cmd数据交互,非中断，在正常的大循环中执行
uint8_t itask_rx_proc(uint8_t *data, uint8_t size)
{
    __BLE_COMMAND_EVENT_TypeDef *rx = (__BLE_COMMAND_EVENT_TypeDef *)data;

    uint8_t rx_length = size - 2;

    tkkpf("rx=0x%x,seq=0x%x\r\n", rx->Cmd, rx->Sequence);
    return (cmd_handle_proc(rx->Cmd, rx->Sequence, rx->Buffer, rx_length));
}

// 数据通道，数据流，没有回复
uint8_t itask_data_proc(uint8_t *data, uint8_t size)
{
    __BLE_DATA_EVENT_TypeDef *rx = (__BLE_DATA_EVENT_TypeDef *)data;
    {
        uint8_t rx_length = size - 1;
        switch (rx->Cmd) {
#pragma region ref模块过来的数据

        case ET_DATA_Motion: {
            skt_update_ref_data(rx->Buffer, rx_length);
        } break;
#pragma endregion

#pragma region 默认
        default: {
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

    if (ret != STD_SUCCESS) {
        if (itask_get_system_status() == SYSTEM_STATUS_UPDATING_IMAGE) {
            itask_set_system_status(SYSTEM_STATUS_NULL);
        }
        if (ble_is_connected()) {
            ble_disconnect();
        }
    }
    return ret;
}

// 新定义: 1、休眠(新版改进，苏醒即休眠)，2、关机，3、休眠，4、复位，5、工作、6、校准，7、测试，8、固件升级
// 除了工作，其他时候尽可能休眠
static volatile uint8_t sSystemStatus = SYSTEM_STATUS_NULL;

uint8_t itask_get_system_status(void)
{
    return sSystemStatus;
}

void itask_set_system_status(uint8_t status)
{
    if (status >= SYSTEM_STATUS_NULL && status < SYSTEM_STATUS_EMAX)
        sSystemStatus = status;
}

// 设备是否正在老化
static uint8_t sDeviceAgeing = 0;

uint8_t itask_get_ageing_status(void)
{
    return sDeviceAgeing;
}

void itask_set_ageing_status(uint8_t status)
{
    sDeviceAgeing = !!(status);
}

void itask_set_speed_test_buff(uint32_t id, uint32_t size)
{
    if ((size > BLE_GATT_CHAR_MAX_LENGTH) || (size == 0)) {
        tkkpf("set speed test: error size = %lu\r\n", size);
        return;
    }
    SpeedTestBuff.ID       = id;
    SpeedTestBuff.DataSize = size;
}

#if 0
static uint8_t sLastSystemStatus = SYSTEM_STATUS_NULL;
/* 更新系统状态 */
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
#endif

/*******************************************************************************
END
*******************************************************************************/
