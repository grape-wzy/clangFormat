/*******************************************************************************
* file    user_hw.h
* author  mackgim
* version 1.0.0
* date
* brief    硬件配置文件
*******************************************************************************/


#ifndef __USRE_HW_H
#define __USRE_HW_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#pragma region Wakeup

#define WAKEUP_GPIO_PIN				        GPIO_PIN_0
#define WAKEUP_GPIO_MODE			        GPIO_MODE_INPUT
#define WAKEUP_GPIO_PULL			        GPIO_NOPULL
#define WAKEUP_GPIO_SPEED			        GPIO_SPEED_MEDIUM
#define WAKEUP_GPIO_ALTERNATE				0
#define WAKEUP_GPIO_PORT			        GPIOA
#define WAKEUP_GPIO_CLK_ENABLE()			__HAL_RCC_GPIOA_CLK_ENABLE()
#define WAKEUP_CTRL(n)						HAL_GPIO_WritePin(WAKEUP_GPIO_PORT, WAKEUP_GPIO_PIN, n?GPIO_PIN_SET:GPIO_PIN_RESET)
#define WAKEUP_TOGGLE()						HAL_GPIO_TogglePin(WAKEUP_GPIO_PORT, WAKEUP_GPIO_PIN)
#define WAKEUP_INPUT()						HAL_GPIO_ReadPin(WAKEUP_GPIO_PORT, WAKEUP_GPIO_PIN)


#define WAKEUP_EXTI_IRQn					EXTI0_1_IRQn
#define WAKEUP_EXTI_IRQHandler				EXTI0_1_IRQHandler
#define WAKEUP_EXTI_PIN						WAKEUP_GPIO_PIN
#define WAKEUP_EXTI_PORT					WAKEUP_GPIO_PORT

#pragma endregion

#pragma region kprint

#define KLOG_USARTx						USART1
#define KLOG_USARTx_BAUD_RATE           460800

#define KLOG_USARTx_SET_CLK_SOURCE()	LL_RCC_SetUSARTClockSource(LL_RCC_USART1_CLKSOURCE_PCLK2)

#define KLOG_USARTx_CLK_ENABLE()		__HAL_RCC_USART1_CLK_ENABLE()
#define KLOG_USARTx_CLK_DISABLE()		__HAL_RCC_USART1_CLK_DISABLE()


#define KLOG_USARTx_FORCE_RESET()		__HAL_RCC_USART1_FORCE_RESET()
#define KLOG_USARTx_RELEASE_RESET()		__HAL_RCC_USART1_RELEASE_RESET()


#define KLOG_USARTx_TX_PIN				GPIO_PIN_6
#define KLOG_USARTx_TX_MODE				GPIO_MODE_AF_PP
#define KLOG_USARTx_TX_PULL				GPIO_PULLUP
#define KLOG_USARTx_TX_SPEED			GPIO_SPEED_FREQ_VERY_HIGH
#define KLOG_USARTx_TX_ALTERNATE		GPIO_AF7_USART1
#define KLOG_USARTx_TX_PORT				GPIOB
#define KLOG_USARTx_TX_CLK_ENABLE()		__HAL_RCC_GPIOB_CLK_ENABLE()


#define KLOG_USARTx_RX_PIN				GPIO_PIN_7
#define KLOG_USARTx_RX_MODE				GPIO_MODE_AF_PP
#define KLOG_USARTx_RX_PULL				GPIO_PULLUP
#define KLOG_USARTx_RX_SPEED			GPIO_SPEED_FREQ_VERY_HIGH
#define KLOG_USARTx_RX_ALTERNATE		GPIO_AF7_USART1
#define KLOG_USARTx_RX_PORT				GPIOB
#define KLOG_USARTx_RX_CLK_ENABLE()		__HAL_RCC_GPIOB_CLK_ENABLE()


	/* Definition for USARTx's NVIC */
#define KLOG_USARTx_IRQn				USART1_IRQn
#define KLOG_USARTx_IRQHandler			USART1_IRQHandler

#define KLOG_DMAMUXx_CLK_ENABLE()		__HAL_RCC_DMAMUX1_CLK_ENABLE()
#define KLOG_DMAx_CLK_ENABLE()			__HAL_RCC_DMA1_CLK_ENABLE()
#define KLOG_DMAx_CLK_DISABLE()			__HAL_RCC_DMA1_CLK_DISABLE()

