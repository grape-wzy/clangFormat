/*******************************************************************************
* file     flash_ramif.c
* author   mackgim
* version  V1.0.0
* date
* brief ：
*******************************************************************************/
#include "standard_lib.h"
#include "flash_driver.h"
#include "user_hw.h"
#include "platform.h"


__RAM_USER_FUNC_HAL HAL_FLASH_Unlock_RAM(void);
__RAM_USER_FUNC_HAL HAL_FLASH_Lock_RAM(void);
__RAM_USER_FUNC_HAL HAL_FLASH_Program_RAM(uint32_t TypeProgram, uint32_t Address, uint64_t Data);
__RAM_USER_FUNC_HAL HAL_FLASHEx_Erase_RAM(FLASH_EraseInitTypeDef* pEraseInit, uint32_t* PageError);


__RAM_USER_FUNC_HAL FLASH_WaitForLastOperation_RAM(uint32_t Timeout);
__RAM_USER_FUNC_VOID FLASH_PageErase_RAM(uint32_t Page);
__RAM_USER_FUNC_VOID FLASH_Program_DoubleWord_RAM(uint32_t Address, uint64_t Data);
__RAM_USER_FUNC_VOID FLASH_AcknowledgePageErase_RAM(void);
__RAM_USER_FUNC_VOID FLASH_FlushCaches_RAM(void);


extern FLASH_ProcessTypeDef pFlash;






#pragma region flah ram操作，基本功能

#define FLASH_TIMEOUT_VALUE_RAM        ((uint32_t)0x3fffffff) /* 50 s */
//#define FLASH_TIMEOUT_VALUE_RAM        ((uint32_t)50000) /* 50 s */

__RAM_USER_FUNC_VOID flash_ram_program(uint32_t addr, uint32_t size)
{

	__disable_irq();
	//__set_FAULTMASK(1); //__disable_interrupt
	flash_ram_erase_running_code();
	flash_ram_update_running_code((uint32_t)addr, size);
	flash_ram_erase_shadow_code();
	//__set_FAULTMASK(0); //__enable_interrupt
	//kprint("update done, and reset.........\r\n");
	//for (int i = 0; i < 0xffff; i++) {}
	//__set_FAULTMASK(1);
	for (int i = 0; i < 0xffff; i++) {}
	NVIC_SystemReset();
}

__RAM_USER_FUNC_UINT8_T flash_ram_erase_running_code(void)
{
	flash_ram_init();
	return flash_ram_erase(FLASH_SPACE_RUNNING_BEGIN_ADDR, FLASH_SPACE_RUNNING_END_ADDR);
}

__RAM_USER_FUNC_UINT8_T flash_ram_erase_shadow_code(void)
{
	flash_ram_init();
	return flash_ram_erase(FLASH_SPACE_SHADOW_BEGIN_ADDR, FLASH_SPACE_SHADOW_END_ADDR);

}

__RAM_USER_FUNC_VOID flash_ram_update_running_code(uint32_t Source, uint32_t size)
{

	uint32_t* buff = (uint32_t*)Source;
	flash_ram_init();

	if (flash_ram_write(FLASH_SPACE_RUNNING_BEGIN_ADDR, buff, (size + 7) / 8 * 8) != STD_SUCCESS)
	{
		return;
	}
	//for (uint32_t i = 0; i < (size / 4); i++)
	//{
	//	if ((*((uint32_t*)FLASH_SPACE_RUNNING_BEGIN_ADDR + i)) != (*((uint32_t*)Source + i)))
	//	{
	//		return;
	//	}
	//}

}

#pragma endregion

#pragma region flah 其他功能

uint8_t flash_check_id(uint32_t ad, uint32_t size)
{
	kprint("ID=%s\r\n", CODE_IDENTIFIER);

	uint32_t addr = ad;
	uint32_t l = size;
	int32_t ret;
	for (uint32_t i = 0; i < l; i++)
	{
		ret = memcmp((char*)(addr + i), (const char*)CODE_IDENTIFIER, sizeof(CODE_IDENTIFIER) - 1);
		if (ret == 0)
		{
			kprint("ID exist, Index=%u, Addr=0x%x, id=%s\r\n", (unsigned int)i, (unsigned int)(addr + i), (char*)(addr + i));
			return STD_SUCCESS;
		}
	}
	kprint("There is no ID \r\n");
	return STD_FAILED;
}

