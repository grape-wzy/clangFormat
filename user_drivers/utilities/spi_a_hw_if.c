/*******************************************************************************
* file     force_hw.c
* author   mackgim
* version  V1.0.0
* date
* brief ��
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "spi_a_hw_if.h"
#include "platform.h"
#include "standard_lib.h"



static SPI_HandleTypeDef aSpiHandle;

static 	uint8_t spi_ll_transmit_receive(SPI_HandleTypeDef* hspi, uint8_t *  pTxData, uint8_t *  pRxData, uint16_t Size,
	uint32_t Timeout);

static void spi_a_hw_mspinit(SPI_HandleTypeDef* hspi);
static void spi_a_hw_mspdeinit(SPI_HandleTypeDef* hspi);

static uint8_t sSpiInit = false;


uint8_t spi_a_hw_init(void)
{
	if (sSpiInit)
	{
		return STD_SUCCESS;
	}
	sSpiInit = true;
	//HAL_SPI_DeInit(&aSpiHandle);
	aSpiHandle.Instance = SPI_A_HW_INSTANCE;
	aSpiHandle.Init.Mode = SPI_A_HW_MODE;
	aSpiHandle.Init.Direction = SPI_A_HW_DIRECTION;
	aSpiHandle.Init.DataSize = SPI_A_HW_DATASIZE;
	aSpiHandle.Init.CLKPolarity = SPI_A_HW_CLKPOLARITY;
	aSpiHandle.Init.CLKPhase = SPI_A_HW_CLKPHASE;
	aSpiHandle.Init.NSS = SPI_A_HW_NSS;
	aSpiHandle.Init.FirstBit = SPI_A_HW_FIRSTBIT;
	aSpiHandle.Init.TIMode = SPI_A_HW_TIMODE;
	aSpiHandle.Init.CRCPolynomial = SPI_A_HW_CRCPOLYNOMIAL;
	aSpiHandle.Init.BaudRatePrescaler = SPI_A_HW_BAUDRATEPRESCALER;
	aSpiHandle.Init.CRCCalculation = SPI_A_HW_CRCCALCULATION;

	HAL_SPI_RegisterCallback(&aSpiHandle, HAL_SPI_MSPINIT_CB_ID, spi_a_hw_mspinit);
	HAL_SPI_RegisterCallback(&aSpiHandle, HAL_SPI_MSPDEINIT_CB_ID, spi_a_hw_mspdeinit);


	HAL_SPI_Init(&aSpiHandle);


	CLEAR_BIT(aSpiHandle.Instance->CR2, SPI_RXFIFO_THRESHOLD);

	/* Check if the SPI is already enabled */
	if ((aSpiHandle.Instance->CR1 & SPI_CR1_SPE) != SPI_CR1_SPE)
	{
		/* Enable SPI peripheral */
		__HAL_SPI_ENABLE(&aSpiHandle);
	}

	return STD_SUCCESS;
}

uint8_t spi_a_hw_deinit(void)
{
	if (!sSpiInit)
	{
		return STD_SUCCESS;
	}
	sSpiInit = false;
	HAL_SPI_DeInit(&aSpiHandle);
	return STD_SUCCESS;
}

