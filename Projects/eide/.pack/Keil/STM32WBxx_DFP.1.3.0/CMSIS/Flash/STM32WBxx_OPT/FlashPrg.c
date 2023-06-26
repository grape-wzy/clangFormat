/* -----------------------------------------------------------------------------
 * Copyright (c) 2023 Arm Ltd.
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software. Permission is granted to anyone to use this
 * software for any purpose, including commercial applications, and to alter
 * it and redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software in
 *    a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 *
 *
 * $Date:        06. March 2023
 * $Revision:    V1.0.0
 *
 * Project:      Flash Programming Functions for ST STM32WB55x/35x M4 Option Bytes
 * --------------------------------------------------------------------------- */

/* History:
 *  Version 1.0.0
 *    Initial release
 */

#include "..\FlashOS.h"        /* FlashOS Structures */

typedef volatile unsigned long    vu32;
typedef          unsigned long     u32;

#define M32(adr) (*((vu32 *) (adr)))

// Peripheral Memory Map
#define IWDG_BASE        (0x40003000)
#define FLASH_BASE       (0x58004000)

#define IWDG            ((IWDG_TypeDef   *) IWDG_BASE)
#define FLASH           ((FLASH_TypeDef  *) FLASH_BASE)

// Independent WATCHDOG
typedef struct {
  vu32 KR;               /* Offset: 0x00 IWDG Key register */
  vu32 PR;               /* Offset: 0x04 IWDG Prescaler register */
  vu32 RLR;              /* Offset: 0x08 IWDG Reload register */
  vu32 SR;               /* Offset: 0x0C IWDG Status register */
} IWDG_TypeDef;

// Flash Registers
typedef struct
{
  vu32 ACR;              /* Offset: 0x00 FLASH Access control register */
  vu32 RESERVED;         /* Offset: 0x04 Reserved */
  vu32 KEYR;             /* Offset: 0x08 FLASH Key register */
  vu32 OPTKEYR;          /* Offset: 0x0C FLASH Option Key register */
  vu32 SR;               /* Offset: 0x10 FLASH Status register */
  vu32 CR;               /* Offset: 0x14 FLASH Control register */
  vu32 ECCR;             /* Offset: 0x18 FLASH ECC registe, */
  vu32 RESERVED1;        /* Offset: 0x1C Reserve, */
  vu32 OPTR;             /* Offset: 0x20 FLASH Option register */
  vu32 PCROP1ASR;        /* Offset: 0x24 FLASH Bank 1 PCROP area A Start address register */
  vu32 PCROP1AER;        /* Offset: 0x28 FLASH Bank 1 PCROP area A End address register */
  vu32 WRP1AR;           /* Offset: 0x2C FLASH Bank 1 WRP area A address register */
  vu32 WRP1BR;           /* Offset: 0x30 FLASH Bank 1 WRP area B address registe, */
  vu32 PCROP1BSR;        /* Offset: 0x34 FLASH Bank 1 PCROP area B Start address register */
  vu32 PCROP1BER;        /* Offset: 0x38 FLASH Bank 1 PCROP area B End address register */
  vu32 IPCCBR;           /* Offset: 0x3C FLASH IPCC data buffer address */
  vu32 RESERVED2[7];     /* Offset: 0x40-0x58 Reserved, */
  vu32 C2ACR;            /* Offset: 0x5C FLASH Core MO+ Access Control register  */
  vu32 C2SR;             /* Offset: 0x60 FLASH Core MO+ Status register */
  vu32 C2CR;             /* Offset: 0x64 FLASH Core MO+ Control register */
  vu32 RESERVED3[6];     /* Offset: 0x68-0x7C Reserved */
  vu32 SFR;              /* Offset: 0x80 FLASH secure start address */
  vu32 SRRVR;            /* Offset: 0x84 FlASH secure SRAM2 start addr and CPU2 reset vector */
} FLASH_TypeDef;


