/*******************************************************************************
* file     flash_ble_if.c
* author   mackgim
* version  V1.0.0
* date
* brief ：
*******************************************************************************/

#include "flash_driver.h"
#include "standard_lib.h"
#include "platform.h"
#include "app_common.h"
#include "shci.h"



#pragma region WB FLASH 操作定义

//WB55专属
/* Private typedef -----------------------------------------------------------*/
typedef enum
{
	SEM_LOCK_SUCCESSFUL,
	SEM_LOCK_BUSY,
}SemStatus_t;

typedef enum
{
	FLASH_ERASE,
	FLASH_WRITE,
}FlashOperationType_t;

/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static uint8_t ProcessSingleFlashOperation(FlashOperationType_t FlashOperationType,
	uint32_t SectorNumberOrDestAddress,
	uint64_t Data);

/* Includes ------------------------------------------------------------------*/

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/

typedef enum
{
	WAITED_SEM_BUSY,
	WAITED_SEM_FREE,
}WaitedSemStatus_t;

typedef enum
{
	WAIT_FOR_SEM_BLOCK_FLASH_REQ_BY_CPU1,
	WAIT_FOR_SEM_BLOCK_FLASH_REQ_BY_CPU2,
}WaitedSemId_t;

/* Exported functions ------------------------------------------------------- */
void FD_Init(void);

/**
 * @brief  Implements the Dual core algorithm to erase multiple sectors in flash with CPU1
 *         It calls for each sector to be erased the API FD_EraseSingleSector()
 *
 * @param  FirstSector:   The first sector to be erased
 *                        This parameter must be a value between 0 and (SFSA - 1)
 * @param  NbrOfSectors:  The number of sectors to erase
 *                        This parameter must be a value between 1 and (SFSA - FirstSector)
 * @retval Number of sectors not erased:
 *                        Depending on the implementation of FD_WaitForSemAvailable(),
 *                        it may still have some sectors not erased when the timing protection has been
 *                        enabled by either CPU1 or CPU2. When the value returned is not 0, the application
 *                        should wait until both timing protection before retrying to erase the last missing sectors.
 *
 *                        In addition, When the returned value is not 0:
 *                        - The Sem2 is NOT released
 *                        - The FLASH is NOT locked
 *                        - SHCI_C2_FLASH_EraseActivity(ERASE_ACTIVITY_OFF) is NOT called
 *                        It is expected that the user will call one more time this function to finish the process
 */
uint8_t FD_EraseSectors(uint32_t FirstSector, uint32_t NbrOfSectors);

/**
 * @brief  Implements the Dual core algorithm to write multiple 64bits data in flash with CPU1
 *         The user shall first make sure the location to be written has been first erase.
 *         Otherwise, the API will loop for ever as it will be not able to write in flash
 *         The only value that can be written even though the destination is not erased is 0.
 *         It calls for each 64bits to be written the API FD_WriteSingleData()
 *
 * @param  DestAddress: Address of the flash to write the first data. It shall be 64bits aligned
 * @param  pSrcBuffer:  Address of the buffer holding the 64bits data to be written in flash
 * @param  NbrOfData:   Number of byte data to be written
 * @retval Number of 64bits data not written:
 *                      Depending on the implementation of FD_WaitForSemAvailable(),
 *                      it may still have 64bits data not written when the timing protection has been
 *                      enabled by either CPU1 or CPU2. When the value returned is not 0, the application
 *                      should wait until both timing protection before retrying to write the last missing 64bits data.
 *
 *                      In addition, When the returned value is not 0:
 *                        - The Sem2 is NOT released
 *                        - The FLASH is NOT locked
 *                        It is expected that the user will call one more time this function to finish the process
 */
uint8_t FD_WriteData(uint32_t DestAddress, const uint8_t* pSrcBuffer, uint32_t NbrOfData);

