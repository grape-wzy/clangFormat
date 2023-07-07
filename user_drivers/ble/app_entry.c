/**
  ******************************************************************************
  * @file    app_entry.c
  * @author  MCD Application Team
  * @brief   Entry point of the Application
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2019-2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */


  /* Includes ------------------------------------------------------------------*/
#include "app_common.h"
#include "platform.h"
#include "standard_lib.h"
#include "app_entry.h"
#include "app_ble.h"
#include "ble.h"
#include "tl.h"
#include "shci_tl.h"
#include "shci.h"
#include "otp.h"
#include "stm32_seq.h"

/* Private includes -----------------------------------------------------------*/


/* Private typedef -----------------------------------------------------------*/


/* Private defines -----------------------------------------------------------*/
#define POOL_SIZE (CFG_TLBLE_EVT_QUEUE_LENGTH*4U*DIVC(( sizeof(TL_PacketHeader_t) + TL_BLE_EVENT_FRAME_SIZE ), 4U))


/* Private macros ------------------------------------------------------------*/


/* Private variables ---------------------------------------------------------*/
PLACE_IN_SECTION("MB_MEM2") ALIGN(4) static uint8_t EvtPool[POOL_SIZE];
PLACE_IN_SECTION("MB_MEM2") ALIGN(4) static TL_CmdPacket_t SystemCmdBuffer;
PLACE_IN_SECTION("MB_MEM2") ALIGN(4) static uint8_t SystemSpareEvtBuffer[sizeof(TL_PacketHeader_t) + TL_EVT_HDR_SIZE + 255U];
PLACE_IN_SECTION("MB_MEM2") ALIGN(4) static uint8_t BleSpareEvtBuffer[sizeof(TL_PacketHeader_t) + TL_EVT_HDR_SIZE + 255];

static WirelessFwInfo_t sWFWInfo = { 0 };
static IPCC_HandleTypeDef sIPCCHandle;
static uint8_t sCPU2IsRunning = false;

/* Private functions prototypes-----------------------------------------------*/
static void Config_HSE(void);
static void Reset_Device(void);
static void System_Init(void);
static void SystemPower_Config(void);
static void appe_Tl_Init(void);

static void APPE_SysStatusNot(SHCI_TL_CmdStatus_t status);
static void APPE_SysEvtReadyProcessing(void* pPayload);
static void APPE_SysUserEvtRx(void* pPayload);


/* Functions Definition ------------------------------------------------------*/
void APPE_Init_Clock(void)
{
	/**
	 * The OPTVERR flag is wrongly set at power on
	 * It shall be cleared before using any HAL_FLASH_xxx() api
	 */
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);

	/**
	 * Reset some configurations so that the system behave in the same way
	 * when either out of nReset or Power On
	 */
	Reset_Device();

	/* Configure HSE Tuning */
	Config_HSE();

	return;
}

void APPE_Init(void)
{

	System_Init();       /**< System initialization */

	SystemPower_Config(); /**< Configure the system Power Mode */

	//APPD_Init();

	appe_Tl_Init();	/* Initialize all transport layers */

	/**
	 * From now, the application is waiting for the ready event ( VS_HCI_C2_Ready )
	 * received on the system channel before starting the Stack
	 * This system event is received with APPE_SysUserEvtRx()
	 */

	return;
}

void APPE_DeInit(void)
{

	//Reset_Device();

	//hipcc.Instance = IPCC;
	//if (HAL_IPCC_DeInit(&hipcc) != HAL_OK)
	//{
	//	kprint("failed\r\n");
	//}
	//LL_PWR_SMPS_Disable();
}

uint32_t APPE_Get_Stack_Size(void)
{
	return (sWFWInfo.MemorySizeFlash * 4 * 1024);
}

uint32_t APPE_Get_Stack_Numerical_Version(void)
{
	return (uint32_t)(sWFWInfo.VersionMajor << 16) + (uint32_t)(sWFWInfo.VersionMinor << 8) + (uint32_t)sWFWInfo.VersionSub;
}

