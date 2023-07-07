/*******************************************************************************
* file    drv_pin.h
* author  zhaoyu.wu
* version 1.0.0
* date
* brief   gpio driver for stm32
*******************************************************************************/

#ifndef __DRV_PIN_H__
#define __DRV_PIN_H__

#include "bsp_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

#define __STM32_PORT(port) GPIO##port##_BASE

#if defined(SOC_SERIES_STM32MP1)
#define GET_PIN(PORTx, PIN) (GPIO##PORTx == GPIOZ) ? (176 + PIN) : ((signed long)((16 * (((signed long)__STM32_PORT(PORTx) - (signed long)GPIOA_BASE) / (0x1000UL))) + PIN))
#else
#define GET_PIN(PORTx, PIN) (signed long)((16 * (((signed long)__STM32_PORT(PORTx) - (signed long)GPIOA_BASE) / (0x0400UL))) + PIN)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __GPIO_IF_H__*/
