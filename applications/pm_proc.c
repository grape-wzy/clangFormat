/*******************************************************************************
* file    pm_proc.c
* author  mackgim
* version 1.0.0
* date
* brief 功耗管理
*******************************************************************************/


#include "pm_proc.h"
#include "platform.h"
#include "user_drivers.h"
#include "pd_proc.h"

#pragma region 宏定义

#define ENABLE_PW_DEBUG 1
#ifdef DEBUG
#if ENABLE_PW_DEBUG
#define pwkpf(...) kprint(__VA_ARGS__)
#define pwnpf(...) nprint(__VA_ARGS__)
#else
#define pwkpf(...)
#define pwnpf(...)
#endif
#else
#define pwkpf(...)
#define pwnpf(...)
#endif

#ifndef DEBUG
#if 1
#define PWR_ENABLE_STOP			1
#define PWR_ENABLE_STANBY		0
#endif
#else //调试时候，根据情况打开休眠
#if 1
#define PWR_ENABLE_STOP			1
#define PWR_ENABLE_STANBY		0
#else
#define PWR_ENABLE_STOP			0
#define PWR_ENABLE_STANBY		0
#endif
#endif

#pragma endregion

#pragma region 函数

static void EnterLowPower(void);
static void ExitLowPower(void);
static void Switch_On_HSI(void);

static void pm_enter_standby(void); // 最低功耗
static void pm_enter_stop(void);// 第二最低功耗
static void pm_enter_sleep(void);// 第三最低功耗

void pwr_shutdown_proc_pre_idle(void);
void pwr_shutdown_proc(void);
void pwr_sleep_proc(uint8_t status);

uint8_t pm_get_wakeup_status(void);

#pragma endregion

#pragma region 变量

#define POWER_OFF_MAX_TIME_OUT (uint64_t)(10000) //unit ms，10s
static uint64_t sPowerOffTimeout = 0;
static uint64_t sPowerOffStartTime = 0;
static uint8_t sPowerOffCountDown = 0;
static uint8_t sPowerOffStartFlag = false;

static uint8_t sDisableSleep = 0;
static uint32_t sMcuRunCount = 0;
uint32_t lowp = 0;
#pragma endregion

#pragma region 基本功能函数

void pwr_init(void)
{

#if (PWR_ENABLE_STANBY == 1)
	pwr_gpio_init();
	sPowerOffTimeout = Clock_Time() + POWER_OFF_MAX_TIME_OUT;
	pwkpf("io=%u\r\n", pwr_check_wakeup());
#endif
	//pm_get_wakeup_status();

}

void pwr_deinit(void)
{

}

void pwr_disable_sleep(uint8_t en)
{
#ifdef DEBUG
	if (en)
	{
		pwkpf("true\r\n");
	}
	else
	{
		pwkpf("false\r\n");
	}
#endif
	sDisableSleep = en;
}

//功耗管理前流程
static void pm_proc_pre_idle(uint8_t status)
{
#if (PWR_ENABLE_STANBY == 1)
	pwr_shutdown_proc_pre_idle();
#endif

#ifdef DEBUG
	if (!UTIL_SEQ_IsSchedulableTask(UTIL_SEQ_DEFAULT))
	{
		log_flush();
	}
#endif
	//pwnpf("stop,%u\r\n", (unsigned int)Clock_Time());
	uint64_t diff = sys_diff_time();
	sys_update_work_time(diff);
}

//功耗管理后流程
static void pm_proc_post_idle(uint8_t status)
{
	uint64_t diff = sys_diff_time();
	//工作状态，外部电源打开，器件同时在消耗能量，需要把这部分时间纳入工作时间中
	if (status == SYSTEM_STATUS_WORKING)
	{
		sys_update_work_time(diff);
	}

#ifdef DEBUG1
	static uint32_t count = 0;
	count++;
	if (count == 1)
	{
		count = 0;
		pwkpf("diff=%lu, %u\r\n", (long unsigned int)diff, (unsigned int)sMcuRunCount);
	}
#endif
}

