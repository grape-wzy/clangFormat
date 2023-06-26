/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    app_ble.c
 * @author  MCD Application Team
 * @brief   BLE Application
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */
 /* USER CODE END Header */

 /* Includes ------------------------------------------------------------------*/
#include "app_common.h"
#include "platform.h"
#include "standard_lib.h"
#include "ble.h"
#include "tl.h"
#include "app_ble.h"
#include "stm32_seq.h"
#include "shci.h"
#include "otp.h"

/* Private includes ----------------------------------------------------------*/


/* Private typedef -----------------------------------------------------------*/
#ifdef DEBUG
#if 1
#define bkpf(...) kprint(__VA_ARGS__)
#define bnpf(...) nprint(__VA_ARGS__)
#else
#define bkpf(...)
#define bnpf(...)
#endif
#else
#define bkpf(...)
#define bnpf(...)
#endif


/* Private defines -----------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
PLACE_IN_SECTION("MB_MEM1") ALIGN(4) static TL_CmdPacket_t BleCmdBuffer;
static __APP_BLE_CALLBACK_TypeDef AppBleCallBack = { 0 };
/**
*   Identity root key used to derive LTK and CSRK
*/
static const uint8_t BLE_CFG_IR_VALUE[16] = CFG_BLE_IRK;

/**
* Encryption root key used to derive LTK and CSRK
*/
static const uint8_t BLE_CFG_ER_VALUE[16] = CFG_BLE_ERK;

/* Private function prototypes -----------------------------------------------*/
static void BLE_UserEvtRx(void* pPayload);
static void BLE_StatusNot(HCI_TL_CmdStatus_t status);
static void Ble_Tl_Init(void);
static void Ble_Hci_Gap_Gatt_Init(void);


/* Functions Definition ------------------------------------------------------*/
void APP_BLE_Init(void)
{

	SHCI_C2_Ble_Init_Cmd_Packet_t ble_init_cmd_packet =
	{
	  {{0,0,0}},                          /**< Header unused */
	  {0,                                 /** pBleBufferAddress not used */
	  0,                                  /** BleBufferSize not used */
	  CFG_BLE_NUM_GATT_ATTRIBUTES,
	  CFG_BLE_NUM_GATT_SERVICES,
	  CFG_BLE_ATT_VALUE_ARRAY_SIZE,
	  CFG_BLE_NUM_LINK,
	  CFG_BLE_DATA_LENGTH_EXTENSION,
	  CFG_BLE_PREPARE_WRITE_LIST_SIZE,
	  CFG_BLE_MBLOCK_COUNT,
	  CFG_BLE_MAX_ATT_MTU,
	  CFG_BLE_SLAVE_SCA,
	  CFG_BLE_MASTER_SCA,
	  CFG_BLE_LSE_SOURCE,
	  CFG_BLE_MAX_CONN_EVENT_LENGTH,
	  CFG_BLE_HSE_STARTUP_TIME,
	  CFG_BLE_VITERBI_MODE,
	  CFG_BLE_OPTIONS,
	  0,
	  CFG_BLE_MAX_COC_INITIATOR_NBR,
	  CFG_BLE_MIN_TX_POWER,
	  CFG_BLE_MAX_TX_POWER,
	  CFG_BLE_RX_MODEL_CONFIG,
	  CFG_BLE_MAX_ADV_SET_NBR,
	  CFG_BLE_MAX_ADV_DATA_LEN,
	  CFG_BLE_TX_PATH_COMPENS,
	  CFG_BLE_RX_PATH_COMPENS
	}
	};

	/**
	 * Initialize Ble Transport Layer
	 */
	Ble_Tl_Init();

	/**
	 * Do not allow standby in the application
	 */
	 //UTIL_LPM_SetOffMode(1 << CFG_LPM_APP_BLE, UTIL_LPM_DISABLE);

	 /**
	  * Register the hci transport layer to handle BLE User Asynchronous Events
	  */
	UTIL_SEQ_RegTask(1 << CFG_TASK_HCI_ASYNCH_EVT_ID, UTIL_SEQ_RFU, hci_user_evt_proc);

	/**
	 * Starts the BLE Stack on CPU2
	 */
	if (SHCI_C2_BLE_Init(&ble_init_cmd_packet) != SHCI_Success)
	{
		//Error_Handler();
	}

	/**
	 * Initialization of HCI & GATT & GAP layer
	 */
	Ble_Hci_Gap_Gatt_Init();

	/**
	 * Initialization of the BLE Services
	 */
	 //SVCCTL_Init();
	AppBleCallBack.user_init();


	//Clock_Wait(1000);
	APP_BLE_Adv_Start();
	bkpf("ok\r\n");

	return;
}