void spi_a_hw_mspinit(SPI_HandleTypeDef* hspi)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	if (hspi->Instance == SPI_A_HW_INSTANCE)
	{
		/* Enable GPIO Ports Clock */
		SPI_A_HW_SCLK_CLK_ENABLE();
		SPI_A_HW_MISO_CLK_ENABLE();
		SPI_A_HW_MOSI_CLK_ENABLE();
		SPI_A_HW_CS_CLK_ENABLE();
		SPI_A_HW_READY_CLK_ENABLE();
		/* Enable SPI clock */
		SPI_A_HW_CLK_ENABLE();

		/* SCLK */
		GPIO_InitStruct.Pin = SPI_A_HW_SCLK_PIN;
		GPIO_InitStruct.Mode = SPI_A_HW_SCLK_MODE;
		GPIO_InitStruct.Pull = SPI_A_HW_SCLK_PULL;
		GPIO_InitStruct.Speed = SPI_A_HW_SCLK_SPEED;
		GPIO_InitStruct.Alternate = SPI_A_HW_SCLK_ALTERNATE;
		HAL_GPIO_Init(SPI_A_HW_SCLK_PORT, &GPIO_InitStruct);

		/* MISO */
		GPIO_InitStruct.Pin = SPI_A_HW_MISO_PIN;
		GPIO_InitStruct.Mode = SPI_A_HW_MISO_MODE;
		GPIO_InitStruct.Pull = SPI_A_HW_MISO_PULL;
		GPIO_InitStruct.Speed = SPI_A_HW_MISO_SPEED;
		GPIO_InitStruct.Alternate = SPI_A_HW_MISO_ALTERNATE;
		HAL_GPIO_Init(SPI_A_HW_MISO_PORT, &GPIO_InitStruct);

		/* MOSI */
		GPIO_InitStruct.Pin = SPI_A_HW_MOSI_PIN;
		GPIO_InitStruct.Mode = SPI_A_HW_MOSI_MODE;
		GPIO_InitStruct.Pull = SPI_A_HW_MOSI_PULL;
		GPIO_InitStruct.Speed = SPI_A_HW_MOSI_SPEED;
		GPIO_InitStruct.Alternate = SPI_A_HW_MOSI_ALTERNATE;
		HAL_GPIO_Init(SPI_A_HW_MOSI_PORT, &GPIO_InitStruct);


		/* NSS/CSN/CS */
		GPIO_InitStruct.Pin = SPI_A_HW_CS_PIN;
		GPIO_InitStruct.Mode = SPI_A_HW_CS_MODE;
		GPIO_InitStruct.Pull = SPI_A_HW_CS_PULL;
		GPIO_InitStruct.Speed = SPI_A_HW_CS_SPEED;
		GPIO_InitStruct.Alternate = SPI_A_HW_CS_ALTERNATE;
		HAL_GPIO_Init(SPI_A_HW_CS_PORT, &GPIO_InitStruct);
		SPI_A_HW_CS(1);


		/* RESET */
		GPIO_InitStruct.Pin = SPI_A_HW_RESET_PIN;
		GPIO_InitStruct.Mode = SPI_A_HW_RESET_MODE;
		GPIO_InitStruct.Pull = SPI_A_HW_RESET_PULL;
		GPIO_InitStruct.Speed = SPI_A_HW_RESET_SPEED;
		GPIO_InitStruct.Alternate = SPI_A_HW_RESET_ALTERNATE;
		HAL_GPIO_Init(SPI_A_HW_RESET_PORT, &GPIO_InitStruct);
		SPI_A_HW_RESET(1);


		/* READY */
		GPIO_InitStruct.Pin = SPI_A_HW_READY_PIN;
		GPIO_InitStruct.Mode = SPI_A_HW_READY_MODE;
		GPIO_InitStruct.Pull = SPI_A_HW_READY_PULL;
		//GPIO_InitStruct.Speed = SPI_A_HW_READY_SPEED;
		//GPIO_InitStruct.Alternate = SPI_A_HW_READY_ALTERNATE;
		HAL_GPIO_Init(SPI_A_HW_READY_PORT, &GPIO_InitStruct);

		/* Enable and set line 4 Interrupt to the lowest priority */
		HAL_NVIC_SetPriority(SPI_A_HW_READY_EXTI_IRQn, SPI_A_HW_DRDY_NVIC_PreemptionPriority, SPI_A_HW_DRDY_NVIC_SubPriority);
		//HAL_NVIC_EnableIRQ(SPI_A_HW_READY_EXTI_IRQn);
    }
}