uint32_t flash_get_end_addr(void)
{
	return FLASH_SPACE_END_ADDR;
}

uint32_t flash_get_running_addr(uint32_t addr)
{
	return ((uint32_t)(FLASH_SPACE_RUNNING_BEGIN_ADDR + addr));
}

uint32_t flash_get_shadow_addr(uint32_t addr)
{
	return ((uint32_t)(FLASH_SPACE_SHADOW_BEGIN_ADDR + addr));
}

uint32_t flash_get_shadow_end_addr(void)
{
	return FLASH_SPACE_SHADOW_END_ADDR;
}

uint32_t flash_get_ble_stack_addr(void)
{
	return FLASH_SPACE_BLE_STACK_BEGIN_ADDR;
}

uint32_t flash_get_mcu_stack_top_addr(void)
{
	return RAM_SPACE_TOP_STACK_ADDR;
}

uint32_t flash_get_running_max_size(void)
{
	return FLASH_SPACE_RUNNING_MAX_SIZE;
}

uint32_t flash_get_shadow_max_size(void)
{
	return FLASH_SPACE_SHADOW_MAX_SIZE;
}

uint32_t flash_get_fw_version(void)
{
	return FW_VERSION;
}

uint32_t flash_get_error_status(void)
{
	uint32_t error;
	error = (FLASH->SR & FLASH_FLAG_SR_ERRORS);
	error |= (FLASH->ECCR & FLASH_FLAG_ECCD);
	return error;
}



#pragma endregion

#pragma region Flash自定义操作

__RAM_USER_FUNC_VOID flash_ram_init(void)
{
	/* Unlock the Program memory */
	HAL_FLASH_Unlock_RAM();

	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_PROGERR | FLASH_FLAG_WRPERR |
		FLASH_FLAG_PGAERR | FLASH_FLAG_SIZERR | FLASH_FLAG_PGSERR | FLASH_FLAG_MISERR | FLASH_FLAG_FASTERR |
		FLASH_FLAG_RDERR | FLASH_FLAG_OPTVERR | FLASH_FLAG_PESD
		| FLASH_FLAG_ECCC | FLASH_FLAG_ECCD);
	/* Unlock the Program memory */
	//| FLASH_FLAG_PEMPTY 
	HAL_FLASH_Lock_RAM();
}

__RAM_USER_FUNC_VOID flash_ram_deinit(void)
{
	/* Unlock the Program memory */
	HAL_FLASH_Unlock_RAM();

	/* Clear all FLASH flags */
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_PROGERR | FLASH_FLAG_WRPERR |
		FLASH_FLAG_PGAERR | FLASH_FLAG_SIZERR | FLASH_FLAG_PGSERR | FLASH_FLAG_MISERR | FLASH_FLAG_FASTERR |
		FLASH_FLAG_RDERR | FLASH_FLAG_OPTVERR | FLASH_FLAG_BSY | FLASH_FLAG_CFGBSY | FLASH_FLAG_ECCC | FLASH_FLAG_ECCD);
	/* Unlock the Program memory */
	//| FLASH_FLAG_PEMPTY
	HAL_FLASH_Lock_RAM();
}