uint32_t APPE_Get_Fus_Numerical_Version(void)
{
	return (uint32_t)(sWFWInfo.FusVersionMajor << 16) + (uint32_t)(sWFWInfo.FusVersionMinor << 8) + (uint32_t)sWFWInfo.FusVersionSub;
}


uint8_t APPE_Get_String_Version(uint8_t* stack_hw, uint8_t* stack_fw, uint8_t* fus_fw)
{

	sprintf((char*)stack_hw, "%s", BLE_MICRO_CONTROLLERS);
	sprintf((char*)stack_fw, "Stack V%d.%d.%d,Type %u", sWFWInfo.VersionMajor, sWFWInfo.VersionMinor, sWFWInfo.VersionSub, sWFWInfo.StackType);
	sprintf((char*)fus_fw, "FUS V%d.%d.%d", sWFWInfo.FusVersionMajor, sWFWInfo.FusVersionMinor, sWFWInfo.FusVersionSub);
	return BLE_STATUS_SUCCESS;
}

uint8_t APPE_Get_CPU2_Status(void)
{
	return sCPU2IsRunning;
}
/*************************************************************
 *
 * LOCAL FUNCTIONS
 *
 *************************************************************/

void Reset_BackupDomain(void)
{
	if ((LL_RCC_IsActiveFlag_PINRST() != FALSE) && (LL_RCC_IsActiveFlag_SFTRST() == FALSE))
	{
		HAL_PWR_EnableBkUpAccess(); /**< Enable access to the RTC registers */

		/**
		 *  Write twice the value to flush the APB-AHB bridge
		 *  This bit shall be written in the register before writing the next one
		 */
		HAL_PWR_EnableBkUpAccess();

		__HAL_RCC_BACKUPRESET_FORCE();
		__HAL_RCC_BACKUPRESET_RELEASE();
	}

	return;
}

void Reset_IPCC(void)
{
	LL_AHB3_GRP1_EnableClock(LL_AHB3_GRP1_PERIPH_IPCC);

	LL_C1_IPCC_ClearFlag_CHx(
		IPCC,
		LL_IPCC_CHANNEL_1 | LL_IPCC_CHANNEL_2 | LL_IPCC_CHANNEL_3 | LL_IPCC_CHANNEL_4
		| LL_IPCC_CHANNEL_5 | LL_IPCC_CHANNEL_6);

	LL_C2_IPCC_ClearFlag_CHx(
		IPCC,
		LL_IPCC_CHANNEL_1 | LL_IPCC_CHANNEL_2 | LL_IPCC_CHANNEL_3 | LL_IPCC_CHANNEL_4
		| LL_IPCC_CHANNEL_5 | LL_IPCC_CHANNEL_6);

	LL_C1_IPCC_DisableTransmitChannel(
		IPCC,
		LL_IPCC_CHANNEL_1 | LL_IPCC_CHANNEL_2 | LL_IPCC_CHANNEL_3 | LL_IPCC_CHANNEL_4
		| LL_IPCC_CHANNEL_5 | LL_IPCC_CHANNEL_6);

	LL_C2_IPCC_DisableTransmitChannel(
		IPCC,
		LL_IPCC_CHANNEL_1 | LL_IPCC_CHANNEL_2 | LL_IPCC_CHANNEL_3 | LL_IPCC_CHANNEL_4
		| LL_IPCC_CHANNEL_5 | LL_IPCC_CHANNEL_6);

	LL_C1_IPCC_DisableReceiveChannel(
		IPCC,
		LL_IPCC_CHANNEL_1 | LL_IPCC_CHANNEL_2 | LL_IPCC_CHANNEL_3 | LL_IPCC_CHANNEL_4
		| LL_IPCC_CHANNEL_5 | LL_IPCC_CHANNEL_6);

	LL_C2_IPCC_DisableReceiveChannel(
		IPCC,
		LL_IPCC_CHANNEL_1 | LL_IPCC_CHANNEL_2 | LL_IPCC_CHANNEL_3 | LL_IPCC_CHANNEL_4
		| LL_IPCC_CHANNEL_5 | LL_IPCC_CHANNEL_6);

	return;
}

