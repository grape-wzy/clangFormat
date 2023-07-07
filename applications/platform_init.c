/*******************************************************************************
* file    PlatformInit.c
* author  mackgim
* version 1.0.0
* date
* brief   STM32的外设，用户外设，用户数据的初始化，系统自检，启动函数等
*******************************************************************************/
/* Includes ------------------------------------------------------------------*/
#include "platform_init.h"
#include "user_drivers.h"
#include "platform.h"
#include "itask.h"
#include "pm_proc.h"
#include "pd_proc.h"
#include "acc_gyro_if.h"
#include "spi_if.h"

#include "drv_pin.h"
#include "driver_cfg.h"

void Error_Handler(void);

/*******************************************************************************
* Function Name  : void ConfigureSTMPeripheral(void)
* Description    : 配置STM32外设
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/

void platform_init(void)
{

	HAL_Init();

	ble_init_clock();

#if ENABLE_RCC_HSE_32M
	SystemClock_32M_Config();
#else
	SystemClock_64M_Config();
#endif

	log_init();
	gtimer_init();
	gpio_init();
	led_ctrl(LED_MODE_Y_CTRL, 1);
	led_ctrl(LED_MODE_G_CTRL, 1);

	kprint("[RCC]: WB55 Start:\r\n");
	nprint("\r\n");
	kprint("[RCC]: SysClockFreq = %u\r\n", (unsigned int)HAL_RCC_GetSysClockFreq());
	kprint("[RCC]: HCLKFreq = %u\r\n", (unsigned int)HAL_RCC_GetHCLKFreq());
	kprint("[RCC]: PCLK1Freq = %u\r\n", (unsigned int)HAL_RCC_GetPCLK1Freq());
	kprint("[RCC]: PCLK2Freq = %u\r\n", (unsigned int)HAL_RCC_GetPCLK2Freq());
	kprint("SMPS = 0x%x\r\n", (unsigned int)LL_RCC_GetSMPSClockSelection());

	log_flush();

	crc32_init();
	iap_init();

	flash_register_callback(ble_is_init);

	rng_init();
	pd_init();

	pwr_init();
	log_flush();

	itask_init();

	led_ctrl(LED_MODE_G_CTRL, 0);
	led_ctrl(LED_MODE_Y_BLINK, 0);

	kprint("ok\r\n");

}

#define TIMEOUT_DURATION 100

static struct ym_spi_interface acc_dev;

uint8_t spi_ReadReg(void *handle, uint8_t reg, uint8_t *data, uint16_t size)
{
    uint8_t read = (reg | 0x80);

    if (YM_EOK != ym_spi_interface_send_then_recv(&acc_dev,
                                                  &read,
                                                  1,
                                                  data,
                                                  size)) {
        return STD_FAILED;
    }

    return 0;
}

uint8_t spi_WriteReg(void *handle, uint8_t reg, uint8_t *data, uint16_t size)
{
    uint8_t read = (reg & 0x7F);

    if (YM_EOK != ym_spi_interface_send_then_send(&acc_dev,
                                                  &read,
                                                  1,
                                                  data,
                                                  size)) {
        return STD_FAILED;
    }

    return 0;
}

void acc_gpro_init(void)
{
    ym_interface_t spi_bus = ym_interface_find_by_name("spi1");
    ym_spi_interface_register(&acc_dev,
                              "acc_gyro",
                              (struct ym_spi_inf_bus *)spi_bus,
                              GET_PIN(A, 4), 0);

    ACC_GYRO_CONFIG_TypeDef acc_gyro_config = {
        AHRS_ACC_GYRO_DATA_RATE,
        AHRS_GYRO_SCALE,
        AHRS_ACC_SCALE,
        AHRS_ACC_GYRO_POWER_MODE,
        spi_WriteReg,
        spi_ReadReg,
        NULL,
        NULL,
    };

    acc_gyro_init(&acc_gyro_config);
}

/*******************************************************************************
* Function Name  : void DefaultConfigureSTMPeripheral(void)
* Description    : 复位配置
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void platform_deinit(void)
{
    //LEDG_DEINIT();
    //LEDY_DEINIT();

    //crc32_deinit();
    //ble_os_deinit();

    //log_deinit();
}

void SystemClock_32M_Config(void)
{
    RCC_OscInitTypeDef       RCC_OscInitStruct   = { 0 };
    RCC_ClkInitTypeDef       RCC_ClkInitStruct   = { 0 };
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = { 0 };

    /** Configure LSE Drive Capability
	*/
    HAL_PWR_EnableBkUpAccess();
    __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);
    /** Configure the main internal regulator output voltage
	*/
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    /** Initializes the RCC Oscillators according to the specified parameters
	* in the RCC_OscInitTypeDef structure.
	*/
    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI48 | RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_LSE;
    RCC_OscInitStruct.HSEState            = RCC_HSE_ON;
    RCC_OscInitStruct.LSEState            = RCC_LSE_ON;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSI48State          = RCC_HSI48_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_NONE;
    //RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    //RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    //RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV2;
    //RCC_OscInitStruct.PLL.PLLN = 8;
    //RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    //RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
    //RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }
    /** Configure the SYSCLKSource, HCLK, PCLK1 and PCLK2 clocks dividers
	*/
    RCC_ClkInitStruct.ClockType    = RCC_CLOCKTYPE_HCLK4 | RCC_CLOCKTYPE_HCLK2 | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSE;
    //RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.AHBCLK2Divider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLK4Divider = RCC_SYSCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
        Error_Handler();
    }
    /** Initializes the peripherals clocks
	*/
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SMPS | RCC_PERIPHCLK_RFWAKEUP | RCC_PERIPHCLK_RTC | RCC_PERIPHCLK_RNG
#ifdef DEBUG
                                               | RCC_PERIPHCLK_USART1