__RAM_USER_FUNC_UINT8_T  flash_ram_erase(uint32_t startaddr, uint32_t endaddr)
{
	FLASH_EraseInitTypeDef EraseInitStruct;
	uint32_t SectorError = 0;
	uint32_t FirstPage = 0, NbOfPages = 0;

	FirstPage = FLASH_GET_PAGE_INDEX(startaddr);

	NbOfPages = (endaddr - startaddr) / FLASH_SPACE_PAGE_SIZE;
	if ((endaddr - startaddr) % FLASH_SPACE_PAGE_SIZE)
	{
		NbOfPages++;
	}



	HAL_FLASH_Unlock_RAM();

	EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
	EraseInitStruct.Page = FirstPage;
	EraseInitStruct.NbPages = NbOfPages;
	HAL_StatusTypeDef ret = HAL_FLASHEx_Erase_RAM(&EraseInitStruct, &SectorError);
	HAL_FLASH_Lock_RAM();

	if (ret == HAL_ERROR)
	{
		return STD_FAILED;
	}
	else if (ret == HAL_BUSY)
	{
		return STD_BUSY;
	}
	else if (ret == HAL_TIMEOUT)
	{
		return STD_TIMEOUT;
	}
	else if (ret == HAL_OK)
	{
		return STD_SUCCESS;
	}
	return STD_SUCCESS;
}

__RAM_USER_FUNC_UINT8_T  flash_ram_write(uint32_t addr, uint32_t* buff, uint32_t buffsize)
{
	uint32_t buffsize_64 = 0;
	uint64_t data = 0;
	if (buffsize % 8)
	{
		//kprint("[flash]: it(%u) is not 8 multiples\r\n", (unsigned int)buffsize);
		return STD_FAILED;
	}
	buffsize_64 = buffsize / 8;
	HAL_FLASH_Unlock_RAM();
	for (uint32_t i = 0; i < buffsize_64; i++)
	{
		data = (uint64_t)buff[2 * i] + ((((uint64_t)buff[2 * i + 1]) & 0xffffffff) << 32);
		//kprint("0x%x, 0x%x-0x%x\r\n", (unsigned int)(addr + i * 8), (unsigned int)data & 0xffffffff, (unsigned int)(data >> 32) & 0xffffffff);

		if (HAL_FLASH_Program_RAM(FLASH_TYPEPROGRAM_DOUBLEWORD, addr + i * 8, data) != HAL_OK)
		{
			//kprint("error\r\n");
			HAL_FLASH_Lock_RAM();
			return STD_FAILED;
		}
	}
	HAL_FLASH_Lock_RAM();

	uint32_t* addr_ptr = (uint32_t*)(addr);
	for (uint32_t i = 0; i < (buffsize / 4); i++)
	{
		if (*(addr_ptr++) != *(buff + i))
		{
			//kprint("[FL]: the %d data : 0x%08x, ram data: 0x%08x.\r\n", (unsigned int)i, (unsigned int)*(addr_ptr - 1), (unsigned int)*(buff + i));
			return STD_FAILED;
		}

	}
	return STD_SUCCESS;

}

#pragma endregion

#pragma region 官方HAL Flash操作，RAM版本

__RAM_USER_FUNC_HAL HAL_FLASH_Unlock_RAM(void)
{
	HAL_StatusTypeDef status = HAL_OK;

	if (READ_BIT(FLASH->CR, FLASH_CR_LOCK) != 0U)
	{
		/* Authorize the FLASH Registers access */
		WRITE_REG(FLASH->KEYR, FLASH_KEY1);
		WRITE_REG(FLASH->KEYR, FLASH_KEY2);

		/* verify Flash is unlock */
		if (READ_BIT(FLASH->CR, FLASH_CR_LOCK) != 0U)
		{
			status = HAL_ERROR;
		}
	}

	return status;

}

__RAM_USER_FUNC_HAL HAL_FLASH_Lock_RAM(void)
{
	HAL_StatusTypeDef status = HAL_OK;

	/* Set the LOCK Bit to lock the FLASH Registers access */
	/* @Note  The lock and unlock procedure is done only using CR registers even from CPU2 */
	SET_BIT(FLASH->CR, FLASH_CR_LOCK);

	/* verify Flash is locked */
	if (READ_BIT(FLASH->CR, FLASH_CR_LOCK) == 0U)
	{
		status = HAL_ERROR;
	}

	return status;
}

__RAM_USER_FUNC_VOID FLASH_Program_DoubleWord_RAM(uint32_t Address, uint64_t Data)
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

