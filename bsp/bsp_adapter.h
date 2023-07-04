#ifndef __MCU_ADAPTER_H__
#define __MCU_ADAPTER_H__

#include "stm32wbxx.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STM32_FLASH_START_ADRESS (FLASH_BASE)
#define STM32_FLASH_SIZE         (FLASH_SIZE)
#define STM32_FLASH_END_ADDRESS  (STM32_FLASH_START_ADRESS + STM32_FLASH_SIZE)

/*
 * SRAM1: 0x20000000 + 0x8000
 * SRAM2: 0x20008000 + 0x8000
 */
#define STM32_SRAM1_BEGIN        (SRAM1_BASE)
#define STM32_SRAM1_SIZE         (SRAM1_SIZE)
#define STM32_SRAM1_END          (STM32_SRAM1_BEGIN + STM32_SRAM1_SIZE)

/* remap for SRAM2 */
#define STM32_SRAM2_SIZE         ()
#define STM32_SRAM2_BEGIN        (SRAM2A_BASE)
#define STM32_SRAM2_END          (STM32_SRAM2_BEGIN + STM32_SRAM2_SIZE)

#if defined(__ARMCC_VERSION)
extern int Image$$RW_IRAM1$$ZI$$Limit;
#define HEAP_BEGIN ((void *)&Image$$RW_IRAM1$$ZI$$Limit)

#elif __ICCARM__
#pragma section = "CSTACK"
#define HEAP_BEGIN (__segment_end("CSTACK"))

#else
extern int __bss_end;
#define HEAP_BEGIN ((void *)&__bss_end)
#endif

#define HEAP_END STM32_SRAM1_END

extern ADC_HandleTypeDef   hadc1;
extern CRC_HandleTypeDef   hcrc;
extern IPCC_HandleTypeDef  hipcc;
extern LPTIM_HandleTypeDef hlptim1;
extern RNG_HandleTypeDef   hrng;
extern RTC_HandleTypeDef   hrtc;
extern SPI_HandleTypeDef   hspi1;
extern DMA_HandleTypeDef   hdma_spi1_rx;
extern DMA_HandleTypeDef   hdma_spi1_tx;
extern UART_HandleTypeDef  huart1;
extern DMA_HandleTypeDef   hdma_usart1_rx;
extern DMA_HandleTypeDef   hdma_usart1_tx;

void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
void MX_GPIO_Init(void);
void MX_DMA_Init(void);
void MX_ADC1_Init(void);
void MX_CRC_Init(void);
void MX_IPCC_Init(void);
void MX_RF_Init(void);
void MX_RNG_Init(void);
void MX_SPI1_Init(void);
void MX_USART1_UART_Init(void);
void MX_LPTIM1_Init(void);
void MX_RTC_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __MCU_ADAPTER_H__ */
