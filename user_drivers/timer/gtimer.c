/*******************************************************************************
* file    gtimer.c
* author  mackgim
* version 1.0.0
* date
* brief	计时系统
*		tick 时间
*		1 tick =   1/ (FREQ / (CLK_DIV))
*		当前配置,  1 tick = 1 * 4 / 32768(不确定) = 122.070 us
*******************************************************************************/

#include "gtimer.h"
#include "platform.h"
#include "standard_lib.h"
#include <time.h>
#include "hw_timerserver.h"
#include "app_conf.h"


#define ENABLE_LPWK_DEUBG 1

#ifdef DEBUG
#if ENABLE_LPWK_DEUBG
#define gtkpt(...) kprint(__VA_ARGS__)
#define gtnpt(...) nprint(__VA_ARGS__)
#else
#define gtkpt(...)
#define gtnpt(...) 
#endif
#else
#define gtkpt(...)
#define gtnpt(...) 
#endif


#pragma region 函数

uint8_t lptim_init(void);
uint8_t lptim_deinit(void);

uint8_t ts_init(void);
uint8_t ts_deinit(void);
void delay_init(void);

#pragma endregion




#pragma region Gtimer

__GTIMER_CALLBACK_TypeDef  sGtimerCb = { NULL,NULL };

void gtimer_init(void)
{
	lptim_init();
	ts_init();
	delay_init();
}

void gtimer_deinit(void)
{
	lptim_deinit();
	ts_deinit();
}

void gtimer_register_callback(void* cb)
{
	sGtimerCb = *(__GTIMER_CALLBACK_TypeDef*)cb;
}

#pragma endregion


#pragma region 重定向ST时钟

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
	//uint8_t ret = lptim_init();
	//if (ret == STD_SUCCESS)
	//{
	//	return HAL_OK;
	//}

	//return HAL_ERROR;
	return HAL_OK;

}

uint32_t HAL_GetTick(void)
{
	return Clock32_Time();
}

void HAL_Delay(uint32_t Delay)
{
	Clock_Wait((uint64_t)Delay);
}


#pragma endregion


#pragma region 系统计时模块, 基于lptim, 最小尺度为122us, lptim tick 

#define GTIMER_LPTIM_IRQn				LPTIM1_IRQn
#define GTIMER_LPTIM_IRQHandler			LPTIM1_IRQHandler
#define GTIMER_LPTIM					LPTIM1

#define GTIMER_LPTIM_PRESCALER			LPTIM_PRESCALER_DIV4
#define GTIMER_LPTIM_FREQ				(LSE_VALUE/4)
#define GTIMER_LPTIM_TICK_PERIOD		(0xffff)

#define GET_TICK()						lptim_get_tick()	

#define TICKS_TO_US(n)					(((n) * 1000000) / GTIMER_LPTIM_FREQ)
#define TICKS_TO_MS(n)					(((n) * 1000) / GTIMER_LPTIM_FREQ)
#define TICKS_TO_S(n)					((n) / GTIMER_LPTIM_FREQ)

#define MS_TO_TICK(n)					(((n) * GTIMER_LPTIM_FREQ) / 1000)
#define US_TO_TICK(n)					(((n) * GTIMER_LPTIM_FREQ) / 1000000)

#define REAL_TICKS_TO_S()				TICKS_TO_S(Clock_Tick())
#define REAL_TICKS_TO_MS()				TICKS_TO_MS(Clock_Tick())



#pragma region 变量

static LPTIM_HandleTypeDef sLPTimHandle;
static uint8_t sLptimFlag = false;
static volatile uint32_t sTimIsrCount = 0;

#pragma endregion

#pragma region 函数

__STATIC_INLINE uint64_t lptim_get_tick(void);

void lptim_mspinit(LPTIM_HandleTypeDef* hlptim);
void lptim_mspdeinit(LPTIM_HandleTypeDef* hlptim);
void lptim_auto_reload_match_cb(LPTIM_HandleTypeDef* hlptim);
void lptim_auto_reload_write_cb(LPTIM_HandleTypeDef* hlptim);

#pragma endregion

#pragma region 功能

