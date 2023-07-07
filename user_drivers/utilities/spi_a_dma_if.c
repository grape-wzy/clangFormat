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

#define ENABLE_SPI_A_DMA	1
#define ENABLE_READY_HARDWARE_INTERRUPT

static SPI_HandleTypeDef aSpiHandle;


#if ENABLE_SPI_A_DMA == 1

static DMA_HandleTypeDef sHdmaSpi1Tx;
static DMA_HandleTypeDef sHdmaSpi1Rx;

enum
{
	TRANSFER_WAIT,
	TRANSFER_COMPLETE,
	TRANSFER_ERROR
};

static void spi_a_hw_error_cb(SPI_HandleTypeDef* hspi);
static void spi_a_hw_cplt_cb(SPI_HandleTypeDef* hspi);

#endif

uint8_t spi_ll_transmit_receive(SPI_HandleTypeDef* hspi, uint8_t* pTxData, uint8_t* pRxData, uint16_t Size, uint32_t Timeout);

static void spi_a_hw_mspinit(SPI_HandleTypeDef* hspi);
static void spi_a_hw_mspdeinit(SPI_HandleTypeDef* hspi);


static uint8_t sSpiInit = false;

#if ENABLE_SPI_A_DMA == 1
static __IO uint8_t wTransferState = TRANSFER_WAIT;
#endif

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

#if ENABLE_SPI_A_DMA == 1
	HAL_SPI_RegisterCallback(&aSpiHandle, HAL_SPI_TX_RX_COMPLETE_CB_ID, spi_a_hw_cplt_cb);
	HAL_SPI_RegisterCallback(&aSpiHandle, HAL_SPI_ERROR_CB_ID, spi_a_hw_error_cb);
#endif

	kprint("ok\r\n");
	return STD_SUCCESS;
}

