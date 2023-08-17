/*
 * @Author       : Zhaoyu.Wu
 * @Date         : 2023-07-18 14:57
 * @LastEditTime : 2023-08-11 15:56
 * @LastEditors  : Zhaoyu.Wu
 * @Description  : Private processing function of equipment
 * @FilePath     : d:/eMed/product/osteotomy_simple_1/applications/cmd_handle_proc.c
 * If you have any questions, email to mr.wuzhaoyu@outlook.com.
 */

#include "skt_if.h"
#include "itask.h"
#include "pd_proc.h"
#include "pm_proc.h"
#include "ble_proc.h"
#include "com_struct.h"
#include "large_event.h"
#include "firmware.h"
#include "event_struct.h"
#include "standard_lib.h"
#include <stdint.h>
#include <stddef.h>

#define REPLAY_CMD(c)     ble_reply_cmd((c), (seq))

#define REPLAY_DATA(b, s) ble_reply_buffer((cmd), (seq), (b), (s))

typedef uint8_t (*proc_func)(uint8_t cmd, uint8_t sequence, uint8_t *buffer);

struct cmd_handle_management {
    uint8_t cmd;
    uint8_t (*func)(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len);
};

static uint8_t cmd_handle_get_device_status(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    __DEVICE_STATUS_TypeDef tDeviceInitStatus;
    tDeviceInitStatus.BootTimes     = gFlashTime.BootTimes;
    tDeviceInitStatus.EverBootTimes = gFlashTime.EverBootTimes;
    tDeviceInitStatus.BorTimes      = gFlashTime.BorTimes;
    tDeviceInitStatus.Power         = pwr_get_power_relative();
    REPLAY_DATA((uint8_t *)&tDeviceInitStatus, sizeof(tDeviceInitStatus));

    return STD_SUCCESS;
}

// 有效设备校验码
static __MCU_MARK_TypeDef mcu_mark_data = { {0, 0}, 0 };

static uint8_t cmd_handle_set_device_mark(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    uint8_t ret = STD_FAILED;

    if (len == sizeof(mcu_mark_data)) {
        memcpy(&mcu_mark_data, buff, len);
        uint64_t t = 0;
        memcpy((void *)&t, (const void *)&mcu_mark_data.TimeStamp[0], sizeof(mcu_mark_data.TimeStamp));
        rtc_set_unix_time_ms(t);
        kprint("stamp=%lu\r\n", (long unsigned int)t);

        ret = pd_check_mcu_mark((void *)&mcu_mark_data);
        ble_done_complete_connt();
        REPLAY_CMD(ET_RES_SUCCESS);
        if (ret == STD_SUCCESS) {
            kprint("ok, it is a allowed and checked client\r\n");
        } else {
            kprint("ok, it is a allowed client\r\n");
        }
    } else {
        kprint("it is not a mcu mark\r\n");
        REPLAY_CMD(ET_RES_FAILED);
    }

    return ret;
}

static uint8_t cmd_handle_get_device_mark(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    REPLAY_DATA((uint8_t *)&mcu_mark_data, sizeof(mcu_mark_data));

    return STD_SUCCESS;
}

static uint8_t cmd_handle_set_time_stamp_ms(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    uint8_t ret = STD_FAILED;

    __MCU_TIMEMS_SYNC_TypeDef sync;

    if (len == sizeof(sync)) {
        REPLAY_CMD(ET_RES_SUCCESS);

        ret = STD_SUCCESS;
        memcpy(&sync, buff, len);
        if (sync.Index == 2) {
            uint64_t x = sync.TimeStampMS;
            rtc_set_unix_time_ms(x);
        }
    } else {
        REPLAY_CMD(ET_RES_FAILED);
    }

    return ret;
}