uint8_t lptim_init(void)
{
	if (sLptimFlag)
	{
		return STD_SUCCESS;
	}
	sLptimFlag = true;
	//gtkpt("freq=%u, us in tick=%u, count=%u\r\n", (unsigned int)freq, (unsigned int)sTimUSPerTick, (unsigned int)sTimPeriodCount);

	sLPTimHandle.Instance = GTIMER_LPTIM;
	sLPTimHandle.Init.Clock.Source = LPTIM_CLOCKSOURCE_APBCLOCK_LPOSC;
	sLPTimHandle.Init.Clock.Prescaler = GTIMER_LPTIM_PRESCALER;
	sLPTimHandle.Init.Trigger.Source = LPTIM_TRIGSOURCE_SOFTWARE;
	sLPTimHandle.Init.OutputPolarity = LPTIM_OUTPUTPOLARITY_HIGH;
	sLPTimHandle.Init.UpdateMode = LPTIM_UPDATE_IMMEDIATE;
	sLPTimHandle.Init.CounterSource = LPTIM_COUNTERSOURCE_INTERNAL;

	HAL_LPTIM_RegisterCallback(&sLPTimHandle, HAL_LPTIM_MSPINIT_CB_ID, lptim_mspinit);
	HAL_LPTIM_RegisterCallback(&sLPTimHandle, HAL_LPTIM_MSPDEINIT_CB_ID, lptim_mspdeinit);

	HAL_LPTIM_Init(&sLPTimHandle);

	//HAL_LPTIM_RegisterCallback(&sLPTimHandle, HAL_LPTIM_COMPARE_MATCH_CB_ID, lptim_compare_match_cb);
	HAL_LPTIM_RegisterCallback(&sLPTimHandle, HAL_LPTIM_AUTORELOAD_MATCH_CB_ID, lptim_auto_reload_match_cb);
	HAL_LPTIM_RegisterCallback(&sLPTimHandle, HAL_LPTIM_AUTORELOAD_WRITE_CB_ID, lptim_auto_reload_write_cb);


	HAL_LPTIM_Counter_Start_IT(&sLPTimHandle, GTIMER_LPTIM_TICK_PERIOD);
	gtkpt("ok\r\n");
	return STD_SUCCESS;
}

uint8_t lptim_deinit(void)
{
	if (!sLptimFlag)
	{
		return STD_SUCCESS;
	}
	sTimIsrCount = 0;
	sLPTimHandle.Instance = GTIMER_LPTIM;
	HAL_LPTIM_DeInit(&sLPTimHandle);
	sLptimFlag = false;
	return STD_SUCCESS;
}

#pragma endregion

#pragma region 回调

void lptim_mspinit(LPTIM_HandleTypeDef* hlptim)
{

	LL_RCC_SetLPTIMClockSource(LL_RCC_LPTIM1_CLKSOURCE_LSE);

	__HAL_RCC_LPTIM1_CLK_ENABLE();
	HAL_NVIC_SetPriority(GTIMER_LPTIM_IRQn, GTIMER_LPTIM_NVIC_PreemptionPriority, GTIMER_LPTIM_NVIC_SubPriority);
	HAL_NVIC_EnableIRQ(GTIMER_LPTIM_IRQn);
}

void lptim_mspdeinit(LPTIM_HandleTypeDef* hlptim)
{

	/* LPTIM1 interrupt DeInit */
	HAL_NVIC_DisableIRQ(LPTIM1_IRQn);
	/* Peripheral clock disable */
	__HAL_RCC_LPTIM1_CLK_DISABLE();
}

#pragma endregion

#pragma region LPTIM 中断


void GTIMER_LPTIM_IRQHandler(void)
{
	/* LPTIM in time Base mode */
	HAL_LPTIM_IRQHandler(&sLPTimHandle);

}


void lptim_auto_reload_match_cb(LPTIM_HandleTypeDef* hlptim)
{
	sTimIsrCount++;
	if (sGtimerCb.irq_cb != NULL)
	{
		sGtimerCb.irq_cb();
	}
#if DEBUG1
	static uint32_t t = 0;
	uint32_t t2 = Clock_Time();
	gtnpt("%u\r\n", (unsigned int)(t2 - t));
	t = t2;
#endif
}

void lptim_auto_reload_write_cb(LPTIM_HandleTypeDef* hlptim)
{
#ifdef DEBUG
	uint64_t time = Clock_Time();
	gtkpt("%u\r\n", (unsigned int)time);
#endif
	//lptim_write_arr_flag = 1;
}


#pragma endregion

#pragma region LPTIM Tick函数

//获取当前tick