uint8_t spi_a_hw_deinit(void)
{
	//if (!sSpiInit)
	//{
	//	return STD_SUCCESS;
	//}
	sSpiInit = false;
	aSpiHandle.Instance = SPI_A_HW_INSTANCE;
	HAL_SPI_RegisterCallback(&aSpiHandle, HAL_SPI_MSPINIT_CB_ID, spi_a_hw_mspinit);
	HAL_SPI_RegisterCallback(&aSpiHandle, HAL_SPI_MSPDEINIT_CB_ID, spi_a_hw_mspdeinit);
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

#if ENABLE_SPI_A_DMA == 1
		/* DMA controller clock enable */
		SPI_A_HW_DMAMUX_CLK_ENABLE();
		SPI_A_HW_DMA_CLK_ENABLE();
#endif




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
		spi_a_hw_reset(1);


#ifdef ENABLE_READY_HARDWARE_INTERRUPT
		/* READY */
		GPIO_InitStruct.Pin = SPI_A_HW_READY_PIN;
		GPIO_InitStruct.Mode = SPI_A_HW_READY_MODE;
		GPIO_InitStruct.Pull = SPI_A_HW_READY_PULL;
		//GPIO_InitStruct.Speed = SPI_A_HW_READY_SPEED;
		//GPIO_InitStruct.Alternate = SPI_A_HW_READY_ALTERNATE;
		HAL_GPIO_Init(SPI_A_HW_READY_PORT, &GPIO_InitStruct);
#else

		LL_EXTI_DisableEvent_0_31(SPI_A_HW_READY_EXTI_PIN);
		LL_EXTI_EnableIT_0_31(SPI_A_HW_READY_EXTI_PIN);

#endif
		/* Enable and set line 4 Interrupt to the lowest priority */
		HAL_NVIC_SetPriority(SPI_A_HW_READY_EXTI_IRQn, SPI_A_HW_DRDY_NVIC_PreemptionPriority, SPI_A_HW_DRDY_NVIC_SubPriority);
		//HAL_NVIC_EnableIRQ(SPI_A_HW_READY_EXTI_IRQn);

#if ENABLE_SPI_A_DMA == 1
		/* SPI1 DMA Init */
		/* SPI1_TX Init */
		sHdmaSpi1Tx.Instance = SPI_A_TX_DMA_CHANNEL;
		sHdmaSpi1Tx.Init.Request = DMA_REQUEST_SPI1_TX;
		sHdmaSpi1Tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
		sHdmaSpi1Tx.Init.PeriphInc = DMA_PINC_DISABLE;
		sHdmaSpi1Tx.Init.MemInc = DMA_MINC_ENABLE;
		sHdmaSpi1Tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
		sHdmaSpi1Tx.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
		sHdmaSpi1Tx.Init.Mode = DMA_NORMAL;
		sHdmaSpi1Tx.Init.Priority = DMA_PRIORITY_HIGH;
		HAL_DMA_Init(&sHdmaSpi1Tx);

		__HAL_LINKDMA(hspi, hdmatx, sHdmaSpi1Tx);

		/* SPI1_RX Init */
		sHdmaSpi1Rx.Instance = SPI_A_RX_DMA_CHANNEL;
		sHdmaSpi1Rx.Init.Request = DMA_REQUEST_SPI1_RX;
		sHdmaSpi1Rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
		sHdmaSpi1Rx.Init.PeriphInc = DMA_PINC_DISABLE;
		sHdmaSpi1Rx.Init.MemInc = DMA_MINC_ENABLE;
		sHdmaSpi1Rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
		sHdmaSpi1Rx.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
		sHdmaSpi1Rx.Init.Mode = DMA_NORMAL;
		sHdmaSpi1Rx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
		HAL_DMA_Init(&sHdmaSpi1Rx);

		__HAL_LINKDMA(hspi, hdmarx, sHdmaSpi1Rx);


		/* DMA interrupt init */
		/* DMA1_Channel3_IRQn interrupt configuration */
		HAL_NVIC_SetPriority(SPI_A_TX_DMA_IRQn, SPI_A_HW_DMA_NVIC_PreemptionPriority, SPI_A_HW_DMA_NVIC_SubPriority);
		HAL_NVIC_EnableIRQ(SPI_A_TX_DMA_IRQn);
		/* DMA1_Channel2_IRQn interrupt configuration */
		HAL_NVIC_SetPriority(SPI_A_RX_DMA_IRQn, SPI_A_HW_DMA_NVIC_PreemptionPriority, SPI_A_HW_DMA_NVIC_SubPriority);
		HAL_NVIC_EnableIRQ(SPI_A_RX_DMA_IRQn);
#endif
	}
}


