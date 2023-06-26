/*******************************************************************************
* file    rng.if.h
* author  mackgim
* version 1.0.0
* date
* brief   随机数
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __RNG_IF_H
#define __RNG_IF_H

#include "stdint.h"


void rng_init(void);
void rng_deinit(void);
uint8_t rng_get_numbers(uint32_t* buff, uint32_t buffersize);


#endif /*__RNG_IF_H */

