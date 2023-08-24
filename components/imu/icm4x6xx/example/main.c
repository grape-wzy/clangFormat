/*
 * ________________________________________________________________________________________________________
 * Copyright (c) 2017 InvenSense Inc. All rights reserved.
 *
 * This software, related documentation and any modifications thereto (collectively Software is subject
 * to InvenSense and its licensors' intellectual property rights under U.S. and international copyright
 * and other intellectual property rights laws.
 *
 * InvenSense and its licensors retain all intellectual property and proprietary rights in and to the Software
 * and any use, reproduction, disclosure or distribution of the Software without an express license agreement
 * from InvenSense is strictly prohibited.
 *
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE SOFTWARE IS
 * PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * INVENSENSE BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
 * ________________________________________________________________________________________________________
 */
#include <asf.h>

/* InvenSense drivers and utils */
#include "utils/InvScheduler.h"
//#include "utils/Message.h"
#include "utils/ErrorHelper.h"


/* Atmel SAMG55 */
#include "ASF/sam/drivers/pio/pio.h"
#include "ASF/sam/drivers/pio/pio_handler.h"
#include "ASF/sam/drivers/twi/twi.h"
#include "ASF/sam/drivers/tc/tc.h"
#include "ASF/common/services/delay/delay.h"

#include "system.h"
#include "main.h"
//#include "sensor.h"

static InvSchedulerTask blinkerLedTask;
uint32_t SysTickReloadValue;


static void msg_printer(int level, const char * str, va_list ap);

/*
 * BlinkerLedTaskMain - Task that blinks the LED.
 */
static void BlinkerLedTaskMain(void * arg)
{
    (void)arg;

    ioport_toggle_pin_level(LED_0_PIN); // toggle the LED
}
struct accGyroDataPacket datapacket;
/* Flag set from device irq handler */
volatile int irq_from_device = 0;

volatile uint32_t ul_ticks = 0;
uint64_t timestamp;
bool sensor[NUM_OF_SENSOR];
/*!
* @brief    Sensor general interrupt handler, calls specific handlers.
*
* This function is called when an external interrupt is triggered by the sensor,
* checks interrupt registers of InvenSense Sensor to determine the source and type of interrupt
* and calls the specific interrupt handler accordingly.
*
* @param[in]    NULL
*
* @param[out]   NULL
*
* @return       NULL
*
*/
void ext_interrupt_handler(void)
{
    /*
     * Multiply ul_ticks by 100 because it is currently ticking over at 10 kHz and we need to bump it up to 1 MHz.
     * this becomes the ms portion of the time stamp.
     * SysTick->VAL / SysTickReloadValue specifies the fraction of the 100 us timer remaining. Multiply this value by
     * 100 to convert from a 100 us to 1 us resolution.
     */
    timestamp = (ul_ticks * MICROSECONDS_PER_SYSTICK) + (((SysTickReloadValue - SysTick->VAL) * MICROSECONDS_PER_SYSTICK) / SysTickReloadValue);
    irq_from_device = 1;
}

void inv_data_handler()
{
    /* Raw A+G */
    if (true == sensor[ACC] || true == sensor[GYR]) {
        memset(&datapacket, 0, sizeof(struct accGyroDataPacket));
        inv_icm4x6xx_get_rawdata(&datapacket);
        INV_LOG(SENSOR_LOG_LEVEL, "Get Sensor Data ACC %d GYR %d TS %lld",
            datapacket.accOutSize, datapacket.gyroOutSize, timestamp);
        for (int i = 0; i < datapacket.accOutSize + datapacket.gyroOutSize; i++) {
            if (datapacket.outBuf[i].sensType == ACC)
                INV_LOG(SENSOR_LOG_LEVEL, "ACC %lld %f  %f  %f",
                    datapacket.outBuf[i].timeStamp, datapacket.outBuf[i].x, datapacket.outBuf[i].y, datapacket.outBuf[i].z);
            else if (datapacket.outBuf[i].sensType == GYR)
                INV_LOG(SENSOR_LOG_LEVEL, "GYR %lld %f  %f  %f",
                    datapacket.outBuf[i].timeStamp, datapacket.outBuf[i].x, datapacket.outBuf[i].y, datapacket.outBuf[i].z);
        }
    }
    #if SUPPORT_PEDOMETER
    if (true == sensor[PEDO]) {
        uint64_t step_cnt = 0;
        inv_icm4x6xx_pedometer_get_stepCnt(&step_cnt);
        INV_LOG(SENSOR_LOG_LEVEL, "Step count %d", step_cnt);
    }
    #endif
    #if SUPPORT_WOM
    if (true == sensor[WOM]) {
        bool wom_detect = false;
        inv_icm4x6xx_wom_get_event(&wom_detect);
        INV_LOG(SENSOR_LOG_LEVEL, "wom Motion %d", wom_detect);
    }
    #endif
}

void sm_delay_ms(uint16_t ms)
{
    delay_ms(ms);
}

void sm_delay_us(uint16_t us)
{
    delay_us(us);
}