// Flash Keys
#define FLASH_KEY1               0x45670123
#define FLASH_KEY2               0xCDEF89AB
#define FLASH_OPTKEY1            0x08192A3B
#define FLASH_OPTKEY2            0x4C5D6E7F

// Flash Control Register definitions
#define FLASH_CR_PG             ((u32)(  1U      ))
#define FLASH_CR_PER            ((u32)(  1U <<  1))
#define FLASH_CR_MER1           ((u32)(  1U <<  2))
#define FLASH_CR_PNB_MSK        ((u32)(0xFF <<  3))
#define FLASH_CR_BKER           ((u32)(  1U << 11))
#define FLASH_CR_MER2           ((u32)(  1U << 15))
#define FLASH_CR_STRT           ((u32)(  1U << 16))
#define FLASH_CR_OPTSTRT        ((u32)(  1U << 17))
#define FLASH_CR_OBL_LAUNCH     ((u32)(  1U << 27))
#define FLASH_CR_OPTLOCK        ((u32)(  1U << 30))
#define FLASH_CR_LOCK           ((u32)(  1U << 31))


/* Flash Status Register definitions */
#define FLASH_SR_EOP            ((u32)(  1U      ))
#define FLASH_SR_OPERR          ((u32)(  1U <<  1))
#define FLASH_SR_PROGERR        ((u32)(  1U <<  3))
#define FLASH_SR_WRPERR         ((u32)(  1U <<  4))
#define FLASH_SR_PGAERR         ((u32)(  1U <<  5))
#define FLASH_SR_SIZERR         ((u32)(  1U <<  6))
#define FLASH_SR_PGSERR         ((u32)(  1U <<  7))
#define FLASH_SR_MISSERR        ((u32)(  1U <<  8))
#define FLASH_SR_FASTERR        ((u32)(  1U <<  9))
#define FLASH_SR_RDERR          ((u32)(  1U << 14))
#define FLASH_SR_OPTVERR        ((u32)(  1U << 15))
#define FLASH_SR_BSY            ((u32)(  1U << 16))

#define FLASH_PGERR             (FLASH_SR_OPERR   | FLASH_SR_PROGERR | FLASH_SR_WRPERR  | \
                                 FLASH_SR_PGAERR  | FLASH_SR_SIZERR  | FLASH_SR_PGSERR  | \
                                 FLASH_SR_MISSERR | FLASH_SR_FASTERR | FLASH_SR_RDERR   | FLASH_SR_OPTVERR )


// Flash option register definitions
#define FLASH_OPTR_RDP          ((u32)(0xFF      ))
#define FLASH_OPTR_RDP_NO       ((u32)(0xAA      ))
#define FLASH_OPTR_IWDG_SW      ((u32)(  1U << 16))


static void DSB(void) {
    __asm("DSB");
}

static void NOP(void) {
    __asm("NOP");
}


/*
 *  Initialize Flash Programming Functions
 *    Parameter:      adr:  Device Base Address
 *                    clk:  Clock Frequency (Hz)
 *                    fnc:  Function Code (1 - Erase, 2 - Program, 3 - Verify)
 *    Return Value:   0 - OK,  1 - Failed
 */

int Init (unsigned long adr, unsigned long clk, unsigned long fnc)
{
  (void)clk;
  (void)fnc;

#if defined FLASH_OPT
  (void)adr;

  if ((FLASH->OPTR & FLASH_OPTR_IWDG_SW) == 0x00000) {   /* Test if IWDG is running (IWDG in HW mode) */
    /* Set IWDG time out to ~32.768 second */
    IWDG->KR  = 0x5555;                                  /* Enable write access to IWDG_PR and IWDG_RLR */
    IWDG->PR  = 0x06;                                    /* Set prescaler to 256 */
    IWDG->RLR = 4095;                                    /* Set reload value to 4095 */
  }

  /* unlock FLASH_CR */
  FLASH->KEYR = FLASH_KEY1;
  FLASH->KEYR = FLASH_KEY2;
  DSB();
  while (FLASH->SR & FLASH_SR_BSY) {                     /* Wait until operation is finished */
    IWDG->KR = 0xAAAA;                                   /* Reload IWDG */
  }

  /* Unlock Option Bytes operation */
  FLASH->OPTKEYR = FLASH_OPTKEY1;
  FLASH->OPTKEYR = FLASH_OPTKEY2;
  DSB();
  while (FLASH->SR & FLASH_SR_BSY) {                     /* Wait until operation is finished */
    IWDG->KR = 0xAAAA;                                   /* Reload IWDG */
  }
#endif /* FLASH_OPT */

  return (0);
}