/**
 * @brief  Implements the Dual core algorithm to erase one sector in flash with CPU1
 *
 *         It expects the following point before calling this API:
 *         - The Sem2 is taken
 *         - The FLASH is unlocked
 *         - SHCI_C2_FLASH_EraseActivity(ERASE_ACTIVITY_ON) has been called
 *         It expects the following point to be done when no more sectors need to be erased
 *         - The Sem2 is released
 *         - The FLASH is locked
 *         - SHCI_C2_FLASH_EraseActivity(ERASE_ACTIVITY_OFF) is called
 *
 *         The two point above are implemented in FD_EraseSectors()
 *         This API needs to be used instead of FD_EraseSectors() in case a provided library is taking
 *         care of these two points and request only a single operation.
 *
 * @param  FirstSector:   The sector to be erased
 *                        This parameter must be a value between 0 and (SFSA - 1)
 * @retval: SINGLE_FLASH_OPERATION_DONE -> The data has been written
 *          SINGLE_FLASH_OPERATION_NOT_EXECUTED -> The data has not been written due to timing protection
 *                                         from either CPU1 or CPU2. On a failure status, the user should check
 *                                         both timing protection before retrying.
 */
uint8_t FD_EraseSingleSector(uint32_t SectorNumber);

/**
 * @brief  Implements the Dual core algorithm to write one 64bits data in flash with CPU1
 *         The user shall first make sure the location to be written has been first erase.
 *         Otherwise, the API will loop for ever as it will be not able to write in flash
 *         The only value that can be written even though the destination is not erased is 0.
 *
 *         It expects the following point before calling this API:
 *         - The Sem2 is taken
 *         - The FLASH is unlocked
 *         It expects the following point to be done when no more sectors need to be erased
 *         - The Sem2 is released
 *         - The FLASH is locked
 *
 *         The two point above are implemented in FD_WriteData()
 *         This API needs to be used instead of FD_WriteData() in case a provided library is taking
 *         care of these two points and request only a single operation.
 *
 * @param  DestAddress: Address of the flash to write the data. It shall be 64bits aligned
 * @param  Data:  64bits Data to be written
 * @retval: SINGLE_FLASH_OPERATION_DONE -> The data has been written
 *          SINGLE_FLASH_OPERATION_NOT_EXECUTED -> The data has not been written due to timing protection
 *                                         from either CPU1 or CPU2. On a failure status, the user should check
 *                                         both timing protection before retrying.
 */
uint8_t FD_WriteSingleData(uint32_t DestAddress, uint64_t Data);

/**
 * By default, this function is implemented weakly in flash_driver.c to return WAITED_SEM_BUSY.
 * When the semaphore is busy, this will result in either FD_WriteSingleData() or FD_EraseSingleSector()
 * to loop until the semaphore is free.
 *
 * This function may be implemented so that when using either an OS or the UTIL_SEQ_WaitEvt() API from the sequencer,
 * it could possible to run other tasks or enter idle mode until the waited semaphore is free.
 * This function shall not take the waited semaphore but just return when it is free.
 *
 * @param  WaitedSemId: The semaphore ID this function should not return until it is free
 * @retval: WAITED_SEM_BUSY -> The function returned before waiting for the semaphore to be free. This will exit the loop
 *                             from either FD_EraseSingleSector() or FD_WriteSingleData() and the number of actions left to
 *                             be processed are reported to the user
 *          WAITED_SEM_FREE -> The semaphore has been checked as free. Both FD_EraseSingleSector() and FD_WriteSingleData()
 *                             try again to process one more time the flash.
 */
WaitedSemStatus_t FD_WaitForSemAvailable(WaitedSemId_t WaitedSemId);
HAL_StatusTypeDef hal_flash_ll_progrm(uint32_t TypeProgram, uint32_t Address, uint64_t Data);
HAL_StatusTypeDef hal_flash_ll_erase(FLASH_EraseInitTypeDef* pEraseInit, uint32_t* PageError);

#pragma endregion

#pragma region 基本功能
typedef uint8_t(*FLASH_BLE_IS_INIT_CALLBACK_TYPE) (void);
static FLASH_BLE_IS_INIT_CALLBACK_TYPE flash_ble_is_init;
void flash_register_callback(void* cb)
{
	flash_ble_is_init = cb;
}

