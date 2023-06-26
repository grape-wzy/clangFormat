/*******************************************************************************
 * file     spi_a_hw_if.h
 * author   mackgim
 * version  V1.0.0
 * date
 * brief ：
 *******************************************************************************/

 /* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __SPI_A_HW_IF_H
#define __SPI_A_HW_IF_H



#ifdef __cplusplus
extern "C" {
#endif

	/* Includes ------------------------------------------------------------------*/
#include "stdint.h"


	uint8_t spi_a_hw_init(void);
	uint8_t spi_a_hw_deinit(void);
	uint8_t spi_a_hw_transmit_receive( uint8_t *pTxData,  uint8_t *pRxData, uint16_t Size, uint32_t Timeout);
	uint8_t spi_a_hw_get_ready(void);
	uint8_t spi_a_hw_set_cs(uint8_t cs);
	uint8_t spi_a_hw_reset(uint8_t enable);
	void spi_a_hw_register_cb(void* cb);
	uint8_t spi_a_hw_enable_irq(uint8_t en);
	uint8_t spi_a_hw_generate_swi(void);

#ifdef __cplusplus
}
#endif

#endif /*__SPI_A_HW_IF_H */

