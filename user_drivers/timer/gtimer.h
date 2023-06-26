/*******************************************************************************
* file    gtimer_if.h
* author  mackgim
* version 1.0.0
* date
* brief  低功耗tim
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GTIMER_IF_H
#define __GTIMER_IF_H

#include "stdint.h"


typedef struct
{
	uint16_t Year;     /*!< Specifies the RTC Date Year.
					  This parameter must be a number between Min_Data = 0 and Max_Data = 99 */

	uint8_t Month;    /*!< Specifies the RTC Date Month (in BCD format).
					  This parameter can be a value of @ref RTC_Month_Date_Definitions */

	uint8_t Date;     /*!< Specifies the RTC Date.
					  This parameter must be a number between Min_Data = 1 and Max_Data = 31 */

	uint8_t WeekDay;  /*!< Specifies the RTC Date WeekDay.
					  This parameter can be a value of @ref RTC_WeekDay_Definitions */


	uint8_t Hours;            /*!< Specifies the RTC Time Hour.
							  This parameter must be a number between Min_Data = 0 and Max_Data = 12 if the RTC_HourFormat_12 is selected.
							  This parameter must be a number between Min_Data = 0 and Max_Data = 23 if the RTC_HourFormat_24 is selected  */

	uint8_t Minutes;          /*!< Specifies the RTC Time Minutes.
							  This parameter must be a number between Min_Data = 0 and Max_Data = 59 */

	uint8_t Seconds;          /*!< Specifies the RTC Time Seconds.
							  This parameter must be a number between Min_Data = 0 and Max_Data = 59 */


}__DATE_TIME_TypeDef;


typedef  struct {
	uint64_t		BasicTimeStamp;//同步时间戳
	uint64_t		LocalTimeStamp;//本地同步时间戳,单位ms
} __UNIX_TIME_TypeDef;

typedef struct
{
	uint64_t RunTime;   //总运行时间,单位为内部tick
	uint64_t UpTime;    //开机时间 ,单位为内部tick
	uint64_t WorkTime;  //工作时间 ,单位为内部tick
} __SYSTEM_TIME64_TypeDef;

typedef struct
{
	uint32_t RunTime;   //总运行时间, 单位为s
	uint32_t UpTime;    //开机时间,  单位为s
	uint32_t WorkTime;  //工作时间,  单位为s
} __SYSTEM_TIME32_TypeDef;

#define CLOCK_SECOND (1000)

typedef struct {
	void(*irq_cb)(void);
	void(*sleep_cb)(void);
}__GTIMER_CALLBACK_TypeDef;

#pragma region 功能函数

void gtimer_init(void);
void gtimer_deinit(void);
void gtimer_register_callback(void* cb);

#pragma endregion


#pragma region 定时进程模块，基于RTC

typedef enum
{
	TS_SingleShot,
	TS_Repeated
} TS_Mode_t;

uint8_t ts_create(uint32_t TimerProcessID, uint8_t* pTimerId, TS_Mode_t TimerMode, void* pftimeout_handler);
void ts_start_ms(uint8_t timer_id, uint32_t timeout_ms);
void ts_start_us(uint8_t timer_id, uint32_t timeout_us);
void ts_stop(uint8_t timer_id);

#pragma endregion


#pragma region 延时模块

void Clock_Wait(uint64_t ms);
void Clock_Wait_us(uint64_t us);

#pragma endregion

#pragma region Clock Time

uint64_t Clock_Tick(void);//系统计时
uint32_t Clock32_Time(void);
uint64_t Clock_Time(void);

#pragma endregion


#pragma region 世界运行时间计时，ms级

void rtc_set_unix_time_ms(uint64_t time);

uint64_t rtc_get_stamp(void);
uint64_t rtc_get_stamp_ms(void);

uint32_t rtc_time_to_stamp(__DATE_TIME_TypeDef* dt);
uint32_t rtc_localtime_to_stamp(__DATE_TIME_TypeDef* dt);

__DATE_TIME_TypeDef rtc_get_time(void);
__DATE_TIME_TypeDef rtc_get_localtime(void);

uint8_t rtc_cmp_timeout(uint64_t t1, uint64_t t2, uint64_t timeout);

#pragma endregion

#pragma region 系统运行时间计时，ms级

uint64_t sys_tick_to_ms(uint64_t tick);
uint64_t sys_ms_to_tick(uint64_t ms);
void sys_set_base_tick(uint64_t run, uint64_t work);
void sys_clear_time(void);
uint64_t sys_diff_time(void);
void sys_update_work_time(uint64_t t);
void sys_get_time(__SYSTEM_TIME32_TypeDef* t);
void sys_get_tick(__SYSTEM_TIME64_TypeDef* t);

#pragma endregion



#endif /* __GTIMER_IF_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