__STATIC_INLINE uint64_t lptim_get_tick(void)
{
	ATOMIC_SECTION_BEGIN();
	uint32_t cnt0, cnt1, circle;

	do
	{
		circle = sTimIsrCount;
		cnt0 = sLPTimHandle.Instance->CNT;// HAL_LPTIM_ReadCounter(&sLPTimHandle);
		cnt1 = sLPTimHandle.Instance->CNT;
		if (__HAL_LPTIM_GET_FLAG(&sLPTimHandle, LPTIM_FLAG_ARRM) != RESET)
		{
			circle++;
			cnt0 = sLPTimHandle.Instance->CNT;// HAL_LPTIM_ReadCounter(&sLPTimHandle);
			cnt1 = sLPTimHandle.Instance->CNT;
		}
	} while (cnt0 != cnt1);
	uint64_t temp = (uint64_t)circle * GTIMER_LPTIM_TICK_PERIOD + (uint64_t)cnt0;

	ATOMIC_SECTION_END();
	return temp;
}

#pragma endregion

#pragma endregion

#pragma region 定时进程模块，基于RTC, 最小尺度为122us, timer server


#pragma region 变量
static RTC_HandleTypeDef sTSRtcHandle;
#pragma endregion

#pragma region 函数
static void ts_mspinit(RTC_HandleTypeDef* hrtc);
static void ts_mspdeinit(RTC_HandleTypeDef* hrtc);
static uint32_t ts_ms_to_tick(uint32_t ms);
static uint32_t ts_us_to_tick(uint32_t us);
#pragma endregion

#pragma region 功能

uint8_t ts_init(void)
{
	sTSRtcHandle.Instance = RTC;
	sTSRtcHandle.Init.HourFormat = RTC_HOURFORMAT_24;
	sTSRtcHandle.Init.AsynchPrediv = CFG_RTC_ASYNCH_PRESCALER;
	sTSRtcHandle.Init.SynchPrediv = CFG_RTC_SYNCH_PRESCALER;
	sTSRtcHandle.Init.OutPut = RTC_OUTPUT_DISABLE;
	sTSRtcHandle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
	sTSRtcHandle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
	sTSRtcHandle.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;

	HAL_RTC_RegisterCallback(&sTSRtcHandle, HAL_RTC_MSPINIT_CB_ID, ts_mspinit);
	HAL_RTC_RegisterCallback(&sTSRtcHandle, HAL_RTC_MSPDEINIT_CB_ID, ts_mspdeinit);

	if (HAL_RTC_Init(&sTSRtcHandle) != HAL_OK)
	{
		gtkpt("failed to init\r\n");
	}
	gtkpt("ok\r\n");

	/* Disable RTC registers write protection */
	LL_RTC_DisableWriteProtection(RTC);

	LL_RTC_WAKEUP_SetClock(RTC, CFG_RTC_WUCKSEL_DIVIDER);

	/* Enable RTC registers write protection */
	LL_RTC_EnableWriteProtection(RTC);

#ifdef DEBUG
	uint32_t wucksel = LL_RTC_WAKEUP_GetClock(RTC);
	gtkpt("wucksel=0x%x, us in tick=%u, ap=%u, sp=%u\r\n", (unsigned int)wucksel, (unsigned int)1000000 / CFG_RTC_FREQUENCY, (unsigned int)CFG_RTC_ASYNCH_PRESCALER,
		(unsigned int)CFG_RTC_SYNCH_PRESCALER);
#endif

	HW_TS_Init(hw_ts_InitMode_Full, &sTSRtcHandle); /**< Initialize the TimerServer */

	return STD_SUCCESS;
}

uint8_t ts_deinit(void)
{
	sTSRtcHandle.Instance = RTC;
	if (HAL_RTC_DeInit(&sTSRtcHandle) != HAL_OK)
	{
		return STD_FAILED;
	}
	return STD_SUCCESS;
}

uint8_t ts_create(uint32_t TimerProcessID, uint8_t* pTimerId, TS_Mode_t TimerMode, void* pftimeout_handler)
{

	HW_TS_ReturnStatus_t ret = HW_TS_Create(TimerProcessID, pTimerId, (HW_TS_Mode_t)TimerMode, (HW_TS_pTimerCb_t)pftimeout_handler);
	if (ret == hw_ts_Successful)
	{
		return STD_SUCCESS;
	}
	gtkpt("failed to create ts\r\n");
	return STD_FAILED;
}

