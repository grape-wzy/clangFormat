#ifndef __EM_DEF_H__
#define __EM_DEF_H__

#include "emconfig.h"

#ifdef EM_USING_LIBC
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <sys/types.h>
#endif /* EM_USING_LIBC */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup BasicDef
 */

/**@{*/

/* EM_uOS version information */
#define EM_VERSION_MAJOR                       (5) /**< Major version number (X.x.x) */
#define EM_VERSION_MINOR                       (0) /**< Minor version number (x.X.x) */
#define EM_VERSION_PATCH                       (0) /**< Patch version number (x.x.X) */

/* e.g. #if (EM_UOS_VERSION >= EM_VERSION_CHECK(4, 1, 0) */
#define EM_VERSION_CHECK(major, minor, revise) (((major)*10000) + ((minor)*100) + (revise))

/* EM_uOS version */
#define EM_UOS_VERSION                         EM_VERSION_CHECK(EM_VERSION_MAJOR, EM_VERSION_MINOR, EM_VERSION_PATCH)

/* EM_uOS basic data type definitions */
typedef int           em_bool_t;  /**< boolean type */
typedef signed long   em_base_t;  /**< Nbit CPU related date type */
typedef unsigned long em_ubase_t; /**< Nbit unsigned CPU related data type */

#ifndef EM_USING_ARCH_DATA_TYPE
#ifdef EM_USING_LIBC
typedef int8_t   em_int8_t;   /**<  8bit integer type */
typedef int16_t  em_int16_t;  /**< 16bit integer type */
typedef int32_t  em_int32_t;  /**< 32bit integer type */
typedef uint8_t  em_uint8_t;  /**<  8bit unsigned integer type */
typedef uint16_t em_uint16_t; /**< 16bit unsigned integer type */
typedef uint32_t em_uint32_t; /**< 32bit unsigned integer type */
typedef int64_t  em_int64_t;  /**< 64bit integer type */
typedef uint64_t em_uint64_t; /**< 64bit unsigned integer type */
typedef size_t   em_size_t;   /**< Type for size number */
typedef ssize_t  em_ssize_t;  /**< Used for a count of bytes or an error indication */
#else
typedef signed char    em_int8_t;   /**<  8bit integer type */
typedef signed short   em_int16_t;  /**< 16bit integer type */
typedef signed int     em_int32_t;  /**< 32bit integer type */
typedef unsigned char  em_uint8_t;  /**<  8bit unsigned integer type */
typedef unsigned short em_uint16_t; /**< 16bit unsigned integer type */
typedef unsigned int   em_uint32_t; /**< 32bit unsigned integer type */
#ifdef ARCH_CPU_64BIT
typedef signed long    em_int64_t;  /**< 64bit integer type */
typedef unsigned long  em_uint64_t; /**< 64bit unsigned integer type */
#else
typedef signed long long   em_int64_t;  /**< 64bit integer type */
typedef unsigned long long em_uint64_t; /**< 64bit unsigned integer type */
#endif                         /* ARCH_CPU_64BIT */
typedef em_ubase_t     em_size_t;   /**< Type for size number */
typedef em_base_t      em_ssize_t;  /**< Used for a count of bytes or an error indication */
#endif                         /* EM_USING_LIBC */
#endif                         /* EM_USING_ARCH_DATA_TYPE */

typedef em_base_t   em_err_t;  /**< Type for error number */
typedef em_uint32_t em_time_t; /**< Type for time stamp */
typedef em_uint32_t em_tick_t; /**< Type for tick count */
typedef em_base_t   em_flag_t; /**< Type for flags */
typedef em_ubase_t  em_dev_t;  /**< Type for device */
typedef em_base_t   em_off_t;  /**< Type for offset */

/* boolean type definitions */
#define EM_FALSE (0)           /**< boolean fails */
#define EM_TRUE  (!(EM_FALSE)) /**< boolean true  */

/* null pointer definition */
#define EM_NULL  ((void *)(0))

/**@}*/

/* maximum value of base type */
#ifdef EM_USING_LIBC
#define EM_UINT8_MAX  UINT8_MAX          /**< Maximum number of UINT8 */
#define EM_UINT16_MAX UINT16_MAX         /**< Maximum number of UINT16 */
#define EM_UINT32_MAX UINT32_MAX         /**< Maximum number of UINT32 */
#else
#define EM_UINT8_MAX  0xff               /**< Maximum number of UINT8 */
#define EM_UINT16_MAX 0xffff             /**< Maximum number of UINT16 */
#define EM_UINT32_MAX 0xffffffff         /**< Maximum number of UINT32 */
#endif                                   /* EM_USING_LIBC */

