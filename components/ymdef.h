#ifndef __YM_DEF_H__
#define __YM_DEF_H__

#include "ymconfig.h"

#ifdef YM_USING_LIBC
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <sys/types.h>
#endif /* YM_USING_LIBC */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup BasicDef
 */

/**@{*/

/* YM_uOS version information */
#define YM_VERSION_MAJOR                       (1) /**< Major version number (X.x.x) */
#define YM_VERSION_MINOR                       (0) /**< Minor version number (x.X.x) */
#define YM_VERSION_PATCH                       (0) /**< Patch version number (x.x.X) */

/* e.g. #if (YM_UOS_VERSION >= YM_VERSION_CHECK(4, 1, 0) */
#define YM_VERSION_CHECK(major, minor, revise) (((major)*10000) + ((minor)*100) + (revise))

/* YM_uOS version */
#define YM_UOS_VERSION                         YM_VERSION_CHECK(YM_VERSION_MAJOR, YM_VERSION_MINOR, YM_VERSION_PATCH)

/* YM_uOS basic data type definitions */
typedef int           ym_bool_t;  /**< boolean type */
typedef signed long   ym_base_t;  /**< Nbit CPU related date type */
typedef unsigned long ym_ubase_t; /**< Nbit unsigned CPU related data type */

#ifndef YM_USING_ARCH_DATA_TYPE
#ifdef YM_USING_LIBC
typedef int8_t   ym_int8_t;   /**<  8bit integer type */
typedef int16_t  ym_int16_t;  /**< 16bit integer type */
typedef int32_t  ym_int32_t;  /**< 32bit integer type */
typedef uint8_t  ym_uint8_t;  /**<  8bit unsigned integer type */
typedef uint16_t ym_uint16_t; /**< 16bit unsigned integer type */
typedef uint32_t ym_uint32_t; /**< 32bit unsigned integer type */
typedef int64_t  ym_int64_t;  /**< 64bit integer type */
typedef uint64_t ym_uint64_t; /**< 64bit unsigned integer type */
typedef size_t   ym_size_t;   /**< Type for size number */
typedef ssize_t  ym_ssize_t;  /**< Used for a count of bytes or an error indication */
#else
typedef signed char    ym_int8_t;   /**<  8bit integer type */
typedef signed short   ym_int16_t;  /**< 16bit integer type */
typedef signed int     ym_int32_t;  /**< 32bit integer type */
typedef unsigned char  ym_uint8_t;  /**<  8bit unsigned integer type */
typedef unsigned short ym_uint16_t; /**< 16bit unsigned integer type */
typedef unsigned int   ym_uint32_t; /**< 32bit unsigned integer type */
#ifdef ARCH_CPU_64BIT
typedef signed long    ym_int64_t;  /**< 64bit integer type */
typedef unsigned long  ym_uint64_t; /**< 64bit unsigned integer type */
#else
typedef signed long long   ym_int64_t;  /**< 64bit integer type */
typedef unsigned long long ym_uint64_t; /**< 64bit unsigned integer type */
#endif                         /* ARCH_CPU_64BIT */
typedef ym_ubase_t     ym_size_t;   /**< Type for size number */
typedef ym_base_t      ym_ssize_t;  /**< Used for a count of bytes or an error indication */
#endif                         /* YM_USING_LIBC */
#endif                         /* YM_USING_ARCH_DATA_TYPE */

typedef ym_base_t   ym_err_t;  /**< Type for error number */
typedef ym_uint32_t ym_time_t; /**< Type for time stamp */
typedef ym_uint32_t ym_tick_t; /**< Type for tick count */
typedef ym_base_t   ym_flag_t; /**< Type for flags */
typedef ym_ubase_t  ym_dev_t;  /**< Type for device */
typedef ym_base_t   ym_off_t;  /**< Type for offset */

/* boolean type definitions */
#define YM_FALSE (0)           /**< boolean fails */
#define YM_TRUE  (!(YM_FALSE)) /**< boolean true  */

/* null pointer definition */
#define YM_NULL  ((void *)(0))

/**@}*/

/* maximum value of base type */
#ifdef YM_USING_LIBC
#define YM_UINT8_MAX  UINT8_MAX          /**< Maximum number of UINT8 */
#define YM_UINT16_MAX UINT16_MAX         /**< Maximum number of UINT16 */
#define YM_UINT32_MAX UINT32_MAX         /**< Maximum number of UINT32 */
#else
#define YM_UINT8_MAX  0xff               /**< Maximum number of UINT8 */
#define YM_UINT16_MAX 0xffff             /**< Maximum number of UINT16 */
#define YM_UINT32_MAX 0xffffffff         /**< Maximum number of UINT32 */
#endif                                   /* YM_USING_LIBC */

#define YM_TICK_MAX        YM_UINT32_MAX /**< Maximum number of tick */

