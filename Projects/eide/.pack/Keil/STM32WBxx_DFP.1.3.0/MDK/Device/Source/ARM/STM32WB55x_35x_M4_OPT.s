;/*******************************************************************************/
;/* Copyright (c) 2023 Arm Limited (or its affiliates).                         */
;/* All rights reserved.                                                        */
;/*                                                                             */
;/* SPDX-License-Identifier: BSD-3-Clause                                       */
;/*                                                                             */
;/* Redistribution and use in source and binary forms, with or without          */
;/* modification, are permitted provided that the following conditions are met: */
;/*   1.Redistributions of source code must retain the above copyright          */
;/*     notice, this list of conditions and the following disclaimer.           */
;/*   2.Redistributions in binary form must reproduce the above copyright       */
;/*     notice, this list of conditions and the following disclaimer in the     */
;/*     documentation and/or other materials provided with the distribution.    */
;/*   3.Neither the name of Arm nor the names of its contributors may be used   */
;/*     to endorse or promote products derived from this software without       */
;/*     specific prior written permission.                                      */
;/*                                                                             */
;/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" */
;/* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE   */
;/* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE  */
;/* ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE     */
;/* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR         */
;/* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF        */
;/* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS    */
;/* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN     */
;/* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)     */
;/* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE  */
;/* POSSIBILITY OF SUCH DAMAGE.                                                 */
;/*******************************************************************************/
;/* STM32WB55x_35x_M4_OPT.s: STM32WB55xx/STM32WB35xx Flash Option Bytes         */
;/*******************************************************************************/
;/* <<< Use Configuration Wizard in Context Menu >>>                            */
;/*******************************************************************************/


;// <e> Flash Option Bytes
FLASH_OPT       EQU     1

;// <h> Flash Read Protection
;//     <i> Read protection is used to protect the software code stored in Flash memory
;//   <o0.0..7> Read Protection Level
;//     <i> Level 0: Read Protection not active
;//     <i> Level 1: Memories Read Protection active)
;//     <i> Level 2: Chip Read Protection active
;//          <0xAA=> Level 0 (No Protection)
;//          <0x00=> Level 1 (Memories Read Protection)
;//          <0xCC=> Level 2 (Chip Read Protection)
;// </h>

;// <h> User Configuration
;//   <o0.29..31> AGC_TRIM <0-7>
;//     <i> Radio automatic gain control trimming
;//   <o0.27> nBOOT0
;//     <i> nBOOT0 option bit
;//          <0=> 0
;//          <1=> 1
;//   <o0.26> nSWBOOT0
;//     <i> Software BOOT0 selection
;//          <0=> Option bit nBOOT0
;//          <1=> PH3/BOOT0 pin
;//   <o0.25> SRAM2_RST
;//     <i> SRAM2 and PKA RAM Erase when system reset
;//          <0=> SRAM2 and PKA RAM erased when a system reset occurs
;//          <1=> SRAM2 and non-secure PKA RAM not erased when a system reset occurs
;//   <o0.24> SRAM2_PE
;//     <i> SRAM2 parity check enable
;//          <0=> SRAM2 parity check enabled
;//          <1=> SRAM2 parity check disabled
;//   <o0.23> nBOOT1
;//     <i> Boot configuration
;//     <i> Together with the BOOT0 pin or option bit nBOOT0 this bit selects 
;//     <i> boot mode from the user flash memory, SRAM1 or the System memory
;//          <0=> System memory
;//          <1=> Embedded SRAM1
;//   <o0.19> WWDG_SW
;//     <i> Window watchdog selection
;//          <0=> Hardware window watchdog
;//          <1=> Software window watchdog
;//   <o0.18> IWDG_STDBY
;//     <i> Independent watchdog counter freeze in Standby mode
;//          <0=> Freeze IWDG counter in Standby mode
;//          <1=> IWDG counter active in Standby mode
;//   <o0.17> IWDG_STOP
;//     <i> Independent watchdog counter freeze in Stop mode
;//          <0=> Freeze IWDG counter in STOP mode
;//          <1=> IWDG counter active in STOP mode
;//   <o0.16> IWDG_SW
;//     <i> Independent watchdog selection
;//          <0=> Hardware independant watchdog
;//          <1=> Software independant watchdog
;//   <o0.14> nRST_SHDW
;//     <i> Generate Reset when entering Shutdown Mode
;//          <0=> Reset generated
;//          <1=> Reset not generated
;//   <o0.13> nRST_STDBY
;//     <i> Generate Reset when entering Standby Mode
;//          <0=> Reset generated
;//          <1=> Reset not generated
;//   <o0.12> nRST_STOP
;//     <i> Generate Reset when entering STOP Mode
;//          <0=> Reset generated
;//          <1=> Reset not generated
;//   <o0.9..11> BOR_LEV
;//     <i>These bits contain the VDD supply level threshold that activates/releases the reset
;//          <0=> BOR Level 0 (Reset level threshold is around 1.7 V)
;//          <1=> BOR Level 1 (Reset level threshold is around 2.0 V)
;//          <2=> BOR Level 2 (Reset level threshold is around 2.2 V)
;//          <3=> BOR Level 3 (Reset level threshold is around 2.5 V)
;//          <4=> BOR Level 4 (Reset level threshold is around 2.8 V)
;// </h>
FLASH_OPTR      EQU    0x3DFFF0AA       ; reset value: 0x3DFFF0AA