uint8_t flash_ll_erase(uint32_t startaddr, uint32_t endaddr)
{
	//FD_Init();
	return FD_EraseSectors(startaddr, endaddr);
}
//需要64bit对齐
uint8_t flash_ll_write(uint32_t addr, void const* buff, uint32_t buffsize)
{
	return FD_WriteData(addr, buff, buffsize);
}

uint8_t flash_erase_shadow_code(void)
{
	return flash_ll_erase(FLASH_SPACE_SHADOW_BEGIN_ADDR, FLASH_SPACE_SHADOW_END_ADDR);
}
//需要64bit对齐
uint8_t flash_write_shadow_code(uint32_t addr, uint32_t* buff, uint32_t buffsize)
{
	return FD_WriteData(addr, (const uint8_t*)buff, buffsize);
}
#pragma endregion

#pragma region Flash功能

void FD_Init(void)
{
	/* Unlock the Program memory */
	HAL_FLASH_Unlock();

	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_PROGERR | FLASH_FLAG_WRPERR |
		FLASH_FLAG_PGAERR | FLASH_FLAG_SIZERR | FLASH_FLAG_PGSERR | FLASH_FLAG_MISERR | FLASH_FLAG_FASTERR |
		FLASH_FLAG_RDERR | FLASH_FLAG_OPTVERR | FLASH_FLAG_PESD
		| FLASH_FLAG_ECCC | FLASH_FLAG_ECCD);
	/* Unlock the Program memory */
	//| FLASH_FLAG_PEMPTY 
	HAL_FLASH_Lock();
}

/* Public functions ----------------------------------------------------------*/
uint8_t FD_EraseSectors(uint32_t startaddr, uint32_t endaddr)
{

	if (startaddr & (FLASH_SPACE_PAGE_SIZE - 1))
	{
		kprint("addr 0x%x, is not page addr\r\n", (unsigned int)startaddr);
		return STD_FAILED;
	}

	uint32_t FirstSector = 0, NbrOfSectors = 0;

	FirstSector = FLASH_GET_PAGE_INDEX(startaddr);

	NbrOfSectors = (endaddr - startaddr) / FLASH_SPACE_PAGE_SIZE;
	if ((endaddr - startaddr) % FLASH_SPACE_PAGE_SIZE)
	{
		NbrOfSectors++;
	}
	kprint("start, addr=0x%x, sectors=%u\r\n", (unsigned int)startaddr, (unsigned int)NbrOfSectors);

	uint8_t ret = STD_SUCCESS;

	/**
	 *  Take the semaphore to take ownership of the Flash IP
	 */
	while (LL_HSEM_1StepLock(HSEM, CFG_HW_FLASH_SEMID));

	HAL_FLASH_Unlock();

	/**
	 *  Notify the CPU2 that some flash erase activity may be executed
	 *  On reception of this command, the CPU2 enables the BLE timing protection versus flash erase processing
	 *  The Erase flash activity will be executed only when the BLE RF is idle for at least 25ms
	 *  The CPU2 will prevent all flash activity (write or erase) in all cases when the BL RF Idle is shorter than 25ms.
	 */


	if ((flash_ble_is_init != NULL) && flash_ble_is_init())
	{
		SHCI_C2_FLASH_EraseActivity(ERASE_ACTIVITY_ON);
	}


	for (uint32_t i = 0; (i < NbrOfSectors) && (ret == STD_SUCCESS); i++)
	{
		ret = FD_EraseSingleSector(FirstSector + i);
	}
	/**
	*  Notify the CPU2 there will be no request anymore to erase the flash
	*  On reception of this command, the CPU2 disables the BLE timing protection versus flash erase processing
	*/
	if ((flash_ble_is_init != NULL) && flash_ble_is_init())
	{
		SHCI_C2_FLASH_EraseActivity(ERASE_ACTIVITY_OFF);
	}

	HAL_FLASH_Lock();

	/**
	 *  Release the ownership of the Flash IP
	 */
	LL_HSEM_ReleaseLock(HSEM, CFG_HW_FLASH_SEMID, 0);

	kprint("erase, ret=0x%x\r\n", ret);
	return ret;
}