//功耗管理中流程
static void pm_proc_idle(uint8_t status)
{
	sMcuRunCount++;
	//1秒钟，唤醒一次，保持灯闪烁一次，提醒开机状态,同时进入这个流程一次
	if (sDisableSleep)
	{
		return;
	}

#if (PWR_ENABLE_STANBY == 1)
	pwr_shutdown_proc();
#endif

#if (PWR_ENABLE_STOP == 1)
	pwr_sleep_proc(status);
#endif

}

/* UTIL_SEQ_PreIdle 接口重定义 */
void UTIL_SEQ_PreIdle(void)
{
    pm_proc_pre_idle(sSystemStatus);
}

/* UTIL_SEQ_Idle 接口重定义 */
void UTIL_SEQ_Idle(void)
{
    pm_proc_idle(sSystemStatus);
}

/* UTIL_SEQ_PostIdle 接口重定义 */
void UTIL_SEQ_PostIdle(void)
{
    pm_proc_post_idle(sSystemStatus);
}

// void UTIL_SEQ_EvtIdle(UTIL_SEQ_bm_t task_id_bm, UTIL_SEQ_bm_t evt_waited_bm)
// {
//     UTIL_SEQ_Run(UTIL_SEQ_DEFAULT);
// }

static uint8_t sEnableShutDown = false;
void pwr_shutdown_proc_pre_idle(void)
{
	/********************************************
 * 激活状态定义：
 * 激活状态 - 放在NFC reader的线圈上
 * 未激活状态 - 离开NFC reader的线圈
 *******************************************/

 /********************************************
 * 关机策略，分两种情况,1、设备未被注册，2、设备已被注册
 * 1、设备未被注册 - 分蓝牙连接和不连接两种情况
 *    1.1、蓝牙连接 - 永不关机
 *    1.2、蓝牙未连接 - 如果处于未激活状态，延时10秒立刻关机
 * 2、设备被注册时 - 永不关机
*******************************************/

//设备已被注册下， 退出
	if (gAccountInfo.IsLogined)
	{
		goto end;
	}

	//蓝牙已被连接， 退出
	if (ble_is_connected() == 1)
	{
		goto end;
	}

	//1,返回激活状态
	if (pwr_check_wakeup())
	{
		goto end;
	}

	//延时开始，倒数计时
	if (!sPowerOffStartFlag)
	{

		sPowerOffStartFlag = true;
		sPowerOffStartTime = Clock_Time();
		sPowerOffTimeout = sPowerOffStartTime + POWER_OFF_MAX_TIME_OUT;
		sPowerOffCountDown = 10;
		sPowerOffStartTime += 1000; //延时一秒
		pwkpf("Count Down : 10\r\n");
	}

	//10s延时到
	uint64_t stopTime = Clock_Time();
	if (stopTime > sPowerOffTimeout)
	{
		sPowerOffCountDown--;
		pwkpf("Count Down : %u, and will shutdown\r\n", sPowerOffCountDown);
		pd_save_time();
		Clock_Wait(100);
		sEnableShutDown = true;
	}
	//1s延时到
	else if (stopTime > sPowerOffStartTime)
	{
		sPowerOffCountDown--;
		sPowerOffStartTime += 1000;
		pwkpf("Count Down : %u\r\n", sPowerOffCountDown);
	}

	return;
end:
	if (sPowerOffStartFlag)
	{
		pwkpf("cancel \r\n");
	}
	sPowerOffStartFlag = false;

}

//关机流程
void pwr_shutdown_proc(void)
{
	if (sEnableShutDown)
	{
		pwr_shutdown_mcu();
	}
}

//休眠流程
void pwr_sleep_proc(uint8_t status)
{
	pwr_stop_mcu();
}

//复位
void pwr_reset_mcu(void)
{
	pwkpf("doing\r\n");
	pd_save_time();
	log_flush();
	Clock_Wait(500);
	HAL_NVIC_SystemReset();
}