/*
 *  De-Initialize Flash Programming Functions
 *    Parameter:      fnc:  Function Code (1 - Erase, 2 - Program, 3 - Verify)
 *    Return Value:   0 - OK,  1 - Failed
 */

int UnInit (unsigned long fnc)
{
  (void)fnc;

#if defined FLASH_OPT
  /* Lock option bytes operation */
  FLASH->CR = FLASH_CR_OPTLOCK;
  DSB();
  while (FLASH->SR & FLASH_SR_BSY) {                     /* Wait until operation is finished */
    IWDG->KR = 0xAAAA;                                   /* Reload IWDG */
  }

  /* Lock FLASH CR */
  FLASH->CR = FLASH_CR_LOCK;
  DSB();
  while (FLASH->SR & FLASH_SR_BSY) {                     /* Wait until operation is finished */
    IWDG->KR = 0xAAAA;                                   /* Reload IWDG */
  }
#endif /* FLASH_OPT */

  return (0);
}


/*
 *  Blank Check Checks if Memory is Blank
 *    Parameter:      adr:  Block Start Address
 *                    sz:   Block Size (in bytes)
 *                    pat:  Block Pattern
 *    Return Value:   0 - OK,  1 - Failed
 */

#if defined FLASH_OPT
int BlankCheck (unsigned long adr, unsigned long sz, unsigned char pat) {
  /* For OPT algorithm Flash is always erased */

  (void)adr;
  (void)sz;
  (void)pat;

  return (0);
}
#endif /* FLASH_OPT */


/*
 *  Erase complete Flash Memory
 *    Return Value:   0 - OK,  1 - Failed
 */

#ifdef FLASH_OPT
int EraseChip (void) {

  FLASH->SR = FLASH_PGERR;                               /* Reset Error Flags */

  FLASH->OPTR      = 0x3DFFF1AA;
  FLASH->PCROP1ASR = 0x000001FF;
  FLASH->PCROP1AER = 0x80000000;
  FLASH->WRP1AR    = 0x000000FF;
  FLASH->WRP1BR    = 0x000000FF;
  FLASH->PCROP1BSR = 0x000001FF;
  FLASH->PCROP1BER = 0x00000000;
  FLASH->IPCCBR    = 0xFFFFC000;

  FLASH->CR = FLASH_CR_OPTSTRT;                          /* Program values */
  DSB();

  while (FLASH->SR & FLASH_SR_BSY) {                     /* Wait until operation is finished */
    IWDG->KR = 0xAAAA;                                   /* Reload IWDG */
  }

  if (FLASH->SR & FLASH_PGERR) {                         /* Check for Error */
    FLASH->SR  = FLASH_PGERR;                            /* Reset Error Flags */
    return (1);                                          /* Failed */
  }

  return (0);                                            /* Done */
}
#endif /* FLASH_OPT */


/*
 *  Erase Sector in Flash Memory
 *    Parameter:      adr:  Sector Address
 *    Return Value:   0 - OK,  1 - Failed
 */

#if defined FLASH_OPT
int EraseSector (unsigned long adr) {
  /* erase sector is not needed for Flash Option bytes */

  (void)adr;

  return (0);                                              /* Done */
}
#endif /* FLASH_OPT */


