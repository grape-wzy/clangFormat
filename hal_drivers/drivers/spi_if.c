/*******************************************************************************
* file     force_hw.c
* author   mackgim
* version  V1.0.0
* date
* brief ��
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "spi_if.h"
#include "gpio_if.h"
#include "platform.h"
#include "standard_lib.h"

#include "kservice.h"
#include "drv_pin.h"
#include "driver_cfg.h"

#define ENABLE_SPI1_DMA 1

typedef enum {
    TRANSFER_WAIT,
    TRANSFER_COMPLETE,
    TRANSFER_ERROR
} spi_transfer_state_t;

struct stm32_spi_bus {
    struct ym_spi_inf_bus     spi_inf_bus;
    SPI_HandleTypeDef         spi_handle;
    __IO spi_transfer_state_t transfer_state;
    uint8_t                   state;
};

static void spi1_hw_cplt_cb(SPI_HandleTypeDef *hspi)
{
    struct stm32_spi_bus *_bus = ym_container_of(hspi, struct stm32_spi_bus, spi_handle);
    _bus->state                = TRANSFER_COMPLETE;
}

static void spi1_hw_error_cb(SPI_HandleTypeDef *hspi)
{
    struct stm32_spi_bus *_bus = ym_container_of(hspi, struct stm32_spi_bus, spi_handle);
    _bus->state                = TRANSFER_ERROR;
}

static ym_err_t spi1_hw_configure(struct ym_spi_interface *spi_inf, struct ym_spi_configuration *cfg)
{
    return YM_EOK;
}

static ym_uint32_t spi1_hw_xfer(struct ym_spi_interface *spi_inf, struct ym_spi_message *message)
{
    HAL_StatusTypeDef state;
    uint64_t          time_end;
    uint32_t          message_length, already_pass_length;
    uint16_t          this_length;
    uint8_t          *recv_buf;
    const uint8_t    *send_buf;

    struct stm32_spi_bus *_bus = (struct stm32_spi_bus *)spi_inf->bus;

    time_end = Clock_Time() + 20000;
    state    = HAL_ERROR;

    if (message->cs_take && !(spi_inf->config.mode & YM_SPI_NO_CS) && (spi_inf->cs_pin != PIN_NONE)) {
        if (spi_inf->config.mode & YM_SPI_CS_HIGH)
            ym_pin_write(spi_inf->cs_pin, PIN_HIGH);
        else
            ym_pin_write(spi_inf->cs_pin, PIN_LOW);
    }

    message_length = message->length;
    recv_buf       = message->recv_buf;
    send_buf       = message->send_buf;

    while (message_length) {
        /* the HAL library use uint16 to save the data length */
        if (message_length > 0xFFFF) {
            this_length = 0xFFFF;
            message_length -= 0xFFFF;
        } else {
            this_length    = message_length;
            message_length = 0;
        }

        /* calculate the start address */
        already_pass_length = message->length - this_length - message_length;
        send_buf            = (uint8_t *)message->send_buf + already_pass_length;
        recv_buf            = (uint8_t *)message->recv_buf + already_pass_length;

        _bus->state = TRANSFER_WAIT;

        /* start once data exchange in DMA mode */
        if (message->send_buf && message->recv_buf) {
            state = HAL_SPI_TransmitReceive_DMA(&_bus->spi_handle, (uint8_t *)send_buf, (uint8_t *)recv_buf, this_length);
        } else if (message->send_buf) {
            state = HAL_SPI_Transmit_DMA(&_bus->spi_handle, (uint8_t *)send_buf, this_length);

            if (message->cs_release && (spi_inf->config.mode & YM_SPI_3WIRE)) {
                /* release the CS by disable SPI when using 3 wires SPI */
                __HAL_SPI_DISABLE(&_bus->spi_handle);
            }

        } else {
            memset((uint8_t *)recv_buf, 0xff, this_length);
            state = HAL_SPI_Receive_DMA(&_bus->spi_handle, (uint8_t *)recv_buf, this_length);
        }

        if (state != HAL_OK) {
            kprint("spi1 transfer error : %d\r\n", state);
            message->length = 0;
            message_length  = 0;
            _bus->state     = TRANSFER_ERROR;

            _bus->spi_handle.State = HAL_SPI_STATE_READY;
        } else {
            while (_bus->state != TRANSFER_COMPLETE) {
                // if (Clock_Time() > time_end) {
                //     kprint("spi1 transfer timeout\r\n");
                //     message->length = 0;
                //     message_length  = 0;
                //     break;
                // }
            }
        }
    }

    if (_bus->state == TRANSFER_ERROR) {
        kprint("spi1 transfer error\r\n");
    }

    if (message->cs_release && !(spi_inf->config.mode & YM_SPI_NO_CS) && (spi_inf->cs_pin != PIN_NONE)) {
        if (spi_inf->config.mode & YM_SPI_CS_HIGH)
            ym_pin_write(spi_inf->cs_pin, PIN_LOW);
        else
            ym_pin_write(spi_inf->cs_pin, PIN_HIGH);
    }

    return message->length;
}

