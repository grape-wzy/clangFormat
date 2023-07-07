#include "misc/include/pin.h"

#include "ymdef.h"
#include "ymconfig.h"

#ifdef DRV_USING_PIN

static struct ym_pin_device _hw_pin;

/* Hardware PIN APIs */
void ym_pin_mode(int pin, pin_mode_e mode, pin_speed_e speed)
{
    YM_ASSERT(_hw_pin.ops != YM_NULL);
    _hw_pin.ops->pin_mode(&_hw_pin, pin, mode, speed);
}

void ym_pin_write(int pin, pin_level_e level)
{
    YM_ASSERT(_hw_pin.ops != YM_NULL);
    _hw_pin.ops->pin_write(&_hw_pin, pin, level);
}

int ym_pin_read(int pin)
{
    YM_ASSERT(_hw_pin.ops != YM_NULL);
    return _hw_pin.ops->pin_read(&_hw_pin, pin);
}

/* PIN IRQ APIs */
int ym_pin_attach_irq(int pin, pin_irq_mode_e irq_mode, void (*hdr)(void *args), void *args)
{
    YM_ASSERT(_hw_pin.ops != YM_NULL);
    if (_hw_pin.ops->pin_attach_irq) {
        return _hw_pin.ops->pin_attach_irq(&_hw_pin, pin, irq_mode, hdr, args);
    }
    return -1;
}

int ym_pin_detach_irq(int pin)
{
    YM_ASSERT(_hw_pin.ops != YM_NULL);
    if (_hw_pin.ops->pin_detach_irq) {
        return _hw_pin.ops->pin_detach_irq(&_hw_pin, pin);
    }
    return -1;
}

int ym_pin_irq_enable(int pin, pin_irq_enable_e enabled)
{
    YM_ASSERT(_hw_pin.ops != YM_NULL);
    if (_hw_pin.ops->pin_irq_enable) {
        return _hw_pin.ops->pin_irq_enable(&_hw_pin, pin, enabled);
    }
    return -1;
}

int ym_pin_register(const char *name, const struct ym_pin_ops *ops, void *user_data)
{
    YM_ASSERT(ops != YM_NULL);

    _hw_pin.ops       = ops;
    _hw_pin.user_data = user_data;

    return 0;
}

ym_weak int ym_hw_pin_init(void)
{
    /* NOTE : This function should be redefined in the drv_pin file */
    return -1;
}

#endif /* DRV_USING_PIN */