void spi_a_hw_mspdeinit(SPI_HandleTypeDef* hspi)
{

	if (hspi->Instance == SPI_A_HW_INSTANCE)
	{
		SPI_A_HW_CLK_DISABLE();

		HAL_GPIO_DeInit(SPI_A_HW_CS_PORT, SPI_A_HW_CS_PIN);
		HAL_GPIO_DeInit(SPI_A_HW_MOSI_PORT, SPI_A_HW_MOSI_PIN);
		HAL_GPIO_DeInit(SPI_A_HW_MISO_PORT, SPI_A_HW_MISO_PIN);
		HAL_GPIO_DeInit(SPI_A_HW_SCLK_PORT, SPI_A_HW_SCLK_PIN);
		HAL_GPIO_DeInit(SPI_A_HW_RESET_PORT, SPI_A_HW_RESET_PIN);
		HAL_GPIO_DeInit(SPI_A_HW_READY_PORT, SPI_A_HW_READY_PIN);
	}
}

uint8_t spi_a_hw_transmit_receive(uint8_t* pTxData, uint8_t* pRxData, uint16_t Size, uint32_t Timeout)
{
    uint8_t ret = STD_SUCCESS;
	SPI_A_HW_CS(0);
	//if ((HAL_SPI_TransmitReceive(&aSpiHandle, pTxData, pRxData, Size, Timeout)) != HAL_OK)
	//{
	//	ret = STD_FAILED;
	//}
	ret = spi_ll_transmit_receive(&aSpiHandle, pTxData, pRxData, Size, Timeout);
	SPI_A_HW_CS(1);
	return ret;
}

uint8_t spi_a_hw_reset(uint8_t enable)
{
	SPI_A_HW_RESET(enable);
	return STD_SUCCESS;
}

uint8_t spi_a_hw_get_ready(void)
{
	return SPI_A_HW_READ_READY();
}

uint8_t spi_a_hw_enable_irq(uint8_t en)
{
	if (en)
	{
		__HAL_GPIO_EXTI_CLEAR_IT(SPI_A_HW_READY_EXTI_PIN);
		HAL_NVIC_EnableIRQ(SPI_A_HW_READY_EXTI_IRQn);
	}
	else
	{

		HAL_NVIC_DisableIRQ(SPI_A_HW_READY_EXTI_IRQn);
		__HAL_GPIO_EXTI_CLEAR_IT(SPI_A_HW_READY_EXTI_PIN);
	}
	return STD_SUCCESS;
}

uint8_t spi_a_hw_set_cs(uint8_t cs)
{
	SPI_A_HW_CS(cs);
	return STD_SUCCESS;
}


#pragma region 中断

typedef void(*HW_IRQ_CALLBACK_TYPE) (void);
static HW_IRQ_CALLBACK_TYPE irq_callback;
void spi_a_hw_register_cb(void* cb)
{
	irq_callback = cb;
}

#pragma endregion

#pragma region Low Level

uint8_t spi_ll_transmit_receive(SPI_HandleTypeDef* hspi, uint8_t* pTxData, uint8_t* pRxData, uint16_t Size,
	uint32_t Timeout)
{

	uint64_t TimeStart = Clock_Time();//必须
	uint16_t* pTx = (uint16_t*)pTxData;
	uint16_t* pRx = (uint16_t*)pRxData;
	for (uint32_t i = 0; i < Size; i++)
	{
		while (!__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_TXE))
		{

			if ((Clock_Time() - TimeStart) > Timeout)
			{
				return STD_FAILED;
			}
		}
		hspi->Instance->DR = pTx[i];

        while (!__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_RXNE))
		{
			if ((Clock_Time() - TimeStart) > Timeout)
			{
				return STD_FAILED;
			}
		}

		pRx[i] = hspi->Instance->DR;

	}


	while ((hspi->Instance->SR & SPI_FLAG_FTLVL) != SPI_FTLVL_EMPTY)
	{
		if ((Clock_Time() - TimeStart) > Timeout)
		{
			return STD_FAILED;
		}
	}


	while (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_BSY) != RESET)
	//while ((__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_BSY) ? SET : RESET) != RESET)
	{
		if ((Clock_Time() - TimeStart) > Timeout)
		{
			return STD_FAILED;
		}
	}



	__IO uint8_t  tmpreg8 = 0;
	while ((hspi->Instance->SR & SPI_FLAG_FRLVL) != SPI_FRLVL_EMPTY)
	{

		/* Flush Data Register by a blank read */
		tmpreg8 = READ_REG(*((__IO uint8_t*) & hspi->Instance->DR));
		/* To avoid GCC warning */
		UNUSED(tmpreg8);

		if ((Clock_Time() - TimeStart) > Timeout)
		{
			return STD_FAILED;
		}
	}

	return STD_SUCCESS;
}


