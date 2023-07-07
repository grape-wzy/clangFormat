#include "ymdef.h"
#include "misc/include/pin.h"
#include "interface/include/spi_interface.h"

#include <string.h>

ym_err_t ym_spi_interface_bus_register(struct ym_spi_inf_bus       *spi_bus,
                                       const char                  *name,
                                       const struct ym_spi_bus_ops *ops)
{
    spi_bus->ops   = ops;
    spi_bus->owner = YM_NULL;

    spi_bus->parent.ops = YM_NULL;

    return ym_interface_register(&spi_bus->parent, name, YM_INTERFACE_FLAG_RDWR);
}

ym_err_t ym_spi_interface_send_then_send(struct ym_spi_interface *spi_inf,
                                         const void              *send_buf1,
                                         ym_size_t                send_length1,
                                         const void              *send_buf2,
                                         ym_size_t                send_length2)
{
    ym_uint32_t           length = YM_EOK;
    struct ym_spi_message message;

    YM_ASSERT(spi_inf != YM_NULL);
    YM_ASSERT(spi_inf->bus != YM_NULL);
    YM_ASSERT(spi_inf->bus->ops != YM_NULL);

    /* not the same owner as current, re-configure SPI bus */
    if ((spi_inf->bus->owner != spi_inf) &&
        (spi_inf->bus->ops->configure != YM_NULL) &&
        (0 != memcmp(&spi_inf->config, &spi_inf->bus->config, sizeof(struct ym_spi_configuration)))) {
        if (YM_EOK != spi_inf->bus->ops->configure(spi_inf, &spi_inf->config)) {
            return -YM_EFAILED;
        }

        /* set SPI bus owner */
        spi_inf->bus->owner = spi_inf;
    }

    /* send data1 */
    message.send_buf   = send_buf1;
    message.recv_buf   = YM_NULL;
    message.length     = send_length1;
    message.cs_take    = 1;
    message.cs_release = 0;
    message.next       = YM_NULL;

    length = spi_inf->bus->ops->xfer(spi_inf, &message);
    if (length == 0) {
        return -YM_EFAILED;
    }

    /* send data2 */
    message.send_buf   = send_buf2;
    message.recv_buf   = YM_NULL;
    message.length     = send_length2;
    message.cs_take    = 0;
    message.cs_release = 1;
    message.next       = YM_NULL;

    length = spi_inf->bus->ops->xfer(spi_inf, &message);
    if (length == 0) {
        return -YM_EFAILED;
    }

    return YM_EOK;
}

ym_err_t ym_spi_interface_send_then_recv(struct ym_spi_interface *spi_inf,
                                         const void              *send_buf,
                                         ym_size_t                send_length,
                                         void                    *recv_buf,
                                         ym_size_t                recv_length)
{
    ym_uint32_t           length = YM_EOK;
    struct ym_spi_message message;

    YM_ASSERT(spi_inf != YM_NULL);
    YM_ASSERT(spi_inf->bus != YM_NULL);
    YM_ASSERT(spi_inf->bus->ops != YM_NULL);

    /* not the same owner as current, re-configure SPI bus */
    if ((spi_inf->bus->owner != spi_inf) &&
        (spi_inf->bus->ops->configure != YM_NULL) &&
        (0 != memcmp(&spi_inf->config, &spi_inf->bus->config, sizeof(struct ym_spi_configuration)))) {
        if (YM_EOK != spi_inf->bus->ops->configure(spi_inf, &spi_inf->config)) {
            return -YM_EFAILED;
        }

        /* set SPI bus owner */
        spi_inf->bus->owner = spi_inf;
    }

    /* send data1 */
    message.send_buf   = send_buf;
    message.recv_buf   = YM_NULL;
    message.length     = send_length;
    message.cs_take    = 1;
    message.cs_release = 0;
    message.next       = YM_NULL;

    spi_inf->bus->ops->xfer(spi_inf, &message);

    /* send data2 */
    message.send_buf   = YM_NULL;
    message.recv_buf   = recv_buf;
    message.length     = recv_length;
    message.cs_take    = 0;
    message.cs_release = 1;
    message.next       = YM_NULL;

    length = spi_inf->bus->ops->xfer(spi_inf, &message);
    if (length == 0) {
        return -YM_EFAILED;
    }

    return YM_EOK;
}

ym_err_t ym_spi_interface_configure(struct ym_spi_interface     *spi_inf,
                                    struct ym_spi_configuration *cfg)
{
    ym_err_t result;