static uint8_t cmd_handle_get_device_time(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    __DEVICE_TIME_TypeDef   dt = { 0 };
    __SYSTEM_TIME32_TypeDef t;

    sys_get_time(&t);
    dt.UpTime   = t.UpTime;
    dt.RunTime  = t.RunTime;
    dt.WorkTime = t.WorkTime;
    REPLAY_DATA((uint8_t *)&dt, sizeof(dt));

    return STD_SUCCESS;
}

static uint8_t cmd_handle_clear_run_time(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    if (!pd_device_is_checked()) {
        REPLAY_CMD(ET_RES_DO_NOTHING);
        return STD_FAILED;
    }

    REPLAY_CMD(ET_RES_SUCCESS);

    pd_clear_time();
    kprint("[task]: RunTime_Clear \r\n");

    return STD_SUCCESS;
}

static uint8_t cmd_handle_get_version(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    uint8_t str_len, str_buff[247], alg_version[64];

    memset(str_buff, 0, sizeof(str_buff));
    memset(alg_version, 0, sizeof(alg_version));

    stk_get_version(alg_version); //TODO: skt here. Get device version
    get_version(str_buff, alg_version);

    kprint("%s\r\n", str_buff);
    str_len = strlen((const char *)str_buff);
    REPLAY_DATA((uint8_t *)&str_buff[0], str_len);

    return STD_SUCCESS;
}

static uint8_t cmd_handle_set_nameplate(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    uint8_t ret = STD_FAILED;

    if (!pd_device_is_checked()) {
        REPLAY_CMD(ET_RES_DO_NOTHING);
        return STD_DO_NOTHING;
    }

    if (len != sizeof(gProductData.Nameplate)) {
        kprint("set nameplate, size(%u) error\r\n", len);
    } else {
        memcpy(&gProductData.Nameplate, buff, len);
        ret = pd_save_product_info();
    }

    if (STD_SUCCESS != ret)
        REPLAY_CMD(ET_RES_FAILED);
    else
        REPLAY_CMD(ET_RES_SUCCESS);

    return ret;
}

static uint8_t cmd_handle_get_nameplate(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    REPLAY_DATA((uint8_t *)&gProductData.Nameplate, sizeof(gProductData.Nameplate));

    return STD_SUCCESS;
}

static uint8_t cmd_handle_get_reg_info(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    REPLAY_DATA((uint8_t *)&gAccountInfo, sizeof(gAccountInfo));

    return STD_SUCCESS;
}

static uint8_t cmd_handle_register(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    uint8_t ret = STD_FAILED;

    ret = pd_save_register_info(1);

    if (STD_SUCCESS != ret)
        REPLAY_CMD(ET_RES_FAILED);
    else
        REPLAY_CMD(ET_RES_SUCCESS);

    return ret;
}

static uint8_t cmd_handle_unregister(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    uint8_t ret = STD_FAILED;

    ret = pd_save_register_info(0);

    if (STD_SUCCESS != ret)
        REPLAY_CMD(ET_RES_FAILED);
    else
        REPLAY_CMD(ET_RES_SUCCESS);

    return ret;
}

static uint8_t cmd_handle_clear_register(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    uint8_t ret = STD_FAILED;

    if (!pd_device_is_checked()) {
        REPLAY_CMD(ET_RES_DO_NOTHING);
        return STD_DO_NOTHING;
    }

    ret = pd_save_register_info(2);

    if (STD_SUCCESS != ret)
        REPLAY_CMD(ET_RES_FAILED);
    else
        REPLAY_CMD(ET_RES_SUCCESS);

    return ret;
}

static uint8_t cmd_handle_device_reset(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    REPLAY_CMD(ET_RES_SUCCESS);

    pwr_reset_mcu();

    return STD_SUCCESS;
}

static uint8_t cmd_handle_device_poweroff(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    if (gAccountInfo.IsLogined == 0) {
        REPLAY_CMD(ET_RES_SUCCESS);
        kprint("power off\r\n");
        pwr_shutdown_mcu();
        return STD_SUCCESS;
    } else {
        REPLAY_CMD(ET_RES_FAILED);
        kprint("deny, power off\r\n");
    }

    return STD_FAILED;
}