// NbrOfByteData 为字节数
uint8_t FD_WriteData(uint32_t DestAddress, const uint8_t* pSrcBuffer, uint32_t NbrOfByteData)
{

#if 0
	kprint("add=0x%x, len=%u, ", (unsigned int)DestAddress, (unsigned int)NbrOfByteData);

	for (uint32_t i = 0; i < NbrOfByteData; i++)
	{
		nprint("0x%x, ", pSrcBuffer[i]);
	}
	nprint("\r\n");

#endif

	uint32_t buffsize_64 = 0;
	uint64_t data = 0;
	uint8_t* datap = (uint8_t*)&data;

	if (NbrOfByteData % 8)
	{
		kprint("it(%u) is not 8 multiples\r\n", (unsigned int)NbrOfByteData);
		return STD_FAILED;
	}

	if (!is_long_long_aligned((void const*)DestAddress))
	{

		kprint("dest addr(0x%x) is not double word\r\n", (unsigned int)DestAddress);
		return STD_FAILED;
	}

	buffsize_64 = NbrOfByteData / 8;

	//判断是否32位对齐
	uint32_t* psrc32 = NULL;
	if (is_word_aligned((void const*)pSrcBuffer))
	{
		psrc32 = (uint32_t*)pSrcBuffer;
	}

	uint8_t ret = STD_SUCCESS;

	/**
	 *  Take the semaphore to take ownership of the Flash IP
	 */
	while (LL_HSEM_1StepLock(HSEM, CFG_HW_FLASH_SEMID));

	HAL_FLASH_Unlock();

	for (uint32_t i = 0; (i < buffsize_64) && (ret == STD_SUCCESS); i++)
	{
		//32位操作
		if (psrc32)
		{
			data = (uint64_t)psrc32[2 * i] + ((((uint64_t)psrc32[2 * i + 1]) & 0xffffffff) << 32);
		}
		//byte操作
		else
		{
			for (uint32_t j = 0; j < 8; j++)
			{
				datap[j] = pSrcBuffer[8 * i + j];
			}
		}

		uint64_t x = *(uint64_t*)(DestAddress + (8 * i));
		if (x != data)
		{
			ret = FD_WriteSingleData(DestAddress + (8 * i), data);

			if (ret != STD_SUCCESS)
			{
				break;
			}
		}
	}

	HAL_FLASH_Lock();

	/**
	 *  Release the ownership of the Flash IP
	 */
	LL_HSEM_ReleaseLock(HSEM, CFG_HW_FLASH_SEMID, 0);


	if (ret != STD_SUCCESS)
	{
		kprint("hw error\r\n");
		return ret;
	}
	for (uint32_t i = 0; i < (NbrOfByteData); i++)
	{
		if (pSrcBuffer[i] != *(uint8_t*)(DestAddress + i))
		{
			kprint("data error, addr=0x%x, offset=%u, 0x%x-0x%x\r\n", (unsigned int)DestAddress, (unsigned int)i, pSrcBuffer[i], *(uint8_t*)(DestAddress + i));
			return STD_FAILED;
		}
	}
	return ret;
}

uint8_t FD_EraseSingleSector(uint32_t SectorNumber)
{
	uint8_t ret;

	/* The last parameter is unused in that case and set to 0 */
	ret = ProcessSingleFlashOperation(FLASH_ERASE, SectorNumber, 0);

	return ret;
}

uint8_t FD_WriteSingleData(uint32_t DestAddress, uint64_t Data)
{
	uint8_t ret;

	ret = ProcessSingleFlashOperation(FLASH_WRITE, DestAddress, Data);

	return ret;
}

/*************************************************************
 *
 * LOCAL FUNCTIONS
 *
 *************************************************************/
