/*******************************************************************************
 * file    main.c
 * author  mackgim
 * version 1.0.0
 * date
 * brief   主函数
 *******************************************************************************/

#include "main.h"
#include "platform.h"
#include "spi_if.h"

#include "acc_gyro_if.h"

#if 0
int main()
{

	platform_init();

	kprint("\r\n");
	kprint("----run----\r\n");
	kprint("\r\n");

	while (1)
	{
		itask_proc();
	}
}

#else
extern void MX_DMA_Init(void);
int main(void)
{
    ACC_GYRO_FDATA_T imu_data[2];

    HAL_Init();

    ble_init_clock();

#if ENABLE_RCC_HSE_32M
    SystemClock_32M_Config();
#else
    SystemClock_64M_Config();
#endif

    ym_hw_pin_init();

    MX_DMA_Init();

    log_init();

    gtimer_init();

    spi1_hw_init();

    acc_gpro_init();

    acc_gyro_enable();

    while (1) {
        acc_gyro_get_result(imu_data, 1);
        HAL_Delay(20);
    }
    // imu_lsm6ds3_ctx.read_reg = spi_a_hw_read_reg
}
#endif