//待机
void pwr_shutdown_mcu(void)
{
	pd_save_time();
	log_flush();
	Clock_Wait(500);

	ATOMIC_SECTION_BEGIN();
	pm_enter_standby();
	ATOMIC_SECTION_END();
}

//停止
void pwr_stop_mcu(void)
{
	ATOMIC_SECTION_BEGIN();
	pm_enter_stop();
	ATOMIC_SECTION_END();
}

//停止
void pwr_stop_mcu_for_delay(void)
{
	if (log_is_busy())
	{
		return;
	}
#if (PWR_ENABLE_STOP == 1)
	//ATOMIC_SECTION_BEGIN();
	pwr_stop_mcu();
	//ATOMIC_SECTION_END();
#endif
}

//休眠
void pwr_sleep_mcu(void)
{
	ATOMIC_SECTION_BEGIN();
	pm_enter_sleep();
	ATOMIC_SECTION_END();
}

#pragma endregion

#pragma region 具体功能函数

void pm_enter_standby(void)
{

#define ENABLE_WAY1_LOW_POWER 3

#if ENABLE_WAY1_LOW_POWER == 1

	EnterLowPower(); // 这句代码在官网BLE_HeartRate例程中的文件里，stm32_lpm_if.c

	LL_PWR_DisableBootC2();
	if ((LL_PWR_IsActiveFlag_C1SB() == 0) || (LL_PWR_IsActiveFlag_C2SB() == 0))
	{
		/* Set the lowest low-power mode for CPU2: shutdown mode */
		LL_C2_PWR_SetPowerMode(LL_PWR_MODE_SHUTDOWN);
	}

	LL_PWR_ClearFlag_WU();

	HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN4_HIGH);

	HAL_SuspendTick();




	HAL_PWREx_EnterSHUTDOWNMode();

	//	LL_PWR_SetPowerMode(LL_PWR_MODE_SHUTDOWN);
	//
	//	LL_LPM_EnableDeepSleep(); /**< Set SLEEPDEEP bit of Cortex System Control Register */
	//
	//	/**
	//	 * This option is used to ensure that store operations are completed
	//	 */
	//#if defined ( __CC_ARM)
	//	__force_stores();
	//#endif
	//
	//	__WFI();

		//CLEAR_BIT(SCB->SCR, ((uint32_t)SCB_SCR_SLEEPDEEP_Msk));

		/* Initialize all configured peripherals */
			//HAL_PWREx_ClearWakeupFlag(PWR_FLAG_WUF4);
	//__HAL_RCC_HSEM_CLK_DISABLE();
	//__HAL_RCC_IPCC_CLK_DISABLE();
	/* USER CODE BEGIN 2 */
#elif ENABLE_WAY1_LOW_POWER == 2



	HAL_SuspendTick();

	EnterLowPower();
	//LL_PWR_DisableBootC2();
	//if ((LL_PWR_IsActiveFlag_C1SB() == 0) || (LL_PWR_IsActiveFlag_C2SB() == 0))
	//{
	//	/* Set the lowest low-power mode for CPU2: shutdown mode */
	//	LL_C2_PWR_SetPowerMode(LL_PWR_MODE_SHUTDOWN);
	//}

	LL_PWR_ClearFlag_WU();
	HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN4_HIGH);



	LL_PWR_SetPowerMode(LL_PWR_MODE_SHUTDOWN);

	LL_LPM_EnableDeepSleep(); /**< Set SLEEPDEEP bit of Cortex System Control Register */
	/**
	 * This option is used to ensure that store operations are completed
	 */
#if defined ( __CC_ARM)
	__force_stores();
#endif

	__WFI();

	return;