void APP_BLE_Register_CallBack(void* cb)
{
	AppBleCallBack = *(__APP_BLE_CALLBACK_TypeDef*)cb;
}

void APP_BLE_Reset(void)
{
	Ble_Hci_Gap_Gatt_Init();

	AppBleCallBack.user_init();

	APP_BLE_Adv_Start();
}


#pragma region 广播

uint8_t APP_BLE_Adv_Start1(void)
{
#define ADV_INTERVAL_MAX_MS 1000 
#define ADV_INTERVAL_MIN_MS 1000
	uint8_t ret = aci_gap_adv_set_configuration(0, 0,
		0x0001 | 0x0002 | 0x0040,
		(ADV_INTERVAL_MIN_MS * 1000) / 625, (ADV_INTERVAL_MAX_MS * 1000) / 625,
		ADV_CH_37| ADV_CH_38| ADV_CH_39,
		0x01, 0x01, NULL, // Own_Address_Type Peer_Address_Type Peer_Address
		0x00,//Adv_Filter_Policy
		0, /* 0 dBm */ // Adv_TX_Power
		0, //Secondary_Adv_Max_Skip
		0x01,  //Secondary_Adv_PHY
		0, /* Adv_SID. */
		0 /* Scan_Req_Notification_Enable */
		);

	if (ret != BLE_STATUS_SUCCESS)
	{
		bkpf("failed, ret=0x%x\r\n", ret);
		return STD_FAILED;
	}
	else
	{
		bkpf("ok, interval=%u\r\n", (unsigned int)(ADV_INTERVAL_MAX_MS + ADV_INTERVAL_MIN_MS) / 2);
	}

	Adv_Set_t	sAdvertisingSets[1];

	sAdvertisingSets[0].Advertising_Handle = 0;
	sAdvertisingSets[0].Duration = 0;
	sAdvertisingSets[0].Max_Extended_Advertising_Events = 0;

	ret = aci_gap_adv_set_enable(ENABLE, 1, sAdvertisingSets);

	if (ret != BLE_STATUS_SUCCESS)
	{
		kprint("failed: 0x%02x\r\n", ret);
	}
	else
	{
		kprint("ok\r\n");
	}


	return STD_SUCCESS;
}

uint8_t APP_BLE_Adv_Start(void)
{
	__BLE_LOCAL_CONFIG_TypeDef* config = (__BLE_LOCAL_CONFIG_TypeDef*)AppBleCallBack.Config;

	tBleStatus ret;

	/* disable scan response */
	hci_le_set_scan_response_data(0, NULL);



	//AdvIntervMin-步进0.625ms
	ret = aci_gap_set_discoverable(ADV_IND,
		(BLE_MIN_ADV_INTERVAL_MS * 1000) / 625, (BLE_MAX_ADV_INTERVAL_MS * 1000) / 625,
		STATIC_RANDOM_ADDR, NO_WHITE_LIST_USE,
		strlen((const char*)config->Name), config->Name,
		0, NULL, 0, 0);  //very good 

	if (ret)
	{
		bkpf("failed, ret=0x%x\r\n", ret);
		return STD_FAILED;
	}
	else
	{
		bkpf("ok, interval=%u\r\n", (unsigned int)(BLE_MIN_ADV_INTERVAL_MS + BLE_MAX_ADV_INTERVAL_MS) / 2);
	}

	return STD_SUCCESS;
}