__RAM_USER_FUNC_HAL HAL_FLASH_Program_RAM(uint32_t TypeProgram, uint32_t Address, uint64_t Data)
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
	status = FLASH_WaitForLastOperation_RAM(FLASH_TIMEOUT_VALUE_RAM);

	if (status == HAL_OK)
	{
		if (TypeProgram == FLASH_TYPEPROGRAM_DOUBLEWORD)
		{
			/* Check the parameters */
			assert_param(IS_FLASH_PROGRAM_ADDRESS(Address));

			/* Program double-word (64-bit) at a specified address */
			FLASH_Program_DoubleWord_RAM(Address, Data);
		}
		else
		{
			return HAL_ERROR;
		}

		/* Wait for last operation to be completed */
		status = FLASH_WaitForLastOperation_RAM(FLASH_TIMEOUT_VALUE_RAM);

		/* If the program operation is completed, disable the PG or FSTPG Bit */
		CLEAR_BIT(FLASH->CR, TypeProgram);
	}

	/* Process Unlocked */
	__HAL_UNLOCK(&pFlash);

	/* return status */
	return status;
}

__RAM_USER_FUNC_HAL HAL_FLASHEx_Erase_RAM(FLASH_EraseInitTypeDef* pEraseInit, uint32_t* PageError)
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
	status = FLASH_WaitForLastOperation_RAM(FLASH_TIMEOUT_VALUE_RAM);

	if (status == HAL_OK)
	{
		if (pEraseInit->TypeErase == FLASH_TYPEERASE_PAGES)
		{
			/*Initialization of PageError variable*/
			*PageError = 0xFFFFFFFFU;

			for (index = pEraseInit->Page; index < (pEraseInit->Page + pEraseInit->NbPages); index++)
			{
				/* Start erase page */
				FLASH_PageErase_RAM(index);

				/* Wait for last operation to be completed */
				status = FLASH_WaitForLastOperation_RAM(FLASH_TIMEOUT_VALUE_RAM);

				if (status != HAL_OK)
				{
					/* In case of error, stop erase procedure and return the faulty address */
					*PageError = index;
					break;
				}
			}

			/* If operation is completed or interrupted, disable the Page Erase Bit */
			FLASH_AcknowledgePageErase_RAM();
		}

		/* Flush the caches to be sure of the data consistency */
		FLASH_FlushCaches_RAM();
	}

	/* Process Unlocked */
	__HAL_UNLOCK(&pFlash);

	return status;
}

__RAM_USER_FUNC_HAL FLASH_WaitForLastOperation_RAM(uint32_t Timeout)
{
	uint32_t error;
	uint32_t Timeout2 = Timeout;
	while (__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY) && (Timeout2 != 0x00))
	{
		Timeout2--;
	}
	if (Timeout2 == 0x00)
	{
		return HAL_TIMEOUT;
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

	Timeout2 = Timeout;
	/* Wait for control register to be written */
	while (__HAL_FLASH_GET_FLAG(FLASH_FLAG_CFGBSY) && (Timeout2 != 0x00))
	{
		Timeout2--;
	}
	if (Timeout2 == 0x00)
	{
		return HAL_TIMEOUT;
	}
	return HAL_OK;
}

__RAM_USER_FUNC_VOID FLASH_PageErase_RAM(uint32_t Page)
{
	/* Check the parameters */
	assert_param(IS_FLASH_PAGE(Page));

	/* Proceed to erase the page */
	MODIFY_REG(FLASH->CR, FLASH_CR_PNB, ((Page << FLASH_CR_PNB_Pos) | FLASH_CR_PER | FLASH_CR_STRT));

}

__RAM_USER_FUNC_VOID FLASH_AcknowledgePageErase_RAM(void)
{
	CLEAR_BIT(FLASH->CR, (FLASH_CR_PER | FLASH_CR_PNB));
}

__RAM_USER_FUNC_VOID FLASH_FlushCaches_RAM(void)
{
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
#pragma endregion