/*
 *  Program Page in Flash Memory
 *    Parameter:      adr:  Page Start Address
 *                    sz:   Page Size
 *                    buf:  Page Data
 *    Return Value:   0 - OK,  1 - Failed
 */

#ifdef FLASH_OPT
int ProgramPage (unsigned long adr, unsigned long sz, unsigned char *buf)
{
  u32 optr;
  u32 pcrop1asr;
  u32 pcrop1aer;
  u32 wrp1ar;
  u32 wrp1br;
  u32 pcrop1bsr;
  u32 pcrop1ber;
  u32 ipccbr;

  (void)adr;
  (void)sz;

/* Option Bytes order in buf
    0  FLASH_OPTR
    4  FLASH_PCROP1ASR
    8  FLASH_PCROP1AER
   12  FLASH_WRP1AR
   16  FLASH_WRP1BR
   20  FLASH_PCROP1BSR
   24  FLASH_PCROP1BER
   28  FLASH_IPCCBR
 */

  optr      = (u32)((*(buf   )) | (*(buf   +1) <<  8) | (*(buf   +2) << 16) | (*(buf   +3) << 24) );
  pcrop1asr = (u32)((*(buf+ 4)) | (*(buf+ 4+1) <<  8) | (*(buf+ 4+2) << 16) | (*(buf+ 4+3) << 24) );
  pcrop1aer = (u32)((*(buf+ 8)) | (*(buf+ 8+1) <<  8) | (*(buf+ 8+2) << 16) | (*(buf+ 8+3) << 24) );
  wrp1ar    = (u32)((*(buf+12)) | (*(buf+12+1) <<  8) | (*(buf+12+2) << 16) | (*(buf+12+3) << 24) );
  wrp1br    = (u32)((*(buf+16)) | (*(buf+16+1) <<  8) | (*(buf+16+2) << 16) | (*(buf+16+3) << 24) );
  pcrop1bsr = (u32)((*(buf+20)) | (*(buf+20+1) <<  8) | (*(buf+20+2) << 16) | (*(buf+20+3) << 24) );
  pcrop1ber = (u32)((*(buf+24)) | (*(buf+24+1) <<  8) | (*(buf+24+2) << 16) | (*(buf+24+3) << 24) );
  ipccbr    = (u32)((*(buf+28)) | (*(buf+28+1) <<  8) | (*(buf+28+2) << 16) | (*(buf+28+3) << 24) );

  FLASH->SR  = FLASH_PGERR;                                /* Reset Error Flags */

  FLASH->OPTR       = (optr       & 0xEF8F7EFFU) | ~(0xEF8F7FFFU);
  FLASH->PCROP1ASR  = (pcrop1asr  & 0x000001FFU) | ~(0x000001FFU);
  FLASH->PCROP1AER  = (pcrop1aer  & 0x800001FFU) | ~(0x800001FFU);
  FLASH->WRP1AR     = (wrp1ar     & 0x00FF00FFU) | ~(0x00FF00FFU);
  FLASH->WRP1BR     = (wrp1br     & 0x00FF00FFU) | ~(0x00FF00FFU);
  FLASH->PCROP1BSR  = (pcrop1bsr  & 0x000001FFU) | ~(0x000001FFU);
  FLASH->PCROP1BER  = (pcrop1ber  & 0x000001FFU) | ~(0x000001FFU);
  FLASH->IPCCBR     = (ipccbr     & 0x00003FFFU) | ~(0x00003FFFU);
  DSB();

  FLASH->CR  = FLASH_CR_OPTSTRT;                         /* Program values */
  DSB();

  while (FLASH->SR & FLASH_SR_BSY) {                     /* Wait until operation is finished */
    IWDG->KR = 0xAAAA;                                   /* Reload IWDG */
  }

  if (FLASH->SR & FLASH_PGERR) {                         /* Check for Error */
    FLASH->SR |= FLASH_PGERR;                            /* Reset Error Flags */
    return (1);                                          /* Failed */
  }

  return (0);                                            /* Done */
}
#endif /* FLASH_OPT */