void spi_a_hw_mspdeinit(SPI_HandleTypeDef* hspi)
{

	if (hspi->Instance == SPI_A_HW_INSTANCE)
	{
		SPI_A_HW_CLK_DISABLE();

		//SPI_A_HW_SCLK_CLK_ENABLE();
		//SPI_A_HW_MISO_CLK_ENABLE();
		//SPI_A_HW_MOSI_CLK_ENABLE();
		//SPI_A_HW_CS_CLK_ENABLE();
		//SPI_A_HW_SYNC_CLK_ENABLE();
		//SPI_A_HW_READY_CLK_ENABLE();

		//GPIO_InitTypeDef GPIO_InitStruct;
		//GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
		//GPIO_InitStruct.Pull = GPIO_PULLDOWN;
		//GPIO_InitStruct.Alternate = 0;

		//GPIO_InitStruct.Pin = SPI_A_HW_CS_PIN;
		//HAL_GPIO_Init(SPI_A_HW_CS_PORT, &GPIO_InitStruct);

		//GPIO_InitStruct.Pin = SPI_A_HW_MOSI_PIN;
		//HAL_GPIO_Init(SPI_A_HW_MOSI_PORT, &GPIO_InitStruct);

		//GPIO_InitStruct.Pin = SPI_A_HW_MISO_PIN;
		//HAL_GPIO_Init(SPI_A_HW_MISO_PORT, &GPIO_InitStruct);

		//GPIO_InitStruct.Pin = SPI_A_HW_SCLK_PIN;
		//HAL_GPIO_Init(SPI_A_HW_SCLK_PORT, &GPIO_InitStruct);

		//GPIO_InitStruct.Pin = SPI_A_HW_RESET_PIN;
		//HAL_GPIO_Init(SPI_A_HW_RESET_PORT, &GPIO_InitStruct);

		//GPIO_InitStruct.Pin = SPI_A_HW_SYNC_PIN;
		//HAL_GPIO_Init(SPI_A_HW_SYNC_PORT, &GPIO_InitStruct);


		//GPIO_InitStruct.Pin = SPI_A_HW_READY_PIN;
		//HAL_GPIO_Init(SPI_A_HW_READY_PORT, &GPIO_InitStruct);


		HAL_GPIO_DeInit(SPI_A_HW_CS_PORT, SPI_A_HW_CS_PIN);
		HAL_GPIO_DeInit(SPI_A_HW_MOSI_PORT, SPI_A_HW_MOSI_PIN);
		HAL_GPIO_DeInit(SPI_A_HW_MISO_PORT, SPI_A_HW_MISO_PIN);
		HAL_GPIO_DeInit(SPI_A_HW_SCLK_PORT, SPI_A_HW_SCLK_PIN);
		HAL_GPIO_DeInit(SPI_A_HW_RESET_PORT, SPI_A_HW_RESET_PIN);
#ifdef ENABLE_READY_HARDWARE_INTERRUPT
		HAL_GPIO_DeInit(SPI_A_HW_READY_PORT, SPI_A_HW_READY_PIN);
#endif
		HAL_GPIO_DeInit(SPI_A_HW_SYNC_PORT, SPI_A_HW_SYNC_PIN);

#if ENABLE_SPI_A_DMA == 1
#ifndef DEBUG
		SPI_A_HW_DMAMUX_CLK_DISABLE();
		SPI_A_HW_DMA_CLK_DISABLE();
#endif
		sHdmaSpi1Tx.Instance = SPI_A_TX_DMA_CHANNEL;
		__HAL_LINKDMA(hspi, hdmatx, sHdmaSpi1Tx);

		sHdmaSpi1Rx.Instance = SPI_A_RX_DMA_CHANNEL;
		__HAL_LINKDMA(hspi, hdmarx, sHdmaSpi1Rx);

		HAL_DMA_DeInit(hspi->hdmatx);
		HAL_DMA_DeInit(hspi->hdmarx);

#endif
	}
}