#define KLOG_USARTx_TX_DMAx_INSTANCE	DMA1_Channel1
#define KLOG_USARTx_TX_DMAx_REQUEST		DMA_REQUEST_USART1_TX
#define KLOG_USARTx_TX_DMAx_IRQn		DMA1_Channel1_IRQn
#define KLOG_USARTx_TX_DMAx_IRQHandler	DMA1_Channel1_IRQHandler

#define KLOG_TX_MAX_BUFF  (2 * 1024)

#pragma endregion

#pragma region flash iap

#define FLASH_SPACE_BASE_ADDR					((uint32_t)0x08000000)
#define FLASH_SPACE_RUNNING_BEGIN_ADDR  		FLASH_SPACE_BASE_ADDR
#define FLASH_SPACE_RUNNING_END_ADDR			((uint32_t)0x08028000)  //160K - 执行代码空间的容量
#define FLASH_SPACE_SHADOW_BEGIN_ADDR			((uint32_t)0x08028000)	//160K - 备用空间起始地址
#define FLASH_SPACE_SHADOW_END_ADDR  			((uint32_t)0x08052000)	//160K - 升级代码备用空间的容量
#define FLASH_SPACE_USER_BEGIN_ADDR				((uint32_t)0x08052000)	//320K - 用户空间起始地址
#define FLASH_SPACE_USER_END_ADDR				((uint32_t)0x08056000)	//16K - 用户空间的容量
#define FLASH_SPACE_BLE_STACK_BEGIN_ADDR		((uint32_t)0x08056000)	//344K - 预留的蓝牙协议栈起始地址， 非真正的地址
#define FLASH_SPACE_END_ADDR  					((uint32_t)0x08080000)  //最大168K

//最大的存储空间,预留128字节给其他信息
#define FLASH_SPACE_SHADOW_MAX_SIZE				(FLASH_SPACE_SHADOW_END_ADDR - FLASH_SPACE_SHADOW_BEGIN_ADDR - 128)

//最大的存储空间
#define FLASH_SPACE_RUNNING_MAX_SIZE			(FLASH_SPACE_RUNNING_END_ADDR - FLASH_SPACE_RUNNING_BEGIN_ADDR)

#define FLASH_SPACE_PAGE_SIZE					FLASH_PAGE_SIZE // page大小，以byte为单位

#define FLASH_GET_PAGE_INDEX(Addr)				((Addr - FLASH_SPACE_BASE_ADDR) / FLASH_SPACE_PAGE_SIZE)//获取page序号

#define RAM_SPACE_TOP_STACK_ADDR  				((uint32_t)0x20030000)//栈Stack最高地址

#define	FW_VERSION 2
#define CODE_IDENTIFIER  "SOTTxWB_MIEJAE932"
#define CODE_NAME  "[WB55-IAP]"
#pragma endregion

#pragma region Flash Data

//物理page数量
#define FLASH_USER_PHY_PAGES				((FLASH_SPACE_USER_END_ADDR - FLASH_SPACE_USER_BEGIN_ADDR) / FLASH_SPACE_PAGE_SIZE)
//实际用于循环保存的page
#define FLASH_USER_DATA_PAGES				(FLASH_USER_PHY_PAGES - 1)
//以word为单位的page大小
#define FLASH_USER_PAGE_SIZE				(FLASH_SPACE_PAGE_SIZE/4) // in 4-byte words

#pragma endregion

#pragma region imu

// SPI Instance
#define SPI_A_HW_INSTANCE					SPI1
#define SPI_A_HW_CLK_ENABLE()				__HAL_RCC_SPI1_CLK_ENABLE()
#define SPI_A_HW_CLK_DISABLE()				__HAL_RCC_SPI1_CLK_DISABLE()

#define SPI_A_HW_CLK_FORCE_RESET()			__HAL_RCC_SPI1_FORCE_RESET()
#define SPI_A_HW_CLK_RELEASE_RESET()		__HAL_RCC_SPI1_RELEASE_RESET()