static uint8_t ProcessSingleFlashOperation(FlashOperationType_t FlashOperationType,
	uint32_t SectorNumberOrDestAddress,
	uint64_t Data)
{
	SemStatus_t cpu1_sem_status = SEM_LOCK_BUSY;
	SemStatus_t cpu2_sem_status = SEM_LOCK_BUSY;
	WaitedSemStatus_t waited_sem_status;
	uint8_t return_status = STD_FAILED;

	uint32_t page_error;
	FLASH_EraseInitTypeDef p_erase_init;

	waited_sem_status = WAITED_SEM_FREE;

	p_erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
	p_erase_init.NbPages = 1;
	p_erase_init.Page = SectorNumberOrDestAddress;

	uint64_t timeout = Clock_Time() + 5000;

	do
	{

		if (Clock_Time() > timeout)
		{
			kprint("timeout1\r\n");
			return STD_FAILED;
		}
		/**
		 * When the PESD bit mechanism is used by CPU2 to protect its timing, the PESD bit should be polled here.
		 * If the PESD is set, the CPU1 will be stalled when reading literals from an ISR that may occur after
		 * the flash processing has been requested but suspended due to the PESD bit.
		 *
		 * Note: This code is required only when the PESD mechanism is used to protect the CPU2 timing.
		 * However, keeping that code make it compatible with the two mechanisms.
		 */
		while (LL_FLASH_IsActiveFlag_OperationSuspended())
		{
			if (Clock_Time() > timeout)
			{
				kprint("timeout2\r\n");
				return STD_FAILED;
			}
		}

		ATOMIC_SECTION_BEGIN();

		/**
		 *  Depending on the application implementation, in case a multitasking is possible with an OS,
		 *  it should be checked here if another task in the application disallowed flash processing to protect
		 *  some latency in critical code execution
		 *  When flash processing is ongoing, the CPU cannot access the flash anymore.
		 *  Trying to access the flash during that time stalls the CPU.
		 *  The only way for CPU1 to disallow flash processing is to take CFG_HW_BLOCK_FLASH_REQ_BY_CPU1_SEMID.
		 */
		cpu1_sem_status = (SemStatus_t)LL_HSEM_GetStatus(HSEM, CFG_HW_BLOCK_FLASH_REQ_BY_CPU1_SEMID);
		if (cpu1_sem_status == SEM_LOCK_SUCCESSFUL)
		{
			/**
			 *  Check now if the CPU2 disallows flash processing to protect its timing.
			 *  If the semaphore is locked, the CPU2 does not allow flash processing
			 *
			 *  Note: By default, the CPU2 uses the PESD mechanism to protect its timing,
			 *  therefore, it is useless to get/release the semaphore.
			 *
			 *  However, keeping that code make it compatible with the two mechanisms.
			 *  The protection by semaphore is enabled on CPU2 side with the command SHCI_C2_SetFlashActivityControl()
			 *
			 */
			cpu2_sem_status = (SemStatus_t)LL_HSEM_1StepLock(HSEM, CFG_HW_BLOCK_FLASH_REQ_BY_CPU2_SEMID);
			if (cpu2_sem_status == SEM_LOCK_SUCCESSFUL)
			{
				/**
				 * When CFG_HW_BLOCK_FLASH_REQ_BY_CPU2_SEMID is taken, it is allowed to only erase one sector or
				 * write one single 64bits data
				 * When either several sectors need to be erased or several 64bits data need to be written,
				 * the application shall first exit from the critical section and try again.
				 */
				if (FlashOperationType == FLASH_ERASE)
				{
					hal_flash_ll_erase(&p_erase_init, &page_error);
				}
				else
				{
					hal_flash_ll_progrm(FLASH_TYPEPROGRAM_DOUBLEWORD, SectorNumberOrDestAddress, Data);
				}
				/**
				 *  Release the semaphore to give the opportunity to CPU2 to protect its timing versus the next flash operation
				 *  by taking this semaphore.
				 *  Note that the CPU2 is polling on this semaphore so CPU1 shall release it as fast as possible.
				 *  This is why this code is protected by a critical section.
				 */
				LL_HSEM_ReleaseLock(HSEM, CFG_HW_BLOCK_FLASH_REQ_BY_CPU2_SEMID, 0);
			}
		}

		ATOMIC_SECTION_END();

		if (cpu1_sem_status != SEM_LOCK_SUCCESSFUL)
		{
			/**
			 * To avoid looping in ProcessSingleFlashOperation(), FD_WaitForSemAvailable() should implement a mechanism to
			 * continue only when CFG_HW_BLOCK_FLASH_REQ_BY_CPU1_SEMID is free
			 */
			waited_sem_status = FD_WaitForSemAvailable(WAIT_FOR_SEM_BLOCK_FLASH_REQ_BY_CPU1);
		}
		else if (cpu2_sem_status != SEM_LOCK_SUCCESSFUL)
		{
			/**
			 * To avoid looping in ProcessSingleFlashOperation(), FD_WaitForSemAvailable() should implement a mechanism to
			 * continue only when CFG_HW_BLOCK_FLASH_REQ_BY_CPU2_SEMID is free
			 */
			waited_sem_status = FD_WaitForSemAvailable(WAIT_FOR_SEM_BLOCK_FLASH_REQ_BY_CPU2);
		}
	} while (((cpu2_sem_status != SEM_LOCK_SUCCESSFUL) || (cpu1_sem_status != SEM_LOCK_SUCCESSFUL))
		&& (waited_sem_status != WAITED_SEM_BUSY));

	/**
	 * In most BLE application, the flash should not be blocked by the CPU2 longer than FLASH_TIMEOUT_VALUE (1000ms)
	 * However, it could be that for some marginal application, this time is longer.
	 * In that case either HAL_FLASHEx_Erase() or HAL_FLASH_Program() will exit with FLASH_TIMEOUT_VALUE value.
	 * This is not a failing case and there is no other way than waiting the operation to be completed.
	 * If for any reason this test is never passed, this means there is a failure in the system and there is no other
	 * way to recover than applying a device reset.
	 *
	 * Note: This code is required only when the PESD mechanism is used to protect the CPU2 timing.
	 * However, keeping that code make it compatible with the two mechanisms.
	 */
	while (__HAL_FLASH_GET_FLAG(FLASH_FLAG_CFGBSY));

	if (waited_sem_status != WAITED_SEM_BUSY)
	{
		/**
		 * The flash processing has been done. It has not been checked whether it has been successful or not.
		 * The only commitment is that it is possible to request a new flash processing
		 */
		return_status = STD_SUCCESS;
	}
	else
	{
		/**
		 * The flash processing has not been executed due to timing protection from either the CPU1 or the CPU2.
		 * This status is reported up to the user that should retry after checking that each CPU do not
		 * protect its timing anymore.
		 */
		return_status = STD_FAILED;
	}

	return return_status;
}