static uint8_t cmd_handle_set_device_mac(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    uint8_t  ret = STD_FAILED;
    uint16_t DeviceType;

    if (!pd_device_is_checked()) {
        REPLAY_CMD(ET_RES_DO_NOTHING);
        return STD_DO_NOTHING;
    }

    do {
        if (len != sizeof(gFlashBleInfo.Mac)) {
            break;
        }

        DeviceType = (uint16_t)(((uint16_t)buff[7] << 8) & 0xff00) + (buff[6] & 0xff);
        if (DeviceType != DEVICE_TPYE_DEFAULT_CODE) {
            kprint("devcie type error, 0x%x - 0x%x\r\n", DEVICE_TPYE_DEFAULT_CODE, DeviceType);
            break;
        }

        memcpy((void *)&gFlashBleInfo.Mac[0], buff, len);
        ret = pd_save_ble_mac();
    } while (0);

    if (STD_SUCCESS != ret)
        REPLAY_CMD(ET_RES_FAILED);
    else
        REPLAY_CMD(ET_RES_SUCCESS);

    return ret;
}

static uint8_t cmd_handle_update_ble_mac(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    uint8_t ret = STD_FAILED;

    if (!pd_device_is_checked()) {
        REPLAY_CMD(ET_RES_DO_NOTHING);
        return STD_DO_NOTHING;
    }

    ret = pd_generate_and_save_ble_mac();

    if (STD_SUCCESS != ret)
        REPLAY_CMD(ET_RES_FAILED);
    else
        REPLAY_CMD(ET_RES_SUCCESS);

    return ret;
}

static uint8_t cmd_handle_get_error(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    sMcuCheck.Check = (uint32_t)skt_is_check(); // TODO: skt here. Device check
    REPLAY_DATA((uint8_t *)&sMcuCheck, sizeof(sMcuCheck));

    return STD_SUCCESS;
}

static uint8_t cmd_handle_get_device_rssi(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    int16_t DeviceRssi = 0;
    // ble_get_rssi(&DeviceRssi);
    REPLAY_DATA((uint8_t *)&DeviceRssi, sizeof(DeviceRssi));

    return STD_SUCCESS;
}

static uint8_t cmd_handle_get_ble_power_level(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    int8_t pl = 2;
    // pl = ble_get_power_level();
    REPLAY_DATA((uint8_t *)&pl, sizeof(pl));

    return STD_SUCCESS;
}

static uint8_t cmd_handle_get_flash_data(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    uint8_t                   ret = STD_FAILED;
    __FLASH_DATA_READ_TypeDef fdr = { 0, 0 };

    if (!pd_device_is_checked()) {
        REPLAY_CMD(ET_RES_DO_NOTHING);
        return STD_DO_NOTHING;
    }

    do {
        if (len != sizeof(fdr)) {
            kprint("flash data, rx size(%u) error\r\n", len);
            break;
        }

        memcpy(&fdr, buff, len);
        if ((fdr.Size > 224) || (fdr.Size == 0) || (fdr.Size & 0x3)) {
            kprint("flash data, data size(%u) error\r\n", (unsigned int)fdr.Size);
            break;
        }

        if ((fdr.Addr + fdr.Size) >= pd_get_user_end_space()) {
            kprint("error, flash addr(0x%x) overflow\r\n", (unsigned int)fdr.Addr);
            break;
        }

        ret = STD_SUCCESS;
    } while (0);

    if (STD_SUCCESS != ret)
        REPLAY_CMD(ET_RES_FAILED);
    else
        REPLAY_DATA((uint8_t *)fdr.Addr, fdr.Size);

    return ret;
}

static __LARGE_EVENT_Typedef sLargeEvent = LARGE_EVNT_DEFAULT;

