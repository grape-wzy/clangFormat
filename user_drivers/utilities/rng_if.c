/*******************************************************************************
* file    random.c
* author  mackgim
* version 1.0.0
* date
* brief   随机数
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "rng_if.h"
#include "platform.h"
#include "standard_lib.h"
#include "utils.h"

#ifdef DEBUG
#if 1
#define rgkpf(...) kprint(__VA_ARGS__)
#else
#define rgkpf(...)
#endif
#else
#define rgkpf(...)
#endif


static RNG_HandleTypeDef sRNGHandle;

void rng_mspinit(RNG_HandleTypeDef* hrng);
void rng_mspdeinit(RNG_HandleTypeDef* hrng);


void rng_init(void)
{
	sRNGHandle.Instance = RNG;
	sRNGHandle.Init.ClockErrorDetection = RNG_CED_ENABLE;
	HAL_RNG_RegisterCallback(&sRNGHandle, HAL_RNG_MSPINIT_CB_ID, rng_mspinit);
	HAL_RNG_RegisterCallback(&sRNGHandle, HAL_RNG_MSPDEINIT_CB_ID, rng_mspdeinit);

	if (HAL_RNG_Init(&sRNGHandle) != HAL_OK)
	{
		rgkpf("failed\r\n");
	}
	else
	{
		rgkpf("ok\r\n");
	}


	while (1)
	{
		uint32_t rng[5];
		if (rng_get_numbers(&rng[0], 5) == STD_SUCCESS)
		{
			kprint("rng=0x%x\r\n", (unsigned int)rng[0]);
			break;
		}
	}

#ifdef DEBUG1
	uint32_t ren[5];
	rng_get_numbers(&ren[0], 5);
	kprint("ran1=0x%x\r\n", (unsigned int)ren);
	rng_get_numbers(&ren[0], 5);
	kprint("ran2=0x%x\r\n", (unsigned int)ren);
#endif
}

void rng_deinit(void)
{
	sRNGHandle.Instance = RNG;
	HAL_RNG_DeInit(&sRNGHandle);
}


void rng_mspinit(RNG_HandleTypeDef* hrng)
{
	if (hrng->Instance == RNG)
	{

		/* Configure the RNG clock source */
		LL_RCC_SetRNGClockSource(LL_RCC_RNG_CLKSOURCE_CLK48);
		LL_RCC_SetCLK48ClockSource(LL_RCC_CLK48_CLKSOURCE_HSI48);
		/* Peripheral clock enable */
		__HAL_RCC_RNG_CLK_ENABLE();
	}

}

void rng_mspdeinit(RNG_HandleTypeDef* hrng)
{
	if (hrng->Instance == RNG)
	{
		__HAL_RCC_RNG_CLK_DISABLE();
	}

}

__STATIC_INLINE uint8_t rng_generate(uint32_t* data)
{

#if 0
	HAL_StatusTypeDef status = HAL_RNG_GenerateRandomNumber(&sRNGHandle, data);
	if (status != HAL_OK)
	{
		return STD_FAILED;
	}
	/* Just be extra sure that we didn't do it wrong */
	if ((__HAL_RNG_GET_FLAG(&sRNGHandle, (RNG_FLAG_CECS | RNG_FLAG_SECS))) != 0)
	{
		kprint("error\r\n");
		return STD_FAILED;
	}
	return STD_SUCCESS;
#else

	while (!LL_RNG_IsActiveFlag_DRDY(RNG));

	if ((__HAL_RNG_GET_FLAG(&sRNGHandle, (RNG_FLAG_CECS | RNG_FLAG_SECS))) != 0)
	{
		kprint("error\r\n");
		return STD_FAILED;
	}
	*data = LL_RNG_ReadRandData32(RNG);
	//if ((LL_RNG_IsActiveFlag_CECS(RNG))|| (LL_RNG_IsActiveFlag_SECS(RNG)))
	//{
	//	nprint("error\r\n");
	//	return STD_FAILED;
	//}
	return STD_SUCCESS;
#endif
}

//获取number个随机数
uint8_t rng_get_numbers(uint32_t* buff, uint32_t buffersize)
{
	if (buffersize == 0)
	{
		return STD_FAILED;
	}

	while (LL_HSEM_1StepLock(HSEM, CFG_HW_RNG_SEMID));

	/* Enable RNG */
	__HAL_RNG_ENABLE(&sRNGHandle);

	/* Enable HSI48 oscillator */
	LL_RCC_HSI48_Enable();
	/* Wait until HSI48 is ready */
	while (!LL_RCC_HSI48_IsReady());

	uint8_t status;

	for (uint32_t i = 0; i < buffersize; i++)
	{

		status = rng_generate(&buff[i]);
		if (status != STD_SUCCESS)
		{
			kprint("(%u) failed to get random\r\n", (unsigned int)i);
			break;
		}
	}

	/* Disable HSI48 oscillator */
	LL_RCC_HSI48_Disable();

	/* Disable RNG */
	__HAL_RNG_DISABLE(&sRNGHandle);

	/* Release RNG semaphore */
	LL_HSEM_ReleaseLock(HSEM, CFG_HW_RNG_SEMID, 0);

	if (status != STD_SUCCESS)
	{
		return status;
	}

	if (buffersize > 1)
	{
		uint32_t errorCount = 0;
		for (uint32_t i = 0; i < (buffersize - 1); i++)
		{
			if (buff[i] == buff[i + 1])
			{
				errorCount++;
			}
		}

		if (errorCount == (buffersize - 1))
		{
			rgkpf("all 0x%x\r\n", (unsigned int)buff[0]);
			return STD_FAILED;
		}
	}


	//防止rng出问题
	uint8_t ret = all_is_same(buff, buffersize, 0);
	if (ret == true)
	{
		rgkpf("all 0\r\n");
		return STD_FAILED;
	}
	ret = all_is_same(buff, buffersize, 0xffffffff);
	if (ret == true)
	{
		rgkpf("all 0xffffffff\r\n");
		return STD_FAILED;
	}

	return STD_SUCCESS;
}


/*******************************************************************************
END
*******************************************************************************/