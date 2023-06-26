/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32wbxx_it.c
  * @author  MCD Application Team
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */


  /* Includes ------------------------------------------------------------------*/
#include "stm32wbxx_it.h"
#include "platform.h"
#include "user_drivers.h"
#include "pm_proc.h"

/******************************************************************************/
/*           User Interruption and Process Handlers          */
/******************************************************************************/

void SysTick_Handler(void)
{

	HAL_IncTick();
}

/******************************************************************************/
/*           Cortex Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
	while (1)
	{
#ifdef DEBUG
		log_flush();
		kprint("i am here\r\n");
		log_flush();
		for (uint32_t i = 0; i < 0xfffff; i++)
		{
			__NOP();
		}
#else
		pwr_reset_mcu();
#endif
	}
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{

	while (1)
	{
#ifdef DEBUG
		//log_init();
		log_flush();
		kprint("i am here\r\n");
		log_flush();
		for (uint32_t i = 0; i < 0xfffff; i++)
		{
			__NOP();
		}
#else
		pwr_reset_mcu();
#endif
	}
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
	while (1)
	{
	}
}

/**
  * @brief This function handles Prefetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
	while (1)
	{
	}
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
	while (1)
	{
	}
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
}

/******************************************************************************/
/* STM32WBxx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32wbxx.s).                    */
/******************************************************************************/


void HSEM_IRQHandler(void)
{
	HAL_HSEM_IRQHandler();
}

void IPCC_C1_TX_IRQHandler(void)
{
	HW_IPCC_Tx_Handler();

	return;
}

void IPCC_C1_RX_IRQHandler(void)
{
	HW_IPCC_Rx_Handler();
	return;
}


void SPI_A_HW_READY_EXTI_IRQHandler(void)
{

	skt_irq_handle();

}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
