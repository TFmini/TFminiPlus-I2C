#ifndef _BSP_I2C_H
#define _BSP_I2C_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "i2c.h"
#include "gpio.h"

#define I2C_MASTER        (&hi2c2)
void BspI2cResetBus(void);

#ifdef __cplusplus
}
#endif
#endif
