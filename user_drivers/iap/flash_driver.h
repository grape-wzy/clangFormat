/*******************************************************************************
* file    flash_driver.h
* author  mackgim
* version 1.0.0
* date
* brief    FLASH驱动头文件
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __FLASH_DRIVER_H
#define __FLASH_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "gcompiler.h"

	__RAM_USER_FUNC_VOID flash_ram_init(void);
	__RAM_USER_FUNC_VOID flash_ram_deinit(void);

	__RAM_USER_FUNC_UINT8_T  flash_ram_erase(uint32_t startaddr, uint32_t endaddr);
	__RAM_USER_FUNC_UINT8_T  flash_ram_write(uint32_t buffaddr, uint32_t* buff, uint32_t buffsize);


	__RAM_USER_FUNC_VOID flash_ram_program(uint32_t addr, uint32_t size);
	__RAM_USER_FUNC_VOID flash_ram_update_running_code(uint32_t Source, uint32_t size);
	__RAM_USER_FUNC_UINT8_T flash_ram_erase_running_code(void);
	__RAM_USER_FUNC_UINT8_T flash_ram_erase_shadow_code(void);

	uint32_t flash_get_end_addr(void);
	uint32_t flash_get_running_addr(uint32_t addr);
	uint32_t flash_get_shadow_addr(uint32_t addr);
	uint32_t flash_get_shadow_end_addr(void);
	uint32_t flash_get_ble_stack_addr(void);
	uint32_t flash_get_mcu_stack_top_addr(void);
	uint32_t flash_get_running_max_size(void);
	uint32_t flash_get_shadow_max_size(void);
	uint32_t flash_get_fw_version(void);

	uint8_t flash_check_id(uint32_t ad, uint32_t size);
	uint32_t flash_get_error_status(void);



	void flash_register_callback(void* cb);

	uint8_t flash_write_shadow_code(uint32_t addr, uint32_t* buff, uint32_t buffsize);
	uint8_t flash_erase_shadow_code(void);

	uint8_t flash_ll_erase(uint32_t startaddr, uint32_t endaddr);
	uint8_t flash_ll_write(uint32_t addr, void const* buff, uint32_t buffsize);

#ifdef __cplusplus
}
#endif
#endif  //_FLASH_IF_H_