static __IO  uint32_t sSPICount = 0;
static const uint16_t sSpiTxBuff[12] = { 0x6800 };
static uint16_t sSpiRxBuff[12] = { 0 };
uint8_t spi_a_hw_transmit_receive2(uint8_t* pTxData, uint8_t* pRxData, uint16_t Size, uint32_t Timeout)
{

	static SPI_HandleTypeDef* hspi = &aSpiHandle;

	uint8_t ret = STD_SUCCESS;
	SPI_A_HW_CS(0);

	CLEAR_BIT(hspi->Instance->CR2, SPI_RXFIFO_THRESHOLD);

	/* Check if the SPI is already enabled */
	if ((hspi->Instance->CR1 & SPI_CR1_SPE) != SPI_CR1_SPE)
	{
		/* Enable SPI peripheral */
		__HAL_SPI_ENABLE(hspi);
		kprint("start\r\n");
	}

	hspi->pTxBuffPtr = pTxData;
	hspi->pRxBuffPtr = pRxData;


	__IO uint16_t n = 0;
	//nprint("0x%x,", sSpiTxBuff[0]);
	uint64_t start = Clock_Time();
	while (1)
	{
		sSPICount = 0;
		do
		{
			if (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_TXE))
			{
				hspi->Instance->DR = *(uint16_t*)hspi->pTxBuffPtr;
				hspi->pTxBuffPtr += sizeof(uint16_t);

				//hspi->Instance->DR = sSpiTxBuff[sSPICount];

				break;
			}
			sSPICount++;
			if (sSPICount == 1000)
			{
				kprint("1\r\n");
				ret = STD_FAILED;
				goto end;
			}

			if ((Clock_Time() - start) > Timeout)
			{
				kprint("5\r\n");
				ret = STD_FAILED;
				goto end;
			}

		} while (1);

		sSPICount = 0;
		do
		{

			if (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_RXNE))
			{
				//sSpiRxBuff[n] = hspi->Instance->DR;
				*(uint16_t*)hspi->pRxBuffPtr = hspi->Instance->DR;
				hspi->pRxBuffPtr += sizeof(uint16_t);
				break;
			}
			sSPICount++;
			if (sSPICount == 1000)
			{
				kprint("2\r\n");
				ret = STD_FAILED;
				goto end;
			}
		} while (1);

		n++;
		if (n == Size)
		{
			break;
		}
	}

	sSPICount = 0;
	while ((hspi->Instance->SR & SPI_FLAG_FTLVL) != SPI_FTLVL_EMPTY)
	{
		sSPICount++;
		if (sSPICount == 1000)
		{
			kprint("3\r\n");
			ret = STD_FAILED;
			goto end;
		}
	}


	sSPICount = 0;
	while (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_BSY) != RESET)
		//while ((__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_BSY) ? SET : RESET) != RESET)
	{
		sSPICount++;
		if (sSPICount == 1000)
		{
			kprint("4\r\n");
			ret = STD_FAILED;
			goto end;
		}
	}


	sSPICount = 0;
	__IO uint8_t  tmpreg8 = 0;
	while ((hspi->Instance->SR & SPI_FLAG_FRLVL) != SPI_FRLVL_EMPTY)
	{

		/* Flush Data Register by a blank read */
		tmpreg8 = READ_REG(*((__IO uint8_t*) & hspi->Instance->DR));
		/* To avoid GCC warning */
		UNUSED(tmpreg8);

		sSPICount++;
		if (sSPICount == 1000)
		{
			kprint("5\r\n");
			ret = STD_FAILED;
			goto end;
		}
	}


	//static uint32_t buffcount = 0;
	//buffcount++;
	//if (buffcount == 10)
	//{
	//	buffcount = 0;
	//	nprint("%d,%d,%d\r\n", (int16_t)sSpiRxBuff[2], (int16_t)sSpiRxBuff[3], (int16_t)sSpiRxBuff[4]);
	//}

	ret = STD_SUCCESS;




