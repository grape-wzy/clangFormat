/*******************************************************************************
* file    gcompliler.h
* author  mackgim
* version 1.0.0
* date    2020-09-30
* brief   DOXYGEN相关
*******************************************************************************/

#ifndef __G_COMPILER_H
#define __G_COMPILER_H

#if defined (__ICCARM__)
#define __I__packed __packed
#define __I__PACKED 
#elif defined (__GNUC__)
#define __I__packed
#define __I__PACKED  __attribute__((packed))
#elif defined (__CC_ARM)
#define __I__packed __packed
#define __I__PACKED
#else
#error Neither ICCARM nor GNUC C detected. Define your "packed" macro.
#define __I__PACKED
#define __I__packed
#endif /* __ICCARM__*/

/* Change this define to 1 if zero-length arrays are not supported by your compiler. */
#ifndef __CC_ARM
#ifndef G_VARIABLE_SIZE
#define G_VARIABLE_SIZE 
#endif
#else
#define G_VARIABLE_SIZE 1
#endif

#if defined ( __CC_ARM   )
/* ARM Compiler
   ------------
   RAM functions are defined using the toolchain options.
   Functions that are executed in RAM should reside in a separate source module.
   Using the 'Options for File' dialog you can simply change the 'Code / Const'
   area of a module to a memory space in physical RAM.
   Available memory areas are declared in the 'Target' tab of the 'Options for Target'
   dialog.
*/
#define __RAM_USER_FUNC  

#define __NOINLINE __attribute__ ( (noinline) ) 


#elif defined ( __ICCARM__ )
/* ICCARM Compiler
   ---------------
   RAM functions are defined using a specific toolchain keyword "__ramfunc".
*/

#define __RAM_USER_FUNC_VOID    __ramfunc void
#define __RAM_USER_FUNC_UINT8_T  __ramfunc uint8_t
#define __RAM_USER_FUNC_UINT32_T  __ramfunc uint32_t
#define __RAM_USER_FUNC_HAL  __ramfunc HAL_StatusTypeDef

#define __NOINLINE _Pragma("optimize = no_inline")
#pragma diag_suppress=Pe161	
#elif defined   (  __GNUC__  )
/* GNU Compiler
   ------------
  RAM functions are defined using a specific toolchain attribute
   "__attribute__((section(".RamFunc")))".
*/


#define __RAM_USER_FUNC_VOID  void  __attribute__((section(".RamFunc")))
#define __RAM_USER_FUNC_UINT8_T  uint8_t  __attribute__((section(".RamFunc")))
#define __RAM_USER_FUNC_UINT32_T uint32_t __attribute__((section(".RamFunc")))
#define __RAM_USER_FUNC_HAL    HAL_StatusTypeDef __attribute__((section(".RamFunc")))

#define __RAM_PLACE_IN_MB_MEM2  __attribute__((section ("MB_MEM2")))


#endif


#endif /* __G_COMPILER_H */