void ts_start_ms(uint8_t timer_id, uint32_t timeout_ms)
{
	HW_TS_Start(timer_id, ts_ms_to_tick(timeout_ms));
}

void ts_start_us(uint8_t timer_id, uint32_t timeout_us)
{
	HW_TS_Start(timer_id, ts_us_to_tick(timeout_us));
}

void ts_stop(uint8_t timer_id)
{
	HW_TS_Stop(timer_id);
}

uint32_t ts_us_to_tick(uint32_t us)
{
	return (uint32_t)((uint64_t)us * CFG_RTC_FREQUENCY / 1000000);
}

uint32_t ts_ms_to_tick(uint32_t ms)
{
	return (uint32_t)((uint64_t)ms * CFG_RTC_FREQUENCY / 1000);
}
#pragma endregion

#pragma region 回调

void ts_mspinit(RTC_HandleTypeDef* hrtc)
{

	if (hrtc->Instance == RTC)
	{

		if (LL_RCC_GetRTCClockSource() != LL_RCC_RTC_CLKSOURCE_LSE)
		{

			/* Enable write access to Backup domain */
			//HAL_PWR_EnableBkUpAccess();
			LL_PWR_EnableBkUpAccess();
			/**
			 *  Write twice the value to flush the APB-AHB bridge
			 *  This bit shall be written in the register before writing the next one
			 */
			LL_PWR_EnableBkUpAccess();
			/* Store the content of BDCR register before the reset of Backup Domain */
			uint32_t bdcr = LL_RCC_ReadReg(BDCR);
			/* RTC Clock selection can be changed only if the Backup Domain is reset */
			LL_RCC_ForceBackupDomainReset();
			LL_RCC_ReleaseBackupDomainReset();

			for (uint32_t i = 0; i < 0xf; i++)
			{
				__NOP();
			}
			//LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSE);

			/* Set the value of the clock source selection */
			MODIFY_REG(bdcr, RCC_BDCR_RTCSEL, RCC_RTCCLKSOURCE_LSE);

			/* Restore the content of BDCR register */
			LL_RCC_WriteReg(BDCR, bdcr);

			gtkpt("BDCR=0x%x\r\n", (unsigned int)LL_RCC_ReadReg(BDCR));

			if (LL_RCC_LSE_IsEnabled() == 1U)
			{
				gtkpt("check lse\r\n");

				/* Wait till LSE is ready */
				while (!LL_RCC_LSE_IsReady());
			}

			gtkpt("set LSE as rtc clock\r\n");
		}
		else
		{
			gtkpt("rtc clock is LSE\r\n");
		}
		/* Peripheral clock enable */
		__HAL_RCC_RTC_ENABLE();
		__HAL_RCC_RTCAPB_CLK_ENABLE();

		HAL_RTCEx_EnableBypassShadow(hrtc);

	}
}

void ts_mspdeinit(RTC_HandleTypeDef* hrtc)
{
	HAL_NVIC_DisableIRQ(CFG_HW_TS_RTC_WAKEUP_HANDLER_ID);
	__HAL_RCC_RTC_DISABLE();
	__HAL_RCC_RTCAPB_CLK_DISABLE();
	//HAL_PWR_DisableBkUpAccess();
}
#pragma endregion

#pragma region 中断
void RTC_WKUP_IRQHandler(void)
{
	HAL_RTCEx_WakeUpTimerIRQHandler(&sTSRtcHandle);
}
#pragma endregion

#pragma endregion

#pragma region 延时相关函数

volatile uint8_t sDelayTSCplt = 0;
//static uint8_t sDelayTSID = 0;
//
//void delay_timeout_handler(void);

void delay_init(void)
{
	//ts_create(0, &(sDelayTSID), TS_SingleShot, delay_timeout_handler);
}

void delay_timeout_handler(void)
{
	sDelayTSCplt = 0;
}

void Clock_Wait_us(uint64_t us)
{
#if 1

	uint64_t timeout = Clock_Tick() + US_TO_TICK(us);

	while (timeout > Clock_Tick())
	{

	}
#else
	//sDelayTSCplt = 1说明有其他的延时在使用，不允许同时使用延时
	while (sDelayTSCplt) {};

	sDelayTSCplt = 1;
	ts_start_us(sDelayTSID, us);
	do
	{
		if (sGtimerCb.sleep_cb != NULL)
		{
			sGtimerCb.sleep_cb();
		}

	} while (sDelayTSCplt);
#endif
}

