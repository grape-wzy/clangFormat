#include <stdint.h>
#include "ymdef.h"
#include "bsp_adapter.h"

#ifdef BSP_USING_SPI1
#ifndef SPI1_BUS_CONFIG
#define SPI1_BUS_CONFIG \
    {                   \
        .index = 0,     \
    }
#endif /* SPI1_BUS_CONFIG */
#endif /* BSP_USING_SPI1 */

#ifdef BSP_SPI1_TX_USING_DMA
#ifndef SPI1_TX_DMA_CONFIG
#define SPI1_TX_DMA_CONFIG                \
    {                                     \
        .dma_rcc  = SPI1_TX_DMA_RCC,      \
        .Instance = SPI1_TX_DMA_INSTANCE, \
        .dma_irq  = SPI1_TX_DMA_IRQ,      \
    }
#endif /* SPI1_TX_DMA_CONFIG */
#endif /* BSP_SPI1_TX_USING_DMA */

#ifdef BSP_SPI1_RX_USING_DMA
#ifndef SPI1_RX_DMA_CONFIG
#define SPI1_RX_DMA_CONFIG                \
    {                                     \
        .dma_rcc  = SPI1_RX_DMA_RCC,      \
        .Instance = SPI1_RX_DMA_INSTANCE, \
        .dma_irq  = SPI1_RX_DMA_IRQ,      \
    }
#endif /* SPI1_RX_DMA_CONFIG */
#endif /* BSP_SPI1_RX_USING_DMA */

#ifdef BSP_USING_SPI2
#ifndef SPI2_BUS_CONFIG
#define SPI2_BUS_CONFIG        \
    {                          \
        .Instance = SPI2,      \
        .bus_name = "spi2",    \
        .irq_type = SPI2_IRQn, \
    }
#endif /* SPI2_BUS_CONFIG */
#endif /* BSP_USING_SPI2 */

#ifdef BSP_SPI2_TX_USING_DMA
#ifndef SPI2_TX_DMA_CONFIG
#define SPI2_TX_DMA_CONFIG                \
    {                                     \
        .dma_rcc  = SPI2_TX_DMA_RCC,      \
        .Instance = SPI2_TX_DMA_INSTANCE, \
        .dma_irq  = SPI2_TX_DMA_IRQ,      \
    }
#endif /* SPI2_TX_DMA_CONFIG */
#endif /* BSP_SPI2_TX_USING_DMA */

#ifdef BSP_SPI2_RX_USING_DMA
#ifndef SPI2_RX_DMA_CONFIG
#define SPI2_RX_DMA_CONFIG                \
    {                                     \
        .dma_rcc  = SPI2_RX_DMA_RCC,      \
        .Instance = SPI2_RX_DMA_INSTANCE, \
        .dma_irq  = SPI2_RX_DMA_IRQ,      \
    }
#endif /* SPI2_RX_DMA_CONFIG */
#endif /* BSP_SPI2_RX_USING_DMA */

struct stm32_spi_config {
    ym_uint8_t index;
    // SPI_TypeDef       *Instance;
    // char              *bus_name;
    // IRQn_Type          irq_type;
    // struct dma_config *dma_rx, *dma_tx;
};

struct stm32_spi {
    SPI_HandleTypeDef handle;
    // struct stm32_spi_config     *config;
    // struct rt_spi_configuration *cfg;

    // struct
    // {
    //     DMA_HandleTypeDef handle_rx;
    //     DMA_HandleTypeDef handle_tx;
    // } dma;

    // rt_uint8_t        spi_dma_flag;
    // struct rt_spi_bus spi_bus;

    // struct rt_completion cpt;
};

enum {
#ifdef BSP_USING_SPI1
    SPI1_INDEX,
#endif
#ifdef BSP_USING_SPI2
    SPI2_INDEX,
#endif
#ifdef BSP_USING_SPI3
    SPI3_INDEX,
#endif
#ifdef BSP_USING_SPI4
    SPI4_INDEX,
#endif
#ifdef BSP_USING_SPI5
    SPI5_INDEX,
#endif
#ifdef BSP_USING_SPI6
    SPI6_INDEX,
#endif
};

static struct stm32_spi_config spi_config[] = {
#ifdef BSP_USING_SPI1
    SPI1_BUS_CONFIG,
#endif

#ifdef BSP_USING_SPI2
    SPI2_BUS_CONFIG,
#endif

#ifdef BSP_USING_SPI3
    SPI3_BUS_CONFIG,
#endif

#ifdef BSP_USING_SPI4
    SPI4_BUS_CONFIG,
#endif

#ifdef BSP_USING_SPI5
    SPI5_BUS_CONFIG,
#endif

#ifdef BSP_USING_SPI6
    SPI6_BUS_CONFIG,
#endif
};

ym_err_t stm_spi_init(void)
{
    MX_SPI1_Init();
}

ym_uint32_t stm_spi_xfer(struct ym_spi_message *message)
{
    HAL_SPI_TransmitReceive_DMA();
}
