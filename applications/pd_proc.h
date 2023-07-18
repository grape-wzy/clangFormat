/*******************************************************************************
* file    pd_proc.c
* author  mackgim
* version 1.0.0
* date
* brief   产品信息
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __PD_PROC_H
#define __PD_PROC_H

#include <stdint.h>


uint8_t pd_init(void);

uint32_t pd_get_user_end_space(void);

uint8_t pd_save_account_info(void);
uint8_t pd_read_account_info(void);

uint8_t pd_save_register_info(uint8_t cmd);
uint8_t pd_save_system_status(uint8_t status);

uint8_t pd_save_time(void);
uint8_t pd_read_time(void);
uint8_t pd_clear_time(void);

uint8_t pd_save_product_info(void);
uint8_t pd_read_product_info(void);


uint8_t pd_save_ble_mac(void);
uint8_t pd_generate_and_save_ble_mac(void);
uint8_t pd_read_ble_mac(void);

uint8_t pd_save_algo_para(void);
uint8_t pd_read_algo_para(void);

uint8_t pd_save_skt_config(void);
uint8_t pd_read_skt_config(void);

uint8_t pd_check_mcu_mark(void* mark);
uint8_t pd_device_is_checked(void);
void pd_clear_device_check(void);

uint8_t pd_proc_private(uint8_t command, uint8_t sequence, uint8_t *buffer, uint8_t buff_len);

#endif /*__PD_PROC_H */