void Reset_Device(void)
{
#if ( CFG_HW_RESET_BY_FW == 1 )
	Reset_BackupDomain();

	Reset_IPCC();
#endif /* CFG_HW_RESET_BY_FW */

	return;
}

void Config_HSE(void)
{
	OTP_ID0_t* p_otp;

	/**
	 * Read HSE_Tuning from OTP
	 */
	p_otp = (OTP_ID0_t*)OTP_Read(0);
	if (p_otp)
	{
		LL_RCC_HSE_SetCapacitorTuning(p_otp->hse_tuning);
	}

	return;
}

void Init_IPCC(void)
{
	sIPCCHandle.Instance = IPCC;
	if (HAL_IPCC_Init(&sIPCCHandle) != HAL_OK)
	{
	}
}
#if 0
void HAL_IPCC_MspInit(IPCC_HandleTypeDef* hipcc)
{
	if (hipcc->Instance == IPCC)
	{
		/* Peripheral clock enable */
		__HAL_RCC_IPCC_CLK_ENABLE();
		/* IPCC interrupt Init */

		HAL_NVIC_SetPriority(IPCC_C1_RX_IRQn, IPCC_C1_RX_NVIC_PreemptionPriority, IPCC_C1_RX_NVIC_SubPriority);
		HAL_NVIC_EnableIRQ(IPCC_C1_RX_IRQn);
		HAL_NVIC_SetPriority(IPCC_C1_TX_IRQn, IPCC_C1_TX_NVIC_PreemptionPriority, IPCC_C1_TX_NVIC_SubPriority);
		HAL_NVIC_EnableIRQ(IPCC_C1_TX_IRQn);
	}

}
#endif

void Init_Smps(void)
{
#if (CFG_USE_SMPS != 0)
	/**
	 *  Configure and enable SMPS
	 *
	 *  The SMPS configuration is not yet supported by CubeMx
	 *  when SMPS output voltage is set to 1.4V, the RF output power is limited to 3.7dBm
	 *  the SMPS output voltage shall be increased for higher RF output power
	 */
	 //1.7 - 6db, 1.4 - 3.7db
	LL_PWR_SMPS_SetStartupCurrent(LL_PWR_SMPS_STARTUP_CURRENT_80MA);
	LL_PWR_SMPS_SetOutputVoltageLevel(LL_PWR_SMPS_OUTPUT_VOLTAGE_1V40);
	LL_PWR_SMPS_Enable();
#endif
}

void Init_Exti(void)
{
	/**< Disable all wakeup interrupt on CPU1  except IPCC(36), HSEM(38) */
	//LL_EXTI_DisableIT_0_31((~0)& (~RTC_EXTI_LINE_WAKEUPTIMER_EVENT)&(~EXTI_IMR1_IM4));
	//LL_EXTI_DisableIT_32_63((~0) & (~(LL_EXTI_LINE_36 | LL_EXTI_LINE_38)));
	LL_EXTI_EnableIT_32_63(LL_EXTI_LINE_36 & LL_EXTI_LINE_38);
	return;
}

void System_Init(void)
{

	Init_IPCC();

	Init_Smps();

	Init_Exti();

	return;
}

void SystemPower_Config(void)
{
	/**
	 * Select HSI as system clock source after Wake Up from Stop mode
	 */
	LL_RCC_SetClkAfterWakeFromStop(LL_RCC_STOP_WAKEUPCLOCK_HSI);

	/* Initialize low power manager */
	//UTIL_LPM_Init();
	/* Initialize the CPU2 reset value before starting CPU2 with C2BOOT */
	LL_C2_PWR_SetPowerMode(LL_PWR_MODE_SHUTDOWN);

#if (CFG_USB_INTERFACE_ENABLE != 0)
	/**
	 *  Enable USB power
	 */
	HAL_PWREx_EnableVddUSB();
#endif

	/**
	* Active SRAM retention for standby support
	*/
	//HAL_PWREx_EnableSRAMRetention();
	return;
}