;// <h> PCROP Configuration
;//   <o1.31> PCROP_RDP
;//     <i>   Bit is set only! Bit is reset when changing RDP level from 1 to 0
;//     <i>   checked: PCROP area erased when RDP level is decreased from 1 to 0 (full mass erase)
;//     <i>   unchecked: PCROP area is not erased when RDP level is decreased from 1 to 0
;//   <o0.0..8> PCROP1A_STRT <0x0-0x1FF>
;//     <i> PCROP1A area start offset
;//     <i> PCROP1A_STRT contains the first 2 Kbytes page of the PCROP1A area
;//     <i> Note that bit 8 is reserved on STM32WB35xx devices
;//   <o1.0..8> PCROP1A_END <0x0-0x1FF>
;//     <i> PCROP1A area end offset
;//     <i> PCROP1A_END contains the last 2 Kbytes page of the PCROP1A area
;//     <i> Note that bit 8 is reserved on STM32WB35xx devices
;//   <o2.0..8> PCROP1B_STRT <0x0-0x1FF>
;//     <i> PCROP1B area start offset
;//     <i> PCROP1B_STRT contains the first 2 Kbytes page of the PCROP1B area
;//     <i> Note that bit 8 is reserved on STM32WB35xx devices
;//   <o3.0..8> PCROP1B_END <0x0-0x1FF>
;//     <i> PCROP1B area end offset
;//     <i> PCROP1B_END contains the last 2 Kbytes page of the PCROP1B area
;//     <i> Note that bit 8 is reserved on STM32WB35xx devices
;// </h>
FLASH_PCROP1ASR EQU    0x000001FF       ; reset value: 0x????????
FLASH_PCROP1AER EQU    0x80000000       ; reset value: 0x????????
FLASH_PCROP1BSR EQU    0x000001FF       ; reset value: 0x????????
FLASH_PCROP1BER EQU    0x00000000       ; reset value: 0x????????

;// <h> WRP Configuration
;//   <o0.0..7> WRP1A_STRT <0x0-0xFF>
;//     <i> WRP first area "A" start offset
;//     <i> Contains the first 4 Kbytes page of the WRP first area
;//     <i> Note that bit 7 is reserved on STM32WB35xx devices
;//   <o0.16..23> WRP1A_END <0x0-0xFF>
;//     <i> WRP first area "A" end offset
;//     <i> Contains the last 4 Kbytes page of the WRP first area
;//     <i> Note that bit 23 is reserved on STM32WB35xx devices
;//   <o1.0..7> WRP1B_STRT <0x0-0xFF>
;//     <i> WRP first area "B" start offset
;//     <i> Contains the first 4 Kbytes page of the WRP second area
;//     <i> Note that bit 7 is reserved on STM32WB35xx devices
;//   <o1.16..23> WRP1B_END <0x0-0xFF>
;//     <i> WRP first area "B" end offset
;//     <i> Contains the last 4 Kbytes page of the WRP second area
;//     <i> Note that bit 23 is reserved on STM32WB35xx devices
;// </h>
FLASH_WRP1AR    EQU    0x000000FF       ; reset value: 0x????????
FLASH_WRP1BR    EQU    0x000000FF       ; reset value: 0x????????

;// <h> IPCC mailbox
;//   <o0.0..13> IPCCDBA <0x0-0x3FFF>
;//     <i> IPCC mailbox data buffer base address offset
;//     <i> Contains the first double-word offset of the IPCC mailbox data buffer area in SRAM2
;// </h>
FLASH_IPCCBR    EQU    0xFFFFC000       ; reset value: 0x????????

;// </e>


                IF      FLASH_OPT <> 0
                AREA    |.ARM.__AT_0x1FFF8000|, CODE, READONLY
                DCD     FLASH_OPTR
                DCD     FLASH_PCROP1ASR
                DCD     FLASH_PCROP1AER
                DCD     FLASH_WRP1AR
                DCD     FLASH_WRP1BR
                DCD     FLASH_PCROP1BSR
                DCD     FLASH_PCROP1BER
                DCD     FLASH_IPCCBR
                ENDIF

                END