/*************************************************************
 *
 *  sem FUNCTIONS
 *
 *************************************************************/

void FD_WaitOsSem()
{
	UTIL_SEQ_WaitEvt(1 << CFG_IDLEEVT_FLASH_OPER_ALLOWED_ID);
}

WaitedSemStatus_t FD_WaitForSemAvailable(WaitedSemId_t WaitedSemId)
{

	/**
	 * The timing protection is enabled by either CPU1 or CPU2. It should be decided here if the driver shall
	 * keep trying to erase/write the flash until successful or if it shall exit ans report to the user that the action
	 * has not been executed.
	 * WAITED_SEM_BUSY returns to the user
	 * WAITED_SEM_FREE keep looping in the driver until the action is executed. This will result in the current tack looping
	 * until this is done. In a bare metal implementation, only the code within interrupt handler can be executed. With an OS,
	 * only task with higher priority can be processed
	 *
	 */

	if (WaitedSemId == WAIT_FOR_SEM_BLOCK_FLASH_REQ_BY_CPU1)
	{
		LL_HSEM_ClearFlag_C1ICR(HSEM, __HAL_HSEM_SEMID_TO_MASK(CFG_HW_BLOCK_FLASH_REQ_BY_CPU1_SEMID)); /* There is a bug in __HAL_HSEM_CLEAR_FLAG() */
		if (LL_HSEM_GetStatus(HSEM, CFG_HW_BLOCK_FLASH_REQ_BY_CPU1_SEMID))
		{
			/*	APP_DBG_MSG("\r\n\rWAIT UNTILL CPU1 ALLOWS FLASH OPERATION\n");*/

			HAL_HSEM_ActivateNotification(__HAL_HSEM_SEMID_TO_MASK(CFG_HW_BLOCK_FLASH_REQ_BY_CPU1_SEMID));
			FD_WaitOsSem();
			HAL_HSEM_DeactivateNotification(__HAL_HSEM_SEMID_TO_MASK(CFG_HW_BLOCK_FLASH_REQ_BY_CPU1_SEMID));
		}
	}

	if (WaitedSemId == WAIT_FOR_SEM_BLOCK_FLASH_REQ_BY_CPU2)
	{
		LL_HSEM_ClearFlag_C1ICR(HSEM, __HAL_HSEM_SEMID_TO_MASK(CFG_HW_BLOCK_FLASH_REQ_BY_CPU2_SEMID)); /* There is a bug in __HAL_HSEM_CLEAR_FLAG() */
		if (LL_HSEM_GetStatus(HSEM, CFG_HW_BLOCK_FLASH_REQ_BY_CPU2_SEMID))
		{
			/*		APP_DBG_MSG("\r\n\rWAIT UNTILL CPU2 ALLOWS FLASH OPERATION\n");*/

			HAL_HSEM_ActivateNotification(__HAL_HSEM_SEMID_TO_MASK(CFG_HW_BLOCK_FLASH_REQ_BY_CPU2_SEMID));
			FD_WaitOsSem();
			HAL_HSEM_DeactivateNotification(__HAL_HSEM_SEMID_TO_MASK(CFG_HW_BLOCK_FLASH_REQ_BY_CPU2_SEMID));
		}
	}

	return WAITED_SEM_FREE;
}

