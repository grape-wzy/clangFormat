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
 * $Revision:    V1.1.2
 *
 * Project:      Flash Programming Functions for ST STM32WB3x Flash
 * --------------------------------------------------------------------------- */

/* History:
 *  Version 1.00
 *    Initial release
 *  Version 1.1.2
 */

#include "..\FlashOS.h"        // FlashOS Structures
#include "stm32wb35xx.h"
typedef volatile unsigned char    vu8;
typedef          unsigned char     u8;
typedef volatile unsigned short   vu16;
typedef          unsigned short    u16;
typedef volatile unsigned long    vu32;
typedef          unsigned long     u32;

#define FLASH_CR_PNB_D                 ((uint32_t)0x00001FF8)
#define M8(adr)  (*((vu8  *) (adr)))
#define M16(adr) (*((vu16 *) (adr)))
#define M32(adr) (*((vu32 *) (adr)))


#define FLASH_PGERR             (FLASH_SR_OPERR  | FLASH_SR_PROGERR | FLASH_SR_WRPERR | FLASH_SR_PGAERR  | FLASH_SR_SIZERR | \
                                 FLASH_SR_PGSERR  | FLASH_SR_MISERR | FLASH_SR_FASTERR | FLASH_SR_RDERR   )



#define FLASH_KEY1                         ((uint32_t)0x45670123) /*!< Flash key1 */
#define FLASH_KEY2                         ((uint32_t)0xCDEF89AB) /*!< Flash key2: used with FLASH_KEY1 to unlock the FLASH registers access */
unsigned long GetBankNum(unsigned long adr) {
  unsigned long bank;

	bank = ((unsigned long)adr >>9) & 0x00001FF8;
  
  return (bank);
}

void DSB(void) {
  __asm("DSB");
}
/*
 *  Initialize Flash Programming Functions
 *    Parameter:      adr:  Device Base Address
 *                    clk:  Clock Frequency (Hz)
 *                    fnc:  Function Code (1 - Erase, 2 - Program, 3 - Verify)
 *    Return Value:   0 - OK,  1 - Failed
 */


int Init (unsigned long adr, unsigned long clk, unsigned long fnc) {
	__disable_irq();
 // Unlock Flash
if(FLASH->CR & FLASH_CR_LOCK){
	FLASH->KEYR = FLASH_KEY1;                             
  FLASH->KEYR = FLASH_KEY2;
}
// Test if IWDG is running (IWDG in HW mode)
   if ((FLASH->OPTR & FLASH_OPTR_IWDG_SW) == 0x00) 
  {
    IWDG->KR  = 0xAAAA;   
		IWDG->KR  = 0x5555;                         // Enable write access to IWDG_PR and IWDG_RLR     
    IWDG->PR  = 0x06;                           // Set prescaler to 256  
    IWDG->RLR = 4095;                           // Set reload value to 4095
    WWDG->CFR = 0x1FF;
    WWDG->CR = 0x7F;

  }
	__enable_irq();
  return (0);
}



/*
 *  De-Initialize Flash Programming Functions
 *    Parameter:      fnc:  Function Code (1 - Erase, 2 - Program, 3 - Verify)
 *    Return Value:   0 - OK,  1 - Failed
 */

int UnInit (unsigned long fnc) {

  FLASH->CR |= FLASH_CR_LOCK;                                // Lock Flash

  return (0);
}


/*
 *  Erase complete Flash Memory
 *    Return Value:   0 - OK,  1 - Failed
 */

int EraseChip (void) {
	__disable_irq();
	if ((FLASH->OPTR & (1<<8)) == (1<<8) ){
		unsigned long SFSA = *(unsigned long *)0x58004080;
   SFSA&=0x000000FF;
   SFSA=((SFSA*0x1000)+0x8000000)-0x1000;
		unsigned long add =0x8000000;
		while(add<=SFSA)
		{
		EraseSector(add);
		add+=0x1000;
		}
		}
	else{
  while (FLASH->SR & FLASH_SR_BSY) {
    IWDG->KR = 0xAAAA;                                    // Reload IWDG
  }
  FLASH->SR  |= FLASH_PGERR;                               // Reset Error Flags
  
  FLASH->CR  |= FLASH_CR_MER;                 // Bank Mass Erase Enabled
  FLASH->CR |=  FLASH_CR_STRT;                               // Start Erase
  
  while (FLASH->SR & FLASH_SR_BSY) {
    IWDG->KR = 0xAAAA;                                    // Reload IWDG
  }

  FLASH->CR &= ~FLASH_CR_MER;                                        // Reset CR

  if (FLASH->SR & FLASH_PGERR) {                          // Check for Error
    FLASH->SR   |= FLASH_PGERR;                             // Reset Error Flags
    return (1);                                           // Failed
  }
}
	__enable_irq();
  return (0);                    
}


