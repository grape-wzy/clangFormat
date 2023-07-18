/*******************************************************************************
* file    itask.c
* author  mackgim
* version 1.0.0
* date
* brief   逻辑任务
*******************************************************************************/

#ifndef __ITASK_H
#define __ITASK_H

#include <stdint.h>

void itask_init(void);
void itask_proc(void);

uint8_t itask_ble_connected();
uint8_t itask_ble_disconnected();
uint8_t itask_rx_proc(uint8_t *data, uint8_t size);
uint8_t itask_data_proc(uint8_t *data, uint8_t size);
uint8_t itask_image_proc(uint8_t *data, uint8_t size);

#endif /*__ITASK_H */