static uint8_t cmd_handle_set_alg_setting(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    uint8_t ret = STD_FAILED;

    if (!pd_device_is_checked()) {
        REPLAY_CMD(ET_RES_DO_NOTHING);
        return STD_DO_NOTHING;
    }

    ret = large_event_is_head(&sLargeEvent, buff, len);
    if (ret == STD_SUCCESS) {
        sLargeEvent.DataP    = &gAlgoPara.Value[0];
        sLargeEvent.DataSize = sizeof(gAlgoPara.Value);
        kprint("init\r\n");
    }

    do {
        ret = large_event_set(&sLargeEvent, buff, len);
        if (ret != STD_SUCCESS) {
            break;
        }

        ret = skt_check_keywork((uint32_t)&gAlgoPara.Value[0]); //TODO: skt here
        if (ret != STD_SUCCESS) {
            break;
        }

        kprint("save alg para\r\n");
        pd_save_algo_para();
    } while (0);

    if (STD_SUCCESS != ret)
        REPLAY_CMD(ET_RES_FAILED);
    else
        REPLAY_CMD(ET_RES_SUCCESS);

    return ret;
}

static uint8_t cmd_handle_get_alg_setting(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    uint8_t ret = STD_FAILED, alg_buffer[255], alg_size = 0;

    ret = large_event_is_head(&sLargeEvent, buff, len);
    if (ret == STD_SUCCESS) {
        sLargeEvent.DataP    = &gAlgoPara.Value[0];
        sLargeEvent.DataSize = sizeof(gAlgoPara.Value);
        kprint("init\r\n");
    }

    ret = large_event_get(&sLargeEvent, buff, len, (uint8_t *)alg_buffer, &alg_size);

    if (STD_SUCCESS != ret)
        REPLAY_CMD(ET_RES_FAILED);
    else
        REPLAY_DATA((uint8_t *)&alg_buffer[0], alg_size);

    return ret;
}

static uint8_t cmd_handle_set_skt_config(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    uint8_t ret = STD_FAILED;

    __FLASH_SKT_CONFIG_TypeDef config;

    do {
        if (len != sizeof(config)) {
            kprint("skt config error, %u\r\n", len);
            break;
        }

        memcpy(&config, buff, len);
        ret = skt_set_config(config); //TODO: skt here
        if (ret != STD_SUCCESS) {
            break;
        }

        if (0 == memcmp(&gSKTConfig, &config, sizeof(gSKTConfig))) {
            break;
        }

        memcpy(&gSKTConfig, &config, sizeof(gSKTConfig));
        ret = pd_save_skt_config();
        kprint("save skt config: %d\r\n", ret);
    } while (0);

    if (STD_SUCCESS != ret)
        REPLAY_CMD(ET_RES_FAILED);
    else
        REPLAY_CMD(ET_RES_SUCCESS);

    return ret;
}

static uint8_t cmd_handle_set_skt_mode(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    uint8_t ret = STD_FAILED;

    __SKT_MODE_TypeDef mode;

    do {
        if (sizeof(mode) != len) {
            kprint("skt mode error, %u\r\n", len);
            break;
        }
        memcpy(&mode, buff, len);
        ret = skt_set_mode(mode); //TODO: skt here
    } while (0);

    if (STD_SUCCESS != ret)
        REPLAY_CMD(ET_RES_FAILED);
    else
        REPLAY_CMD(ET_RES_SUCCESS);

    return ret;
}

static uint8_t cmd_handle_set_skt_reg_pos(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    uint8_t ret = STD_FAILED;
    int32_t pos;

    do {
        if (sizeof(pos) != len) {
            kprint("skt pos error, %u\r\n", len);
            break;
        }

        memcpy(&pos, buff, len);
        ret = skt_set_tibia_reg_pos(pos); //TODO: skt here
    } while (0);

    if (STD_SUCCESS != ret)
        REPLAY_CMD(ET_RES_FAILED);
    else
        REPLAY_CMD(ET_RES_SUCCESS);

    return ret;
}

