/*******************************************************************************
* file    pm_proc.h
* author  mackgim
* version 1.0.0
* date
* brief   power management
*		  功耗管理
*         关机：复位进行关机
*         正常流程中，尽量进入stop
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __PM_PROC_H
#define __PM_PROC_H

#include "stdint.h"



#define STANDBY_WAIT_FOREVER	(0xffffffff)
#define STANDBY_NOT_WAIT		(0)

void pwr_init(void);
void pwr_deinit(void);

void pwr_disable_sleep(uint8_t en);

void pwr_gpio_init(void);
void pwr_gpio_deinit(void);
uint8_t pwr_check_wakeup(void);

void pwr_check_mcu_alive(void);

uint8_t pwr_get_power_relative(void);

void pwr_reset_mcu(void);
void pwr_shutdown_mcu(void); // 其他函数，控制关机的指令
void pwr_stop_mcu(void);
void pwr_stop_mcu_for_delay(void);
void pwr_sleep_mcu(void);

#endif /* __PM_PROC_H*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