//HSEM中断回调
void HAL_HSEM_FreeCallback(uint32_t SemMask)
{
	if (SemMask == __HAL_HSEM_SEMID_TO_MASK(CFG_HW_BLOCK_FLASH_REQ_BY_CPU1_SEMID))
	{
		UTIL_SEQ_SetEvt(1 << CFG_IDLEEVT_FLASH_OPER_ALLOWED_ID);
	}

	if (SemMask == __HAL_HSEM_SEMID_TO_MASK(CFG_HW_BLOCK_FLASH_REQ_BY_CPU2_SEMID))
	{
		UTIL_SEQ_SetEvt(1 << CFG_IDLEEVT_FLASH_OPER_ALLOWED_ID);
	}
}

#pragma endregion



#pragma region flash low level


static void flash_program_doubleword(uint32_t Address, uint64_t Data)
{
	/* Set PG bit */
	SET_BIT(FLASH->CR, FLASH_CR_PG);

	/* Program first word */
	*(uint32_t*)Address = (uint32_t)Data;

	/* Barrier to ensure programming is performed in 2 steps, in right order
	  (independently of compiler optimization behavior) */
	__ISB();

	/* Program second word */
	*(uint32_t*)(Address + 4U) = (uint32_t)(Data >> 32U);
}

HAL_StatusTypeDef  flash_wait_for_last_operation(uint32_t Timeout)
{
	uint32_t error;
	uint64_t tickend = Clock_Time() + Timeout;

	/* Wait for the FLASH operation to complete by polling on BUSY flag to be reset.
	   Even if the FLASH operation fails, the BUSY flag will be reset and an error
	   flag will be set */
	while (__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY))
	{
		if (Clock_Time() > tickend)
		{
			return HAL_TIMEOUT;
		}
	}

	/* Check FLASH operation error flags */
	error = FLASH->SR;

	/* Check FLASH End of Operation flag */
	if ((error & FLASH_FLAG_EOP) != 0U)
	{
		/* Clear FLASH End of Operation pending bit */
		__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP);
	}

	/* Now update error variable to only error value */
	error &= FLASH_FLAG_SR_ERRORS;

	/* clear error flags */
	__HAL_FLASH_CLEAR_FLAG(error);

	if (error != 0U)
	{
		/*Save the error code*/
		pFlash.ErrorCode = error;

		return HAL_ERROR;
	}

	/* Wait for control register to be written */
	while (__HAL_FLASH_GET_FLAG(FLASH_FLAG_CFGBSY))
	{
		if (Clock_Time() > tickend)
		{
			return HAL_TIMEOUT;
		}
	}

	return HAL_OK;
}