/*
 *  Erase Sector in Flash Memory
 *    Parameter:      adr:  Sector Address
 *    Return Value:   0 - OK,  1 - Failed
 */

int EraseSector (unsigned long adr) {
	
		__disable_irq();
  unsigned long b; 

  b = GetBankNum(adr);                                    // Get Bank Number   (0..1  )
 
   while (FLASH->SR & (FLASH_SR_BSY ));
  while (FLASH->SR & (FLASH_SR_CFGBSY));
   while (FLASH->SR & FLASH_SR_PESD);
  FLASH->SR  |= FLASH_PGERR;                               // Reset Error Flags
 FLASH->CR &= ~(FLASH_CR_PNB_D);
  FLASH->CR  |= (FLASH_CR_PER | b);                              // Page Erase Enabled
               
  FLASH->CR |=  FLASH_CR_STRT;                               // Start Erase

  while (FLASH->SR & FLASH_SR_BSY) {
    IWDG->KR = 0xAAAA;                                    // Reload IWDG
  }

  FLASH->CR  &= ~ FLASH_CR_PER;                // Reset CR

  if (FLASH->SR & FLASH_PGERR) {                          // Check for Error
    FLASH->SR  |= FLASH_PGERR;                             // Reset Error Flags
    return (1);                                           // Failed
  }
if ((* (unsigned long *)0x08000000) == 0xFFFFFFFF)   //empty check
  {
    FLASH->ACR |= 0x10000;
  }
	__enable_irq();
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

	__disable_irq();
unsigned long tab[8];
	int i;
 while (FLASH->SR & FLASH_SR_BSY) {
	 IWDG->KR = 0xAAAA;                                    // Reload IWDG
  }
  FLASH->SR  = FLASH_PGERR;                               // Reset Error Flags
	
  /*Flash programming enabled*/
	 FLASH->CR = FLASH_CR_PG ;	                              // Programming Enabled

 while (sz) {
    FLASH->CR = FLASH_CR_PG ;                              // Programming Enabled
if(sz>=8){
    M32(adr    ) = *((u32 *)(buf + 0));                 // Program the first word of the Double Word
    M32(adr + 4) = *((u32 *)(buf + 4));                 // Program the second word of the Double Word
    DSB();

    while (FLASH->SR & FLASH_SR_BSY) {
      IWDG->KR = 0xAAAA;                                // Reload IWDG
    }

    if (FLASH->SR & FLASH_PGERR) {                      // Check for Error
      FLASH->SR  = FLASH_PGERR;                         // Reset Error Flags
      return (1);                                       // Failed
    }

    adr += 8;                                           // Go to next DoubleWord
    buf += 8;
    sz  -= 8;
  }
		/*last word programming*/
		else{
	     for(i=0;i<sz;i++)
        {
          tab[i]= *((unsigned char *)buf);
          buf=buf+1;
        }
        
        for(i=0;i<8-sz;i++)
        {
          tab[i+sz]=0xFF;
        }
				tab[0]=tab[0]+(tab[1]<<8)+(tab[2]<<16)+(tab[3]<<24);
				tab[1]=tab[4]+(tab[5]<<8)+(tab[6]<<16)+(tab[7]<<24);
				 M32(adr    ) = *((u32 *)(tab + 0));                  /* Program the first word of the Double Word */
    M32(adr + 4) = *((u32 *)(tab + 1));                  /* Program the second word of the Double Word */
    DSB();
sz =0;
    while (FLASH->SR & FLASH_SR_BSY);

    if (FLASH->SR & FLASH_PGERR) {                       /* Check for Error */
      FLASH->SR  = FLASH_PGERR;                          /* Reset Error Flags */
      return (1);                                        /* Failed */
    }
  }
}
   FLASH->CR &= (~FLASH_CR_PG);
 if ((* (unsigned long *)0x08000000) != 0xFFFFFFFF)  //empty check
  {
    FLASH->ACR &= 0xFFFEFFFF;
  }
	__enable_irq();
  return (0);                                             // Done
}
