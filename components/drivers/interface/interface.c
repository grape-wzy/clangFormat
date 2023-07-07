// TODO: 对接外设接口：I2C, SPI, UART, CAN, etc...
/*
 * 向上实现通用的open, close, init, read, write, config函数接口层，向下通过register函数实现具体接口的注册，
 * 向下对接xxx_interface，向上的函数接口讲最终根据interface对象调用到每个接口的具体驱动函数。
 * xxx_interface接口将再次向下对接真正的外设驱动层，并调用HAL库实现对硬件接口的读写能力。
 * 所有外设的接收功能尽量都以中断形式实现，在xxx_interface驱动接口中实现数据的接收和存储，并可通过用户配置的notify函数实现对用户层的数据到来的通知功能。
 */

#include "ymdef.h"
#include "interface/include/interface.h"

#include <string.h>

static struct ym_interface *_basic_interface = YM_NULL;

static ym_uint8_t ym_interface_get_unused_id(void)
{
    ym_uint8_t           id;
    struct ym_interface *inf_index;

    if (_basic_interface == YM_NULL)
        return 0;

    for (id = 0; id < 0xFF; id++) {
        for (inf_index = _basic_interface; inf_index != YM_NULL; inf_index = inf_index->next) {
            if (inf_index->id != id) return id;
        }
    }

    return id;
}

static ym_err_t ym_interface_init(ym_interface_t inf,
                                  const char    *name,
                                  ym_uint16_t    flags)
{
    struct ym_interface *inf_index;

    if (inf == YM_NULL)
        return -YM_ERROR;

    ym_uint8_t id = ym_interface_get_unused_id();
    if (id == 0xFF)
        return -YM_ERROR;

    inf->id          = id;
    inf->attr_flag   = flags;
    inf->ref_count   = 0;
    inf->open_flag   = 0;
    inf->next        = YM_NULL;
    inf->rx_indicate = YM_NULL;
    inf->tx_complete = YM_NULL;
    memcpy(inf->name, name, (sizeof(inf->name) / sizeof(inf->name[0])));

    if (_basic_interface == YM_NULL) {
        _basic_interface = inf;
    } else {
        inf_index = _basic_interface;
        while (inf_index != YM_NULL && inf_index->next != YM_NULL)
            inf_index = inf_index->next;

        inf_index->next = inf;
    }

    return YM_EOK;
}

/**
 * @brief This function finds a interface object by specified name.
 *
 * @param[in] name is the interface object's name.
 *
 * @return the registered interface object on successful, or YM_NULL on failure.
 */
ym_interface_t ym_interface_find_by_name(const char *name)
{
    struct ym_interface *inf = _basic_interface;

    if (name == YM_NULL)
        return YM_NULL;

    while (inf != YM_NULL) {
        if (0 == strncmp(name, inf->name, (sizeof(inf->name) / sizeof(inf->name[0])))) {
            return inf;
        }
        inf = inf->next;
    }

    return YM_NULL;
}

/**
 * @brief This function finds a interface object by specified interface id.
 *
 * @param[in] id is the interface object's id.
 *
 * @return the registered interface object on successful, or YM_NULL on failure.
 */
ym_interface_t ym_interface_find_by_id(ym_uint8_t id)
{
    struct ym_interface *inf = _basic_interface;

    while (inf != YM_NULL) {
        if (id == inf->id) {
            return inf;
        }
        inf = inf->next;
    }

    return YM_NULL;
}

/**
 * @brief This function registers a interface driver with a specified name.
 *
 * @param[in] inf is the pointer of interface driver structure.
 *
 * @param[in] name is the interface driver's name.
 *
 * @param[in] flags is the capabilities flag of interface.
 *
 * @return the error code, YM_EOK on initialization successfully.
 */
ym_err_t ym_interface_register(ym_interface_t inf,
                               const char    *name,
                               ym_uint16_t    flags)
{
    ym_uint8_t id;

    if (inf == YM_NULL)
        return -YM_ERROR;

    if (ym_interface_find_by_name(name) != YM_NULL)
        return -YM_ERROR;

    return ym_interface_init(inf, name, flags);
}
