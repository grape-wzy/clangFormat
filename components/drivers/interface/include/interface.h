#ifndef __INTERFACE__H__
#define __INTERFACE__H__

#include "ymconfig.h"
#include "ymdef.h"

/**
 * interface flags definitions
 */
#define YM_INTERFACE_FLAG_DEACTIVATE 0x000 /**< interface is not not initialized */

#define YM_INTERFACE_FLAG_RDONLY     0x001 /**< read only */
#define YM_INTERFACE_FLAG_WRONLY     0x002 /**< write only */
#define YM_INTERFACE_FLAG_RDWR       0x003 /**< read and write */

#define YM_INTERFACE_FLAG_REMOVABLE  0x004 /**< removable interface */
#define YM_INTERFACE_FLAG_STANDALONE 0x008 /**< standalone interface */
#define YM_INTERFACE_FLAG_ACTIVATED  0x010 /**< interface is activated */
#define YM_INTERFACE_FLAG_SUSPENDED  0x020 /**< interface is suspended */
#define YM_INTERFACE_FLAG_STREAM     0x040 /**< stream mode */

#define YM_INTERFACE_FLAG_INT_RX     0x100 /**< INT mode on Rx */
#define YM_INTERFACE_FLAG_DMA_RX     0x200 /**< DMA mode on Rx */
#define YM_INTERFACE_FLAG_INT_TX     0x400 /**< INT mode on Tx */
#define YM_INTERFACE_FLAG_DMA_TX     0x800 /**< DMA mode on Tx */

#define YM_INTERFACE_OFLAG_CLOSE     0x000 /**< interface is closed */
#define YM_INTERFACE_OFLAG_RDONLY    0x001 /**< read only access */
#define YM_INTERFACE_OFLAG_WRONLY    0x002 /**< write only access */
#define YM_INTERFACE_OFLAG_RDWR      0x003 /**< read and write */
#define YM_INTERFACE_OFLAG_OPEN      0x008 /**< interface is opened */
#define YM_INTERFACE_OFLAG_MASK      0xf0f /**< mask of open flag */

struct ym_interface;
/**
 * operations set for interface object
 */
struct ym_interface_ops {
    /* common ops interface */
    ym_err_t (*init)(struct ym_interface *inf);
    ym_err_t (*open)(struct ym_interface *inf, ym_uint16_t oflag);
    ym_err_t (*close)(struct ym_interface *inf);
    ym_size_t (*read)(struct ym_interface *inf, ym_off_t pos, void *buffer, ym_size_t size);
    ym_size_t (*write)(struct ym_interface *inf, ym_off_t pos, const void *buffer, ym_size_t size);
    ym_err_t (*control)(struct ym_interface *inf, ym_uint32_t cmd, void *args);
};

struct ym_interface {
    char name[YM_NAME_LEN];

    ym_uint8_t  id;               /**< 0 - 255 */
    ym_uint8_t  ref_count;        /**< reference count */
    ym_uint16_t attr_flag;        /**< interface attribute flag */
    ym_uint16_t open_flag;        /**< interface open flag */

    struct ym_interface_ops *ops; /**< interface operations */

    /* interface call back */
    ym_err_t (*rx_indicate)(struct ym_interface *inf, ym_size_t size);
    ym_err_t (*tx_complete)(struct ym_interface *inf, void *buffer);

    struct ym_interface *next;

    void *user_data; /**< private user data */
};
typedef struct ym_interface *ym_interface_t;

ym_interface_t ym_interface_find_by_name(const char *name);
ym_interface_t ym_interface_find_by_id(ym_uint8_t id);

ym_err_t ym_interface_register(ym_interface_t inf,
                               const char    *name,
                               ym_uint16_t    flags);

#endif /* __INTERFACE__H__ */
