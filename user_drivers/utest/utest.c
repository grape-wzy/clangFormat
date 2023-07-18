/*
 * @Author       : Zhaoyu.Wu
 * @Date         : 2023-07-18 09:46
 * @LastEditTime : 2023-07-18 10:41
 * @LastEditors  : Zhaoyu.Wu
 * @Description  : unit test
 * @FilePath     : d:/eMed/product/osteotomy_simple_1/user_drivers/utest/utest.c
 * If you have any questions, email to mr.wuzhaoyu@outlook.com.
 */

#include "log.h"
#include "rng_if.h"
#include "pd_proc.h"
#include "record_if.h"
#include "standard_lib.h"

#ifdef DEBUG

static void fault_test_by_unalign(void)
{
    volatile int *SCB_CCR = (volatile int *)0xE000ED14; // SCB->CCR
    volatile int *p;
    volatile int  value;

    *SCB_CCR |= (1 << 3); /* bit3: UNALIGN_TRP. */

    p     = (int *)0x00;
    value = *p;
    printf("addr:0x%02X value:0x%08X\r\n", (int)p, value);

    p     = (int *)0x04;
    value = *p;
    printf("addr:0x%02X value:0x%08X\r\n", (int)p, value);

    p     = (int *)0x03;
    value = *p;
    printf("addr:0x%02X value:0x%08X\r\n", (int)p, value);
}

static void fault_test_by_div0(void)
{
    volatile int *SCB_CCR = (volatile int *)0xE000ED14; // SCB->CCR
    int           x, y, z;

    *SCB_CCR |= (1 << 4);                               /* bit4: DIV_0_TRP. */

    x = 10;
    y = 0;
    z = x / y;
    printf("z:%d\n", z);
}

static void utest_proc(void)
{
    uint8_t code = log_get();
    switch (code) {
    case '1': {
        kprint("XXX\r\n");
        uint32_t x[115];
        uint8_t  ret = rng_get_numbers((uint32_t *)&x[0], 115);
        if (ret != STD_SUCCESS) {
            kprint("failed to get rand\r\n");
        } else {
            kprint("ok to get rand\r\n");
        }
        for (uint8_t i = 0; i < 5; i++) {
            nprint("x[%u]=0x%x, ", i, (unsigned int)x[i]);
        }
        nprint("\r\n");
    } break;
    case '2': {
        kprint("25666\r\n");
        fault_test_by_unalign();
        // skt_test();
    } break;
    case '3': {
        kprint("3\r\n");
    } break;
    case '4': {
        kprint("4\r\n");
        // ahrs_calib(0);
        // uint64_t* ap = (uint64_t*)FLASH_SPACE_SHADOW_END_ADDR;
        // uint64_t* bp = (uint64_t*)(FLASH_SPACE_SHADOW_END_ADDR + 8);
        // uint8_t ret = STD_SUCCESS;

        // uint64_t a = 0x1111222233334445;
        // for (uint32_t i = 0; i < (FLASH_PAGE_SIZE / 8); i++)
        //{
        //	a++;
        //	ret = flash_ll_write(FLASH_SPACE_SHADOW_END_ADDR + FLASH_PAGE_SIZE - 8 * (i + 1), (uint8_t*)&a, sizeof(a));
        //	if (ret != STD_SUCCESS)
        //	{
        //		kprint("write, ret=0x%x\r\n", ret);
        //		break;
        //	}
        // }

        // kprint("1,a=0x%lx, b=0x%lx\r\n", (long unsigned int) * ap, (long unsigned int) * bp);
        // uint64_t a = 0x1111222233334445;
        // ret = flash_ll_write(FLASH_SPACE_SHADOW_END_ADDR + 8, (uint8_t*)&a, sizeof(a));
        // kprint("write, ret=0x%x\r\n", ret);
        // uint64_t b = 0x5555666677778889;
        // ret = flash_ll_write(FLASH_SPACE_SHADOW_END_ADDR, (uint8_t*)&b, sizeof(b));
        // kprint("write, ret=0x%x\r\n", ret);
        // kprint("2, a=0x%lx, b=0x%lx\r\n", (long unsigned int) * ap, (long unsigned int) * bp);
    } break;
    case '5': {
        kprint("5\r\n");

        // uint64_t a = 0xffffffffffff4445;
        // flash_ll_write(FLASH_SPACE_SHADOW_END_ADDR, (uint8_t*)&a, sizeof(a));

        // uint64_t b = 0xffffffff12344445;
        // flash_ll_write(FLASH_SPACE_SHADOW_END_ADDR, (uint8_t*)&b, sizeof(b));

        // kprint("end\r\n");
        // imu_get_all_reg();
    } break;
    case '6': {
        recorder_stat();
    } break;
    case '7': {
        for (uint32_t i = 0; i < 1000; i++) {
            uint8_t ret = pd_save_time();
            if (ret != STD_SUCCESS) {
                kprint("faile, ret=0x%x,i=%d\r\n", ret, (unsigned int)i);
                break;
            }
            log_flush();
        }
        kprint("test end\r\n");
    } break;
    case '8': {
        recorder_gc();
    } break;
    }
}

static void utest_rx_evt(void)
{
    static uint8_t teskTask = 0;
    if (teskTask == 0) {
        teskTask = 1;
        UTIL_SEQ_RegTask(1 << CFG_TASK_TEST_PROC, UTIL_SEQ_RFU, utest_proc);
    }
    UTIL_SEQ_SetTask(1 << CFG_TASK_TEST_PROC, CFG_PRIO_NBR_3);
}
#endif

void utest_init(void)
{
#ifdef DEBUG
    log_register_rx_cb(utest_rx_evt);
#endif
}