#endif
                                               | RCC_PERIPHCLK_LPTIM1;
    //PeriphClkInitStruct.PLLSAI1.PLLN = 6;
    //PeriphClkInitStruct.PLLSAI1.PLLP = RCC_PLLP_DIV2;
    //PeriphClkInitStruct.PLLSAI1.PLLQ = RCC_PLLQ_DIV2;
    //PeriphClkInitStruct.PLLSAI1.PLLR = RCC_PLLR_DIV2;
    //PeriphClkInitStruct.PLLSAI1.PLLSAI1ClockOut = RCC_PLLSAI1_USBCLK;
    //PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_PLLSAI1;
#ifdef DEBUG
	PeriphClkInitStruct.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
#endif
	PeriphClkInitStruct.Lptim1ClockSelection = RCC_LPTIM1CLKSOURCE_LSE;

	PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
	PeriphClkInitStruct.RngClockSelection = RCC_RNGCLKSOURCE_HSI48;
	PeriphClkInitStruct.RFWakeUpClockSelection = RCC_RFWKPCLKSOURCE_LSE;
	PeriphClkInitStruct.SmpsClockSelection = RCC_SMPSCLKSOURCE_HSI;
	PeriphClkInitStruct.SmpsDivSelection = RCC_SMPSCLKDIV_RANGE1;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
	{
		Error_Handler();
    }
}

void SystemClock_64M_Config(void)
{

	/* 1 - Set Voltage scaling to LL_PWR_REGU_VOLTAGE_SCALE1 before increase Clock Frequency */
	LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);

	/* 2 - Wait Voltage Scaling 1 before increase frequency */
	while (LL_PWR_IsActiveFlag_VOS() != 0)
	{
	}

	LL_FLASH_SetLatency(LL_FLASH_LATENCY_3);
	while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_3)
	{
	}

	LL_RCC_HSE_Enable();
	while (!LL_RCC_HSE_IsReady());

	/* HSI configuration and activation */
	LL_RCC_HSI_Enable();
	while (!LL_RCC_HSI_IsReady());
	/* Adjusts the Internal High Speed oscillator (HSI) calibration value.*/
	LL_RCC_HSI_SetCalibTrimming(RCC_HSICALIBRATION_DEFAULT);

	/* Enable HSI48 oscillator */
	LL_RCC_HSI48_Enable();
	/* Wait until HSI48 is ready */
	while (!LL_RCC_HSI48_IsReady());

	LL_PWR_EnableBkUpAccess();
	LL_RCC_LSE_SetDriveCapability(LL_RCC_LSEDRIVE_LOW);
	LL_RCC_LSE_Enable();
	while (!LL_RCC_LSE_IsReady());

	/* Main PLL configuration and activation */
	LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_2, 8, LL_RCC_PLLR_DIV_2);
	LL_RCC_PLL_Enable();
	LL_RCC_PLL_EnableDomain_SYS();
	while (LL_RCC_PLL_IsReady() != 1)
	{
	}


	/* Set CPU1 prescaler*/
	LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1); //HCLK1

	/* Set CPU2 prescaler*/
	LL_C2_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_2);//HCLK2

	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
	{
	}

	/* Set AHB SHARED prescaler*/
	LL_RCC_SetAHB4Prescaler(LL_RCC_SYSCLK_DIV_1);//HCLK4

	/* Set APB1 prescaler*/
	LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);//PCLK1

	/* Set APB2 prescaler*/
	LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);//PLCK2

	//SystemCoreClock = HAL_RCC_GetHCLKFreq();
	/* Update CMSIS variable (which can be updated also through SystemCoreClockUpdate function) */
	//LL_SetSystemCoreClock(64000000);
	LL_SetSystemCoreClock(HAL_RCC_GetHCLKFreq());


	HAL_Init();

	/* Configure the SMPS interface clock division factor */
	LL_RCC_SetSMPSPrescaler(LL_RCC_SMPS_DIV_1);

	/* Configure the SMPS interface clock source */
	LL_RCC_SetSMPSClockSource(LL_RCC_SMPS_CLKSOURCE_HSI);

	/* Configure the RFWKP interface clock source */
	LL_RCC_SetRFWKPClockSource(LL_RCC_RFWKP_CLKSOURCE_LSE);
}