end:
	SPI_A_HW_CS(1);
	return ret;
}

#pragma endregion


#if 0

#pragma region Low Level

uint8_t spi_ll_transmit_receive(SPI_HandleTypeDef* hspi,  uint8_t *  pTxData, uint8_t *  pRxData, uint16_t Size,
	uint32_t Timeout)
{

	CLEAR_BIT(hspi->Instance->CR2, SPI_RXFIFO_THRESHOLD);

	/* Check if the SPI is already enabled */
	if ((hspi->Instance->CR1 & SPI_CR1_SPE) != SPI_CR1_SPE)
	{
		/* Enable SPI peripheral */
		__HAL_SPI_ENABLE(hspi);
		kprint("start\r\n");
	}

	hspi->pTxBuffPtr = pTxData;
	hspi->pRxBuffPtr = pRxData;


	uint16_t n = 0;
	//nprint("0x%x,", pTx[0]);
	uint32_t count = 0;
	while(1)
	{
		count = 0;
		do
		{
			if (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_TXE))
			{
				hspi->Instance->DR = *(uint16_t *)hspi->pTxBuffPtr;
				hspi->pTxBuffPtr += sizeof(uint16_t);
				break;
			}
			count++;
			if (count == 1000)
			{
				kprint("1\r\n");
				return STD_FAILED;
			}
		}while (1);

		count = 0;
		do
		{

			if (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_RXNE))
			{
				*(uint16_t*)hspi->pRxBuffPtr = *(__IO uint16_t *)&hspi->Instance->DR;
				hspi->pRxBuffPtr += sizeof(uint16_t);
				break;
			}
			count++;
			if (count == 1000)
			{
				kprint("2\r\n");
				return STD_FAILED;
			}
		} while (1);

		n++;
		if (n == Size)
		{
			break;
		}
	}
	//nprint("%d,%d,%d\r\n",(int16_t)pRx[2], (int16_t)pRx[3], (int16_t)pRx[4]);
	count = 0;
	while ((hspi->Instance->SR & SPI_FLAG_FTLVL) != SPI_FTLVL_EMPTY)
	{
		count++;
		if (count == 1000)
		{
			kprint("3\r\n");
			return STD_FAILED;
		}
	}


	count = 0;
	while (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_BSY) != RESET)
		//while ((__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_BSY) ? SET : RESET) != RESET)
	{
		count++;
		if (count == 1000)
		{
			kprint("4\r\n");
			return STD_FAILED;
		}
	}


	count = 0;
	__IO uint8_t  tmpreg8 = 0;
	while ((hspi->Instance->SR & SPI_FLAG_FRLVL) != SPI_FRLVL_EMPTY)
	{

		/* Flush Data Register by a blank read */
		tmpreg8 = READ_REG(*((__IO uint8_t*) & hspi->Instance->DR));
		/* To avoid GCC warning */
		UNUSED(tmpreg8);

		count++;
		if (count == 1000)
		{
			kprint("5\r\n");
			return STD_FAILED;
		}
	}

	return STD_SUCCESS;
}

