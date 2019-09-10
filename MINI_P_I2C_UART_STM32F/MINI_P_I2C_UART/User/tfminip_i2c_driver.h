#ifndef _TFMINIP_I2C_DRIVER_H
#define _TFMINIP_I2C_DRIVER_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include <stdint.h>

#define I2C_OK      (0)
#define I2C_ERROR   (1)

/** 
  * @描述   I2C主机写函数原型
  * @参数1  I2C从机地址
  * @参数2  数据所在的内存地址
  * @参数3  需要发送的数据量，单位Byte
  * @返回值 数据传输状态，I2C_OK或I2C_ERROR
  */
typedef uint8_t (*I2cWriteFuncPtr)(uint8_t, uint8_t*, uint32_t);

/** 
  * @描述   I2C主机读函数原型
  * @参数1  I2C从机地址
  * @参数2  目标内存地址
  * @参数3  需要读的数据量，单位Byte
  * @返回值 数据传输状态，I2C_OK或I2C_ERROR
  */
typedef uint8_t (*I2cReadFuncPtr)(uint8_t, uint8_t*, uint32_t);

/** 
  * @描述   I2C总线复位函数。某些I2C模块当发生总线错误时，无法自动恢复，需要特定的控制时序才能恢复正常。
  *         如果实际使用的I2C模块可以自动恢复，则可以不必实现此函数，但仍需按照函数原型，在调用MinipI2cInit时注册一个空函数。
  * @参数   无
  * @返回值 无
  */
typedef void (*I2cBusResetFuncPtr)(void);

/** 
  * @描述   延时函数
  * @参数   延时时间，单位ms
  * @返回值 无
  */
typedef void (*DelayMsFuncPtr)(uint32_t);

typedef struct
{
	uint8_t num;
	uint8_t addr_list[127];
}MinipDevListStruct;

typedef struct
{
	uint16_t dist;
	uint16_t amp;
	uint32_t tick_ms;
}MinipDataStruct;

typedef struct
{
	uint8_t major;
	uint8_t minor;
	uint8_t revision;
}MinipFirmwareVersion;

void MinipI2cInit(I2cWriteFuncPtr i2c_write, I2cReadFuncPtr i2c_read, I2cBusResetFuncPtr i2c_reset, DelayMsFuncPtr delay_ms);
MinipDevListStruct MinipI2cScanBus(void);
MinipDevListStruct MinipAddrDynamicAllocation(uint8_t default_addr);
void MinipAddrExclude(uint8_t num, uint8_t *ex_list);

uint8_t  MinipReadData(uint8_t addr, MinipDataStruct *data);
uint8_t  MinipReadVersion(uint8_t addr, MinipFirmwareVersion *verion);
uint8_t  MinipSoftReset(uint8_t addr);
uint8_t  MinipSetSampleRate(uint8_t addr, uint16_t rate);
uint8_t  MinipGetSampleRate(uint8_t addr, uint16_t *rate);
uint8_t  MinipSampleTrig(uint8_t addr);
uint8_t  MinipEnable(uint8_t addr);
uint8_t  MinipDisable(uint8_t addr);
uint8_t  MinipGetStatus(uint8_t addr, uint8_t *status);
uint8_t  MinipSetSlaveAddr(uint8_t addr, uint8_t new_addr);
uint8_t  MinipRestoreDefault(uint8_t addr);
uint8_t  MinipSaveSettings(uint8_t addr);
uint8_t  MinipSetAmpThreshold(uint8_t addr, uint8_t amp_th);
uint8_t  MinipGetAmpThreshold(uint8_t addr, uint8_t *amp_th);
uint8_t  MinipSetDistLimit(uint8_t addr, uint16_t min, uint16_t max);
uint8_t  MinipGetDistLimit(uint8_t addr, uint16_t *min, uint16_t *max);
uint8_t  MinipTimestampSync(uint8_t addr, uint32_t std);

#ifdef __cplusplus
}
#endif
#endif