uint8_t spi_a_hw_transmit_receive(uint8_t* pTxData, uint8_t* pRxData, uint16_t Size, uint32_t Timeout)
{
	uint8_t ret = STD_SUCCESS;
	SPI_A_HW_CS(0);

#if ENABLE_SPI_A_DMA == 1

    uint64_t To = Clock_Time() + Timeout;
    // uint32_t To = Timeout * 0xffff;
    wTransferState = TRANSFER_WAIT;
    // ATOMIC_SECTION_BEGIN();
    uint8_t hal_status = HAL_SPI_TransmitReceive_DMA(&aSpiHandle, pTxData, pRxData, Size);
    // ATOMIC_SECTION_END();
    if (hal_status != HAL_OK) {
        kprint("dma error\r\n");
        ret = STD_FAILED;
    } else {
        while (wTransferState == TRANSFER_WAIT) {
            if (Clock_Time() > To) {
                static uint8_t status = 0;
                if (status == 0) {
                    status = 1;
                    led_ctrl(LED_MODE_Y_CTRL, 1);
                } else {
                    status = 0;
                    led_ctrl(LED_MODE_Y_CTRL, 0);
                }
                ret = STD_FAILED;
                kprint("timeout\r\n");
                break;
            }
        }
        if (wTransferState == TRANSFER_ERROR) {
            kprint("error\r\n");
            ret = STD_FAILED;
        }
    }

#else
	if ((HAL_SPI_TransmitReceive(&aSpiHandle, pTxData, pRxData, Size, Timeout)) != HAL_OK)
	{
		ret = STD_FAILED;
	}
#endif
	//ret = spi_ll_transmit_receive(&aSpiHandle, pTxData, pRxData, Size, Timeout);
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


	//return SPI_A_HW_READ_READY();

	if (__HAL_GPIO_EXTI_GET_IT(SPI_A_HW_READY_EXTI_PIN) != RESET)
	{
		__HAL_GPIO_EXTI_CLEAR_IT(SPI_A_HW_READY_EXTI_PIN);
		return true;
	}

	return false;
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

uint8_t spi_a_hw_generate_swi(void)
{
#ifdef ENABLE_READY_HARDWARE_INTERRUPT
#else
	LL_EXTI_GenerateSWI_0_31(SPI_A_HW_READY_EXTI_PIN);
#endif
	return STD_SUCCESS;
}

#pragma region 中断

typedef void(*HW_IRQ_CALLBACK_TYPE) (void);
static HW_IRQ_CALLBACK_TYPE irq_callback;
void spi_a_hw_register_cb(void* cb)
{
	irq_callback = cb;
}

#if ENABLE_SPI_A_DMA == 1
#if 0
void SPI_A_TX_DMA_IRQHandler(void)
{
	HAL_DMA_IRQHandler(&sHdmaSpi1Tx);
}

void SPI_A_RX_DMA_IRQHandler(void)
{
	HAL_DMA_IRQHandler(&sHdmaSpi1Rx);
}
#endif
#endif

#pragma endregion

#pragma region Low Level

uint8_t spi_ll_transmit_receive(SPI_HandleTypeDef* hspi, uint8_t* pTxData, uint8_t* pRxData, uint16_t Size, uint32_t Timeout)
{
	CLEAR_BIT(aSpiHandle.Instance->CR2, SPI_RXFIFO_THRESHOLD);

	/* Check if the SPI is already enabled */
	if ((aSpiHandle.Instance->CR1 & SPI_CR1_SPE) != SPI_CR1_SPE)
	{
		/* Enable SPI peripheral */
		__HAL_SPI_ENABLE(&aSpiHandle);
	}
	uint64_t TimeEnd = Clock_Time() + Timeout;//必须
	uint16_t* pTx = (uint16_t*)pTxData;
	uint16_t* pRx = (uint16_t*)pRxData;
	for (uint32_t i = 0; i < Size; i++)
	{
		while (!__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_TXE))
		{

			if (Clock_Time() > TimeEnd)
			{
				return STD_FAILED;
			}
		}
		hspi->Instance->DR = pTx[i];


		while (!__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_RXNE))
		{
			if (Clock_Time() > TimeEnd)
			{
				return STD_FAILED;
			}
		}

		pRx[i] = hspi->Instance->DR;

	}


	while ((hspi->Instance->SR & SPI_FLAG_FTLVL) != SPI_FTLVL_EMPTY)
	{
		if (Clock_Time() > TimeEnd)
		{
			return STD_FAILED;
		}
	}


	while (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_BSY) != RESET)
		//while ((__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_BSY) ? SET : RESET) != RESET)
	{
		if (Clock_Time() > TimeEnd)
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

		if (Clock_Time() > TimeEnd)
		{
			return STD_FAILED;
		}
	}

	return STD_SUCCESS;
}
#pragma endregion

#pragma region dma 回调

#if ENABLE_SPI_A_DMA == 1

void spi_a_hw_cplt_cb(SPI_HandleTypeDef* hspi)
{
	wTransferState = TRANSFER_COMPLETE;
}

void spi_a_hw_error_cb(SPI_HandleTypeDef* hspi)
{
	wTransferState = TRANSFER_ERROR;
	kprint("error\r\n");
}

#endif

#pragma endregion



/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
