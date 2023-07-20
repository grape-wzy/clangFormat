/*******************************************************************************
* file    skt_if.h
* author  mackgim
* version 1.0.0
* date
* brief   combine accelerometer and gyroscope readings in order to obtain
accurate information about the inclination of your device relative
to the ground plane
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __SKT_IF_H
#define __SKT_IF_H

#include "stdint.h"
#include "com_struct.h"

typedef struct
{
    int32_t Mode;
    int32_t UpAxis;
} __SKT_MODE_TypeDef;

typedef struct
{
    uint32_t Mode;
    float    VV;
    float    FE;
} __SKT_NAVIGATION_ANGLE_TypeDef;

typedef struct {
    uint8_t (*send)(uint8_t *, uint8_t);
    void (*led_ctrl)(void);
} __SKT_CALLBACK_TypeDef;

uint8_t skt_init(uint8_t device, uint32_t key_addr);
uint8_t skt_is_check(void);
uint8_t skt_check_keywork(uint32_t key_addr);
uint8_t skt_get_role(void);

void stk_get_version(uint8_t *version);

void skt_register_cb(void *cb);

uint8_t skt_set_config(__FLASH_SKT_CONFIG_TypeDef config);
uint8_t skt_set_mode(__SKT_MODE_TypeDef work_mode);
uint8_t skt_set_tibia_reg_pos(int32_t pos);
uint8_t skt_set_navigation_angle(__SKT_NAVIGATION_ANGLE_TypeDef nav);
uint8_t skt_update_ref_data(uint8_t *buff, uint8_t buffsize);

uint8_t skt_start(void);
uint8_t skt_stop(void);

void skt_irq_handle(void);

uint8_t skt_test(void);
#endif /*__SKT_IF_H */