#elif ENABLE_WAY1_LOW_POWER == 3

	LL_PWR_EnableBkUpAccess();
	LL_PWR_EnableBkUpAccess();

	//LL_RCC_DisableRTC(); //  __HAL_RCC_RTC_DISABLE();

	LL_RCC_LSI1_Disable();

	LL_RCC_LSE_Disable();

	//LL_PWR_DisableSRAM2Retention();
	//EnterLowPower();

	HAL_SuspendTick();

	LL_PWR_ClearFlag_WU();

	HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN4_HIGH);
	//LL_PWR_SetPowerMode( LL_PWR_MODE_STANDBY );
	LL_PWR_SetPowerMode(LL_PWR_MODE_SHUTDOWN);

	LL_LPM_EnableDeepSleep(); /**< Set SLEEPDEEP bit of Cortex System Control Register */

	/**
	 * This option is used to ensure that store operations are completed
	 */
#if defined ( __CC_ARM)
	__force_stores();
#endif

	__WFI();
#else

	LL_C2_PWR_SetPowerMode(LL_PWR_MODE_SHUTDOWN);
	LL_PWR_DisableBootC2();

	//TL_EvtPacket_t p_rsp;

	//shci_send(ACI_HAL_STACK_RESET, 0, 0, &p_rsp);
	HAL_PWREx_ClearWakeupFlag(PWR_FLAG_WUF4);
	HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN4_HIGH);
	LL_PWR_SetPowerMode(LL_PWR_MODE_SHUTDOWN);
	LL_LPM_EnableDeepSleep();

	__WFI();

#endif


}

void Switch_On_HSI(void)
{

	LL_RCC_HSI_Enable();
	while (!LL_RCC_HSI_IsReady());
	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);
	LL_RCC_SetSMPSClockSource(LL_RCC_SMPS_CLKSOURCE_HSI);
	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI);

	LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);
	while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_0)
	{
	}
	return;
}

void EnterLowPower(void)
{
	/**
	 * This function is called from CRITICAL SECTION
	 */

	while (LL_HSEM_1StepLock(HSEM, CFG_HW_RCC_SEMID));

	if (!LL_HSEM_1StepLock(HSEM, CFG_HW_ENTRY_STOP_MODE_SEMID))
	{
		lowp = 0x10;
		if (LL_PWR_IsActiveFlag_C2DS() || LL_PWR_IsActiveFlag_C2SB())
		{

			/* Release ENTRY_STOP_MODE semaphore */
			LL_HSEM_ReleaseLock(HSEM, CFG_HW_ENTRY_STOP_MODE_SEMID, 0);
			lowp = 0x20;
			/**
			 * The switch on HSI before entering Stop Mode is required on Cut2.0
			 * It is useless from Cut2.1
			 */
			Switch_On_HSI();
		}
	}
	else
	{
		lowp = 0x30;
		/**
		 * The switch on HSI before entering Stop Mode is required on Cut2.0
		 * It is useless from Cut2.1
		 */
		Switch_On_HSI();
	}

	/* Release RCC semaphore */
	LL_HSEM_ReleaseLock(HSEM, CFG_HW_RCC_SEMID, 0);

	return;
}

/**
  * @brief Restore the system to exit stop mode
  * @param none
  * @retval none
  */

void ExitLowPower(void)
{
	/* Release ENTRY_STOP_MODE semaphore */
	LL_HSEM_ReleaseLock(HSEM, CFG_HW_ENTRY_STOP_MODE_SEMID, 0);

	while (LL_HSEM_1StepLock(HSEM, CFG_HW_RCC_SEMID));
	lowp += (LL_RCC_GetSysClkSource() << 8);

	/* Restore the clock configuration of the application in this user section */
	/* USER CODE BEGIN ExitLowPower_1 */
#if ENABLE_RCC_HSE_32M
	if (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSE)
	{
		__HAL_FLASH_SET_LATENCY(FLASH_LATENCY_1);
		while (LL_FLASH_GetLatency() != FLASH_LATENCY_1)
		{
		}
		LL_RCC_HSE_Enable();
		while (!LL_RCC_HSE_IsReady());
		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSE);
		while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSE);
	}
