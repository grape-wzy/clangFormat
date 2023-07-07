/*******************************************************************************
 * file     spi_if.h
 * author   mackgim
 * version  V1.0.0
 * date
 * brief ：
 *******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __SPI_IF_H__
#define __SPI_IF_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"

uint8_t spi1_hw_init(void);
uint8_t spi1_hw_deinit(void); // do not use it

#ifdef __cplusplus
}
#endif

#endif /*__SPI_IF_H__ */
