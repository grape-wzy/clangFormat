/**
  ******************************************************************************
  * @file    stm32l0xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  *
  * COPYRIGHT(c) 2015 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "user_drivers.h"
#include "user_hw.h"
#include "itask.h"
#include "iproc.h"
extern TIM_HandleTypeDef        TimHandle;





//static uint32_t xCount = 0, yCount = 0;
//void SysTick_Handler(void)
//{
//	osSystickHandler();	
//}

/**
* @brief  This function handles TIM interrupt request.
* @param  None
* @retval None
*/
void TIM2_IRQHandler(void)
{
	HAL_TIM_IRQHandler(&TimHandle);
	itask_os_is_alive();
}



void BNRG_SPI_EXTI_IRQHandler(void)
{
	if (__HAL_GPIO_EXTI_GET_IT(BNRG_SPI_EXTI_PIN) != RESET)
	{
		__HAL_GPIO_EXTI_CLEAR_IT(BNRG_SPI_EXTI_PIN);
		ble_rx_isr();
	}

}





#ifndef ENBALE_BACK_TRACE
void HardFault_Handler(void)
{
	
	cprint("[HardFault]: here \r\n");
	//klog_level_32("[HardFault]: Fuck \r\n");
	uint32_t timeout = 0x1fffff;
	while (timeout--);
	timeout = 0x1fffff;
	while (timeout--);
	HAL_NVIC_SystemReset();
}
#endif

void NMI_Handler(void)
{
	cprint("[NMI_Handler]: here \r\n");
}

/**
* @brief  This function handles Memory Manage exception.
* @param  None
* @retval None
*/
void MemManage_Handler(void)
{
	/* Go to infinite loop when Memory Manage exception occurs */
	cprint("[MemManage_Handler]: here \r\n");
	while (1)
	{
	}
}

/**
* @brief  This function handles Bus Fault exception.
* @param  None
* @retval None
*/
void BusFault_Handler(void)
{
	cprint("[BusFault_Handler]: here \r\n");
	/* Go to infinite loop when Bus Fault exception occurs */
	while (1)
	{
	}
}

/**
* @brief  This function handles Usage Fault exception.
* @param  None
* @retval None
*/
void UsageFault_Handler(void)
{
	cprint("[UsageFault_Handler]: here \r\n");
	/* Go to infinite loop when Usage Fault exception occurs */
	while (1)
	{
	}
}
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