#else
	if (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
	{
		LL_FLASH_SetLatency(LL_FLASH_LATENCY_3);
		while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_3)
		{
		}

		LL_RCC_HSE_Enable();
		while (!LL_RCC_HSE_IsReady());

		if (!LL_RCC_PLL_IsReady())
		{
			LL_RCC_PLL_Enable();
			LL_RCC_PLL_EnableDomain_SYS();
			while (LL_RCC_PLL_IsReady() != 1)
			{
			}
		}

		LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
		while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
		{
		}
		/* USER CODE END ExitLowPower_1 */
	}
#endif
	else
	{
		//lowp = 10;
		/* If the application is not running on HSE restore the clock configuration in this user section */
		/* USER CODE BEGIN ExitLowPower_2 */

		/* USER CODE END ExitLowPower_2 */
	}

	/* Release RCC semaphore */
	LL_HSEM_ReleaseLock(HSEM, CFG_HW_RCC_SEMID, 0);

	return;
}


void PWR_EnterStopMode(void)
{
	/* USER CODE BEGIN PWR_EnterStopMode_1 */

	/* USER CODE END PWR_EnterStopMode_1 */
	  /**
	   * When HAL_DBGMCU_EnableDBGStopMode() is called to keep the debugger active in Stop Mode,
	   * the systick shall be disabled otherwise the cpu may crash when moving out from stop mode
	   *
	   * When in production, the HAL_DBGMCU_EnableDBGStopMode() is not called so that the device can reach best power consumption
	   * However, the systick should be disabled anyway to avoid the case when it is about to expire at the same time the device enters
	   * stop mode ( this will abort the Stop Mode entry ).
	   */
	   //HAL_SuspendTick();

		 /**
		  * This function is called from CRITICAL SECTION
		  */
	EnterLowPower();

	/************************************************************************************
	 * ENTER STOP MODE
	 ***********************************************************************************/
	LL_PWR_SetPowerMode(LL_PWR_MODE_STOP2);

	LL_LPM_EnableDeepSleep(); /**< Set SLEEPDEEP bit of Cortex System Control Register */

	/**
	 * This option is used to ensure that store operations are completed
	 */
#if defined ( __CC_ARM)
	__force_stores();
#endif

	__WFI();

	/* USER CODE BEGIN PWR_EnterStopMode_2 */

	/* USER CODE END PWR_EnterStopMode_2 */
	return;
}

void PWR_ExitStopMode(void)
{
	/**
	 * This function is called from CRITICAL SECTION
	 */
	ExitLowPower();

	//HAL_ResumeTick();
	return;
}


void pm_enter_stop(void)
{

	PWR_EnterStopMode();
	PWR_ExitStopMode();
	//kprint("here\r\n");
}

void pm_enter_sleep(void)
{
	//HAL_SuspendTick();
	//__HAL_FLASH_SLEEP_POWERDOWN_ENABLE();
	//PWR_LOWPOWERREGULATOR_ON  PWR_MAINREGULATOR_ON
	HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
	HAL_PWREx_DisableLowPowerRunMode();
	//HAL_ResumeTick();
}



#pragma endregion

