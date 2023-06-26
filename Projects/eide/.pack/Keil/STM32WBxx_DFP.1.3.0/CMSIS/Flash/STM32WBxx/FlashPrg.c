/* -----------------------------------------------------------------------------
 * Copyright (c) 2014 ARM Ltd.
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
 * $Date:        02. April 2021
 * $Revision:    V1.00
 *
 * Project:      Flash Device Description for ST STM32WBxx Flash
 * --------------------------------------------------------------------------- */

/* History:
 *  Version 1.00
 *    Initial release
 */

#include "..\FlashOS.H"        // FlashOS Structures

typedef volatile unsigned char    vu8;
typedef          unsigned char     u8;
typedef volatile unsigned short   vu16;
typedef          unsigned short    u16;
typedef volatile unsigned long    vu32;
typedef          unsigned long     u32;

#define M8(adr)  (*((vu8  *) (adr)))
#define M16(adr) (*((vu16 *) (adr)))
#define M32(adr) (*((vu32 *) (adr)))

// Peripheral Memory Map
#define IWDG_BASE         0x40003000
#define FLASH_BASE        0x58004000// 0x40022000
//0x40023C00

#define IWDG            ((IWDG_TypeDef *) IWDG_BASE)
#define FLASH           ((FLASH_TypeDef*) FLASH_BASE)

// Independent WATCHDOG
typedef struct {
  vu32 KR;
  vu32 PR;
  vu32 RLR;
  vu32 SR;
} IWDG_TypeDef;

// Flash Registers
typedef struct {
  vu32 ACR;
  vu32 PDKEYR;
  vu32 KEYR;
  vu32 OPTKEYR;
  vu32 SR;
  vu32 CR;
  vu32 ECCR;
  vu32 RESERVED0;
  vu32 OPTR;
	vu32 PCROP1SR;         /*!< FLASH bank1 PCROP start address register, Address offset: 0x24 */
  vu32 PCROP1ER;         /*!< FLASH bank1 PCROP end address register,   Address offset: 0x28 */
  vu32 WRP1AR;           /*!< FLASH bank1 WRP area A address register,  Address offset: 0x2C */
  vu32 WRP1BR;           /*!< FLASH bank1 WRP area B address register,  Address offset: 0x30 */
  vu32 RESERVED2[4];   /*!< Reserved2,                                Address offset: 0x34 */
  vu32 PCROP2SR;         /*!< FLASH bank2 PCROP start address register, Address offset: 0x44 */
  vu32 PCROP2ER;         /*!< FLASH bank2 PCROP end address register,   Address offset: 0x48 */
  vu32 WRP2AR;           /*!< FLASH bank2 WRP area A address register,  Address offset: 0x4C */
  vu32 WRP2BR;           /*!< FLASH bank2 WRP area B address register,  Address offset: 0x50 */
  vu32 PCROP2BSR;           /*!< FLASH PCROP2BSR address register,  Address offset: 0x54 */
   vu32 PCROP2BER;       /*!< PCROP2BER,                               Address offset: 0x58 */
  vu32 ACR_M0;           /*!< FLASH bank2 WRP area B address register,  Address offset: 0x5C */
  vu32 SR_M0;             /*!< FLASH status register,                    Address offset: 0x60 */
  vu32 CR_M0;             /*!< FLASH status register,                    Address offset: 0x64 */
 
} FLASH_TypeDef;


// Flash Keys
#define RDPRT_KEY                0x00A5
#define FLASH_KEY1                  (0x45670123ul)
#define FLASH_KEY2                  (0xCDEF89ABul)
#define FLASH_SR_BSY1                ((vu32)0x00010000)
#define FLASH_SR_BSY2                ((vu32)0x00020000)
#define FLASH_SR_CFGBSY                ((vu32)0x00040000)
#define FLASH_SR_PE_SUSPENEDED         ((vu32)0x00080000)
#define FLASH_CR_BKER_D               ((vu32)0x00004000)
#define FLASH_CR_PNB_D                 ((vu32)0x00001FF8)


// Flash Control Register definitions
#define FLASH_SR_EOP                        ((unsigned int)(1U <<  1))
#define FLASH_PG                ((unsigned int)(1U <<  0))
#define FLASH_PER               ((unsigned int)(1U <<  1))
#define FLASH_MER1              ((unsigned int)(1U <<  2))
#define FLASH_PNB_MSK           ((unsigned int)(0xFFU << 3))
#define FLASH_BKER              ((unsigned int)(1U << 11))
#define FLASH_MER2              ((unsigned int)(1U << 15))
#define FLASH_STRT              ((unsigned int)(1U << 16))
#define FLASH_LOCK              ((unsigned int)(1U << 31))


