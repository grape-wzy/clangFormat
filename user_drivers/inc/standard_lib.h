/*******************************************************************************
* file    standard_lib.h
* author  mackgim
* version 1.0.0
* date
* brief   标准库
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STANDARD_LIB_H
#define __STANDARD_LIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
//#include <time.h>
#include <inttypes.h>
#include "user_global.h"
#include "cmsis_compiler.h"

#ifdef USE_FREERTOS
#include "cmsis_os.h"
#endif

#define STD_HELLO            (0x01)
#define STD_ERROR            (0x02)
#define STD_SUCCESS          (0x03)
#define STD_FAILED           (0x04)
#define STD_BUSY             (0x05)
#define STD_DENIED           (0x06)
#define STD_PING             (0x07)
#define STD_DO_NOTHING       (0x08)
#define STD_NEXT             (0x09)
#define STD_NODATA           (0x0a)
#define STD_TIMEOUT          (0x0b)
#define STD_INVALID_ARG      (0x0c)
#define STD_NO_SPACE         (0x0d)
#define STD_NO_INIT          (0x0e)

#define SIZE_OFF_SET(s, m) (uint32_t)(&(((s *)0)->m))
#define SIZE_OF_STRUCT(s, m) sizeof((((s *)0)->m))



#pragma region 对齐判断

//#define is_long_long_aligned(x)   (((uintptr_t)x & 0x07) == 0)
//#define is_word_aligned(x)		  (((uintptr_t)x & 0x03) == 0)

__STATIC_INLINE uint8_t is_long_long_aligned(void const *p)
{
    return (((uintptr_t)p & 0x07) == 0);
}

__STATIC_INLINE uint8_t is_word_aligned(void const *p)
{
    return (((uintptr_t)p & 0x03) == 0);
}

#pragma endregion


#pragma region ATOMIC

#define ATOMIC_SECTION_BEGIN() uint32_t uwPRIMASK_Bit = __get_PRIMASK(); \
                                __disable_irq(); \
/* Must be called in the same or in a lower scope of ATOMIC_SECTION_BEGIN */

#define ATOMIC_SECTION_END() __set_PRIMASK(uwPRIMASK_Bit)

#pragma endregion


#ifdef DEBUG


//带函数名和时间,log输出, dma传输打印
#define LOG_ENTER_CRITICAL_SECTION( )       ATOMIC_SECTION_BEGIN()

#define LOG_EXIT_CRITICAL_SECTION( )        ATOMIC_SECTION_END()


//#define TraceB0(func_name, ...) do {printf("[%u]:", (unsigned int)Clock_Time()); printf(__VA_ARGS__);} while(0)
//#define TraceB1(func_name, ...) do {printf("[%s][%u]: ", (char *)func_name, (unsigned int)Clock_Time()); printf(__VA_ARGS__);} while(0)
//#define TraceBX(flags, func_name, ...) TraceB ##flags(func_name, ##__VA_ARGS__)
//#define fprint(flag, ...)   TraceBX(flag, __func__, ##__VA_ARGS__)

#define TraceA0(...)                                     \
    do {                                                 \
        printf("[%lu]:", (long unsigned int)time_print); \
        printf(__VA_ARGS__);                             \
    } while (0)

#define TraceA1(...)                                                            \
    do {                                                                        \
        printf("[%s][%lu]: ", (char *)__func__, (long unsigned int)time_print); \
        printf(__VA_ARGS__);                                                    \
    } while (0)

#define TraceAX(flags, ...) TraceA##flags(__VA_ARGS__)

//带函数名和时间,log输出, dma传输打印
#define kprint(...)                         \
    do {                                    \
        uint64_t time_print = Clock_Time(); \
        LOG_ENTER_CRITICAL_SECTION();       \
        TraceAX(1, __VA_ARGS__);            \
        LOG_EXIT_CRITICAL_SECTION();        \
    } while (0)

//带时间,log输出, dma传输打印
#define tprint(...)                         \
    do {                                    \
        uint64_t time_print = Clock_Time(); \
        LOG_ENTER_CRITICAL_SECTION();       \
        TraceAX(0, __VA_ARGS__);            \
        LOG_EXIT_CRITICAL_SECTION();        \
    } while (0)

//仅仅log输出，不附带其他信息
#define nprint(...)                   \
    do {                              \
        LOG_ENTER_CRITICAL_SECTION(); \
        printf(__VA_ARGS__);          \
        LOG_EXIT_CRITICAL_SECTION();  \
    } while (0)

//仅仅打印到缓存中
#define cprint(...)                         \
    do {                                    \
        uint64_t time_print = Clock_Time(); \
        LOG_ENTER_CRITICAL_SECTION();       \
        log_set_mode(1);                    \
        TraceAX(0, __VA_ARGS__);            \
        log_set_mode(0);                    \
        LOG_EXIT_CRITICAL_SECTION();        \
    } while (0)

//切换顺序打印
//#define cprint(...)  do{kp_set_mode(0);fprint(1, __VA_ARGS__);}while(0)

#else
#define kprint(...)
#define tprint(...)
#define cprint(...)
#define nprint(...)
#define fprint(flag, ...)

#endif

#ifdef COM_LOG_FUN


