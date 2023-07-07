

#include "bsp_adapter.h"
#include "misc/include/pin.h"

#define PIN_NUM(port, no) (((((port)&0xFu) << 4) | ((no)&0xFu)))
#define PIN_PORT(pin)     ((uint8_t)(((pin) >> 4) & 0xFu))
#define PIN_NO(pin)       ((uint8_t)((pin)&0xFu))

#if defined(SOC_SERIES_STM32MP1)
#if defined(GPIOZ)
#define gpioz_port_base (175) /* PIN_STPORT_MAX * 16 - 16 */
#define PIN_STPORT(pin) ((pin > gpioz_port_base) ? ((GPIO_TypeDef *)(GPIOZ_BASE)) : ((GPIO_TypeDef *)(GPIOA_BASE + (0x1000u * PIN_PORT(pin)))))
#else
#define PIN_STPORT(pin) ((GPIO_TypeDef *)(GPIOA_BASE + (0x1000u * PIN_PORT(pin))))
#endif /* GPIOZ */
#else
#define PIN_STPORT(pin) ((GPIO_TypeDef *)(GPIOA_BASE + (0x400u * PIN_PORT(pin))))
#endif /* SOC_SERIES_STM32MP1 */

#define PIN_STPIN(pin) ((uint16_t)(1u << PIN_NO(pin)))

#if defined(GPIOZ)
#define __STM32_PORT_MAX 12u
#elif defined(GPIOK)
#define __STM32_PORT_MAX 11u
#elif defined(GPIOJ)
#define __STM32_PORT_MAX 10u
#elif defined(GPIOI)
#define __STM32_PORT_MAX 9u
#elif defined(GPIOH)
#define __STM32_PORT_MAX 8u
#elif defined(GPIOG)
#define __STM32_PORT_MAX 7u
#elif defined(GPIOF)
#define __STM32_PORT_MAX 6u
#elif defined(GPIOE)
#define __STM32_PORT_MAX 5u
#elif defined(GPIOD)
#define __STM32_PORT_MAX 4u
#elif defined(GPIOC)
#define __STM32_PORT_MAX 3u
#elif defined(GPIOB)
#define __STM32_PORT_MAX 2u
#elif defined(GPIOA)
#define __STM32_PORT_MAX 1u
#else
#define __STM32_PORT_MAX 0u
#error Unsupported STM32 GPIO peripheral.
#endif

#define PIN_STPORT_MAX __STM32_PORT_MAX

struct pin_irq_map {
    unsigned short pin_bit;
    IRQn_Type      irq_no;
};

static void stm32_pin_mode(struct ym_pin_device *device, int pin, pin_mode_e mode, pin_speed_e speed)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    if (PIN_PORT(pin) >= PIN_STPORT_MAX) {
        return;
    }

    /* Configure GPIO_InitStructure */
    GPIO_InitStruct.Pin   = PIN_STPIN(pin);
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = (speed == PIN_SPEED_MEDIUM) ? GPIO_SPEED_FREQ_MEDIUM
                            : (speed == PIN_SPEED_HIGH) ? GPIO_SPEED_FREQ_VERY_HIGH
                                                        : GPIO_SPEED_FREQ_LOW;

    if (mode == PIN_MODE_OUTPUT) {
        /* output setting */
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
    } else if (mode == PIN_MODE_INPUT) {
        /* input setting: not pull. */
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
    } else if (mode == PIN_MODE_INPUT_PULLUP) {
        /* input setting: pull up. */
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
    } else if (mode == PIN_MODE_INPUT_PULLDOWN) {
        /* input setting: pull down. */
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    } else if (mode == PIN_MODE_OUTPUT_OD) {
        /* output setting: od. */
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
    } else if (mode == PIN_MODE_ANALOG) {
        /* input setting: analog. */
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
    }

    HAL_GPIO_Init(PIN_STPORT(pin), &GPIO_InitStruct);
}

static void stm32_pin_write(struct ym_pin_device *device, int pin, pin_level_e level)
{
    GPIO_TypeDef *gpio_port;
    uint16_t      gpio_pin;

    if (PIN_PORT(pin) < PIN_STPORT_MAX) {
        gpio_port = PIN_STPORT(pin);
        gpio_pin  = PIN_STPIN(pin);

        HAL_GPIO_WritePin(gpio_port, gpio_pin, (GPIO_PinState)level);
    }
}

static int stm32_pin_read(struct ym_pin_device *device, int pin)
{
    GPIO_TypeDef *gpio_port;
    uint16_t      gpio_pin;
    int           value = PIN_LOW;

    if (PIN_PORT(pin) < PIN_STPORT_MAX) {
        gpio_port = PIN_STPORT(pin);
        gpio_pin  = PIN_STPIN(pin);
        value     = HAL_GPIO_ReadPin(gpio_port, gpio_pin);
    }

    return value;
}

static int stm32_pin_attach_irq(struct ym_pin_device *device, int pin, pin_irq_mode_e irq_mode,
                                void (*hdr)(void *args), void *args)
{
    return -2;
}

static int stm32_pin_dettach_irq(struct ym_pin_device *device, int pin)
{
    return -2;
}

static int stm32_pin_irq_enable(struct ym_pin_device *device, int pin, pin_irq_enable_e enabled)
{
    return -2;
}

static const struct ym_pin_ops _stm32_pin_ops = {
    stm32_pin_mode,
    stm32_pin_write,
    stm32_pin_read,
    stm32_pin_attach_irq,
    stm32_pin_dettach_irq,
    stm32_pin_irq_enable,
};

int ym_hw_pin_init(void)
{
#if defined(__HAL_RCC_GPIOA_CLK_ENABLE)
    __HAL_RCC_GPIOA_CLK_ENABLE();
#endif

#if defined(__HAL_RCC_GPIOB_CLK_ENABLE)
    __HAL_RCC_GPIOB_CLK_ENABLE();
#endif

#if defined(__HAL_RCC_GPIOC_CLK_ENABLE)
    __HAL_RCC_GPIOC_CLK_ENABLE();
#endif

#if defined(__HAL_RCC_GPIOD_CLK_ENABLE)
    __HAL_RCC_GPIOD_CLK_ENABLE();
#endif

#if defined(__HAL_RCC_GPIOE_CLK_ENABLE)
    __HAL_RCC_GPIOE_CLK_ENABLE();
#endif

#if defined(__HAL_RCC_GPIOF_CLK_ENABLE)
    __HAL_RCC_GPIOF_CLK_ENABLE();
#endif

#if defined(__HAL_RCC_GPIOG_CLK_ENABLE)
#ifdef SOC_SERIES_STM32L4
    HAL_PWREx_EnableVddIO2();
#endif
    __HAL_RCC_GPIOG_CLK_ENABLE();
#endif

#if defined(__HAL_RCC_GPIOH_CLK_ENABLE)
    __HAL_RCC_GPIOH_CLK_ENABLE();
#endif

#if defined(__HAL_RCC_GPIOI_CLK_ENABLE)
    __HAL_RCC_GPIOI_CLK_ENABLE();
#endif

#if defined(__HAL_RCC_GPIOJ_CLK_ENABLE)
    __HAL_RCC_GPIOJ_CLK_ENABLE();
#endif

#if defined(__HAL_RCC_GPIOK_CLK_ENABLE)
    __HAL_RCC_GPIOK_CLK_ENABLE();
#endif

    return ym_pin_register("pin", &_stm32_pin_ops, 0);
}
