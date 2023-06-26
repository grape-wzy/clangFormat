/*******************************************************************************
 * file    SystemInit.h
 * author  mackgim
 * version 1.0.0
 * date    
 * brief   STM32的外设，用户外设，用户数据的初始化，系统自检，启动函数等
 *******************************************************************************/
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __PLATFORMINIT_H
#define __PLATFORMINIT_H

/* Includes ------------------------------------------------------------------*/

/*STM32外设相关变量、宏定义、函数的声明*/


/*用户外设相关变量、宏定义、函数的声明*/

void platform_init(void);
void platform_deinit(void);

void SystemClock_32M_Config(void);
void SystemClock_64M_Config(void);
#endif /* __SYSTEMINIT_H*/