    YM_ASSERT(spi_inf != YM_NULL);
    YM_ASSERT(cfg != YM_NULL);

    /* set configuration */
    spi_inf->config.data_width = cfg->data_width;
    spi_inf->config.mode       = cfg->mode & YM_SPI_MODE_MASK;
    spi_inf->config.max_hz     = cfg->max_hz;

    if ((spi_inf->bus != YM_NULL) &&
        (spi_inf->bus->owner == spi_inf) &&
        (spi_inf->bus->ops->configure != YM_NULL) &&
        (0 != memcmp(&spi_inf->config, &spi_inf->bus->config, sizeof(struct ym_spi_configuration)))) {
        if (YM_EOK == spi_inf->bus->ops->configure(spi_inf, &spi_inf->config)) {
            memcpy(&spi_inf->bus->config, &spi_inf->config, sizeof(struct ym_spi_configuration));
        }
    }

    return YM_EOK;
}

struct ym_spi_message *ym_spi_interface_transfer_message(struct ym_spi_interface *spi_inf,
                                                         struct ym_spi_message   *message)
{
    struct ym_spi_message *msg_index;

    YM_ASSERT(spi_inf != YM_NULL);
    YM_ASSERT(spi_inf->bus != YM_NULL);
    YM_ASSERT(spi_inf->bus->ops != YM_NULL);

    /* get first message */
    msg_index = message;
    if (msg_index == YM_NULL)
        return msg_index;

    /* not the same owner as current, re-configure SPI bus */
    if ((spi_inf->bus->owner != spi_inf) &&
        (spi_inf->bus->ops->configure != YM_NULL) &&
        (0 != memcmp(&spi_inf->config, &spi_inf->bus->config, sizeof(struct ym_spi_configuration)))) {
        if (YM_EOK != spi_inf->bus->ops->configure(spi_inf, &spi_inf->config)) {
            return msg_index;
        }

        /* set SPI bus owner */
        spi_inf->bus->owner = spi_inf;
    }

    /* transmit each SPI message */
    while (msg_index != YM_NULL) {
        /* transmit SPI message */
        if (0 == spi_inf->bus->ops->xfer(spi_inf, msg_index)) {
            return msg_index;
        }

        msg_index = msg_index->next;
    }

    return msg_index;
}

/* SPI interface to interface ops */

static ym_size_t ym_spi_interface_read(struct ym_interface *inf, ym_off_t pos, void *buffer, ym_size_t size)
{
    struct ym_spi_interface *spi_inf = (struct ym_spi_interface *)inf;
    return 0;
}

static ym_size_t ym_spi_interface_write(struct ym_interface *inf, ym_off_t pos, const void *buffer, ym_size_t size)
{
    struct ym_spi_interface *spi_inf = (struct ym_spi_interface *)inf;
    return 0;
}

//TODO: 完成接口函数
static struct ym_interface_ops spi_interface_ops = {
    .init    = YM_NULL,
    .open    = YM_NULL,
    .close   = YM_NULL,
    .read    = ym_spi_interface_read,
    .write   = ym_spi_interface_write,
    .control = YM_NULL,
};

ym_err_t ym_spi_interface_register(struct ym_spi_interface *spi_inf,
                                   const char              *name,
                                   struct ym_spi_inf_bus   *spi_bus,
                                   int                      pin,
                                   ym_uint16_t              flags)
{
    if ((spi_bus == NULL) ||
        (name == NULL) ||
        (spi_bus->ops == YM_NULL) ||
        (((spi_bus->parent.open_flag & YM_INTERFACE_OFLAG_MASK) | YM_INTERFACE_OFLAG_CLOSE) == YM_INTERFACE_OFLAG_CLOSE)) {
        return -YM_ERROR;
    }

    spi_bus->owner = spi_inf;

    spi_inf->bus        = spi_bus;
    spi_inf->cs_pin     = pin;
    spi_inf->parent.ops = &spi_interface_ops;

    memset(&spi_inf->config, 0, sizeof(spi_inf->config));

    ym_pin_write(spi_inf->cs_pin, PIN_HIGH);
    ym_pin_mode(spi_inf->cs_pin, PIN_MODE_OUTPUT, PIN_SPEED_LOW);

    return ym_interface_register(&spi_inf->parent, name, (flags | YM_INTERFACE_FLAG_RDWR));
}