void SystemClock_64M_Config1(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = { 0 };

	/** Configure LSE Drive Capability
	*/
	HAL_PWR_EnableBkUpAccess();
	__HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);
	/** Configure the main internal regulator output voltage
	*/
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
	/** Initializes the RCC Oscillators according to the specified parameters
	* in the RCC_OscInitTypeDef structure.
	*/
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48
		| RCC_OSCILLATORTYPE_HSI
		| RCC_OSCILLATORTYPE_HSE
		| RCC_OSCILLATORTYPE_LSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.LSEState = RCC_LSE_ON;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV2;
	RCC_OscInitStruct.PLL.PLLN = 8;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}
	/** Configure the SYSCLKSource, HCLK, PCLK1 and PCLK2 clocks dividers
	*/
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK4 | RCC_CLOCKTYPE_HCLK2
		| RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
		| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.AHBCLK2Divider = RCC_SYSCLK_DIV2;
	RCC_ClkInitStruct.AHBCLK4Divider = RCC_SYSCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
	{
		Error_Handler();
	}
	/** Initializes the peripherals clocks
	*/
	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SMPS
		| RCC_PERIPHCLK_RFWAKEUP
		| RCC_PERIPHCLK_RTC
		| RCC_PERIPHCLK_RNG
#ifdef DEBUG
		| RCC_PERIPHCLK_USART1
#endif
		//| RCC_PERIPHCLK_LPTIM1
		;
	//PeriphClkInitStruct.PLLSAI1.PLLN = 6;
	//PeriphClkInitStruct.PLLSAI1.PLLP = RCC_PLLP_DIV2;
	//PeriphClkInitStruct.PLLSAI1.PLLQ = RCC_PLLQ_DIV2;
	//PeriphClkInitStruct.PLLSAI1.PLLR = RCC_PLLR_DIV2;
	//PeriphClkInitStruct.PLLSAI1.PLLSAI1ClockOut = RCC_PLLSAI1_USBCLK;
	//PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_PLLSAI1;
#ifdef DEBUG
	PeriphClkInitStruct.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
#endif
	//PeriphClkInitStruct.Lptim1ClockSelection = RCC_LPTIM1CLKSOURCE_LSE;

	PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
	PeriphClkInitStruct.RngClockSelection = RCC_RNGCLKSOURCE_HSI48;
	PeriphClkInitStruct.RFWakeUpClockSelection = RCC_RFWKPCLKSOURCE_LSE;
	PeriphClkInitStruct.SmpsClockSelection = RCC_SMPSCLKSOURCE_HSI;
	PeriphClkInitStruct.SmpsDivSelection = RCC_SMPSCLKDIV_RANGE1;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

}


void Error_Handler(void)
{
	/* User can add his own implementation to report the HAL error return state */
	kprint("Error\r\n");
	//log_flush();
}

/*******************************************************************************
END
*******************************************************************************/