static uint8_t cmd_handle_set_skt_navigation_angle(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    uint8_t ret = STD_FAILED;

    __SKT_NAVIGATION_ANGLE_TypeDef nav;

    do {
        if (sizeof(nav) != len) {
            kprint("skt nav angle error, %u\r\n", len);
            break;
        }

        memcpy(&nav, buff, len);
        ret = skt_set_navigation_angle(nav); //TODO: skt here
    } while (0);

    if (STD_SUCCESS != ret)
        REPLAY_CMD(ET_RES_FAILED);
    else
        REPLAY_CMD(ET_RES_SUCCESS);

    return ret;
}

static uint8_t cmd_handle_cmd_start(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    uint8_t ret = STD_FAILED, status;

    status = itask_get_system_status();

    if (status == SYSTEM_STATUS_NULL) {
        ret = skt_start(); //TODO: skt here
    } else if (status == SYSTEM_STATUS_WORKING) {
        kprint("already started\r\n");
        ret = STD_SUCCESS;
    } else {
        kprint("deny to start\r\n");
    }

    if (STD_SUCCESS != ret) {
        REPLAY_CMD(ET_RES_FAILED);
        kprint("failed to start: 0x%x\r\n", status);
    } else {
        REPLAY_CMD(ET_RES_SUCCESS);
        kprint("ok to start\r\n");
        itask_set_system_status(SYSTEM_STATUS_WORKING);
    }

    return ret;
}

static uint8_t cmd_handle_cmd_stop(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    uint8_t ret = STD_FAILED, status;

    itask_set_ageing_status(0);

    status = itask_get_system_status();
    if (status == SYSTEM_STATUS_WORKING) {
        skt_stop(); //TODO: skt here
        itask_set_system_status(SYSTEM_STATUS_NULL);
        ret = STD_SUCCESS;

        if (ble_is_connected()) {
            led_ctrl(LED_MODE_G_CTRL, 1);
        } else {
            led_ctrl(LED_MODE_G_CTRL, 0);
        }

        kprint("stop\r\n");
    } else if (status == SYSTEM_STATUS_NULL) {
        ret = STD_SUCCESS;
        kprint("stopped\r\n");
    } else {
        kprint("deny to stop - 0x%x\r\n", status);
    }

    if (STD_SUCCESS != ret) {
        REPLAY_CMD(ET_RES_FAILED);
    } else {
        REPLAY_CMD(ET_RES_SUCCESS);
    }

    return ret;
}

static uint8_t cmd_handle_cmd_device_ageing(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    kprint("enable ageing, but nothing to do\r\n"); // TODO: device_ageing
    itask_set_ageing_status(1);

    REPLAY_CMD(ET_RES_SUCCESS);
    return STD_SUCCESS;
}

static uint8_t cmd_handle_test_speed_start_ble_upload(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    uint8_t  ret       = STD_FAILED;
    uint32_t data_size = 0;

    if (!pd_device_is_checked()) {
        REPLAY_CMD(ET_RES_DO_NOTHING);
        return STD_DO_NOTHING;
    }

    do {
        if (len != sizeof(data_size)) {
            kprint("speed test: error\r\n");
            break;
        }
        memcpy(&data_size, buff, sizeof(data_size));
        if ((data_size > BLE_GATT_CHAR_MAX_LENGTH) || (data_size == 0)) {
            kprint("speed test: error size = %lu\r\n", data_size);
            break;
        }

        if (itask_get_system_status() == SYSTEM_STATUS_TESTING) {
            kprint("already in speed test\r\n");
            ret = STD_SUCCESS;
            break;
        }

        kprint("Start Speed test, size=%lu\r\n", data_size);

        itask_set_system_status(SYSTEM_STATUS_TESTING);
        itask_set_speed_test_buff(1, data_size);
        UTIL_SEQ_SetTask(1 << CFG_TASK_TEST_BLE_SPEED_ID, CFG_PRIO_NBR_4);

        ret = STD_SUCCESS;
    } while (0);

    if (STD_SUCCESS != ret)
        REPLAY_CMD(ET_RES_FAILED);
    else
        REPLAY_CMD(ET_RES_SUCCESS);

    return ret;
}

