/*******************************************************************************
* file    gpio_if.c
* author  mackgim
* version 1.0.0
* date
* brief   io 操作
*******************************************************************************/

#include "gpio_if.h"
#include "platform.h"
#include "standard_lib.h"


#define LED_DEFAULT_OFF_INTERVAL	(960)
#define LED_DEFAULT_ON_INTERVAL		(40)

//#define LED_OFF_INTERVAL	sLedDefaultOff
//#define LED_ON_INTERVAL	sLedDefaultOn
#define LED_OFF_INTERVAL	LED_DEFAULT_OFF_INTERVAL
#define LED_ON_INTERVAL		LED_DEFAULT_ON_INTERVAL

void led_ts_callback(void);
static uint8_t sLedTSID;
uint32_t sLedInterval = 0;;

void gpio_init(void)
{

	LEDG_CLK_ENABLE();
    LEDY_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct;

    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    GPIO_InitStruct.Pin = LEDG_PIN;
    HAL_GPIO_Init(LEDG_PORT, &GPIO_InitStruct);
    ledg_ctrl(0);

    GPIO_InitStruct.Pin = LEDY_PIN;
    HAL_GPIO_Init(LEDY_PORT, &GPIO_InitStruct);
    ledy_ctrl(0);

#ifdef USE_SENSOR_PWR_PIN
    PWR_GPIO_CLK_ENABLE();
    GPIO_InitStruct.Pin = PWR_GPIO_PIN;
    HAL_GPIO_Init(PWR_GPIO_PORT, &GPIO_InitStruct);
    pwr_ctrl(0);
#endif

#ifdef USE_DEVICE_PIN
    DEVICE_GPIO_CLK_ENABLE();
    /* Which Device*/
    GPIO_InitStruct.Pin  = DEVICE_GPIO_PIN;
    GPIO_InitStruct.Mode = DEVICE_GPIO_MODE;
    GPIO_InitStruct.Pull = DEVICE_GPIO_PULL;
    HAL_GPIO_Init(DEVICE_GPIO_PORT, &GPIO_InitStruct);
#endif

    ts_create(0, &(sLedTSID), TS_SingleShot, led_ts_callback);
	sLedInterval = LED_DEFAULT_ON_INTERVAL;
}

void gpio_deinit(void)
{
    HAL_GPIO_DeInit(LEDG_PORT, LEDG_PIN);
    HAL_GPIO_DeInit(LEDY_PORT, LEDY_PIN);
#ifdef USE_SENSOR_PWR_PIN
    HAL_GPIO_DeInit(PWR_GPIO_PORT, PWR_GPIO_PIN);
#endif
#ifdef USE_DEVICE_PIN
    HAL_GPIO_DeInit(DEVICE_GPIO_PORT, DEVICE_GPIO_PIN);
#endif
}

void pwr_ctrl(uint8_t value)
{
#ifdef USE_SENSOR_PWR_PIN
    PWR_GPIO_CTRL(value?0:1);
    Clock_Wait(10);
#endif
}

uint8_t read_device_type(void)
{
#ifdef USE_DEVICE_PIN
    //return DEVICE_GPIO_READ();

    //20220623， 模块的主从定义切换
    //原来gpio为0是master， 为1是ref
    //现在gpio为1是master，为0是ref
    if (DEVICE_GPIO_READ()) {
        return 0;
    } else {
        return 1;
    }
#endif
}

void ledg_ctrl(uint8_t value)
{
#if 0
	LEDG_CTRL(0);
#else
    if (value == 2)
    {
        LEDG_TOGGLE();
        return;
    }
    LEDG_CTRL(value);
#endif
}

void ledy_ctrl(uint8_t value)
{
#if 0
	LEDY_CTRL(0);
#else
    if (value == 2)
    {
        LEDY_TOGGLE();
        return;
    }
    LEDY_CTRL(value);
#endif
}



/* Mode定义
 * 0 - 控制ledy状态
 * 1 - 控制ledg状态
 * 2 - 控制ledy闪烁, timer循环控制
 * 3 - 控制ledg闪烁，只控制能亮的时间
 **/

static uint8_t sLedMode = 0;
static volatile uint8_t sLedTimeStart = 0;

/**
 * @brief
 *
 * @param mode
 *
 * @param status
 *
 * @return
 */
void led_ctrl(uint8_t mode, uint8_t status)
{
	sLedMode = mode;
	switch (mode)
	{
	case LED_MODE_Y_CTRL:
	{
		//		kprint("set mode 0\r\n");
		//		ledg_ctrl(0);
		ledy_ctrl(status);
	}
	break;
	case LED_MODE_G_CTRL:
	{
		//		kprint("set mode 1\r\n");
		//		ledy_ctrl(0);
		ledg_ctrl(status);
	}
	break;
	case LED_MODE_Y_BLINK:
	{
		//		kprint("set mode 2\r\n");
		//ledg_ctrl(0);
		//		ledy_ctrl(1);
		sLedInterval = LED_ON_INTERVAL;
		if (sLedTimeStart == 0)
		{
			sLedTimeStart = 1;
			ts_start_ms(sLedTSID, sLedInterval);
		}

	}
	break;
	case LED_MODE_G_BLINK:
	{
		//		kprint("set mode 3\r\n");
		//		ledy_ctrl(0);
		ledg_ctrl(1);
		sLedInterval = LED_ON_INTERVAL;
		if (sLedTimeStart == 0)
		{
			sLedTimeStart = 1;
			ts_start_ms(sLedTSID, sLedInterval);
		}
	}
	break;
	case LED_MODE_Y_G_BLINK:
	{
		//		kprint("set mode 3\r\n");
		ledy_ctrl(1);
		ledg_ctrl(1);
		sLedInterval = LED_ON_INTERVAL;
		if (sLedTimeStart == 0)
		{
			sLedTimeStart = 1;
			ts_start_ms(sLedTSID, sLedInterval);
		}
	}
	break;
	}
}

void led_ts_callback(void)
{
	sLedTimeStart = 0;
	if (sLedMode == LED_MODE_Y_BLINK)
	{
		if (sLedInterval == LED_ON_INTERVAL)
		{
			sLedInterval = LED_OFF_INTERVAL;
			ledy_ctrl(0);
		}
		else
		{
			sLedInterval = LED_ON_INTERVAL;
			ledy_ctrl(1);
		}
		ts_start_ms(sLedTSID, sLedInterval);
	}
	else  if (sLedMode == LED_MODE_G_BLINK)
	{
		ledg_ctrl(0);
	}
	else if (sLedMode == LED_MODE_Y_G_BLINK)
	{
		ledg_ctrl(0);
		ledy_ctrl(0);
	}
}




#pragma region 固件升级时，灯的状态

void led_blink_while_upgrading_fw(void)
{
	uint8_t m = 0;
	for (uint32_t i = 0; i < 10; i++)
	{
		m = (~m) & 0x01;
		led_ctrl(LED_MODE_Y_CTRL, m);
		led_ctrl(LED_MODE_G_CTRL, m);
		Clock_Wait(100);
	}
	led_ctrl(LED_MODE_Y_CTRL, 1);
	led_ctrl(LED_MODE_G_CTRL, 1);

	log_flush();
}

#pragma endregion