// SPI Configuration
#define SPI_A_HW_MODE						SPI_MODE_MASTER
#define SPI_A_HW_DIRECTION					SPI_DIRECTION_2LINES
#define SPI_A_HW_DATASIZE					SPI_DATASIZE_8BIT
#define SPI_A_HW_CLKPOLARITY				SPI_POLARITY_LOW
#define SPI_A_HW_CLKPHASE	        		SPI_PHASE_1EDGE
#define SPI_A_HW_NSS						SPI_NSS_SOFT
#define SPI_A_HW_FIRSTBIT	        		SPI_FIRSTBIT_MSB
#define SPI_A_HW_TIMODE						SPI_TIMODE_DISABLED
#define SPI_A_HW_CRCPOLYNOMIAL				10
#ifdef ENABLE_RCC_HSE_32M
#define SPI_A_HW_BAUDRATEPRESCALER			SPI_BAUDRATEPRESCALER_32
#else
#define SPI_A_HW_BAUDRATEPRESCALER			SPI_BAUDRATEPRESCALER_32
#endif
#define SPI_A_HW_CRCCALCULATION				SPI_CRCCALCULATION_DISABLED

// SCLK: PA.5
#define SPI_A_HW_SCLK_PIN                   GPIO_PIN_5
#define SPI_A_HW_SCLK_MODE					GPIO_MODE_AF_PP
#define SPI_A_HW_SCLK_PULL                  GPIO_PULLUP
#define SPI_A_HW_SCLK_SPEED		        	GPIO_SPEED_FREQ_LOW
#define SPI_A_HW_SCLK_ALTERNATE				GPIO_AF5_SPI1
#define SPI_A_HW_SCLK_PORT					GPIOA
#define SPI_A_HW_SCLK_CLK_ENABLE()	        __HAL_RCC_GPIOA_CLK_ENABLE()

// MISO (Master Input Slave Output): PA.6
#define SPI_A_HW_MISO_PIN					GPIO_PIN_6
#define SPI_A_HW_MISO_MODE					GPIO_MODE_AF_PP
#define SPI_A_HW_MISO_PULL                  GPIO_PULLUP
#define SPI_A_HW_MISO_SPEED		        	GPIO_SPEED_FREQ_LOW
#define SPI_A_HW_MISO_ALTERNATE				GPIO_AF5_SPI1
#define SPI_A_HW_MISO_PORT					GPIOA
#define SPI_A_HW_MISO_CLK_ENABLE()	        __HAL_RCC_GPIOA_CLK_ENABLE()
#define SPI_A_HW_READ_MISO()				(HAL_GPIO_ReadPin(SPI_A_HW_MISO_PORT, SPI_A_HW_MISO_PIN))

// MOSI (Master Output Slave Input): PA.7
#define SPI_A_HW_MOSI_PIN					GPIO_PIN_7
#define SPI_A_HW_MOSI_MODE					GPIO_MODE_AF_PP
#define SPI_A_HW_MOSI_PULL                  GPIO_PULLUP
#define SPI_A_HW_MOSI_SPEED					GPIO_SPEED_FREQ_LOW
#define SPI_A_HW_MOSI_ALTERNATE				GPIO_AF5_SPI1
#define SPI_A_HW_MOSI_PORT					GPIOA
#define SPI_A_HW_MOSI_CLK_ENABLE()	        __HAL_RCC_GPIOA_CLK_ENABLE()

// NSS/CSN/CS: PA.3
#define SPI_A_HW_CS_PIN						GPIO_PIN_4
#define SPI_A_HW_CS_MODE			        GPIO_MODE_OUTPUT_PP
#define SPI_A_HW_CS_PULL			        GPIO_PULLUP
#define SPI_A_HW_CS_SPEED					GPIO_SPEED_FREQ_LOW
#define SPI_A_HW_CS_ALTERNATE		        0
#define SPI_A_HW_CS_PORT			        GPIOA
#define SPI_A_HW_CS_CLK_ENABLE()			__HAL_RCC_GPIOA_CLK_ENABLE()
#define SPI_A_HW_CS(n)						HAL_GPIO_WritePin(SPI_A_HW_CS_PORT, SPI_A_HW_CS_PIN, n?GPIO_PIN_SET:GPIO_PIN_RESET)

