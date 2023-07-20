/*******************************************************************************
 * file     saber_if.c
 * author   mackgim
 * version  V1.0.0
 * date
 * brief ：
 *******************************************************************************/

/* Includes ------------------------------------------------------------------*/

#include "ahrs.h"
#include "standard_lib.h"
#include "skt_if.h"

uint8_t skt_init(uint8_t device, uint32_t key_addr)
{
    return ahrs_init();
}

void skt_register_cb(void *cb)
{
    __SKT_CALLBACK_TypeDef *sSktCB = (__SKT_CALLBACK_TypeDef *)cb;
    ahrs_register_cb(sSktCB->send);
}

uint8_t skt_get_role(void)
{
    return 0;
}

void stk_get_version(uint8_t *version)
{
    sprintf((char *)version, "YM Raccoons V0.0.1");
}

uint8_t skt_set_config(__FLASH_SKT_CONFIG_TypeDef config)
{
    return STD_SUCCESS;
}

uint8_t skt_is_check(void)
{
    return true;
}

uint8_t skt_check_keywork(uint32_t key_addr)
{
    return true;
}

uint8_t skt_set_mode(__SKT_MODE_TypeDef work_mode)
{
    return STD_SUCCESS;
}

uint8_t skt_set_tibia_reg_pos(int32_t pos)
{
    return STD_SUCCESS;
}

uint8_t skt_set_navigation_angle(__SKT_NAVIGATION_ANGLE_TypeDef nav)
{
    return STD_SUCCESS;
}

uint8_t skt_start(void)
{
    return ahrs_start();
}

uint8_t skt_stop(void)
{
    return ahrs_stop();
}

uint8_t skt_update_ref_data(uint8_t *buff, uint8_t buffsize)
{
    return STD_SUCCESS;
}

uint8_t skt_test(void)
{
    return 0;
}

/*******************************************************************************
END
*******************************************************************************/