uint8_t APP_BLE_Adv_Stop(void)
{
	tBleStatus ret = aci_gap_set_non_discoverable();
	if (ret != BLE_STATUS_SUCCESS)
	{
		bkpf("failed, ret=0x%x\r\n", ret);
		return STD_FAILED;
	}

	bkpf("ok\r\n");

	return STD_SUCCESS;
}
#pragma endregion


#pragma region LOCAL FUNCTIONS
/*************************************************************
 *
 * LOCAL FUNCTIONS
 *
 *************************************************************/
static void Ble_Tl_Init(void)
{
	HCI_TL_HciInitConf_t Hci_Tl_Init_Conf;

	Hci_Tl_Init_Conf.p_cmdbuffer = (uint8_t*)&BleCmdBuffer;
	Hci_Tl_Init_Conf.StatusNotCallBack = BLE_StatusNot;
	hci_init(BLE_UserEvtRx, (void*)&Hci_Tl_Init_Conf);

	return;
}

static void Ble_Hci_Gap_Gatt_Init(void) 
{

	__BLE_LOCAL_CONFIG_TypeDef* config = (__BLE_LOCAL_CONFIG_TypeDef*)AppBleCallBack.Config;

	uint8_t role;
	uint16_t gap_service_handle, gap_dev_name_char_handle, gap_appearance_char_handle;
	uint16_t appearance[1] = { BLE_CFG_GAP_APPEARANCE };

	/**
	 * Initialize HCI layer
	 */
	 /*HCI Reset to synchronise BLE Stack*/
	hci_reset();

	/**
	 * Write the BD Address
	 */


	 /**
	  * Static random Address
	  * The two upper bits shall be set to 1
	  * The lowest 32bits is read from the UDN to differentiate between devices
	  * The RNG may be used to provide a random number on each power on
	  */

	aci_hal_write_config_data(CONFIG_DATA_RANDOM_ADDRESS_OFFSET, CONFIG_DATA_RANDOM_ADDRESS_LEN, (uint8_t*)config->ID);


	/**
	 * Write Identity root key used to derive LTK and CSRK
	 */
	aci_hal_write_config_data(CONFIG_DATA_IR_OFFSET, CONFIG_DATA_IR_LEN, (uint8_t*)BLE_CFG_IR_VALUE);

	/**
	 * Write Encryption root key used to derive LTK and CSRK
	 */
	aci_hal_write_config_data(CONFIG_DATA_ER_OFFSET, CONFIG_DATA_ER_LEN, (uint8_t*)BLE_CFG_ER_VALUE);

	/**
	 * Set TX Power to 0dBm.
	 */
	aci_hal_set_tx_power_level(1, CFG_TX_POWER);

	/**
	 * Initialize GATT interface
	 */
	aci_gatt_init();

	/**
	 * Initialize GAP interface
	 */
	role = 0;

#if (BLE_CFG_PERIPHERAL == 1)
	role |= GAP_PERIPHERAL_ROLE;
#endif

#if (BLE_CFG_CENTRAL == 1)
	role |= GAP_CENTRAL_ROLE;
#endif

	if (role > 0)
	{
		aci_gap_init(role,
#if ((CFG_BLE_ADDRESS_TYPE == RESOLVABLE_PRIVATE_ADDR) || (CFG_BLE_ADDRESS_TYPE == NON_RESOLVABLE_PRIVATE_ADDR))
			2,
#else
			0,
#endif
			strlen((const char*)config->Name) - 1,
			&gap_service_handle,
			&gap_dev_name_char_handle,
			&gap_appearance_char_handle);

		bkpf("gap handle=0x%x, dev name handle=0x%x, appearance handle=0x%x\r\n"
			, gap_service_handle, gap_dev_name_char_handle, gap_appearance_char_handle);

		if (aci_gatt_update_char_value(gap_service_handle, gap_dev_name_char_handle, 0, strlen((const char*)config->Name) - 1, (uint8_t*)&config->Name[1]))
		{
			bkpf("failed to update\r\n");
		}
	}

	if (aci_gatt_update_char_value(gap_service_handle,
		gap_appearance_char_handle,
		0,
		2,
		(uint8_t*)&appearance))
	{
		bkpf("failed\r\n");
	}


	/**
	 * Initialize Default PHY
	 */
	if (hci_le_set_default_phy(ALL_PHYS_PREFERENCE, TX_2M_PREFERRED, RX_2M_PREFERRED) != ERR_CMD_SUCCESS)
	{
		bkpf("failed to set phy\r\n");
	}

	/**
	 * Initialize IO capability
	 */
	aci_gap_set_io_capability(CFG_IO_CAPABILITY);

	/**
	 * Initialize authentication
	 */

	aci_gap_set_authentication_requirement(CFG_BONDING_MODE,
		CFG_MITM_PROTECTION,
		CFG_SC_SUPPORT,
		CFG_KEYPRESS_NOTIFICATION_SUPPORT,
		CFG_ENCRYPTION_KEY_SIZE_MIN,
		CFG_ENCRYPTION_KEY_SIZE_MAX,
		CFG_USED_FIXED_PIN,
		CFG_FIXED_PIN,
		CFG_BLE_ADDRESS_TYPE
	);

	/**
	 * Initialize whitelist
	 */
	if (CFG_BONDING_MODE)
	{
		aci_gap_configure_whitelist();
	}

	bkpf("ok\r\n");
}

