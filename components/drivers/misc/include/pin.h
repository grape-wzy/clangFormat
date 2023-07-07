/*******************************************************************************
* file    drv_pin.h
* author  zhaoyu.wu
* version 1.0.0
* date
* brief   gpio driver for stm32
*******************************************************************************/

#ifndef __PIN_H__
#define __PIN_H__

#define PIN_NONE         (-1)

#define PIN_IRQ_PIN_NONE PIN_NONE

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PIN_LOW = 0x00,
    PIN_HIGH,
} pin_level_e;

typedef enum {
    PIN_MODE_OUTPUT = 0x00,
    PIN_MODE_INPUT,
    PIN_MODE_INPUT_PULLUP,
    PIN_MODE_INPUT_PULLDOWN,
    PIN_MODE_OUTPUT_OD,
    PIN_MODE_ANALOG,
} pin_mode_e;

typedef enum {
    PIN_SPEED_LOW = 0x00,
    PIN_SPEED_MEDIUM,
    PIN_SPEED_HIGH,
} pin_speed_e;

typedef enum {
    PIN_IRQ_MODE_RISING = 0x00,
    PIN_IRQ_MODE_FALLING,
    PIN_IRQ_MODE_RISING_FALLING,
    PIN_IRQ_MODE_HIGH_LEVEL,
    PIN_IRQ_MODE_LOW_LEVEL,
} pin_irq_mode_e;

typedef enum {
    PIN_IRQ_DISABLE = 0x00,
    PIN_IRQ_ENABLE,
} pin_irq_enable_e;

struct ym_pin_device {
    void *user_data;

    const struct ym_pin_ops *ops;
};

struct ym_pin_ops {
    void (*pin_mode)(struct ym_pin_device *device, int pin, pin_mode_e mode, pin_speed_e speed);
    void (*pin_write)(struct ym_pin_device *device, int pin, pin_level_e level);
    int (*pin_read)(struct ym_pin_device *device, int pin);
    int (*pin_attach_irq)(struct ym_pin_device *device, int pin, pin_irq_mode_e irq_mode,
                          void (*hdr)(void *args), void *args);
    int (*pin_detach_irq)(struct ym_pin_device *device, int pin);
    int (*pin_irq_enable)(struct ym_pin_device *device, int pin, pin_irq_enable_e enabled);
};

/**
 * @brief Initializes the driver component of the PIN.
 *        Called during system initialization.
 *        This function should be redefined in the drv_pin file.
 *
 * @return [0]initialization successfully, [other] initialization failure.
 */
int ym_hw_pin_init(void);

/**
 * @brief Register the pin operation function to the pin component
 *      which be implemented in the pin driver file.
 *
 * @param[in] name is the device driver's name.
 * @param[in] ops is the operating functions for the pin device.
 * @param[in] user_data is the user's private data.
 *
 * @return [0]register successfully, [other] initialization failure.
 */
int ym_pin_register(const char *name, const struct ym_pin_ops *ops, void *user_data);

void ym_pin_mode(int pin, pin_mode_e mode, pin_speed_e speed);
void ym_pin_write(int pin, pin_level_e level);
int  ym_pin_read(int pin);

int ym_pin_attach_irq(int pin, pin_irq_mode_e irq_mode, void (*hdr)(void *args), void *args);
int ym_pin_detach_irq(int pin);
int ym_pin_irq_enable(int pin, pin_irq_enable_e enabled);

#ifdef __cplusplus
}
#endif

#endif /* __PIN_H__*/