/*
 *  Verify Flash Contents
 *    Parameter:      adr:  Start Address
 *                    sz:   Size (in bytes)
 *                    buf:  Data
 *    Return Value:   (adr+sz) - OK, Failed Address
 */

#ifdef FLASH_OPT
unsigned long Verify (unsigned long adr, unsigned long sz, unsigned char *buf)
{
  u32 optr;
  u32 pcrop1asr;
  u32 pcrop1aer;
  u32 wrp1ar;
  u32 wrp1br;
  u32 pcrop1bsr;
  u32 pcrop1ber;
  u32 ipccbr;

  (void)adr;
  (void)sz;

  optr      = (u32)((*(buf   )) | (*(buf   +1) <<  8) | (*(buf   +2) << 16) | (*(buf   +3) << 24) );
  pcrop1asr = (u32)((*(buf+ 4)) | (*(buf+ 4+1) <<  8) | (*(buf+ 4+2) << 16) | (*(buf+ 4+3) << 24) );
  pcrop1aer = (u32)((*(buf+ 8)) | (*(buf+ 8+1) <<  8) | (*(buf+ 8+2) << 16) | (*(buf+ 8+3) << 24) );
  wrp1ar    = (u32)((*(buf+12)) | (*(buf+12+1) <<  8) | (*(buf+12+2) << 16) | (*(buf+12+3) << 24) );
  wrp1br    = (u32)((*(buf+16)) | (*(buf+16+1) <<  8) | (*(buf+16+2) << 16) | (*(buf+16+3) << 24) );
  pcrop1bsr = (u32)((*(buf+20)) | (*(buf+20+1) <<  8) | (*(buf+20+2) << 16) | (*(buf+20+3) << 24) );
  pcrop1ber = (u32)((*(buf+24)) | (*(buf+24+1) <<  8) | (*(buf+24+2) << 16) | (*(buf+24+3) << 24) );
  ipccbr    = (u32)((*(buf+28)) | (*(buf+28+1) <<  8) | (*(buf+28+2) << 16) | (*(buf+28+3) << 24) );
  DSB();

  /* Fail address returns the number of the OPT word passed with the assembler file */
  if (((* (u32 *)0x1FFF8000) & 0xEF8F7EFFU) != (optr      & 0xEF8F7EFFU)) {    /* Check OPTR values */
    return (adr + 0);
  }

  if (((* (u32 *)0x1FFF8008) & 0x000001FFU) != (pcrop1asr & 0x000001FFU)) {    /* Check PCROP1ASR values */
    return (adr + 1);
  }

  if (((* (u32 *)0x1FFF8010) & 0x800001FFU) != (pcrop1aer & 0x800001FFU)) {    /* Check PCROP1AER values */
    return (adr + 2);
  }

  if (((* (u32 *)0x1FFF8018) & 0x00FF00FFU) != (wrp1ar    & 0x00FF00FFU)) {    /* Check WRP1AR values */
    return (adr + 3);
  }

  if (((* (u32 *)0x1FFF8020) & 0x00FF00FFU) != (wrp1br    & 0x00FF00FFU)) {    /* Check WRP1BR values */
    return (adr + 4);
  }

  if (((* (u32 *)0x1FFF8028) & 0x000001FFU) != (pcrop1bsr & 0x000001FFU)) {    /* Check PCROP1BSR values */
    return (adr + 5);
  }

  if (((* (u32 *)0x1FFF8030) & 0x000001FFU) != (pcrop1ber & 0x000001FFU)) {    /* Check PCROP1BER values */
    return (adr + 6);
  }

  if (((* (u32 *)0x1FFF8068) & 0x00003FFFU) != (ipccbr    & 0x00003FFFU)) {    /* Check IPCCBR values */
    return (adr + 7);
  }

  return (adr + sz);
}
#endif /* FLASH_OPT */
