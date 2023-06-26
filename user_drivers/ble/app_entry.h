/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_entry.h
  * @author  MCD Application Team
  * @brief   Interface to the application
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
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef APP_ENTRY_H
#define APP_ENTRY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

/* Private includes ----------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported macros ------------------------------------------------------------*/

/* Exported functions ---------------------------------------------*/
  void APPE_Init_Clock( void );
  void APPE_Init( void );
  void APPE_DeInit(void);

  uint32_t APPE_Get_Stack_Size(void);
  uint32_t APPE_Get_Stack_Numerical_Version(void);
  uint32_t APPE_Get_Fus_Numerical_Version(void);
  uint8_t APPE_Get_String_Version(uint8_t* stack_hw, uint8_t* stack_fw, uint8_t* fus_fw);
  uint8_t APPE_Get_CPU2_Status(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*APP_ENTRY_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