void platform_init()
{
    /* Hardware initialization */
    sysclk_init();
    board_init();
    sysclk_enable_peripheral_clock(ID_TC0);

    /* Configure Device - Host Interface */
    configure_console();

    /* Setup message logging */
    INV_MSG_SETUP(INV_MSG_ENABLE, msg_printer);

    /* Initialize External Sensor Interrupt */
    ext_int_initialize(&ext_interrupt_handler);

    /* Initialize SPI/I2C*/
    interface_initialize();

    /* Set SysTick timer to MICROSECONDS_PER_SYSTICK us. */
    SysTickReloadValue = sysclk_get_cpu_hz() / MICROSECONDS_PER_SECOND * MICROSECONDS_PER_SYSTICK;

    /* Configure sysTick Timer */
    SysTick_Config(SysTickReloadValue);

    /* Clear timer-stamp and any remaining interrupts */
    __disable_irq();

    ul_ticks = 0;

    /* Clear pio interrupt */
    pio_clear(PIN_EXT_INTERRUPT_PIO, PIN_EXT_INTERRUPT_MASK);
    irq_from_device = 0;
    __enable_irq();

    inv_icm4x6xx_set_serif(idd_io_hal_read_reg, idd_io_hal_write_reg);
    inv_icm4x6xx_set_delay(sm_delay_ms, sm_delay_us);

}

int main (void)
{
    platform_init();

    INV_LOG(SENSOR_LOG_LEVEL, "###################################");
    INV_LOG(SENSOR_LOG_LEVEL, "#   ICM4X6XX MCU Driver V3.0      #");
    INV_LOG(SENSOR_LOG_LEVEL, "###################################");

    int ret = 0;
    #if SUPPORT_SELFTEST
    bool st_acc_result = false;
    bool st_gyr_result = false;
    ret += inv_icm4x6xx_acc_selftest(&st_acc_result);
    INV_LOG(SENSOR_LOG_LEVEL, "ACC Selftest ret %d result %d", ret, st_acc_result);
    ret += inv_icm4x6xx_gyro_selftest(&st_gyr_result);
    INV_LOG(SENSOR_LOG_LEVEL, "GYR Selftest ret %d result %d", ret, st_gyr_result);
    #endif

    ret = inv_icm4x6xx_initialize();
    if (ret != 0) {
        INV_LOG(SENSOR_LOG_LEVEL, "Chip Initialize Failed. Do nothing");
        //Release system resource Todo if need?
        return ret;
    }

    ret += inv_icm4x6xx_acc_set_rate(200, 2);//200Hz, watermark 2.
    ret += inv_icm4x6xx_acc_enable();
    sensor[ACC] = true;
    ret += inv_icm4x6xx_gyro_set_rate(100, 4);//100Hz, watermark 4.
    ret += inv_icm4x6xx_gyro_enable();
    sensor[GYR] = true;
    #if SUPPORT_PEDOMETER
    ret += inv_icm4x6xx_pedometer_enable();
    sensor[PEDO] = true;
    #endif
    #if SUPPORT_WOM
    ret += inv_icm4x6xx_wom_enable();
    sensor[WOM] = true;
    #endif
    if (ret != 0) {
        INV_LOG(SENSOR_LOG_LEVEL, "Feature enable Failed. Do nothing %d", ret);
        //Release system resource Todo if need?
        return ret;
    }

    InvScheduler_init(&scheduler);
    InvScheduler_initTask(&scheduler, &blinkerLedTask, "blinkerLedTask", BlinkerLedTaskMain, 0, INVSCHEDULER_TASK_PRIO_MIN+1, MICROSECONDS_PER_SECOND/MICROSECONDS_PER_SYSTICK); // keep the LED blink at 1Hz
    InvScheduler_startTask(&blinkerLedTask, 0);

    int i = 0;
    while (1) {
        InvScheduler_dispatchTasks(&scheduler);

        if (irq_from_device == 1) {
            inv_data_handler();
            __disable_irq();
            irq_from_device = 0;
            __enable_irq();
            i++;
        }
        /* seperate acc/gyro raw data test and apex tests */
        if (i == 100) {
            ret += inv_icm4x6xx_acc_disable();
            sensor[ACC] = false;
            //delay_ms(10);
            ret += inv_icm4x6xx_gyro_disable();
            //delay_ms(10);
            sensor[GYR] = false;
            i++;
        } else if (i == 150) {
            #if SUPPORT_PEDOMETER
            ret += inv_icm4x6xx_pedometer_disable();
            sensor[PEDO] = false;
            #endif
            #if SUPPORT_WOM
            ret += inv_icm4x6xx_wom_disable();
            sensor[WOM] = false;
            #endif
            break;
        }
    }
    return 0;
}

/*
 * Printer function for message facility
 */
static void msg_printer(int level, const char * str, va_list ap)
{
#ifdef INV_MSG_ENABLE
    static char out_str[BUFFER_SIZE]; /* static to limit stack usage */
    unsigned idx = 0;
    const char * ptr = out_str;
    const char * s[INV_MSG_LEVEL_MAX] = {
        "",    // INV_MSG_LEVEL_OFF
        "[E] ", // INV_MSG_LEVEL_ERROR
        "[W] ", // INV_MSG_LEVEL_WARNING
        "[I] ", // INV_MSG_LEVEL_INFO
        "[V] ", // INV_MSG_LEVEL_VERBOSE
        "[D] ", // INV_MSG_LEVEL_DEBUG
    };
    idx += snprintf(&out_str[idx], sizeof(out_str) - idx, "%s", s[level]);
    if(idx >= (sizeof(out_str)))
        return;
    idx += vsnprintf(&out_str[idx], sizeof(out_str) - idx, str, ap);
    if(idx >= (sizeof(out_str)))
        return;
    idx += snprintf(&out_str[idx], sizeof(out_str) - idx, "\r\n");
    if(idx >= (sizeof(out_str)))
        return;

    while(*ptr != '\0') {
        usart_serial_putchar(DEBUG_UART, *ptr);
        ++ptr;
    }
#else
    (void)level, (void)str, (void)ap;
#endif
}