// Flash Status Register definitions
#define FLASH_EOP               ((unsigned int)(1U <<  0))
#define FLASH_OPERR             ((unsigned int)(1U <<  1))
#define FLASH_PROGERR           ((unsigned int)(1U <<  3))
#define FLASH_WRPERR            ((unsigned int)(1U <<  4))
#define FLASH_PGAERR            ((unsigned int)(1U <<  5))
#define FLASH_SIZERR            ((unsigned int)(1U <<  6))
#define FLASH_PGSERR            ((unsigned int)(1U <<  7))
#define FLASH_MISSERR           ((unsigned int)(1U <<  8))
#define FLASH_FASTERR           ((unsigned int)(1U <<  9))
#define FLASH_RDERR             ((unsigned int)(1U << 14))

#define FLASH_BSY               ((unsigned int)(1U << 16))

#define FLASH_PGERR             (FLASH_OPERR  | FLASH_PROGERR | FLASH_PROGERR | FLASH_WRPERR  | FLASH_PGAERR | \
                                 FLASH_SIZERR | FLASH_PGSERR  | FLASH_MISSERR | FLASH_FASTERR | FLASH_RDERR   )

#if   defined STM32L4x_1024      /* 2 * 512 kB */
  #define FLASH_BANK_SIZE       (0x08080000U)
#elif defined STM32L4x_512       /* 2 * 256 kB  or 1 * 512 kB*/
  #define FLASH_BANK_SIZE       (0x08040000U)
#elif defined STM32L4x_256       /* 2 * 128 kB  or 1 * 256 kB*/
  #define FLASH_BANK_SIZE       (0x08020000U)
#elif defined STM32WB_M4       /* 2 * 128 kB  or 1 * 256 kB*/
  #define FLASH_BANK_SIZE       (0x08020000U)
#elif defined STM32WB_M0Plus       /* 2 * 128 kB  or 1 * 256 kB*/
  #define FLASH_BANK_SIZE       (0x08020000U)
#else
  #error STM32L4xx Flash size defined nort defined!
#endif


unsigned long GetBankNum(unsigned long adr) {
  unsigned long bank;
  
//  if ((FLASH->OPTR & (1U << 21)) == (1U << 21)) {
//    /* Dual-Bank Flash */
//    if (adr >= FLASH_BANK_SIZE) {
//      bank = 1u;           /* FLASH_CR.BKER (bit11) 1: Bank 2 is selected for page erase */
//    }
//    else {
//      bank = 0u;           /* FLASH_CR.BKER (bit11) 0: Bank 1 is selected for page erase */
//    }
//  } 
//  /* Single-Bank Flash */
//  else {
//    bank = 0u;             /* FLASH_CR.BKER (bit11) 0: Bank 1 is selected for page erase */
//  }
	
	bank = ((unsigned long)adr >>9) & 0x00001FF8;
  
  return (bank);
}


/*
 *  Initialize Flash Programming Functions
 *    Parameter:      adr:  Device Base Address
 *                    clk:  Clock Frequency (Hz)
 *                    fnc:  Function Code (1 - Erase, 2 - Program, 3 - Verify)
 *    Return Value:   0 - OK,  1 - Failed
 */


int Init (unsigned long adr, unsigned long clk, unsigned long fnc) {

  FLASH->KEYR = FLASH_KEY1;                             // Unlock Flash
  FLASH->KEYR = FLASH_KEY2;

//  FLASH->ACR  = 0x00000000;                             // Zero Wait State, no Cache, no Prefetch
//  FLASH->SR   = FLASH_PGERR;                            // Reset Error Flags

//  if ((FLASH->OPTR & 0x10000) == 0x00000) {             // Test if IWDG is running (IWDG in HW mode)
//    // Set IWDG time out to ~32.768 second
//    IWDG->KR  = 0x5555;                                 // Enable write access to IWDG_PR and IWDG_RLR
//    IWDG->PR  = 0x06;                                   // Set prescaler to 256
//    IWDG->RLR = 4095;                                   // Set reload value to 4095
//  }

  return (0);
}



/*
 *  De-Initialize Flash Programming Functions
 *    Parameter:      fnc:  Function Code (1 - Erase, 2 - Program, 3 - Verify)
 *    Return Value:   0 - OK,  1 - Failed
 */

int UnInit (unsigned long fnc) {

  FLASH->CR |= FLASH_LOCK;                                // Lock Flash

  return (0);
}


/*  
 *  Blank Check Checks if Memory is Blank
 *    Parameter:      adr:  Block Start Address
 *                    sz:   Block Size (in bytes)
 *                    pat:  Block Pattern
 *    Return Value:   0 - OK,  1 - Failed
 */

int BlankCheck (unsigned long adr, unsigned long sz, unsigned char pat) {
  /* force erase even if the content is 'Initial Content of Erased Memory'.
     Only a erased sector can be programmed. I think this is because of ECC */
  return (1);
}