#define EM_TICK_MAX        EM_UINT32_MAX /**< Maximum number of tick */

/* maximum value of ipc type */
#define EM_SEM_VALUE_MAX   EM_UINT16_MAX /**< Maximum number of semaphore .value */
#define EM_MUTEX_VALUE_MAX EM_UINT16_MAX /**< Maximum number of mutex .value */
#define EM_MUTEX_HOLD_MAX  EM_UINT8_MAX  /**< Maximum number of mutex .hold */
#define EM_MB_ENTRY_MAX    EM_UINT16_MAX /**< Maximum number of mailbox .entry */
#define EM_MQ_ENTRY_MAX    EM_UINT16_MAX /**< Maximum number of message queue .entry */

/* Common Utilities */

#define EM_UNUSED(x)       ((void)x)

/* Compiler Related Definitions */
#if defined(__ARMCC_VERSION) /* ARM Compiler */
#define em_section(x) __attribute__((section(x)))
#define em_used       __attribute__((used))
#define em_align(n)   __attribute__((aligned(n)))
#define em_weak       __attribute__((weak))
#define em_inline     static __inline
/* module compiling */
#ifdef EM_USING_MODULE
#define RTT_API __declspec(dllimport)
#else
#define RTT_API __declspec(dllexport)
#endif                             /* EM_USING_MODULE */
#elif defined(__IAR_SYSTEMS_ICC__) /* for IAR Compiler */
#define em_section(x) @x
#define em_used       __root
#define PRAGMA(x)     _Pragma(#x)
#define em_align(n)   PRAGMA(data_alignment = n)
#define em_weak       __weak
#define em_inline     static inline
#define RTT_API
#elif defined(__GNUC__) /* GNU GCC Compiler */
#ifndef EM_USING_LIBC
/* the version of GNU GCC must be greater than 4.x */
typedef __builtin_va_list __gnuc_va_list;
typedef __gnuc_va_list    va_list;
#define va_start(v, l) __builtin_va_start(v, l)
#define va_end(v)      __builtin_va_end(v)
#define va_arg(v, l)   __builtin_va_arg(v, l)
#endif /* EM_USING_LIBC */
#define __EM_STRINGIFY(x...) #x
#define EM_STRINGIFY(x...)   __EM_STRINGIFY(x)
#define em_section(x)        __attribute__((section(x)))
#define em_used              __attribute__((used))
#define em_align(n)          __attribute__((aligned(n)))
#define em_weak              __attribute__((weak))
#define em_inline            static __inline
#define RTT_API
#elif defined(__ADSPBLACKFIN__) /* for VisualDSP++ Compiler */
#define em_section(x) __attribute__((section(x)))
#define em_used       __attribute__((used))
#define em_align(n)   __attribute__((aligned(n)))
#define em_weak       __attribute__((weak))
#define em_inline     static inline
#define RTT_API
#elif defined(_MSC_VER)
#define em_section(x)
#define em_used
#define em_align(n) __declspec(align(n))
#define em_weak
#define em_inline static __inline
#define RTT_API
#elif defined(__TI_COMPILER_VERSION__)
/* The way that TI compiler set section is different from other(at least
 * GCC and MDK) compilers. See ARM Optimizing C/C++ Compiler 5.9.3 for more
 * details. */
#define em_section(x) __attribute__((section(x)))
#ifdef __TI_EABI__
#define em_used __attribute__((retain)) __attribute__((used))
#else
#define em_used __attribute__((used))
#endif
#define PRAGMA(x)   _Pragma(#x)
#define em_align(n) __attribute__((aligned(n)))
#ifdef __TI_EABI__
#define em_weak __attribute__((weak))
#else
#define em_weak
#endif
#define em_inline static inline
#define RTT_API
#elif defined(__TASKING__)
#define em_section(x) __attribute__((section(x)))
#define em_used       __attribute__((used, protect))
#define PRAGMA(x)     _Pragma(#x)
#define em_align(n)   __attribute__((__align(n)))
#define em_weak       __attribute__((weak))
#define em_inline     static inline
#define RTT_API
#else
#error not supported tool chain
#endif /* __ARMCC_VERSION */

#ifdef __cplusplus
}
#endif

#endif /* __EM_DEF_H__ */