#pragma region 复位源检测
uint8_t pm_get_wakeup_status(void)
{
	uint8_t status = 0;

	uint32_t status1 = HAL_PWREx_GetWakeupFlag(PWR_FLAG_WUF1);
	uint32_t status4 = HAL_PWREx_GetWakeupFlag(PWR_FLAG_WUF4);
	pwkpf("wakeup1 status=0x%x\r\n", (unsigned int)status1);
	pwkpf("wakeup4 status=0x%x\r\n", (unsigned int)status4);


	if (__HAL_PWR_GET_FLAG(PWR_FLAG_FRCBYPI) != RESET)
	{
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_FRCBYPI);
		pwkpf("wakeup SMPS\r\n");

		status |= 0x01;
	}
	else
	{
		status &= 0xfe;
		pwkpf("not wakeup SMPS\r\n");
	}

	if (__HAL_PWR_GET_FLAG(PWR_FLAG_WU) != RESET)
	{
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
		pwkpf("wakeup\r\n");

		status |= 0x01;
	}
	else
	{
		status &= 0xfe;
		pwkpf("not wakeup\r\n");
	}

	if (__HAL_PWR_GET_FLAG(PWR_FLAG_WUF1) != RESET)
	{
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WUF1);
		pwkpf("wakeup1\r\n");

		status |= 0x01;
	}
	else
	{
		status &= 0xfe;
		pwkpf("not wakeup1\r\n");
	}

	if (__HAL_PWR_GET_FLAG(PWR_FLAG_WUF2) != RESET)
	{
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WUF2);
		pwkpf("wakeup2\r\n");

		status |= 0x01;
	}
	else
	{
		status &= 0xfe;
		pwkpf("not wakeup2\r\n");
	}

	if (__HAL_PWR_GET_FLAG(PWR_FLAG_WUF3) != RESET)
	{
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WUF3);
		pwkpf("wakeup3\r\n");

		status |= 0x01;
	}
	else
	{
		status &= 0xfe;
		pwkpf("not wakeup3\r\n");
	}

	if (__HAL_PWR_GET_FLAG(PWR_FLAG_WUF4) != RESET)
	{
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WUF4);
		pwkpf("wakeup4\r\n");

		status |= 0x01;
	}
	else
	{
		status &= 0xfe;
		pwkpf("not wakeup4\r\n");
	}

	if (__HAL_PWR_GET_FLAG(PWR_FLAG_WUF5) != RESET)
	{
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WUF5);
		pwkpf("wakeup5\r\n");

		status |= 0x01;
	}
	else
	{
		status &= 0xfe;
		pwkpf("not wakeup5\r\n");
	}

	if (__HAL_PWR_GET_FLAG(PWR_FLAG_WUFI) != RESET)
	{
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WUFI);
		pwkpf("wakeup internal wakeup line\r\n");

		status |= 0x01;
	}
	else
	{
		status &= 0xfe;
		pwkpf("not wakeup internal wakeup line\r\n");
	}

	if (__HAL_PWR_GET_FLAG(PWR_FLAG_BLEACTI) != RESET)
	{
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_BLEACTI);
		pwkpf("wakeup ble Activity\r\n");

		status |= 0x01;
	}
	else
	{
		status &= 0xfe;
		pwkpf("not wakeup ble Activity\r\n");
	}

	if (__HAL_PWR_GET_FLAG(PWR_FLAG_RFPHASEI) != RESET)
	{
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_RFPHASEI);
		pwkpf("wakeup rf\r\n");

		status |= 0x01;
	}
	else
	{
		status &= 0xfe;
		pwkpf("not wakeup rf\r\n");
	}

	if (__HAL_PWR_GET_FLAG(PWR_FLAG_BHWF) != RESET)
	{
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_BHWF);
		pwkpf("wakeup ble host\r\n");

		status |= 0x01;
	}
	else
	{
		status &= 0xfe;
		pwkpf("not wakeup ble host\r\n");
	}

	if (__HAL_PWR_GET_FLAG(PWR_FLAG_HOLDC2I) != RESET)
	{
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_HOLDC2I);
		pwkpf("wakeup cpu2\r\n");

		status |= 0x01;
	}
	else
	{
		status &= 0xfe;
		pwkpf("not wakeup cpu2\r\n");
	}

	if (__HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET)
	{
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
		pwkpf("standby\r\n");
		status |= 0x02;
	}
	else
	{
		pwkpf("not standby\r\n");
		status &= 0xfD;
	}

	if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET)
	{

		__HAL_RCC_CLEAR_RESET_FLAGS();
		pwkpf("iwdg reset\r\n");
		status |= 0x04;
	}
	else
	{
		pwkpf("not iwdg reset\r\n");
		status &= 0xfB;
	}

	if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST) != RESET)
	{
		__HAL_RCC_CLEAR_RESET_FLAGS();
		pwkpf("Software reset\r\n");
		status |= 0x04;
	}
	else
	{
		pwkpf("not Software reset\r\n");
		status &= 0xfB;
	}


	if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST) != RESET)
	{
		__HAL_RCC_CLEAR_RESET_FLAGS();
		pwkpf("pin reset\r\n");
		status |= 0x04;
	}
	else
	{
		pwkpf("not pin reset\r\n");
		status &= 0xfB;
	}

	if (__HAL_RCC_GET_FLAG(RCC_FLAG_BORRST) != RESET)
	{
		__HAL_RCC_CLEAR_RESET_FLAGS();
		pwkpf("bor reset\r\n");
		status |= 0x04;
	}
	else
	{
		pwkpf("not bor reset\r\n");
		status &= 0xfB;
	}


	if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET)
	{
		__HAL_RCC_CLEAR_RESET_FLAGS();
		pwkpf("iwd reset\r\n");
		status |= 0x04;
	}
	else
	{
		pwkpf("not iwd reset\r\n");
		status &= 0xfB;
	}

	if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST) != RESET)
	{
		__HAL_RCC_CLEAR_RESET_FLAGS();
		pwkpf("wwd reset\r\n");
		status |= 0x04;
	}
	else
	{
		pwkpf("not wwd reset\r\n");
		status &= 0xfB;
	}

	if (__HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST) != RESET)
	{
		__HAL_RCC_CLEAR_RESET_FLAGS();
		pwkpf("lpw reset\r\n");
		status |= 0x04;
	}
	else
	{
		pwkpf("not lpw reset\r\n");
		status &= 0xfB;
	}
	return status;
}
#pragma endregion