/*
 *  Erase complete Flash Memory
 *    Return Value:   0 - OK,  1 - Failed
 */

int EraseChip (void) {

  while (FLASH->SR & FLASH_BSY) {
    IWDG->KR = 0xAAAA;                                    // Reload IWDG
  }
  FLASH->SR  = FLASH_PGERR;                               // Reset Error Flags
  
  FLASH->CR  = (FLASH_MER1 | FLASH_MER2);                 // Bank A/B Mass Erase Enabled
  FLASH->CR |=  FLASH_STRT;                               // Start Erase
  
  while (FLASH->SR & FLASH_BSY) {
    IWDG->KR = 0xAAAA;                                    // Reload IWDG
  }

  FLASH->CR  =  0;                                        // Reset CR

  if (FLASH->SR & FLASH_PGERR) {                          // Check for Error
    FLASH->SR  = FLASH_PGERR;                             // Reset Error Flags
    return (1);                                           // Failed
  }

  return (0);                                             // Done
}


/*
 *  Erase Sector in Flash Memory
 *    Parameter:      adr:  Sector Address
 *    Return Value:   0 - OK,  1 - Failed
 */

int EraseSector (unsigned long adr) {
  unsigned long b; //, n;

  b = GetBankNum(adr);                                    // Get Bank Number   (0..1  )
//  if (b == 1U) {
//    n = ((adr & ~FLASH_BANK_SIZE) >> 11) & 0xFFU;         // Get Sector Number (0..255)
//  }
//  else {
//    n = ((adr                   ) >> 11) & 0xFFU;         // Get Sector Number (0..255)
//  }
 
   while (FLASH->SR & (FLASH_SR_BSY1 | FLASH_SR_BSY2));
  while (FLASH->SR & FLASH_SR_CFGBSY);
   while (FLASH->SR & FLASH_SR_PE_SUSPENEDED);
  FLASH->SR  = FLASH_PGERR;                               // Reset Error Flags

  FLASH->CR  = (FLASH_PER | b);                              // Page Erase Enabled
               
  FLASH->CR |=  FLASH_STRT;                               // Start Erase

  while (FLASH->SR & FLASH_BSY) {
    IWDG->KR = 0xAAAA;                                    // Reload IWDG
  }

  FLASH->CR  =  0;                                        // Reset CR

  if (FLASH->SR & FLASH_PGERR) {                          // Check for Error
    FLASH->SR  = FLASH_PGERR;                             // Reset Error Flags
    return (1);                                           // Failed
  }

  return (0);                                             // Done
}



/*
 *  Program Page in Flash Memory
 *    Parameter:      adr:  Page Start Address
 *                    sz:   Page Size
 *                    buf:  Page Data
 *    Return Value:   0 - OK,  1 - Failed
 */

int ProgramPage (unsigned long adr, unsigned long sz, unsigned char *buf) {

  sz = (sz + 7) & ~7;                                     // Adjust size for two words

//  while (FLASH->SR & FLASH_BSY) {
//    IWDG->KR = 0xAAAA;                                    // Reload IWDG
//  }
//  FLASH->SR  = FLASH_PGERR;                               // Reset Error Flags
	/*Clear the error status*/
  FLASH->SR = 0x000001FF;
  /*Flash programming enabled*/
  //FLASH->CR |= FLASH_PG;
	 FLASH->CR = FLASH_PG ;	                              // Programming Enabled

  while (sz) {
   
		
		 while (FLASH->SR & (FLASH_SR_BSY1 | FLASH_SR_BSY2));
  while (FLASH->SR & FLASH_SR_CFGBSY);
   while (FLASH->SR & FLASH_SR_PE_SUSPENEDED);

    M32(adr    ) = *((u32 *)(buf + 0));                   // Program the first word of the Double Word
    M32(adr + 4) = *((u32 *)(buf + 4));                   // Program the second word of the Double Word
//    while (FLASH->SR & FLASH_BSY) {
//      IWDG->KR = 0xAAAA;                                  // Reload IWDG
//    }
    
                                        // Reset CR

    /*wait until the operation ends*/
    while (FLASH->SR & (FLASH_SR_BSY1 | FLASH_SR_BSY2));
  while (FLASH->SR & FLASH_SR_CFGBSY);
   while (FLASH->SR & FLASH_SR_PE_SUSPENEDED);
		
		 /*check for error*/
    if ((FLASH->SR & FLASH_SR_EOP))
    {
      return (1);                                         // Failed
    }

    adr += 8;                                             // Go to next DoubleWord
    buf += 8;
    sz  -= 8;
  }
   FLASH->CR &= (~FLASH_PG);
  return (0);                                             // Done
}
