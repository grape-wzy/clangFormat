/* -----------------------------------------------------------------------------
 * Copyright (c) 2023 ARM Ltd.
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
 * Project:      Flash Device Description for ST STM32WB55x/35x M4 Option Bytes
 * --------------------------------------------------------------------------- */

/* History:
 *  Version 1.0.0
 *    Initial release
 */

#include "..\FlashOS.h"        /* FlashOS Structures */


#if defined FLASH_OPT

#ifdef STM32WB55x_35x_M4
  struct FlashDevice const FlashDevice  =  {
    FLASH_DRV_VERS,                    /* Driver Version, do not modify! */
    "STM32WB55x/35x Flash Options",    /* Device Name */
    ONCHIP,                            /* Device Type */
    0x1FFF8000,                        /* Device Start Address (virtual address) */
    0x00000020,                        /* Device Size in Bytes (32) */
    32,                                /* Programming Page Size */
    0,                                 /* Reserved, must be 0 */
    0xFF,                              /* Initial Content of Erased Memory */
    3000,                              /* Program Page Timeout 3 Sec */
    3000,                              /* Erase Sector Timeout 3 Sec */
    /* Specify Size and Address of Sectors */
    0x0020, 0x000000,                  /* Sector Size 32B */
    SECTOR_END
  };
#endif

#endif /* FLASH_OPT */