static uint8_t cmd_handle_test_speed_stop_ble_upload(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    uint8_t ret = STD_FAILED;

    if (!pd_device_is_checked()) {
        REPLAY_CMD(ET_RES_DO_NOTHING);
        return STD_DO_NOTHING;
    }

    if (itask_get_system_status() == SYSTEM_STATUS_TESTING) {
        itask_set_system_status(SYSTEM_STATUS_NULL);
        kprint("Stop Speed test\r\n");
        ret = STD_SUCCESS;
    }

    if (STD_SUCCESS != ret)
        REPLAY_CMD(ET_RES_FAILED);
    else
        REPLAY_CMD(ET_RES_SUCCESS);

    return ret;
}

static uint8_t cmd_handle_cmd_firmware_start(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    uint8_t ret = STD_FAILED;

    if (!pd_device_is_checked()) {
        REPLAY_CMD(ET_RES_DO_NOTHING);
        return STD_DO_NOTHING;
    }

    if (itask_get_system_status() == SYSTEM_STATUS_NULL) {
        if (fw_start(buff, (uint32_t)len) == STD_SUCCESS) {
            itask_set_system_status(SYSTEM_STATUS_UPDATING_IMAGE);
            REPLAY_CMD(ET_RES_SUCCESS);
            ret = STD_SUCCESS;
        } else {
            REPLAY_CMD(ET_RES_FAILED);
            ret = STD_FAILED;
        }
    } else {
        REPLAY_CMD(ET_RES_DENIED);
        ret = STD_DENIED;
    }

    return ret;
}

static uint8_t cmd_handle_cmd_firmware_stop(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    uint8_t ret = STD_FAILED;

    if (!pd_device_is_checked()) {
        REPLAY_CMD(ET_RES_DO_NOTHING);
        return STD_DO_NOTHING;
    }

    if (itask_get_system_status() == SYSTEM_STATUS_UPDATING_IMAGE) {
        if (fw_stop(buff, (uint32_t)len) == STD_SUCCESS) {
            itask_set_system_status(SYSTEM_STATUS_NULL);
            REPLAY_CMD(ET_RES_SUCCESS);
            ret = STD_SUCCESS;
        } else {
            REPLAY_CMD(ET_RES_FAILED);
            ret = STD_FAILED;
        }
    } else {
        REPLAY_CMD(ET_RES_DENIED);
        ret = STD_DENIED;
    }

    return ret;
}

static uint8_t cmd_handle_cmd_firmware_is_ready(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    uint8_t ret = STD_FAILED;

    if (!pd_device_is_checked()) {
        REPLAY_CMD(ET_RES_DO_NOTHING);
        return STD_DO_NOTHING;
    }

    ret = fw_is_ready(buff, (uint32_t)len);

    if (STD_SUCCESS != ret)
        REPLAY_CMD(ET_RES_FAILED);
    else
        REPLAY_CMD(ET_RES_SUCCESS);

    return ret;
}

static uint8_t cmd_handle_set_firmware_config(uint8_t cmd, uint8_t seq, uint8_t *buff, uint8_t len)
{
    uint8_t ret = STD_FAILED;

    if (!pd_device_is_checked()) {
        REPLAY_CMD(ET_RES_DO_NOTHING);
        return STD_DO_NOTHING;
    }

    ret = fw_set_config(buff, (uint32_t)len);

    if (STD_SUCCESS != ret)
        REPLAY_CMD(ET_RES_FAILED);
    else
        REPLAY_CMD(ET_RES_SUCCESS);

    return ret;
}

