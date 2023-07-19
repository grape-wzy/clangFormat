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
    return STD_ERROR;
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
#if 0
    switch (rx->Cmd) {
#pragma region 初始化指令
    case ET_GET_Device_Status: {
        __DEVICE_STATUS_TypeDef tDeviceInitStatus;
        tDeviceInitStatus.BootTimes     = gFlashTime.BootTimes;
        tDeviceInitStatus.EverBootTimes = gFlashTime.EverBootTimes;
        tDeviceInitStatus.BorTimes      = gFlashTime.BorTimes;
        tDeviceInitStatus.Power         = pwr_get_power_relative();
        ble_reply_data((uint8_t *)&tDeviceInitStatus, sizeof(tDeviceInitStatus));
    } break;
    case ET_SET_Device_Mark: {
        if (rx_length == sizeof(sMcuMark)) {
            memcpy(&sMcuMark, rx->Buffer, rx_length);
            uint64_t t = 0;
            memcpy((void *)&t, (const void *)&sMcuMark.TimeStamp[0], sizeof(sMcuMark.TimeStamp));
            rtc_set_unix_time_ms(t);
            kprint("stamp=%lu\r\n", (long unsigned int)t);

            uint8_t ret = pd_check_mcu_mark((void *)&sMcuMark);
            ble_done_complete_connt();
            ble_reply_command(STD_SUCCESS);
            if (ret == STD_SUCCESS) {
                tkkpf("ok, it is a allowed and checked client\r\n");
            } else {
                tkkpf("ok, it is a allowed client\r\n");
            }
        } else {
            kprint("it is not a mcu mark\r\n");
            ble_reply_command(STD_FAILED);
        }
    } break;
    case ET_GET_Device_Mark: {
        ble_reply_data((uint8_t *)&sMcuMark, sizeof(sMcuMark));
    } break;
#pragma endregion

#pragma region 时间类指令
    case ET_SET_TimeStamp_MS: {
        __MCU_TIMEMS_SYNC_TypeDef sync;
        if (rx_length == sizeof(sync)) {
            ble_reply_command(STD_SUCCESS);
            memcpy(&sync, rx->Buffer, rx_length);
            if (sync.Index == 2) {
                uint64_t x = sync.TimeStampMS;
                rtc_set_unix_time_ms(x);
            }
        }
    } break;
    case ET_GET_Device_Time: {
        __DEVICE_TIME_TypeDef   dt = { 0 };
        __SYSTEM_TIME32_TypeDef t;
        sys_get_time(&t);
        dt.UpTime   = t.UpTime;
        dt.RunTime  = t.RunTime;
        dt.WorkTime = t.WorkTime;
        ble_reply_data((uint8_t *)&dt, sizeof(dt));
    } break;
    case ET_CMD_RunTime_Clear: {
        if (!pd_device_is_checked()) {
            ble_reply_command(STD_DO_NOTHING);
            break;
        }
        pd_clear_time();
        ble_reply_command(STD_SUCCESS);
        tkkpf("[task]: RunTime_Clear \r\n");
    } break;
#pragma endregion

#pragma region 版本查询
    case ET_GET_Version: {
        uint8_t buff[247]       = { 0 };
        uint8_t alg_version[64] = { 0 };
        stk_get_version(alg_version);
        get_version(buff, alg_version);
        tkkpf("%s\r\n", buff);
        uint8_t len = strlen((const char *)buff);
        ble_reply_data((uint8_t *)&buff[0], len);
    } break;
#pragma endregion

#pragma region 名牌设置与查询
    case ET_SET_Nameplate: {
        if (!pd_device_is_checked()) {
            ble_reply_command(STD_DO_NOTHING);
            break;
        }
        if (sizeof(gProductData.Nameplate) != rx_length) {
            kprint("set nameplate, size(%u) error\r\n", rx_length);
            ble_reply_command(STD_FAILED);
        } else {
            memcpy(&gProductData.Nameplate, rx->Buffer, rx_length);
            uint8_t ret = pd_save_product_info();
            ble_reply_command(ret);
        }
    } break;
    case ET_GET_Nameplate: {
        ble_reply_data((uint8_t *)&gProductData.Nameplate, sizeof(gProductData.Nameplate));
    } break;
#pragma endregion

#pragma region 注册信息
    case ET_GET_Reg_Info: {
        ble_reply_data((uint8_t *)&gAccountInfo, sizeof(gAccountInfo));
    } break;
    case ET_CMD_Register: {
        uint8_t regcmd = 1;
        if (pd_save_register_info(regcmd) != STD_SUCCESS) {
            ble_reply_command(STD_FAILED);
        } else {
            ble_reply_command(STD_SUCCESS);
        }
    } break;
    case ET_CMD_Unregister: {
        uint8_t regcmd = 0;
        if (pd_save_register_info(regcmd) != STD_SUCCESS) {
            ble_reply_command(STD_FAILED);
        } else {
            ble_reply_command(STD_SUCCESS);
        }
    } break;
    case ET_CMD_Clear_Reg: {
        if (!pd_device_is_checked()) {
            ble_reply_command(STD_DO_NOTHING);
            break;
        }
        uint8_t regcmd = 2;
        if (pd_save_register_info(regcmd) != STD_SUCCESS) {
            ble_reply_command(STD_FAILED);
        } else {
            ble_reply_command(STD_SUCCESS);
        }
    } break;
#pragma endregion

#pragma region MCU控制指令

    case ET_CMD_Device_Reset: {
        tkkpf("reset\r\n");

        ble_reply_command(STD_SUCCESS);
        pwr_reset_mcu();
    } break;
    case ET_CMD_Device_PowerOff: {
        if (gAccountInfo.IsLogined == 0) {
            tkkpf("power off\r\n");

            ble_reply_command(STD_SUCCESS);
            pwr_shutdown_mcu();
        } else {
            ble_reply_command(STD_FAILED);
            tkkpf("deny, power off\r\n");
        }
    } break;

#pragma endregion

#pragma region 蓝牙配置相关指令
    case ET_SET_Device_Mac: {
        if (!pd_device_is_checked()) {
            ble_reply_command(STD_DO_NOTHING);
            break;
        }

        if (rx_length != sizeof(gFlashBleInfo.Mac)) {
            ble_reply_command(STD_FAILED);
            break;
        }
        uint8_t *pbuff      = rx->Buffer;
        uint16_t DeviceType = (uint16_t)(((uint16_t)pbuff[7] << 8) & 0xff00) + (pbuff[6] & 0xff);
        if (DeviceType != DEVICE_TPYE_DEFAULT_CODE) {
            kprint("devcie type error, 0x8020-0x%x\r\n", DeviceType);
            ble_reply_command(STD_FAILED);
            break;
        }
        memcpy((void *)&gFlashBleInfo.Mac[0], rx->Buffer, rx_length);

        uint8_t ret = pd_save_ble_mac();

        ble_reply_command(ret);
    } break;
    case ET_CMD_Update_Ble_Mac: {
        if (!pd_device_is_checked()) {
            ble_reply_command(STD_DO_NOTHING);
            break;
        }
        uint8_t ret = pd_generate_and_save_ble_mac();

        ble_reply_command(ret);
    } break;
#pragma endregion

#pragma region 查询基本信息
    case ET_GET_Error: {
        sMcuCheck.Check = (uint32_t)skt_is_check();
        ble_reply_data((uint8_t *)&sMcuCheck, sizeof(sMcuCheck));
    } break;
    case ET_GET_Device_RSSI: // 信号强度
    {
        int16_t DeviceRssi = 0;
        //				ble_get_rssi(&DeviceRssi);
        ble_reply_data((uint8_t *)&DeviceRssi, sizeof(DeviceRssi));
    } break;
    case ET_GET_Ble_Power_Level: {
        //			int8_t pl = ble_get_power_level();
        int8_t pl = 2;
        ble_reply_data((uint8_t *)&pl, sizeof(pl));
    } break;
#pragma endregion

#pragma region 获取flash数据
    case ET_GET_Flash_Data: {
        if (!pd_device_is_checked()) {
            ble_reply_command(STD_DO_NOTHING);
            break;
        }
        __FLASH_DATA_READ_TypeDef fdr = { 0 };

        if (sizeof(fdr) != rx_length) {
            kprint("flash data, rx size(%u) error\r\n", rx_length);
            ble_reply_command(STD_FAILED);
            break;
        }
        memcpy(&fdr, rx->Buffer, rx_length);
        if ((fdr.Size > 224) || (fdr.Size == 0) || (fdr.Size & 0x3)) {
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
        if ((fdr.Addr + fdr.Size) >= pd_get_user_end_space()) {
            kprint("error, flash addr(0x%x) overflow\r\n", (unsigned int)fdr.Addr);
            ble_reply_command(STD_FAILED);
            break;
        }
        ble_reply_data((uint8_t *)fdr.Addr, fdr.Size);
    } break;
#pragma endregion

#pragma region 算法参数操作
    case ET_SET_Alg_Setting: {
        if (!pd_device_is_checked()) {
            ble_reply_command(STD_DO_NOTHING);
            break;
        }
        if (itask_set_alg_para(rx->Buffer, rx_length) == STD_SUCCESS) {
            ble_reply_command(STD_SUCCESS);
        } else {
            ble_reply_command(STD_FAILED);
        }
    } break;
    case ET_GET_Alg_Setting: {
        uint8_t buffer[255];
        uint8_t buffersize = 0;

        if (itask_get_alg_para(rx->Buffer, rx_length, buffer, &buffersize) == STD_SUCCESS) {
            ble_reply_data((uint8_t *)&buffer[0], buffersize);
        } else {
            ble_reply_command(STD_FAILED);
        }
    } break;
#pragma endregion

#pragma region SKT算法配置指令
    case ET_SET_SKT_Config: {
        __FLASH_SKT_CONFIG_TypeDef config;
        if (sizeof(config) != rx_length) {
            kprint("skt config error, %u\r\n", rx_length);
            ble_reply_command(STD_FAILED);
            break;
        }
        memcpy(&config, rx->Buffer, rx_length);
        uint8_t ret = skt_set_config(config);

        if (ret == STD_SUCCESS) {
            int32_t x = memcmp(&gSKTConfig, &config, sizeof(gSKTConfig));
            if (x != 0) {
                memcpy(&gSKTConfig, &config, sizeof(gSKTConfig));
                ret = pd_save_skt_config();
                kprint("save skt config\r\n");
            }
        }

        ble_reply_command(ret);
    } break;
    case ET_SET_SKT_Mode: {
        __SKT_MODE_TypeDef mode;
        if (sizeof(mode) != rx_length) {
            kprint("skt mode error, %u\r\n", rx_length);
            ble_reply_command(STD_FAILED);
            break;
        }
        memcpy(&mode, rx->Buffer, rx_length);
        uint8_t ret = skt_set_mode(mode);
        ble_reply_command(ret);
    } break;
    case ET_SET_SKT_Reg_Pos: {
        int32_t pos;
        if (sizeof(pos) != rx_length) {
            kprint("skt pos error, %u\r\n", rx_length);
            ble_reply_command(STD_FAILED);
            break;
        }
        memcpy(&pos, rx->Buffer, rx_length);
        uint8_t ret = skt_set_tibia_reg_pos(pos);
        ble_reply_command(ret);
    } break;
    case ET_SET_SKT_Navigation_Angle: {
        __SKT_NAVIGATION_ANGLE_TypeDef nav;
        if (sizeof(nav) != rx_length) {
            kprint("skt nav angle error, %u\r\n", rx_length);
            ble_reply_command(STD_FAILED);
            break;
        }
        memcpy(&nav, rx->Buffer, rx_length);
        uint8_t ret = skt_set_navigation_angle(nav);
        ble_reply_command(ret);
    } break;
#pragma endregion

#pragma region 工作指令

    case ET_CMD_Start: {
        if (sSystemStatus == SYSTEM_STATUS_NULL) {
            uint8_t ret = skt_start();
            ble_reply_command(ret);

            if (ret == STD_SUCCESS) {
                tkkpf("ok to start\r\n");
                sSystemStatus = SYSTEM_STATUS_WORKING;
            } else {
                tkkpf("failed to start\r\n");
            }
        } else if (sSystemStatus == SYSTEM_STATUS_WORKING) {
            tkkpf("started\r\n");
            ble_reply_command(STD_SUCCESS);
        } else {
            tkkpf("deny to start\r\n");
            ble_reply_command(STD_FAILED);
        }
    } break;
    case ET_CMD_Stop: {
        sDeviceAgeing = false;
        if (sSystemStatus == SYSTEM_STATUS_WORKING) {
            skt_stop();
            tkkpf("stop\r\n");
            sSystemStatus = SYSTEM_STATUS_NULL;
            ble_reply_command(STD_SUCCESS);

            if (ble_is_connected()) {
                led_ctrl(LED_MODE_G_CTRL, 1);
            } else {
                led_ctrl(LED_MODE_G_CTRL, 0);
            }
        } else if (sSystemStatus == SYSTEM_STATUS_NULL) {
            tkkpf("stopped\r\n");
            ble_reply_command(STD_SUCCESS);
        } else {
            tkkpf("deny to stop - 0x%x\r\n", sSystemStatus);
            ble_reply_command(STD_FAILED);
        }
    } break;
    case ET_CMD_Device_Ageing: {
        tkkpf("enable ageing\r\n");
        sDeviceAgeing = true;
        ble_reply_command(STD_SUCCESS);
    } break;
#pragma endregion

#pragma region 测试类指令
    ///////////////////////////////////test//////////////////////////////////
    case ET_TEST_Speed_Start_BLE_UpLoad: {
        if (!pd_device_is_checked()) {
            ble_reply_command(STD_DO_NOTHING);
            break;
        }
        if ((sSystemStatus != SYSTEM_STATUS_NULL) || (rx_length != sizeof(SpeedTestBuff.DataSize))) {
            tkkpf("speed test: error\r\n");
            ble_reply_command(STD_FAILED);
            break;
        }
        memcpy(&SpeedTestBuff.DataSize, rx->Buffer, rx_length);
        if ((SpeedTestBuff.DataSize > BLE_GATT_CHAR_MAX_LENGTH) || (SpeedTestBuff.DataSize == 0)) {
            tkkpf("speed test: error size = %u\r\n", (unsigned int)SpeedTestBuff.DataSize);
            ble_reply_command(STD_FAILED);
            break;
        }
        sSystemStatus    = SYSTEM_STATUS_TESTING;
        SpeedTestBuff.ID = 1;
        tkkpf("Start Speed test, size=%u\r\n", (unsigned int)SpeedTestBuff.DataSize);
        ble_reply_command(STD_SUCCESS);
        UTIL_SEQ_SetTask(1 << CFG_TASK_TEST_BLE_SPEED_ID, CFG_PRIO_NBR_4);
    } break;
    case ET_TEST_Speed_Stop_BLE_UpLoad: {
        if (!pd_device_is_checked()) {
            ble_reply_command(STD_DO_NOTHING);
            break;
        }
        if (sSystemStatus == SYSTEM_STATUS_TESTING) {
            sSystemStatus = SYSTEM_STATUS_NULL;
            tkkpf("Stop Speed test\r\n");
            ble_reply_command(STD_SUCCESS);
        } else {
            ble_reply_command(STD_FAILED);
        }
    } break;

#pragma endregion

#pragma region 固件升级类
    case ET_CMD_Firmware_Start: {
        if (!pd_device_is_checked()) {
            ble_reply_command(STD_DO_NOTHING);
            break;
        }
        if (sSystemStatus == SYSTEM_STATUS_NULL) {
            if (fw_start(rx->Buffer, (uint32_t)rx_length) == STD_SUCCESS) {
                sSystemStatus = SYSTEM_STATUS_UPDATING_IMAGE;
                ble_reply_command(STD_SUCCESS);
            } else {
                ble_reply_command(STD_FAILED);
            }
        } else {
            ble_reply_command(STD_DENIED);
        }
    } break;
    case ET_CMD_Firmware_Stop: {
        if (!pd_device_is_checked()) {
            ble_reply_command(STD_DO_NOTHING);
            break;
        }
        // if (DownLoad_Stop(sEvtRxFrame.buff) ==true)
        if (sSystemStatus == SYSTEM_STATUS_UPDATING_IMAGE) {
            if (fw_stop(rx->Buffer, (uint32_t)rx_length) == STD_SUCCESS) {
                ble_reply_command(STD_SUCCESS);
                sSystemStatus = SYSTEM_STATUS_NULL;
            } else {
                ble_reply_command(STD_FAILED);
            }
        } else {
            ble_reply_command(STD_DENIED);
        }
    } break;
    case ET_CMD_Firmware_IsReady: {
        if (!pd_device_is_checked()) {
            ble_reply_command(STD_DO_NOTHING);
            break;
        }

        uint8_t ret = fw_is_ready(rx->Buffer, (uint32_t)rx_length);
        ble_reply_command(ret);
    } break;
    case ET_SET_Firmware_Config: {
        if (!pd_device_is_checked()) {
            ble_reply_command(STD_DO_NOTHING);
            break;
        }
        uint8_t ret = fw_set_config(rx->Buffer, (uint32_t)rx_length);
        ble_reply_command(ret);
    } break;
#pragma endregion

#pragma region 默认
    default: {
        tkkpf("donot tell cmd - 0x%x\r\n", rx->Cmd);
        ble_reply_command(STD_DENIED);
    }
        return STD_DENIED;
#pragma endregion
    }

    return STD_SUCCESS;
#endif
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
