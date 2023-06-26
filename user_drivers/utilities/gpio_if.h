/*******************************************************************************
* file    gpio_hw.h
* author  mackgim
* version 1.0.0
* date    
* brief   gpio
*******************************************************************************/

#ifndef __GPIO_IF_H
#define __GPIO_IF_H

#include "stdint.h"


/* Mode定义
 * 0 - 控制ledy状态
 * 1 - 控制ledg状态
 * 2 - 控制ledy闪烁, timer循环控制
 * 3 - 控制ledg闪烁，只控制能亮的时间
 **/
#define LED_MODE_Y_CTRL (0)
#define LED_MODE_G_CTRL (1)
#define LED_MODE_Y_BLINK (2)
#define LED_MODE_G_BLINK (3)
#define LED_MODE_Y_G_BLINK (4)

void gpio_init(void);
void gpio_deinit(void);

void led_blink_while_upgrading_fw(void);

uint8_t read_device_type(void);
void pwr_ctrl(uint8_t value);
void ledg_ctrl(uint8_t value);
void ledy_ctrl(uint8_t value);

void led_ctrl(uint8_t mode, uint8_t status);
#endif /* __GPIO_HW_H*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