#define SPI_A_HW_DMAMUX_CLK_ENABLE()        __HAL_RCC_DMAMUX1_CLK_ENABLE()
#define SPI_A_HW_DMA_CLK_ENABLE()           __HAL_RCC_DMA1_CLK_ENABLE()

#define SPI_A_HW_DMAMUX_CLK_DISABLE()       __HAL_RCC_DMAMUX1_CLK_DISABLE()
#define SPI_A_HW_DMA_CLK_DISABLE()          __HAL_RCC_DMA1_CLK_DISABLE()

#define SPI_A_RX_DMA_CHANNEL                DMA1_Channel2
#define SPI_A_RX_DMA_IRQn                   DMA1_Channel2_IRQn
#define SPI_A_RX_DMA_IRQHandler             DMA1_Channel2_IRQHandler

#define SPI_A_TX_DMA_CHANNEL                DMA1_Channel3
#define SPI_A_TX_DMA_IRQn                   DMA1_Channel3_IRQn
#define SPI_A_TX_DMA_IRQHandler             DMA1_Channel3_IRQHandler

#ifdef USE_SENSOR_SYNC_PIN
#define SPI_A_HW_SYNC_PIN                   GPIO_PIN_0
#define SPI_A_HW_SYNC_MODE			        GPIO_MODE_OUTPUT_PP
#define SPI_A_HW_SYNC_PULL			        GPIO_PULLUP
#define SPI_A_HW_SYNC_SPEED					GPIO_SPEED_FREQ_LOW
#define SPI_A_HW_SYNC_ALTERNATE		        0
#define SPI_A_HW_SYNC_PORT			        GPIOB
#define SPI_A_HW_SYNC_CLK_ENABLE()			__HAL_RCC_GPIOB_CLK_ENABLE()
#endif

#ifdef USE_SENSOR_READY_PIN
// ready : PB.4
#define SPI_A_HW_READY_PIN                  GPIO_PIN_1
#define SPI_A_HW_READY_MODE					GPIO_MODE_IT_RISING
#define SPI_A_HW_READY_PULL					GPIO_PULLUP
#define SPI_A_HW_READY_SPEED				GPIO_SPEED_FREQ_VERY_HIGH
#define SPI_A_HW_READY_ALTERNATE			0
#define SPI_A_HW_READY_PORT					GPIOB
#define SPI_A_HW_READY_CLK_ENABLE()	        __HAL_RCC_GPIOB_CLK_ENABLE()
#define SPI_A_HW_READ_READY()				(HAL_GPIO_ReadPin(SPI_A_HW_READY_PORT, SPI_A_HW_READY_PIN))
#define SPI_A_HW_READY_CTRL(n)				HAL_GPIO_WritePin(SPI_A_HW_READY_PORT, SPI_A_HW_READY_PIN, n ? GPIO_PIN_SET : GPIO_PIN_RESET)

//#define SPI_A_HW_READY_EXTI_LINE			LL_EXTI_LINE_4
#define SPI_A_HW_READY_EXTI_IRQn			EXTI4_IRQn
#define SPI_A_HW_READY_EXTI_IRQHandler		EXTI4_IRQHandler
#define SPI_A_HW_READY_EXTI_PIN				SPI_A_HW_READY_PIN
#define SPI_A_HW_READY_EXTI_PORT			SPI_A_HW_READY_PORT
#endif

#ifdef USE_SENSOR_RESET_PIN
// RESET: PA.5
#define SPI_A_HW_RESET_PIN                  GPIO_PIN_8
#define SPI_A_HW_RESET_MODE			        GPIO_MODE_OUTPUT_PP
#define SPI_A_HW_RESET_PULL			        GPIO_NOPULL
#define SPI_A_HW_RESET_SPEED				GPIO_SPEED_FREQ_VERY_HIGH
#define SPI_A_HW_RESET_ALTERNATE			0
#define SPI_A_HW_RESET_PORT			        GPIOA
#define SPI_A_HW_RESET_CLK_ENABLE()			__HAL_RCC_GPIOA_CLK_ENABLE()
#define SPI_A_HW_RESET(n)					HAL_GPIO_WritePin(SPI_A_HW_RESET_PORT, SPI_A_HW_RESET_PIN, n?GPIO_PIN_SET:GPIO_PIN_RESET)
#endif
#pragma endregion