//只能循环中延时使用
void Clock_Wait(uint64_t ms)
{
	Clock_Wait_us((uint64_t)ms * 1000);
}
#pragma endregion

#pragma region Clock Time, ms级


static uint64_t sLastClockTickNow = 0;
uint64_t Clock_Tick(void)
{
	return GET_TICK() - sLastClockTickNow;
}


uint32_t Clock32_Time(void)
{
	return REAL_TICKS_TO_MS();
}


uint64_t Clock_Time(void)
{
	return REAL_TICKS_TO_MS();
}

#pragma endregion

#pragma region 世界运行时间计时，ms级

//struct tm {
//
//	int tm_sec;       /* 秒 – 取值区间为[0,59] */
//
//	int tm_min;       /* 分 - 取值区间为[0,59] */
//
//	int tm_hour;      /* 时 - 取值区间为[0,23] */
//
//	int tm_mday;     /* 一个月中的日期 - 取值区间为[1,31] */
//
//	int tm_mon;     /* 月份（从一月开始，0代表一月） - 取值区间为[0,11] */
//
//	int tm_year;     /* 年份，其值等于实际年份减去1900 */
//
//	int tm_wday;    /* 星期 – 取值区间为[0,6]，其中0代表星期天，1代表星期一 */
//
//	int tm_yday;    /* 从每年1月1日开始的天数– 取值区间[0,365]，其中0代表1月1日 */
//
//	int tm_isdst;    /* 夏令时标识符，夏令时tm_isdst为正；不实行夏令时tm_isdst为0 */
//
//};

static	__UNIX_TIME_TypeDef sRealTime = { 0 };

void rtc_set_unix_time_ms(uint64_t time)
{
	sRealTime.LocalTimeStamp = Clock_Time();
	sRealTime.BasicTimeStamp = time;
	gtkpt("unix time=%u\r\n", (unsigned int)sRealTime.BasicTimeStamp);
}

uint64_t rtc_get_stamp(void)
{
	uint64_t cc = Clock_Time();
	uint64_t diff = 0;
	if (cc >= sRealTime.LocalTimeStamp)
	{
		diff = cc - sRealTime.LocalTimeStamp;
	}
	else
	{
		diff = 0xffffffffffffffff + cc - sRealTime.LocalTimeStamp;
	}

	return ((sRealTime.BasicTimeStamp + diff) / 1000);
}

uint64_t rtc_get_stamp_ms(void)
{
	uint64_t cc = Clock_Time();
	uint64_t diff = 0;
	if (cc > sRealTime.LocalTimeStamp)
	{
		diff = cc - sRealTime.LocalTimeStamp;
	}
	else
	{
		diff = 0xffffffffffffffff - sRealTime.LocalTimeStamp + cc;
	}

	return (sRealTime.BasicTimeStamp + diff);

}

uint32_t rtc_time_to_stamp(__DATE_TIME_TypeDef* dt)
{

	struct tm tim = { 0 };
	time_t tt;
	tim.tm_year = dt->Year - 1900;
	tim.tm_mon = dt->Month - 1;
	tim.tm_mday = dt->Date;
	tim.tm_hour = dt->Hours;
	tim.tm_min = dt->Minutes;
	tim.tm_sec = dt->Seconds;
	tt = mktime(&tim);
	return tt;
}

uint32_t rtc_localtime_to_stamp(__DATE_TIME_TypeDef* dt)
{

	struct tm tim = { 0 };
	time_t tt;
	tim.tm_year = dt->Year - 1900;
	tim.tm_mon = dt->Month - 1;
	tim.tm_mday = dt->Date;
	tim.tm_hour = dt->Hours;
	tim.tm_min = dt->Minutes;
	tim.tm_sec = dt->Seconds;
	tt = mktime(&tim);
	tt = tt - 28800;
	return tt;
}

__DATE_TIME_TypeDef rtc_get_time(void)
{
	time_t ct = (time_t)rtc_get_stamp();

	//struct tm* tp = localtime((time_t*)&ct);
	//struct tm* tp = gmtime((time_t*)&ct);
	struct tm tc;
	struct tm* tp = &tc;
	gmtime_r((time_t*)&ct, &tc);

	__DATE_TIME_TypeDef x;
	x.Year = (uint16_t)tp->tm_year + 1900;
	x.Month = tp->tm_mon + 1;
	x.Date = tp->tm_mday;
	x.Hours = tp->tm_hour;
	x.Minutes = tp->tm_min;
	x.Seconds = tp->tm_sec;

	return x;
}