////带函数名和时间,log输出, dma传输打印
#define TraceLogB0(level,func_name, ...)		 klog(level,NULL,__VA_ARGS__)
#define TraceLogB1(level,func_name, ...) klog(level,(char *)func_name,__VA_ARGS__)
#define TraceLogBX(flags, level, func_name, ...) TraceLogB ##flags(level, func_name, ##__VA_ARGS__)
#define usblog(flag, level, ...)   TraceLogBX(flag, level, __func__, ##__VA_ARGS__)

//带函数名和时间,log输出, dma传输打印
#define uprint1(...) usblog(1,LOG_LEVEL_32, __VA_ARGS__)
#define uprint0(...) usblog(0,LOG_LEVEL_32, __VA_ARGS__)

//#define klog_by_level(key,...) klog(key,__VA_ARGS__)
//#define uprint(...)		klog_by_level(LOG_LEVEL_32,__VA_ARGS__)
//#define klog_level_01(...) klog_by_level(LOG_LEVEL_01,__VA_ARGS__)
//#define klog_level_02(...) klog_by_level(LOG_LEVEL_02,__VA_ARGS__)
//#define klog_level_03(...) klog_by_level(LOG_LEVEL_03,__VA_ARGS__)
//#define klog_level_04(...) klog_by_level(LOG_LEVEL_04,__VA_ARGS__)
//#define klog_level_05(...) klog_by_level(LOG_LEVEL_05,__VA_ARGS__)
//#define klog_level_06(...) klog_by_level(LOG_LEVEL_06,__VA_ARGS__)
//#define klog_level_07(...) klog_by_level(LOG_LEVEL_07,__VA_ARGS__)
//#define klog_level_08(...) klog_by_level(LOG_LEVEL_08,__VA_ARGS__)
//#define klog_level_09(...) klog_by_level(LOG_LEVEL_09,__VA_ARGS__)
//#define klog_level_10(...) klog_by_level(LOG_LEVEL_10,__VA_ARGS__)
//#define klog_level_11(...) klog_by_level(LOG_LEVEL_11,__VA_ARGS__)
//#define klog_level_12(...) klog_by_level(LOG_LEVEL_12,__VA_ARGS__)
//#define klog_level_13(...) klog_by_level(LOG_LEVEL_13,__VA_ARGS__)
//#define klog_level_14(...) klog_by_level(LOG_LEVEL_14,__VA_ARGS__)
//#define klog_level_15(...) klog_by_level(LOG_LEVEL_15,__VA_ARGS__)
//#define klog_level_16(...) klog_by_level(LOG_LEVEL_16,__VA_ARGS__)
//#define klog_level_17(...) klog_by_level(LOG_LEVEL_17,__VA_ARGS__)
//#define klog_level_18(...) klog_by_level(LOG_LEVEL_18,__VA_ARGS__)
//#define klog_level_19(...) klog_by_level(LOG_LEVEL_19,__VA_ARGS__)
//#define klog_level_20(...) klog_by_level(LOG_LEVEL_20,__VA_ARGS__)
//#define klog_level_21(...) klog_by_level(LOG_LEVEL_21,__VA_ARGS__)
//#define klog_level_22(...) klog_by_level(LOG_LEVEL_22,__VA_ARGS__)
//#define klog_level_23(...) klog_by_level(LOG_LEVEL_23,__VA_ARGS__)
//#define klog_level_24(...) klog_by_level(LOG_LEVEL_24,__VA_ARGS__)
//#define klog_level_25(...) klog_by_level(LOG_LEVEL_25,__VA_ARGS__)
//#define klog_level_26(...) klog_by_level(LOG_LEVEL_26,__VA_ARGS__)
//#define klog_level_27(...) klog_by_level(LOG_LEVEL_27,__VA_ARGS__)
//#define klog_level_28(...) klog_by_level(LOG_LEVEL_28,__VA_ARGS__)
//#define klog_level_29(...) klog_by_level(LOG_LEVEL_29,__VA_ARGS__)
//#define klog_level_30(...) klog_by_level(LOG_LEVEL_30,__VA_ARGS__)
//#define klog_level_31(...) klog_by_level(LOG_LEVEL_31,__VA_ARGS__)
//#define klog_level_32(...) klog_by_level(LOG_LEVEL_32,__VA_ARGS__)

#else
#define uprint1(...)
#define uprint0(...)

//
//#define klog_level_01(...)
//#define klog_level_02(...)
//#define klog_level_03(...)
//#define klog_level_04(...)
//#define klog_level_05(...)
//#define klog_level_06(...)
//#define klog_level_07(...)
//#define klog_level_08(...)
//#define klog_level_09(...)
//#define klog_level_10(...)
//#define klog_level_11(...)
//#define klog_level_12(...)
//#define klog_level_13(...)
//#define klog_level_14(...)
//#define klog_level_15(...)
//#define klog_level_16(...)
//#define klog_level_17(...)
//#define klog_level_18(...)
//#define klog_level_19(...)
//#define klog_level_20(...)
//#define klog_level_21(...)
//#define klog_level_22(...)
//#define klog_level_23(...)
//#define klog_level_24(...)
//#define klog_level_25(...)
//#define klog_level_26(...)
//#define klog_level_27(...)
//#define klog_level_28(...)
//#define klog_level_29(...)
//#define klog_level_30(...)
//#define klog_level_31(...)
//#define klog_level_32(...)
#define uprint(...)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __STANDARDLIB_H*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