void appe_Tl_Init(void)
{

	TL_MM_Config_t tl_mm_config;
	SHCI_TL_HciInitConf_t SHci_Tl_Init_Conf;
	/**< Reference table initialization */
	TL_Init();

	UTIL_SEQ_RegTask(1 << CFG_TASK_SYSTEM_HCI_ASYNCH_EVT_ID, UTIL_SEQ_RFU, shci_user_evt_proc);

	/**< System channel initialization */
	SHci_Tl_Init_Conf.p_cmdbuffer = (uint8_t*)&SystemCmdBuffer;
	SHci_Tl_Init_Conf.StatusNotCallBack = APPE_SysStatusNot;
	shci_init(APPE_SysUserEvtRx, (void*)&SHci_Tl_Init_Conf);

	/**< Memory Manager channel initialization */
	tl_mm_config.p_BleSpareEvtBuffer = BleSpareEvtBuffer;
	tl_mm_config.p_SystemSpareEvtBuffer = SystemSpareEvtBuffer;
	tl_mm_config.p_AsynchEvtPool = EvtPool;
	tl_mm_config.AsynchEvtPoolSize = POOL_SIZE;
	TL_MM_Init(&tl_mm_config);

	TL_Enable();

	return;
}


#pragma region 任务

/**
 * The type of the payload for a system user event is tSHCI_UserEvtRxParam
 * When the system event is both :
 *    - a ready event (subevtcode = SHCI_SUB_EVT_CODE_READY)
 *    - reported by the FUS (sysevt_ready_rsp == FUS_FW_RUNNING)
 * The buffer shall not be released
 * ( eg ((tSHCI_UserEvtRxParam*)pPayload)->status shall be set to SHCI_TL_UserEventFlow_Disable )
 * When the status is not filled, the buffer is released by default
 */
void APPE_SysUserEvtRx(void* pPayload)
{
	TL_AsynchEvt_t* p_sys_event;
	p_sys_event = (TL_AsynchEvt_t*)(((tSHCI_UserEvtRxParam*)pPayload)->pckt->evtserial.evt.payload);
	//UNUSED(pPayload);
	nprint("\r\n\r\n");
	kprint("sub evt code=0x%x\r\n", p_sys_event->subevtcode);
	switch (p_sys_event->subevtcode)
	{
	case SHCI_SUB_EVT_CODE_READY:
	{
		sCPU2IsRunning = true;

		SHCI_GetWirelessFwInfo(&sWFWInfo);

		kprint("Wireless Firmware version %d.%d.%d\r\n", sWFWInfo.VersionMajor, sWFWInfo.VersionMinor, sWFWInfo.VersionSub);
		kprint("Wireless Firmware branch %d\r\n", sWFWInfo.VersionBranch);
		kprint("Wireless Firmware Release %d\r\n", sWFWInfo.VersionReleaseType);
		//stack 1 是full协议栈， stack 3是light协议栈
		kprint("Wireless Firmware Stack %d， size=0x%x\r\n", sWFWInfo.StackType, sWFWInfo.MemorySizeFlash);
		kprint("FUS version %d.%d.%d\r\n", sWFWInfo.FusVersionMajor, sWFWInfo.FusVersionMinor, sWFWInfo.FusVersionSub);

		APPE_SysEvtReadyProcessing(pPayload);
	}
	break;
	default:
	{
		kprint("unknown event code(0x%x)\r\n", p_sys_event->subevtcode);
	}
	break;
	}
	return;
}

void shci_notify_asynch_evt(void* pdata)
{
	UNUSED(pdata);
	UTIL_SEQ_SetTask(1 << CFG_TASK_SYSTEM_HCI_ASYNCH_EVT_ID, CFG_SCH_PRIO_0);
	return;
}

void shci_cmd_resp_release(uint32_t flag)
{
	UNUSED(flag);
	UTIL_SEQ_SetEvt(1 << CFG_IDLEEVT_SYSTEM_HCI_CMD_EVT_RSP_ID);
	return;
}

void shci_cmd_resp_wait(uint32_t timeout)
{
	UNUSED(timeout);
	UTIL_SEQ_WaitEvt(1 << CFG_IDLEEVT_SYSTEM_HCI_CMD_EVT_RSP_ID);
	return;
}

void APPE_SysStatusNot(SHCI_TL_CmdStatus_t status)
{
	UNUSED(status);
	return;
}

