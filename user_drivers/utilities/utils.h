/*******************************************************************************
* file    utils.h
* author  mackgim
* version 1.0.0
* date
* brief   utils
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __UTILS_H
#define __UTILS_H

#ifdef __cplusplus
extern "C"
#endif

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
/*!
* this macro evaluates an error variable \a ERR against an error code \a EC.
* in case it is not equal it jumps to the given label \a LABEL.
*/
#define EVAL_ERR_NE_GOTO(EC, ERR, LABEL)                                   \
    if ((EC) != (ERR)) goto LABEL;

/*! 
 * this macro evaluates an error variable \a ERR against an error code \a EC.
 * in case it is equal it jumps to the given label \a LABEL.
 */
#define EVAL_ERR_EQ_GOTO(EC, ERR, LABEL)                                   \
    if ((EC) == (ERR)) goto LABEL;

#define SIZEOF_ARRAY(a)     (sizeof(a) / sizeof((a)[0]))    /*!< Compute the size of an array           */
#ifndef MAX
#define MAX(a, b)           (((uint32_t)(a) > (uint32_t)(b)) ? (uint32_t)(a) : (uint32_t)(b))    /*!< Return the maximum of the 2 values     */
#endif
#ifndef MIN
#define MIN(a, b)           (((uint32_t)(a) < (uint32_t)(b)) ? (uint32_t)(a) : (uint32_t)(b))    /*!< Return the minimum of the 2 values     */
#endif
#define BITMASK_1           (0x01)                        /*!< Bit mask for lsb bit                   */
#define BITMASK_2           (0x03)                        /*!< Bit mask for two lsb bits              */
#define BITMASK_3           (0x07)                        /*!< Bit mask for three lsb bits            */
#define BITMASK_4           (0x0F)                        /*!< Bit mask for four lsb bits             */
#define U16TOU8(a)          ((a) & 0x00FF)                /*!< Cast 16-bit unsigned to 8-bit unsigned */
#define GETU16(a)           (uint16_t)(((a)[0] << 8) | (a)[1])/*!< Cast two Big Endian 8-bits byte array to 16-bits unsigned */


#define REVERSE_BYTES(pData, nDataSize) \
  unsigned char swap, *lo = ((unsigned char *)(pData)), *hi = ((unsigned char *)(pData)) + (nDataSize) - 1; \
  while (lo < hi) { swap = *lo; *lo++ = *hi; *hi-- = swap; }


#define ST_MEMMOVE          memmove     /*!< map memmove to string library code */
#define ST_MEMCPY           memcpy      /*!< map memcpy to string library code  */
#define ST_MEMSET           memset      /*!< map memset to string library code  */
#define ST_BYTECMP          memcmp      /*!< map bytecmp to string library code */

#define NO_WARNING(v)      ((void) (v)) /*!< Macro to suppress compiler warning */


#ifndef NULL
  #define NULL (void*)0                 /*!< represents a NULL pointer */
#endif /* !NULL */


uint32_t uint_to_char(uint32_t input, uint8_t * output);
uint8_t all_is_same(uint32_t * buff, uint32_t count, uint32_t cmp);
uint8_t check_bcc(uint8_t * data, uint32_t size);
uint32_t bytes_to_str(uint8_t * byte, uint32_t size, uint8_t * str);

#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
