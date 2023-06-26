/**
 * Copyright (c) 2016 - 2020, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include "fstorage_nvmc.h"
#include "platform.h"
#include "standard_lib.h"
#include "flash_driver.h"



static nrf_fstorage_info_t m_flash_info =
{
	.erase_unit = FLASH_PAGE_SIZE,
    .program_unit = 8,
    .rmap         = true,
    .wmap         = false,
};


 /* An operation initiated by fstorage is ongoing. */
static uint32_t m_flash_operation_ongoing;


/* Send event to the event handler. */
static void event_send(nrf_fstorage_t        const * p_fs,
                       nrf_fstorage_evt_id_t         evt_id,
                       void const *                  p_src,
                       uint32_t                      addr,
                       uint32_t                      len,
                       void                        * p_param)
{
    if (p_fs->evt_handler == NULL)
    {
        /* Nothing to do. */
        return;
    }

    nrf_fstorage_evt_t evt =
    {
        .result  = NRF_SUCCESS,
        .id      = evt_id,
        .addr    = addr,
        .p_src   = p_src,
        .len     = len,
        .p_param = p_param,
    };

    p_fs->evt_handler(&evt);
}


static ret_code_t init(nrf_fstorage_t * p_fs, void * p_param)
{
	UNUSED(p_param);

    p_fs->p_flash_info = &m_flash_info;

    return NRF_SUCCESS;
}


static ret_code_t uninit(nrf_fstorage_t * p_fs, void * p_param)
{
	UNUSED(p_fs);
	UNUSED(p_param);

	m_flash_operation_ongoing = 0;
    return NRF_SUCCESS;
}


static ret_code_t read_impl(nrf_fstorage_t const * p_fs, uint32_t src, void * p_dest, uint32_t len)
{
	UNUSED(p_fs);

    memcpy(p_dest, (uint32_t*)src, len);

    return NRF_SUCCESS;
}


static ret_code_t write_impl(nrf_fstorage_t const * p_fs,
                        uint32_t               dest,
                        void           const * p_src,
                        uint32_t               len,
                        void                 * p_param)
{
	
	ret_code_t ret = NRF_SUCCESS;
	if (m_flash_operation_ongoing == 1)
    {
        return NRF_ERROR_BUSY;
    }

	if (flash_ll_write(dest, p_src, len) != STD_SUCCESS)
	{
		ret = 	NRF_ERROR_INTERNAL;
	}
	
    /* Clear the flag before sending the event, to allow API calls in the event context. */
	 m_flash_operation_ongoing = 0;

    event_send(p_fs, NRF_FSTORAGE_EVT_WRITE_RESULT, p_src, dest, len, p_param);

    return ret;
}


static ret_code_t erase(nrf_fstorage_t const * p_fs,
                        uint32_t               page_addr,
                        uint32_t               len,
                        void                 * p_param)
{
    uint32_t progress = 0;
	ret_code_t ret = NRF_SUCCESS;
	
	if (m_flash_operation_ongoing == 1)
    {
        return NRF_ERROR_BUSY;
    }

    while (progress != len)
    {

	    uint32_t startAddr = page_addr + (progress * m_flash_info.erase_unit);
	    uint32_t endAddr = startAddr +  m_flash_info.erase_unit;
	    
	    if (flash_ll_erase(startAddr, endAddr) != STD_SUCCESS)
	    {
		    ret = 	NRF_ERROR_INTERNAL;
		    break;
		    
	    }

        progress++;
    }

    /* Clear the flag before sending the event, to allow API calls in the event context. */
	m_flash_operation_ongoing = 0;

    event_send(p_fs, NRF_FSTORAGE_EVT_ERASE_RESULT, NULL, page_addr, len, p_param);

    return ret;
}


static uint8_t const * rmap(nrf_fstorage_t const * p_fs, uint32_t addr)
{
	UNUSED(p_fs);

    return (uint8_t*)addr;
}


static uint8_t * wmap(nrf_fstorage_t const * p_fs, uint32_t addr)
{
	UNUSED(p_fs);
	UNUSED(addr);

    /* Not supported. */
    return NULL;
}


static bool is_busy(nrf_fstorage_t const * p_fs)
{
	UNUSED(p_fs);

    return m_flash_operation_ongoing;
}


/* The exported API. */
nrf_fstorage_api_t nrf_fstorage_nvmc =
{
    .init    = init,
    .uninit  = uninit,
    .read    = read_impl,
    .write   = write_impl,
    .erase   = erase,
    .rmap    = rmap,
    .wmap    = wmap,
    .is_busy = is_busy
};