const struct ym_spi_bus_ops stm32_spi_ops = {
    .xfer      = spi1_hw_xfer,
    .configure = spi1_hw_configure,
};

static struct stm32_spi_bus stm32_spi1;

uint8_t spi1_hw_init(void)
{
    if (stm32_spi1.state) {
        return STD_SUCCESS;
    }
    stm32_spi1.state = true;

    //HAL_SPI_DeInit(&aSpiHandle);
    stm32_spi1.spi_handle.Instance               = SPI1;
    stm32_spi1.spi_handle.Init.Mode              = SPI_MODE_MASTER;
    stm32_spi1.spi_handle.Init.Direction         = SPI_DIRECTION_2LINES;
    stm32_spi1.spi_handle.Init.DataSize          = SPI_DATASIZE_8BIT;
    stm32_spi1.spi_handle.Init.CLKPolarity       = SPI_POLARITY_LOW;
    stm32_spi1.spi_handle.Init.CLKPhase          = SPI_PHASE_1EDGE;
    stm32_spi1.spi_handle.Init.NSS               = SPI_NSS_SOFT;
    stm32_spi1.spi_handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
    stm32_spi1.spi_handle.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    stm32_spi1.spi_handle.Init.TIMode            = SPI_TIMODE_DISABLE;
    stm32_spi1.spi_handle.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    stm32_spi1.spi_handle.Init.CRCPolynomial     = 7;
    stm32_spi1.spi_handle.Init.CRCLength         = SPI_CRC_LENGTH_DATASIZE;
    stm32_spi1.spi_handle.Init.NSSPMode          = SPI_NSS_PULSE_DISABLE;

    /* Init the SPI peripheral, and the function will call the HAL_SPI_MspInit (defined in stm32wbxx_hal_msp.c) */
    HAL_SPI_Init(&stm32_spi1.spi_handle);

    HAL_SPI_RegisterCallback(&stm32_spi1.spi_handle, HAL_SPI_TX_RX_COMPLETE_CB_ID, spi1_hw_cplt_cb);
    HAL_SPI_RegisterCallback(&stm32_spi1.spi_handle, HAL_SPI_TX_COMPLETE_CB_ID, spi1_hw_cplt_cb);
    HAL_SPI_RegisterCallback(&stm32_spi1.spi_handle, HAL_SPI_RX_COMPLETE_CB_ID, spi1_hw_cplt_cb);
    HAL_SPI_RegisterCallback(&stm32_spi1.spi_handle, HAL_SPI_ERROR_CB_ID, spi1_hw_error_cb);

    ym_spi_interface_bus_register(&stm32_spi1.spi_inf_bus, "spi1", &stm32_spi_ops);
    stm32_spi1.spi_inf_bus.parent.open_flag = (YM_INTERFACE_OFLAG_RDWR | YM_INTERFACE_OFLAG_OPEN);

    kprint("ok\r\n");
    return STD_SUCCESS;
}

uint8_t spi1_hw_deinit(void)
{
    stm32_spi1.spi_handle.Instance = SPI1;

    HAL_SPI_DeInit(&stm32_spi1.spi_handle);

    // ym_pin_mode(stm32_spi1.pin, PIN_MODE_ANALOG, PIN_SPEED_LOW);

    stm32_spi1.state = false;

    return STD_SUCCESS;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