/* maximum value of ipc type */
#define YM_SYM_VALUE_MAX   YM_UINT16_MAX /**< Maximum number of semaphore .value */
#define YM_MUTEX_VALUE_MAX YM_UINT16_MAX /**< Maximum number of mutex .value */
#define YM_MUTEX_HOLD_MAX  YM_UINT8_MAX  /**< Maximum number of mutex .hold */
#define YM_MB_ENTRY_MAX    YM_UINT16_MAX /**< Maximum number of mailbox .entry */
#define YM_MQ_ENTRY_MAX    YM_UINT16_MAX /**< Maximum number of message queue .entry */

/* Common Utilities */

#define YM_UNUSED(x)       ((void)x)

/* Compiler Related Definitions */
#if defined(__ARMCC_VERSION) /* ARM Compiler */
#define ym_section(x) __attribute__((section(x)))
#define ym_used       __attribute__((used))
#define ym_align(n)   __attribute__((aligned(n)))
#define ym_weak       __attribute__((weak))
#define ym_inline     static __inline
/* module compiling */
#ifdef YM_USING_MODULE
#define RTT_API __declspec(dllimport)
#else
#define RTT_API __declspec(dllexport)
#endif                             /* YM_USING_MODULE */
#elif defined(__IAR_SYSTEMS_ICC__) /* for IAR Compiler */
#define ym_section(x) @x
#define ym_used       __root
#define PRAGMA(x)     _Pragma(#x)
#define ym_align(n)   PRAGMA(data_alignment = n)
#define ym_weak       __weak
#define ym_inline     static inline
#define RTT_API
#elif defined(__GNUC__) /* GNU GCC Compiler */
#ifndef YM_USING_LIBC
/* the version of GNU GCC must be greater than 4.x */
typedef __builtin_va_list __gnuc_va_list;
typedef __gnuc_va_list    va_list;
#define va_start(v, l) __builtin_va_start(v, l)
#define va_end(v)      __builtin_va_end(v)
#define va_arg(v, l)   __builtin_va_arg(v, l)
#endif /* YM_USING_LIBC */
#define __YM_STRINGIFY(x...) #x
#define YM_STRINGIFY(x...)   __YM_STRINGIFY(x)
#define ym_section(x)        __attribute__((section(x)))
#define ym_used              __attribute__((used))
#define ym_align(n)          __attribute__((aligned(n)))
#define ym_weak              __attribute__((weak))
#define ym_inline            static __inline
#define RTT_API
#elif defined(__ADSPBLACKFIN__) /* for VisualDSP++ Compiler */
#define ym_section(x) __attribute__((section(x)))
#define ym_used       __attribute__((used))
#define ym_align(n)   __attribute__((aligned(n)))
#define ym_weak       __attribute__((weak))
#define ym_inline     static inline
#define RTT_API
#elif defined(_MSC_VER)
#define ym_section(x)
#define ym_used
#define ym_align(n) __declspec(align(n))
#define ym_weak
#define ym_inline static __inline
#define RTT_API
#elif defined(__TI_COMPILER_VERSION__)
/* The way that TI compiler set section is different from other(at least
 * GCC and MDK) compilers. See ARM Optimizing C/C++ Compiler 5.9.3 for more
 * details. */
#define ym_section(x) __attribute__((section(x)))
#ifdef __TI_EABI__
#define ym_used __attribute__((retain)) __attribute__((used))
#else
#define ym_used __attribute__((used))
#endif
#define PRAGMA(x)   _Pragma(#x)
#define ym_align(n) __attribute__((aligned(n)))
#ifdef __TI_EABI__
#define ym_weak __attribute__((weak))
#else
#define ym_weak
#endif
#define ym_inline static inline
#define RTT_API
#elif defined(__TASKING__)
#define ym_section(x) __attribute__((section(x)))
#define ym_used       __attribute__((used, protect))
#define PRAGMA(x)     _Pragma(#x)
#define ym_align(n)   __attribute__((__align(n)))
#define ym_weak       __attribute__((weak))
#define ym_inline     static inline
#define RTT_API
#else
#error not supported tool chain
#endif /* __ARMCC_VERSION */

#define YM_EOK          0 /**< There is no error */
#define YM_EHELLO       1
#define YM_ERROR        2
#define YM_ESUCCESS     3
#define YM_EFAILED      4
#define YM_EBUSY        5
#define YM_EDENIED      6
#define YM_EPING        7
#define YM_EDO_NOTHING  8
#define YM_ENEXT        9
#define YM_ENODATA      10
#define YM_ETIMEOUT     11
#define YM_EINVALID_ARG 12
#define YM_ENO_SPACE    13
#define YM_ENO_INIT     14

#define YM_ASSERT(EX)


/**@}*/

/**
 * @ingroup BasicDef
 *
 * @def YM_ALIGN(size, align)
 * Return the most contiguous size aligned at specified width. YM_ALIGN(13, 4)
 * would return 16.
 */
#define YM_ALIGN(size, align)           (((size) + (align) - 1) & ~((align) - 1))

/**
 * @ingroup BasicDef
 *
 * @def YM_ALIGN_DOWN(size, align)
 * Return the down number of aligned at specified width. YM_ALIGN_DOWN(13, 4)
 * would return 12.
 */
#define YM_ALIGN_DOWN(size, align)      ((size) & ~((align) - 1))

#ifdef __cplusplus
}
#endif

#endif /* __YM_DEF_H__ */