static __IO  uint32_t sSPICount = 0;
static const uint16_t sSpiTxBuff[12] = { 0x6800 };
static uint16_t sSpiRxBuff[12] = { 0 };
uint8_t spi_a_hw_transmit_receive2(uint8_t* pTxData, uint8_t* pRxData, uint16_t Size, uint32_t Timeout)
{

	static SPI_HandleTypeDef* hspi = &aSpiHandle;

	uint8_t ret = STD_SUCCESS;
	SPI_A_HW_CS(0);

	CLEAR_BIT(hspi->Instance->CR2, SPI_RXFIFO_THRESHOLD);

	/* Check if the SPI is already enabled */
	if ((hspi->Instance->CR1 & SPI_CR1_SPE) != SPI_CR1_SPE)
	{
		/* Enable SPI peripheral */
		__HAL_SPI_ENABLE(hspi);
		kprint("start\r\n");
	}

	hspi->pTxBuffPtr = pTxData;
	hspi->pRxBuffPtr = pRxData;


	__IO uint16_t n = 0;
	//nprint("0x%x,", sSpiTxBuff[0]);
	uint64_t start = Clock_Time();
	while (1)
	{
		sSPICount = 0;
		do
		{
			if (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_TXE))
			{
				//hspi->Instance->DR = *(uint16_t*)hspi->pTxBuffPtr;
				//hspi->pTxBuffPtr += sizeof(uint16_t);
				hspi->Instance->DR = sSpiTxBuff[sSPICount];

				break;
			}
			sSPICount++;
			if (sSPICount == 1000)
			{
				kprint("1\r\n");
				ret = STD_FAILED;
				goto end;
			}

			if ((Clock_Time() - start) > Timeout)
			{
				kprint("5\r\n");
				ret = STD_FAILED;
				goto end;
			}

		} while (1);

		sSPICount = 0;
		do
		{

			if (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_RXNE))
			{
				sSpiRxBuff[n] = hspi->Instance->DR;
				break;
			}
			sSPICount++;
			if (sSPICount == 1000)
			{
				kprint("2\r\n");
				ret = STD_FAILED;
				goto end;
			}
		} while (1);

		n++;
		if (n == Size)
		{
			break;
		}
	}

	sSPICount = 0;
	while ((hspi->Instance->SR & SPI_FLAG_FTLVL) != SPI_FTLVL_EMPTY)
	{
		sSPICount++;
		if (sSPICount == 1000)
		{
			kprint("3\r\n");
			ret = STD_FAILED;
			goto end;
		}
	}


	sSPICount = 0;
	while (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_BSY) != RESET)
		//while ((__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_BSY) ? SET : RESET) != RESET)
	{
		sSPICount++;
		if (sSPICount == 1000)
		{
			kprint("4\r\n");
			ret = STD_FAILED;
			goto end;
		}
	}


	sSPICount = 0;
	__IO uint8_t  tmpreg8 = 0;
	while ((hspi->Instance->SR & SPI_FLAG_FRLVL) != SPI_FRLVL_EMPTY)
	{

		/* Flush Data Register by a blank read */
		tmpreg8 = READ_REG(*((__IO uint8_t*) & hspi->Instance->DR));
		/* To avoid GCC warning */
		UNUSED(tmpreg8);

		sSPICount++;
		if (sSPICount == 1000)
		{
			kprint("5\r\n");
			ret = STD_FAILED;
			goto end;
		}
	}


	static uint32_t buffcount = 0;
	buffcount++;
	if (buffcount == 10)
	{
		buffcount = 0;
		nprint("%d,%d,%d\r\n", (int16_t)sSpiRxBuff[2], (int16_t)sSpiRxBuff[3], (int16_t)sSpiRxBuff[4]);
	}

	ret =  STD_SUCCESS;




end:
	SPI_A_HW_CS(1);
	return ret;
}


#pragma endregion
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