__DATE_TIME_TypeDef rtc_get_localtime(void)
{
	time_t ct = (time_t)rtc_get_stamp() + 28800;

	//struct tm* tp = localtime((time_t*)&ct); //malloc 返回0
	//struct tm* tp = gmtime((time_t*)&ct);

	struct tm tc;
	struct tm* tp = &tc;
	//struct tm* tp1 = gmtime_r((time_t*)&ct, &tc);
	gmtime_r((time_t*)&ct, &tc);
	//gtkpt("tp1=0x%x, tp=0x%x\r\n", (unsigned int)tp, (unsigned int)tp1);
	__DATE_TIME_TypeDef x;
	x.Year = (uint16_t)tp->tm_year + 1900;
	x.Month = tp->tm_mon + 1;
	x.Date = tp->tm_mday;
	x.Hours = tp->tm_hour;
	x.Minutes = tp->tm_min;
	x.Seconds = tp->tm_sec;

	return x;
}

//比较时间差是否超过设定范围
uint8_t rtc_cmp_timeout(uint64_t t1, uint64_t t2, uint64_t timeout)
{
	uint64_t diff;
	if (t1 > t2)
	{
		diff = t1 - t2;

	}
	else
	{
		diff = t2 - t1;
	}

	if (diff > timeout)
	{
		return true;
	}
	else
	{
		return false;
	}
}
#pragma endregion

#pragma region 系统运行时间计时，ms级


static __SYSTEM_TIME64_TypeDef sSystemBaseTick = { 0 }; //保持在flash中的系统起始时间
static __SYSTEM_TIME64_TypeDef sSystemUpTick = { 0 };   //从这次开机起运行的时间
static uint64_t sLastSysTick = 0;//记录的最新tick
//tick转ms
uint64_t sys_tick_to_ms(uint64_t tick)
{
	return TICKS_TO_MS(tick);
}

uint64_t sys_ms_to_tick(uint64_t ms)
{
	return MS_TO_TICK(ms);
}


void sys_set_base_tick(uint64_t run, uint64_t work)
{
	sSystemBaseTick.RunTime = run;
	sSystemBaseTick.WorkTime = work;
}

//清除时间数据
void sys_clear_time(void)
{
	ATOMIC_SECTION_BEGIN();

	uint64_t rtctime = rtc_get_stamp_ms(); //保留世界时间
	memset(&sSystemBaseTick, 0, sizeof(sSystemBaseTick));
	memset(&sSystemUpTick, 0, sizeof(sSystemUpTick));
	sLastClockTickNow = GET_TICK();
	sLastSysTick = 0;
	ATOMIC_SECTION_END();

	rtc_set_unix_time_ms(rtctime); //重新同步世界时间
}


//计算时间差
uint64_t sys_diff_time(void)
{
	uint64_t t = Clock_Tick();
	uint64_t diff = t - sLastSysTick;
	sLastSysTick = t;
	return diff;
}

//更新工作时间
void sys_update_work_time(uint64_t t)
{
	sSystemUpTick.WorkTime += t;
}

//获取系统时间参数,单位S
void sys_get_time(__SYSTEM_TIME32_TypeDef* t)
{
	//获取工作时间
	uint64_t diff = sys_diff_time();
	sSystemUpTick.WorkTime += diff;

	//获取秒时间
	t->UpTime = (uint32_t)REAL_TICKS_TO_S();
	t->WorkTime = TICKS_TO_S(sSystemBaseTick.WorkTime + sSystemUpTick.WorkTime);
	t->RunTime = TICKS_TO_S(sSystemBaseTick.RunTime + Clock_Tick());

	//gtkpt("uptime=%lu, runtime=%lu, worktime=%lu\r\n", t->UpTime, t->RunTime, t->WorkTime);
}

//获取系统时间tick
void sys_get_tick(__SYSTEM_TIME64_TypeDef* t)
{
	//获取工作时间
	uint64_t diff = sys_diff_time();
	sSystemUpTick.WorkTime += diff;

	//获取tick时间

	t->UpTime = Clock_Tick();
	t->WorkTime = sSystemBaseTick.WorkTime + sSystemUpTick.WorkTime;
	t->RunTime = sSystemBaseTick.RunTime + Clock_Tick();
}
#pragma endregion

