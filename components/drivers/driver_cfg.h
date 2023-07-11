#ifndef __DRV_CFG_H__
#define __DRV_CFG_H__

#include "ymconfig.h"

#include "interface/include/interface.h"

#ifdef DRV_USING_PIN
#include "misc/include/pin.h"
#endif

#ifdef DRV_USING_SPI
#include "interface/include/spi_interface.h"
#endif

#endif /* __DRV_CFG_H__ */