#pragma endregion

#pragma region WRAP FUNCTIONS

void hci_notify_asynch_evt(void* pdata)
{
	UTIL_SEQ_SetTask(1 << CFG_TASK_HCI_ASYNCH_EVT_ID, CFG_SCH_PRIO_0);
	return;
}

void hci_cmd_resp_release(uint32_t flag)
{
	UTIL_SEQ_SetEvt(1 << CFG_IDLEEVT_HCI_CMD_EVT_RSP_ID);
	return;
}

void hci_cmd_resp_wait(uint32_t timeout)
{
	UTIL_SEQ_WaitEvt(1 << CFG_IDLEEVT_HCI_CMD_EVT_RSP_ID);
	return;
}

static void BLE_UserEvtRx(void* pPayload)
{
	//SVCCTL_UserEvtFlowStatus_t svctl_return_status;
	tHCI_UserEvtRxParam* pParam;

	pParam = (tHCI_UserEvtRxParam*)pPayload;
	AppBleCallBack.user_evt_rx((void*)&(pParam->pckt->evtserial));
	pParam->status = HCI_TL_UserEventFlow_Enable;

	//svctl_return_status = SVCCTL_UserEvtRx((void*)&(pParam->pckt->evtserial));
	//if (svctl_return_status != SVCCTL_UserEvtFlowDisable)
	//{
	//	pParam->status = HCI_TL_UserEventFlow_Enable;
	//}
	//else
	//{
	//	pParam->status = HCI_TL_UserEventFlow_Disable;
	//}
}

static void BLE_StatusNot(HCI_TL_CmdStatus_t status)
{
	uint32_t task_id_list;
	switch (status)
	{
	case HCI_TL_CmdBusy:
		/**
		 * All tasks that may send an aci/hci commands shall be listed here
		 * This is to prevent a new command is sent while one is already pending
		 */
		task_id_list = (1 << CFG_LAST_TASK_ID_WITH_HCICMD) - 1;
		UTIL_SEQ_PauseTask(task_id_list);

		break;

	case HCI_TL_CmdAvailable:
		/**
		 * All tasks that may send an aci/hci commands shall be listed here
		 * This is to prevent a new command is sent while one is already pending
		 */
		task_id_list = (1 << CFG_LAST_TASK_ID_WITH_HCICMD) - 1;
		UTIL_SEQ_ResumeTask(task_id_list);

		break;

	default:
		break;
	}
	return;
}

void SVCCTL_ResumeUserEventFlow(void)
{
	hci_resume_flow();
	return;
}

#pragma endregion
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