#pragma endregion

#pragma region 事件

void APPE_SysEvtReadyProcessing(void* pPayload)
{
	TL_AsynchEvt_t* p_sys_event;
	SHCI_C2_Ready_Evt_t* p_sys_ready_event;

	p_sys_event = (TL_AsynchEvt_t*)(((tSHCI_UserEvtRxParam*)pPayload)->pckt->evtserial.evt.payload);
	p_sys_ready_event = (SHCI_C2_Ready_Evt_t*)p_sys_event->payload;

	if (p_sys_ready_event->sysevt_ready_rsp == WIRELESS_FW_RUNNING)
	{
		uint8_t msg = read_reboot_msg();
		if (msg == CFG_OTA_REBOOT_ON_CPU2_START_FUS)
		{
			uint8_t state = SHCI_C2_FUS_GetState(NULL);
			kprint("state=0x%x\r\n", state);
			log_flush();
			state = SHCI_C2_FUS_GetState(NULL);
			kprint("state=0x%x\r\n", state);
			log_flush();
		}
		else if ((msg > CFG_OTA_REBOOT_ON_CPU2_START_FUS) && (msg < CFG_OTA_REBOOT_ON_CPU2_UPGRADED))
		{
			set_reboot_msg(CFG_OTA_REBOOT_ON_CPU2_UPGRADED);
			kprint("upgraded fw\r\n");
			log_flush();
			NVIC_SystemReset();
		}

		/**
		 * The wireless firmware is running on the CPU2
		 */
		kprint("code ready - wireless fw is running \r\n");

		/* Traces channel initialization */
		//APPD_EnableCPU2();
#if 0
		SHCI_C2_CONFIG_Cmd_Param_t config_param = { 0 };
		/* Enable all events Notification */
		config_param.PayloadCmdSize = SHCI_C2_CONFIG_PAYLOAD_CMD_SIZE;
		config_param.EvtMask1 = SHCI_C2_CONFIG_EVTMASK1_BIT0_ERROR_NOTIF_ENABLE
			+ SHCI_C2_CONFIG_EVTMASK1_BIT1_BLE_NVM_RAM_UPDATE_ENABLE
			+ SHCI_C2_CONFIG_EVTMASK1_BIT2_THREAD_NVM_RAM_UPDATE_ENABLE
			+ SHCI_C2_CONFIG_EVTMASK1_BIT3_NVM_START_WRITE_ENABLE
			+ SHCI_C2_CONFIG_EVTMASK1_BIT4_NVM_END_WRITE_ENABLE
			+ SHCI_C2_CONFIG_EVTMASK1_BIT5_NVM_START_ERASE_ENABLE
			+ SHCI_C2_CONFIG_EVTMASK1_BIT6_NVM_END_ERASE_ENABLE;
		(void)SHCI_C2_Config(&config_param);
#endif
		APP_BLE_Init();
	}
	else
	{
		/**
		 * The FUS firmware is running on the CPU2
		 * In the scope of this application, there should be no case when we get here
		 */
		kprint("only fus fw is running!!!!!!! \r\n");
		/* The packet shall not be released as this is not supported by the FUS */
		((tSHCI_UserEvtRxParam*)pPayload)->status = SHCI_TL_UserEventFlow_Disable;


		uint8_t state = SHCI_C2_FUS_GetState(NULL);
		if (state == FUS_STATE_VALUE_ERROR)
		{
			/**
			* This is the first time in the life of the product the FUS is involved. After this command, it will be properly initialized
			* Request the device to reboot to install the wireless firmware
			*/
			kprint("reset\r\n");
			log_flush();
			NVIC_SystemReset(); /* it waits until reset */
		}
		else if (state != 0)
		{
			kprint("fus busy 1, 0x%x\r\n", state);
			while (1)
			{


				/**
				 * Wait for the FUS to reboot the system when the upgrade is done
				 * In case an error is detected during the upgrade process, restart the device
				 * The BLE_Ota state machine will request a SHCI_C2_FUS_StartWs() on the next reboot.
				 */
				Clock_Wait(1000);   /* Poll the FUS each 10s to make sure process is going fine */
				state = SHCI_C2_FUS_GetState(NULL);
				kprint("fus busy 2, 0x%x\r\n", state);
				log_flush();
				if ((state < FUS_STATE_VALUE_FW_UPGRD_ONGOING) || (state > FUS_STATE_VALUE_FUS_UPGRD_ONGOING_END))
				{
					kprint("fus idle, 0x%x\r\n", state);
					log_flush();
					//几乎运行不到这里， 运行到这里时，基本出错了
					NVIC_SystemReset();
				}
			}
		}
		else//==0
		{
			uint8_t msg = read_reboot_msg();

			//第一步,删除fw
			if (msg == CFG_OTA_REBOOT_ON_CPU2_START_FUS)
			{
				kprint("fus start deleting stack\r\n");
				log_flush();
				set_reboot_msg(CFG_OTA_REBOOT_ON_CPU2_DELETED_FW);
				SHCI_C2_FUS_FwDelete();

				while (1)
				{
					Clock_Wait(1000);
					state = SHCI_C2_FUS_GetState(NULL);
					kprint("deleting: state=0x%x\r\n", state);
					log_flush();
					if ((state < FUS_STATE_VALUE_FW_UPGRD_ONGOING) || (state > FUS_STATE_VALUE_FUS_UPGRD_ONGOING_END))
					{
						kprint("end deleting: state=0x%x\r\n", state);
						log_flush();
						NVIC_SystemReset();
					}
				}
			}
			else if (msg == CFG_OTA_REBOOT_ON_CPU2_WRITE_FW)
			{
				kprint("fus start upgrading stack\r\n");
				log_flush();
				set_reboot_msg(CFG_OTA_REBOOT_ON_CPU2_UPGRADING);
				SHCI_C2_FUS_FwUpgrade(0, 0);
				while (1)
				{
					Clock_Wait(1000);
					state = SHCI_C2_FUS_GetState(NULL);
					kprint("upgrading: state=0x%x\r\n", state);
					log_flush();
					if ((state < FUS_STATE_VALUE_FW_UPGRD_ONGOING) || (state > FUS_STATE_VALUE_FUS_UPGRD_ONGOING_END))
					{
						kprint("end upgrading: state=0x%x\r\n", state);
						log_flush();
						NVIC_SystemReset();
					}
				}
			}
			else if (msg == CFG_OTA_REBOOT_ON_CPU2_UPGRADING)
			{
				kprint("fus upgraded stack\r\n");

				//stack已经有版本号，说明是蓝牙stack而非fus,直接复位
				uint32_t version = APPE_Get_Stack_Numerical_Version();
				if (version != 0)
				{

					kprint("stack version=0x%x, and start stack\r\n", (unsigned int)version);
					log_flush();

					set_reboot_msg(CFG_OTA_REBOOT_ON_CPU2_UPGRADED);

					SHCI_C2_FUS_StartWs();
					Clock_Wait(50);
				}
				else
				{
					kprint("failed to update stack, version is 0, and again\r\n");
					//msg = CFG_OTA_REBOOT_ON_CPU2_START_FUS;
					set_reboot_msg(CFG_OTA_REBOOT_ON_CPU2_UPGRADED);
					log_flush();
				}
				NVIC_SystemReset();
			}
			//其他乱七八糟的状态，直接尝试启动蓝牙协议栈
			else
			{
				kprint("it is a illegal message(0x%x)\r\n", (unsigned int)msg);
				log_flush();
				//stack已经有版本号，说明是蓝牙stack已经存在，启动蓝牙协议栈,然后复位
				uint32_t version = APPE_Get_Stack_Numerical_Version();
				if (version != 0)
				{
					kprint("stack version=0x%x, , try to start stack\r\n", (unsigned int)version);
					log_flush();

					set_reboot_msg(CFG_OTA_REBOOT_ON_CPU2_FW_CHECKED);

					SHCI_C2_FUS_StartWs();
					Clock_Wait(50);
					NVIC_SystemReset();
				}
				else
				{
					kprint("version = 0, use st link to download stack\r\n");
				}
            }
		}
	}
}

#pragma endregion

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
