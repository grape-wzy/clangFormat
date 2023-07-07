#ifndef __SPI_INTERFACE_H__
#define __SPI_INTERFACE_H__

#include "ymdef.h"
#include "interface/include/interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * At CPOL=0 the base value of the clock is zero
 *  - For CPHA=0, data are captured on the clock's rising edge (low->high transition)
 *    and data are propagated on a falling edge (high->low clock transition).
 *  - For CPHA=1, data are captured on the clock's falling edge and data are
 *    propagated on a rising edge.
 * At CPOL=1 the base value of the clock is one (inversion of CPOL=0)
 *  - For CPHA=0, data are captured on clock's falling edge and data are propagated
 *    on a rising edge.
 *  - For CPHA=1, data are captured on clock's rising edge and data are propagated
 *    on a falling edge.
 */
#define YM_SPI_CPHA          (1 << 0) /* bit[0]:CPHA, clock phase */
#define YM_SPI_CPOL          (1 << 1) /* bit[1]:CPOL, clock polarity */

#define YM_SPI_LSB           (0 << 2) /* bit[2]: 0-LSB */
#define YM_SPI_MSB           (1 << 2) /* bit[2]: 1-MSB */

#define YM_SPI_MASTER        (0 << 3) /* SPI master device */
#define YM_SPI_SLAVE         (1 << 3) /* SPI slave device */

#define YM_SPI_CS_HIGH       (1 << 4) /* Chipselect active high */
#define YM_SPI_NO_CS         (1 << 5) /* No chipselect */
#define YM_SPI_3WIRE         (1 << 6) /* SI/SO pin shared */
#define YM_SPI_READY         (1 << 7) /* Slave pulls low to pause */

#define YM_SPI_MODE_MASK     (YM_SPI_CPHA | YM_SPI_CPOL | YM_SPI_MSB | YM_SPI_SLAVE | YM_SPI_CS_HIGH | YM_SPI_NO_CS | YM_SPI_3WIRE | YM_SPI_READY)

#define YM_SPI_MODE_0        (0 | 0)                     /* CPOL = 0, CPHA = 0 */
#define YM_SPI_MODE_1        (0 | YM_SPI_CPHA)           /* CPOL = 0, CPHA = 1 */
#define YM_SPI_MODE_2        (YM_SPI_CPOL | 0)           /* CPOL = 1, CPHA = 0 */
#define YM_SPI_MODE_3        (YM_SPI_CPOL | YM_SPI_CPHA) /* CPOL = 1, CPHA = 1 */

#define YM_SPI_BUS_MODE_SPI  (1 << 0)
#define YM_SPI_BUS_MODE_QSPI (1 << 1)

/**
 * SPI message structure
 */
struct ym_spi_message {
    const void            *send_buf;
    void                  *recv_buf;
    struct ym_spi_message *next;

    ym_uint32_t cs_take    :1;
    ym_uint32_t cs_release :1;
    ym_uint32_t length     :30;
};

/**
 * SPI configuration structure
 */
struct ym_spi_configuration {
    ym_uint8_t  mode;
    ym_uint8_t  data_width;
    ym_uint16_t reserved;

    ym_uint32_t max_hz;
};

struct ym_spi_interface;

struct ym_spi_bus_ops {
    ym_err_t (*configure)(struct ym_spi_interface *spi_inf, struct ym_spi_configuration *cfg);
    ym_uint32_t (*xfer)(struct ym_spi_interface *spi_inf, struct ym_spi_message *message);
};

struct ym_spi_inf_bus {
    struct ym_interface parent;

    struct ym_spi_configuration  config;
    const struct ym_spi_bus_ops *ops;

    struct ym_spi_interface *owner;
};

/**
 * SPI object structure
 */
struct ym_spi_interface {
    struct ym_interface         parent;
    struct ym_spi_inf_bus      *bus;
    struct ym_spi_configuration config;

    int   cs_pin;
    void *res;
};

/* register a SPI bus. Called by the driver file of spi. */
ym_err_t ym_spi_interface_bus_register(struct ym_spi_inf_bus       *spi_bus,
                                       const char                  *name,
                                       const struct ym_spi_bus_ops *ops);

ym_err_t ym_spi_interface_send_then_send(struct ym_spi_interface *spi_inf,
                                         const void              *send_buf1,
                                         ym_size_t                send_length1,
                                         const void              *send_buf2,
                                         ym_size_t                send_length2);

ym_err_t ym_spi_interface_send_then_recv(struct ym_spi_interface *spi_inf,
                                         const void              *send_buf,
                                         ym_size_t                send_length,
                                         void                    *recv_buf,
                                         ym_size_t                recv_length);

ym_err_t ym_spi_interface_configure(struct ym_spi_interface     *spi_inf,
                                    struct ym_spi_configuration *cfg);

struct ym_spi_message *ym_spi_interface_transfer_message(struct ym_spi_interface *spi_inf,
                                                         struct ym_spi_message   *message);

ym_err_t ym_spi_interface_register(struct ym_spi_interface *spi_inf,
                                   const char              *name,
                                   struct ym_spi_inf_bus   *spi_bus,
                                   int                      pin,
                                   ym_uint16_t              flags);

#ifdef __cplusplus
}
#endif

#endif /* __SPI_INTERFACE_H__ */