#pragma region Power Switch

//设备IO
#ifdef USE_DEVICE_PIN
#define DEVICE_GPIO_PIN					GPIO_PIN_1
#define DEVICE_GPIO_MODE				GPIO_MODE_INPUT
#define DEVICE_GPIO_PULL				GPIO_NOPULL
#define DEVICE_GPIO_PORT				GPIOB
#define DEVICE_GPIO_CLK_ENABLE()		__HAL_RCC_GPIOB_CLK_ENABLE()
#define DEVICE_GPIO_READ()				(HAL_GPIO_ReadPin(DEVICE_GPIO_PORT, DEVICE_GPIO_PIN))
#endif

#ifdef USE_SENSOR_PWR_PIN
//传感器电源
#define PWR_GPIO_PIN					GPIO_PIN_2
#define PWR_GPIO_PORT					GPIOA
#define PWR_GPIO_CTRL(n)				HAL_GPIO_WritePin(PWR_GPIO_PORT, PWR_GPIO_PIN, n?GPIO_PIN_SET:GPIO_PIN_RESET)
#define PWR_GPIO_CLK_ENABLE()			__HAL_RCC_GPIOA_CLK_ENABLE()
#endif

#define MCU_PWR_PIN                     GPIO_PIN_11
#define MCU_PWR_PORT                    GPIOA
#define MCU_PWR_CTRL(n)                 HAL_GPIO_WritePin(MCU_PWR_PORT, MCU_PWR_PIN, n ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define MCU_PWR_READ()                  HAL_GPIO_ReadPin(MCU_PWR_PORT, MCU_PWR_PIN)
#define MCU_PWR_CLK_ENABLE()            __HAL_RCC_GPIOA_CLK_ENABLE()

#pragma endregion

#pragma region LED
//绿灯
#define LEDG_PIN                        GPIO_PIN_4
#define LEDG_PORT                       GPIOB
#define LEDG_CTRL(n)					HAL_GPIO_WritePin(LEDG_PORT, LEDG_PIN, n?GPIO_PIN_SET:GPIO_PIN_RESET)
#define LEDG_TOGGLE()					HAL_GPIO_TogglePin(LEDG_PORT, LEDG_PIN);
#define LEDG_CLK_ENABLE()               __HAL_RCC_GPIOB_CLK_ENABLE()

//黄灯灯
#define LEDY_PIN						GPIO_PIN_3
#define LEDY_PORT                       GPIOB
#define LEDY_CTRL(n)					HAL_GPIO_WritePin(LEDY_PORT, LEDY_PIN, n?GPIO_PIN_SET:GPIO_PIN_RESET)
#define LEDY_TOGGLE()					HAL_GPIO_TogglePin(LEDY_PORT, LEDY_PIN);
#define LEDY_CLK_ENABLE()               __HAL_RCC_GPIOB_CLK_ENABLE()

#pragma endregion

#pragma region 中断优先级配置

#define 	GTIMER_LPTIM_NVIC_PreemptionPriority      			0
#define 	GTIMER_LPTIM_NVIC_SubPriority             			0

#define 	KLOG_USARTx_NVIC_PreemptionPriority      			1
#define 	KLOG_USARTx_NVIC_SubPriority						0

#define 	KLOG_TX_DMAx_NVIC_PreemptionPriority      			1
#define 	KLOG_TX_DMAx_NVIC_SubPriority						0

#define 	IPCC_C1_RX_NVIC_PreemptionPriority      			2
#define 	IPCC_C1_RX_NVIC_SubPriority             			0

#define 	IPCC_C1_TX_NVIC_PreemptionPriority      			2
#define 	IPCC_C1_TX_NVIC_SubPriority             			0

#define 	HSEM_NVIC_PreemptionPriority      					2
#define 	HSEM_NVIC_SubPriority             					0

#define 	SPI_A_HW_DRDY_NVIC_PreemptionPriority      			2
#define 	SPI_A_HW_DRDY_NVIC_SubPriority						0

#define 	SPI_A_HW_DMA_NVIC_PreemptionPriority      			1
#define 	SPI_A_HW_DMA_NVIC_SubPriority						0

#pragma endregion

#ifdef __cplusplus
}
#endif

#endif /* __USRE_HW_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
