/* -----------------------------------------------------------------------------
 * Copyright (c) 2014 - 2015 ARM Ltd.
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
 * Project:      Flash Device Description for ST STM32WB1x Flash
 * --------------------------------------------------------------------------- */

/* History:
 *  Version 1.0.0
 *    Initial release
 *  Version 1.1.2
 */

#include "..\FlashOS.H"        // FlashOS Structures

#ifdef FLASH_MEM


#ifdef STM32WB1x_320_M4
struct FlashDevice const FlashDevice  =  {
   FLASH_DRV_VERS,             // Driver Version, do not modify!
   "STM32WB1x_M4 320 Flash",        // Device Name 
   ONCHIP,                     // Device Type
   0x08000000,                 // Device Start Address
   0x00050000,                 // Device Size in Bytes (320KB) 
   1024,                       // Programming Page Size
   0,                          // Reserved, must be 0
   0xFF,                       // Initial Content of Erased Memory
   400,                        // Program Page Timeout 400 mSec
   400,                        // Erase Sector Timeout 400 mSec

// Specify Size and Address of Sectors
   0x800, 0x000000,            // Sector Size  2kB (160 Sectors)
   SECTOR_END
};
#endif




#endif // FLASH_MEM