#pragma region wakeup gpio 配置

void pwr_gpio_init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	WAKEUP_GPIO_CLK_ENABLE();

	GPIO_InitStruct.Pin = WAKEUP_GPIO_PIN;
	GPIO_InitStruct.Mode = WAKEUP_GPIO_MODE;
	GPIO_InitStruct.Pull = WAKEUP_GPIO_PULL;
	GPIO_InitStruct.Speed = WAKEUP_GPIO_SPEED;
	GPIO_InitStruct.Alternate = WAKEUP_GPIO_ALTERNATE;
	HAL_GPIO_Init(WAKEUP_GPIO_PORT, &GPIO_InitStruct);
}

void pwr_gpio_deinit(void)
{
	HAL_GPIO_DeInit(WAKEUP_GPIO_PORT, WAKEUP_GPIO_PIN);
}


//检查是否被唤醒
uint8_t pwr_check_wakeup(void)
{
	return WAKEUP_INPUT();
}
#pragma endregion

#pragma region 故障检测，看门狗，软狗
//防止卡死在中断中
//5秒钟运行一次
//检测10秒钟内，main函数是否正常运行,否则复位
//20210824 改成4秒钟，检测一次。8秒钟循环没有运行，复位
void pwr_check_mcu_alive(void)
{
	static uint32_t sLastMcuRunCount = 0;
	static uint32_t count = 0;
	//pwkpf("here\r\n");
	count++;
	//软看门狗
	if (count == 2)//8秒钟
	{
		count = 0;
		if (sLastMcuRunCount == sMcuRunCount)
		{
			pwkpf("[main]: dead1!\r\n");
			pwr_reset_mcu();
		}

		sLastMcuRunCount = sMcuRunCount;
	}
}

#pragma endregion

#pragma region 根据时间进行电量计算

//根据时间计算出功耗情况
#define MAX_BATTERY_TIME   (4*60*60) //最大的运行时间
uint8_t pwr_get_power_relative(void)
{
	//把空载时间合算为工作时间,假定两者能耗相差50倍
	__SYSTEM_TIME32_TypeDef t;
	sys_get_time(&t);

	uint32_t tm = (uint32_t)(t.WorkTime) + t.RunTime / 50;
	uint32_t rl = tm * 100 / MAX_BATTERY_TIME;
	if (rl > 100)
	{
		rl = 100;
	}
	return 100 - rl;
}

#pragma endregion