static struct cmd_handle_management priv_proc_funcs[] = {
    {ET_GET_Device_Status,            cmd_handle_get_device_status          },
    { ET_SET_Device_Mark,             cmd_handle_set_device_mark            },
    { ET_GET_Device_Mark,             cmd_handle_get_device_mark            },
    { ET_SET_TimeStamp_MS,            cmd_handle_set_time_stamp_ms          },
    { ET_GET_Device_Time,             cmd_handle_get_device_time            },
    { ET_CMD_RunTime_Clear,           cmd_handle_clear_run_time             },
    { ET_GET_Version,                 cmd_handle_get_version                },
    { ET_SET_Nameplate,               cmd_handle_set_nameplate              },
    { ET_GET_Nameplate,               cmd_handle_get_nameplate              },
    { ET_GET_Reg_Info,                cmd_handle_get_reg_info               },
    { ET_CMD_Register,                cmd_handle_register                   },
    { ET_CMD_Unregister,              cmd_handle_unregister                 },
    { ET_CMD_Clear_Reg,               cmd_handle_clear_register             },
    { ET_CMD_Device_Reset,            cmd_handle_device_reset               },
    { ET_CMD_Device_PowerOff,         cmd_handle_device_poweroff            },
    { ET_SET_Device_Mac,              cmd_handle_set_device_mac             },
    { ET_CMD_Update_Ble_Mac,          cmd_handle_update_ble_mac             },
    { ET_GET_Error,                   cmd_handle_get_error                  },
    { ET_GET_Device_RSSI,             cmd_handle_get_device_rssi            },
    { ET_GET_Ble_Power_Level,         cmd_handle_get_ble_power_level        },
    { ET_GET_Flash_Data,              cmd_handle_get_flash_data             },
    { ET_SET_Alg_Setting,             cmd_handle_set_alg_setting            },
    { ET_GET_Alg_Setting,             cmd_handle_get_alg_setting            },
    { ET_SET_SKT_Config,              cmd_handle_set_skt_config             },
    { ET_SET_SKT_Mode,                cmd_handle_set_skt_mode               },
    { ET_SET_SKT_Reg_Pos,             cmd_handle_set_skt_reg_pos            },
    { ET_SET_SKT_Navigation_Angle,    cmd_handle_set_skt_navigation_angle   },
    { ET_CMD_Start,                   cmd_handle_cmd_start                  },
    { ET_CMD_Stop,                    cmd_handle_cmd_stop                   },
    { ET_CMD_Device_Ageing,           cmd_handle_cmd_device_ageing          },
    { ET_TEST_Speed_Start_BLE_UpLoad, cmd_handle_test_speed_start_ble_upload},
    { ET_TEST_Speed_Stop_BLE_UpLoad,  cmd_handle_test_speed_stop_ble_upload },
    { ET_CMD_Firmware_Start,          cmd_handle_cmd_firmware_start         },
    { ET_CMD_Firmware_Stop,           cmd_handle_cmd_firmware_stop          },
    { ET_CMD_Firmware_IsReady,        cmd_handle_cmd_firmware_is_ready      },
    { ET_SET_Firmware_Config,         cmd_handle_set_firmware_config        },
};

uint8_t cmd_handle_proc(uint8_t command, uint8_t sequence, uint8_t *buffer, uint8_t buff_len)
{
    for (uint32_t i = 0; i < (sizeof(priv_proc_funcs) / sizeof(priv_proc_funcs[0])); i++) {
        if ((command == priv_proc_funcs[i].cmd) && (priv_proc_funcs[i].func != NULL)) {
            return (priv_proc_funcs[i].func(command, sequence, buffer, buff_len));
        }
    }

    ble_reply_cmd(ET_RES_DENIED, sequence);
    kprint("do not tell cmd - 0x%x\r\n", command);
    return STD_DENIED;
}