HAL_StatusTypeDef hal_flash_ll_progrm(uint32_t TypeProgram, uint32_t Address, uint64_t Data)
{
	HAL_StatusTypeDef status;

	/* Check the parameters */
	assert_param(IS_FLASH_TYPEPROGRAM(TypeProgram));
	assert_param(IS_ADDR_ALIGNED_64BITS(Address));
	assert_param(IS_FLASH_PROGRAM_ADDRESS(Address));

	/* Process Locked */
	__HAL_LOCK(&pFlash);

	/* Reset error code */
	pFlash.ErrorCode = HAL_FLASH_ERROR_NONE;

	/* Verify that next operation can be proceed */
	status = flash_wait_for_last_operation(FLASH_TIMEOUT_VALUE);

	if (status == HAL_OK)
	{
		/* Check the parameters */
		assert_param(IS_FLASH_PROGRAM_ADDRESS(Address));

		/* Program double-word (64-bit) at a specified address */
		flash_program_doubleword(Address, Data);


		/* Wait for last operation to be completed */
		status = flash_wait_for_last_operation(FLASH_TIMEOUT_VALUE);

		/* If the program operation is completed, disable the PG or FSTPG Bit */
		CLEAR_BIT(FLASH->CR, TypeProgram);
	}

	/* Process Unlocked */
	__HAL_UNLOCK(&pFlash);

	/* return status */
	return status;
}


HAL_StatusTypeDef hal_flash_ll_erase(FLASH_EraseInitTypeDef* pEraseInit, uint32_t* PageError)
{
	HAL_StatusTypeDef status;
	uint32_t index;

	/* Check the parameters */
	assert_param(IS_FLASH_TYPEERASE(pEraseInit->TypeErase));

	/* Process Locked */
	__HAL_LOCK(&pFlash);

	/* Reset error code */
	pFlash.ErrorCode = HAL_FLASH_ERROR_NONE;

	/* Verify that next operation can be proceed */
	status = flash_wait_for_last_operation(FLASH_TIMEOUT_VALUE);

	if (status == HAL_OK)
	{
		if (pEraseInit->TypeErase == FLASH_TYPEERASE_PAGES)
		{
			/*Initialization of PageError variable*/
			*PageError = 0xFFFFFFFFU;

			for (index = pEraseInit->Page; index < (pEraseInit->Page + pEraseInit->NbPages); index++)
			{
				/* Start erase page */
				FLASH_PageErase(index);

				/* Wait for last operation to be completed */
				status = flash_wait_for_last_operation(FLASH_TIMEOUT_VALUE);

				if (status != HAL_OK)
				{
					/* In case of error, stop erase procedure and return the faulty address */
					*PageError = index;
					break;
				}
			}

			/* If operation is completed or interrupted, disable the Page Erase Bit */
			CLEAR_BIT(FLASH->CR, (FLASH_CR_PER | FLASH_CR_PNB));
		}

		/* Flush the caches to be sure of the data consistency */
		/* Flush instruction cache  */
		if (READ_BIT(FLASH->ACR, FLASH_ACR_ICEN) == FLASH_ACR_ICEN)
		{
			/* Disable instruction cache  */
			__HAL_FLASH_INSTRUCTION_CACHE_DISABLE();
			/* Reset instruction cache */
			__HAL_FLASH_INSTRUCTION_CACHE_RESET();
			/* Enable instruction cache */
			__HAL_FLASH_INSTRUCTION_CACHE_ENABLE();
		}

		/* Flush data cache */
		if (READ_BIT(FLASH->ACR, FLASH_ACR_DCEN) == FLASH_ACR_DCEN)
		{
			/* Disable data cache  */
			__HAL_FLASH_DATA_CACHE_DISABLE();
			/* Reset data cache */
			__HAL_FLASH_DATA_CACHE_RESET();
			/* Enable data cache */
			__HAL_FLASH_DATA_CACHE_ENABLE();
		}
	}

	/* Process Unlocked */
	__HAL_UNLOCK(&pFlash);

	return status;
}
#pragma endregion
