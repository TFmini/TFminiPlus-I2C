#include "tfminip_i2c_driver.h"
#include "string.h"

#ifndef NULL
	#define NULL ((void*)0)
#endif

static I2cWriteFuncPtr     i2c_write_func = NULL;
static I2cReadFuncPtr      i2c_read_func  = NULL;
static I2cBusResetFuncPtr  i2c_reset_func = NULL;
static DelayMsFuncPtr      delay_ms_func  = NULL;
static MinipDevListStruct  sDevList;         // 总线上的MINIP从机地址列表
static MinipDevListStruct  sDevListEx;       // 需要排除的其他设备的从机地址列表
static uint8_t             sInitFlag = 0;

/** 
  * @描述   I2C发收结合函数
  * @参数   addr：从机地址
  * @参数   cmd_buf：发送数据地址
  * @参数   cmd_len：发送数据长度
  * @参数   ack_buf：接收数据地址
  * @参数   ack_len：接收数据长度
  * @参数   wait_time：数据发送和接收间隔
  * @返回值 数据传输状态，I2C_OK或I2C_ERROR
  */
static uint8_t i2c_transcieve(uint8_t addr, uint8_t *cmd_buf, uint8_t cmd_len, uint8_t *ack_buf, uint8_t ack_len, uint32_t wait_time)
{
	if(!sInitFlag)
	{
		return I2C_ERROR;
	}

	if(I2C_OK == i2c_write_func(addr, cmd_buf, cmd_len))
	{
		delay_ms_func(wait_time);
		if(I2C_OK == i2c_read_func(addr, ack_buf, ack_len))
		{
			return I2C_OK;
		}
		else
		{
			return I2C_ERROR;
		}
	}
	else
	{
		return I2C_ERROR;
	}
}

/** 
  * @描述   判断一个从机地址是否存在于指定的地址列表中
  * @参数   addr：待判断的从机地址
  * @参数   list：指定的地址列表
  * @返回值 存在返回1，不存在返回0
  */
static uint8_t is_addr_in_list(uint8_t addr, MinipDevListStruct list)
{
	for(uint8_t n = 0; n < list.num; n++)
	{
		if(addr == list.addr_list[n])
		{
			return 1;
		}
	}
	return 0;
}

/** 
  * @描述   I2C初始化函数，注册访问i2c总线必要的函数指针
  * @参数   i2c_write：I2C写函数指针
  * @参数   i2c_read：I2C读函数指针
  * @参数   i2c_reset：I2C总线复位函数指针
  * @参数   delay_ms：延时函数指针
  * @返回值 无
  */
void MinipI2cInit(I2cWriteFuncPtr i2c_write, I2cReadFuncPtr i2c_read, I2cBusResetFuncPtr i2c_reset, DelayMsFuncPtr delay_ms)
{
	if(!(i2c_write && i2c_read && i2c_reset && delay_ms))
	{
		return;
	}
	i2c_write_func = i2c_write;
	i2c_read_func  = i2c_read;
	i2c_reset_func = i2c_reset;
	delay_ms_func  = delay_ms;
	memset(&sDevList,   0, sizeof(MinipDevListStruct));
	memset(&sDevListEx, 0, sizeof(MinipDevListStruct));
	sInitFlag = 1;
}

/** 
  * @描述   I2C总线设备查询函数
  * @参数   无
  * @返回值 I2C总线上的设备列表
  */
MinipDevListStruct MinipI2cScanBus(void)
{
	memset(&sDevList, 0, sizeof(MinipDevListStruct));
	if(!sInitFlag)
	{
		return sDevList;
	}
	
	uint8_t dummy = 0xFF;
	i2c_reset_func();
	for(uint8_t n = 1; n <= 127; n++)
	{
		// 跳过排除列表中的地址
		if(is_addr_in_list(n, sDevListEx))
		{
			continue;
		}

		if(I2C_OK == i2c_write_func(n, &dummy, 1))
		{
			sDevList.addr_list[sDevList.num] = n;
			sDevList.num++;
		}
		else
		{
			i2c_reset_func();
		}
	}

	return sDevList;
}

/** 
  * @描述   执行一次动态地址分配操作，使用方法为：总线上新接入一台雷达，调用一次此函数。
  *         维护一个总线上的雷达列表，列表中的雷达从机地址均不相同，范围[1, 127]。
  *         当函数调用时，发现总线上有一台默认从机地址的雷达时，将为该雷达分配一个新的地址，并加入所维护的雷达列表中。
  * @参数   default_addr：雷达的默认从机地址
  * @返回值 无
  */
MinipDevListStruct MinipAddrDynamicAllocation(uint8_t default_addr)
{
	sDevList = MinipI2cScanBus();
	// 检查当前的地址列表中是否有默认地址
	if(is_addr_in_list(default_addr, sDevList))
	{
		// 寻找一个地址，不在当前地址列表、不在排除地址列表、不是默认地址
		for(uint8_t n = 1; n <= 127; n++)
		{
			if((0 == is_addr_in_list(n, sDevList)) && (0 == is_addr_in_list(n, sDevListEx)) && (n != default_addr))
			{
				(void)MinipSetSlaveAddr(default_addr, n);
				sDevList = MinipI2cScanBus();  // 修改从机地址后，重新扫描一次总线。
			}
		}
	}
	return sDevList;
}

/** 
  * @描述   添加从机地址的排除列表。由于I2C总线上可能挂着除TFMINI PLUS之外的设备，因此操作总线时，排除这些设备的地址
  * @参数   num：排除地址的个数
  * @参数   ex_list：排除地址的列表，uint8_t型的数组
  * @返回值 无
  */
void MinipAddrExclude(uint8_t num, uint8_t *ex_list)
{
	memset(&sDevListEx, 0, sizeof(MinipDevListStruct));
	memcpy(sDevListEx.addr_list, ex_list, num * sizeof(uint8_t));
	sDevListEx.num = num;
}

/** 
  * @描述   读取雷达测距结果函数
  * @参数   addr：指定雷达的从机地址
  * @参数   data：测距结果结构体指针
  * @返回值 数据传输状态，I2C_OK或I2C_ERROR
  */
uint8_t MinipReadData(uint8_t addr, MinipDataStruct *data)
{
	static uint8_t cmd[5] = {0x5A, 5, 0x00, 0x07, 0};
	static uint8_t ack[11];
	if(I2C_OK == i2c_transcieve(addr, cmd, 5, ack, 11, 0))
	{
		data->dist = (uint16_t)ack[2] + (((uint16_t)ack[3]) << 8);
		data->amp  = (uint16_t)ack[4] + (((uint16_t)ack[5]) << 8);
		data->tick_ms = (uint32_t)ack[6] + (((uint32_t)ack[7]) << 8) + (((uint32_t)ack[8]) << 16) + (((uint32_t)ack[9]) << 24);
		return I2C_OK;
	}
	else
	{
		return I2C_ERROR;
	}
}

/** 
  * @描述   读取雷达固件版本号函数
  * @参数   addr：指定雷达的从机地址
  * @参数   verion：固件版本号结构体指针
  * @返回值 数据传输状态，I2C_OK或I2C_ERROR
  */
uint8_t MinipReadVersion(uint8_t addr, MinipFirmwareVersion *verion)
{
	uint8_t cmd[4] = {0x5A, 4, 0x01, 0};
	uint8_t ack[7];
	if(I2C_OK == i2c_transcieve(addr, cmd, 4, ack, 7, 10))
	{
		verion->major = ack[5];
		verion->minor = ack[4];
		verion->revision = ack[3];
		return I2C_OK;
	}
	else
	{
		return I2C_ERROR;
	}
}

/** 
  * @描述   软件复位雷达函数
  * @参数   addr：指定雷达的从机地址，当addr为0时，对总线上的所有雷达有效
  * @返回值 数据传输状态，I2C_OK或I2C_ERROR
  */
uint8_t  MinipSoftReset(uint8_t addr)
{
	if(!sInitFlag)
	{
		return I2C_ERROR;
	}

	uint8_t cmd[4] = {0x5A, 4, 0x02, 0};
	return i2c_write_func(addr, cmd, 4);
}

/** 
  * @描述   设定雷达帧率函数
  * @参数   addr：指定雷达的从机地址，当addr为0时，对总线上的所有雷达有效
  * @参数   rate：帧率，范围1-1000，雷达可实现的帧率为1000/n，n为正整数。当rate为0时，雷达进入单次触发模式
  * @返回值 数据传输状态，I2C_OK或I2C_ERROR
  */
uint8_t  MinipSetSampleRate(uint8_t addr, uint16_t rate)
{
	if(!sInitFlag)
	{
		return I2C_ERROR;
	}

	uint8_t cmd[6] = {0x5A, 6, 0x03, 0, 0, 0};
	cmd[3] = rate & 0xFF;
	cmd[4] = (rate >> 8) & 0xFF;
	return i2c_write_func(addr, cmd, 6);
}

/** 
  * @描述   读取雷达帧率函数。
  * @参数   addr：指定雷达的从机地址
  * @参数   rate：帧率指针
  * @返回值 数据传输状态，I2C_OK或I2C_ERROR
  */
uint8_t MinipGetSampleRate(uint8_t addr, uint16_t *rate)
{
	uint8_t cmd[5] = {0x5A, 5, 0x3F, 0x03, 0};
	uint8_t ack[6];
	if(I2C_OK == i2c_transcieve(addr, cmd, 5, ack, 6, 10))
	{
		*rate = (uint16_t)ack[3] + (((uint16_t)ack[4]) << 8);
		return I2C_OK;
	}
	else
	{
		return I2C_ERROR;
	}
}

/** 
  * @描述   对雷达进行单次触发函数，只有当雷达帧率为0，即单次触发模式下，此函数才有效
  * @参数   addr：指定雷达的从机地址
  * @返回值 数据传输状态，I2C_OK或I2C_ERROR
  */
uint8_t  MinipSampleTrig(uint8_t addr)
{
	if(!sInitFlag)
	{
		return I2C_ERROR;
	}

	uint8_t cmd[4] = {0x5A, 4, 0x04, 0};
	return i2c_write_func(addr, cmd, 4);
}

/** 
  * @描述   使能雷达函数
  * @参数   addr：指定雷达的从机地址，当addr为0时，对总线上的所有雷达有效
  * @返回值 数据传输状态，I2C_OK或I2C_ERROR
  */
uint8_t  MinipEnable(uint8_t addr)
{
	if(!sInitFlag)
	{
		return I2C_ERROR;
	}

	uint8_t cmd[5] = {0x5A, 5, 0x07, 1, 0};
	return i2c_write_func(addr, cmd, 5);
}

/** 
  * @描述   关闭雷达函数
  * @参数   addr：指定雷达的从机地址，当addr为0时，对总线上的所有雷达有效
  * @返回值 数据传输状态，I2C_OK或I2C_ERROR
  */
uint8_t  MinipDisable(uint8_t addr)
{
	if(!sInitFlag)
	{
		return I2C_ERROR;
	}

	uint8_t cmd[5] = {0x5A, 5, 0x07, 0, 0};
	return i2c_write_func(addr, cmd, 5);
}

/** 
  * @描述   获取雷达状态函数
  * @参数   addr：指定雷达的从机地址
  * @参数   status：状态指针，0雷达关闭，1雷达使能
  * @返回值 数据传输状态，I2C_OK或I2C_ERROR
  */
uint8_t  MinipGetStatus(uint8_t addr, uint8_t *status)
{
	uint8_t cmd[5] = {0x5A, 5, 0x3F, 0x07, 0};
	uint8_t ack[5];
	if(I2C_OK == i2c_transcieve(addr, cmd, 5, ack, 5, 10))
	{
		*status = ack[3];
		return I2C_OK;
	}
	else
	{
		return I2C_ERROR;
	}
}

/** 
  * @描述   修改雷达从机地址函数
  * @参数   addr：指定雷达的从机地址
  * @参数   new_addr：新的从机地址，范围1-127
  * @返回值 数据传输状态，I2C_OK或I2C_ERROR
  */
uint8_t  MinipSetSlaveAddr(uint8_t addr, uint8_t new_addr)
{
	if(!sInitFlag)
	{
		return I2C_ERROR;
	}

	uint8_t cmd[5] = {0x5A, 5, 0x0B, 0, 0};
	cmd[3] = new_addr;
	return i2c_write_func(addr, cmd, 5);
}

/** 
  * @描述   恢复出厂设置函数
  * @参数   addr：指定雷达的从机地址，当addr为0时，对总线上的所有雷达有效
  * @返回值 数据传输状态，I2C_OK或I2C_ERROR
  */
uint8_t  MinipRestoreDefault(uint8_t addr)
{
	if(!sInitFlag)
	{
		return I2C_ERROR;
	}

	uint8_t cmd[4] = {0x5A, 4, 0x10, 0};
	return i2c_write_func(addr, cmd, 4);
}

/** 
  * @描述   保存当前设置函数
  * @参数   addr：指定雷达的从机地址，当addr为0时，对总线上的所有雷达有效
  * @返回值 数据传输状态，I2C_OK或I2C_ERROR
  */
uint8_t  MinipSaveSettings(uint8_t addr)
{
	if(!sInitFlag)
	{
		return I2C_ERROR;
	}

	uint8_t cmd[4] = {0x5A, 4, 0x11, 0};
	return i2c_write_func(addr, cmd, 4);
}

/** 
  * @描述   设置雷达AMP阈值函数
  * @参数   addr：指定雷达的从机地址，当addr为0时，对总线上的所有雷达有效
  * @参数   amp_th：AMP踢点阈值，数值为实际AMP阈值的10分之1，即实际阈值为amp_th*10，AMP低于该值时，距离值输出为0
  * @返回值 数据传输状态，I2C_OK或I2C_ERROR
  */
uint8_t  MinipSetAmpThreshold(uint8_t addr, uint8_t amp_th)
{
	if(!sInitFlag)
	{
		return I2C_ERROR;
	}

	uint8_t cmd[5] = {0x5A, 5, 0x22, 0, 0};
	cmd[3] = amp_th;
	return i2c_write_func(addr, cmd, 5);
}

/** 
  * @描述   获取雷达AMP阈值函数
  * @参数   addr：指定雷达的从机地址
  * @参数   amp_th：AMP踢点阈值指针，数值为实际AMP阈值的10分之1
  * @返回值 数据传输状态，I2C_OK或I2C_ERROR
  */
uint8_t  MinipGetAmpThreshold(uint8_t addr, uint8_t *amp_th)
{
	uint8_t cmd[5] = {0x5A, 5, 0x3F, 0x22, 0};
	uint8_t ack[5];
	if(I2C_OK == i2c_transcieve(addr, cmd, 5, ack, 5, 10))
	{
		*amp_th = ack[3];
		return I2C_OK;
	}
	else
	{
		return I2C_ERROR;
	}
}

/** 
  * @描述   设置距离限制函数
  * @参数   addr：指定雷达的从机地址，当addr为0时，对总线上的所有雷达有效
  * @参数   min：雷达输出的最小距离值，单位cm
  * @参数   max：雷达输出的最大距离值，单位cm
  * @返回值 数据传输状态，I2C_OK或I2C_ERROR
  */
uint8_t  MinipSetDistLimit(uint8_t addr, uint16_t min, uint16_t max)
{
	if(!sInitFlag)
	{
		return I2C_ERROR;
	}

	uint8_t cmd[9] = {0x5A, 9, 0x3A};
	cmd[3] = min & 0xFF;
	cmd[4] = (min >> 8) & 0xFF;
	cmd[5] = max & 0xFF;
	cmd[6] = (max >> 8) & 0xFF;
	cmd[7] = 0;
	return i2c_write_func(addr, cmd, 9);
}

/** 
  * @描述   获取距离限制值函数
  * @参数   addr：指定雷达的从机地址
  * @参数   min：雷达输出的最小距离值指针，单位cm
  * @参数   max：雷达输出的最大距离值指针，单位cm
  * @返回值 数据传输状态，I2C_OK或I2C_ERROR
  */
uint8_t  MinipGetDistLimit(uint8_t addr, uint16_t *min, uint16_t *max)
{
	uint8_t cmd[5] = {0x5A, 5, 0x3F, 0x3A, 0};
	uint8_t ack[9];
	if(I2C_OK == i2c_transcieve(addr, cmd, 5, ack, 9, 10))
	{
		*min = (uint16_t)ack[3] + (((uint16_t)ack[4]) << 8);
		*max = (uint16_t)ack[5] + (((uint16_t)ack[6]) << 8);
		return I2C_OK;
	}
	else
	{
		return I2C_ERROR;
	}

}

/** 
  * @描述   Minip时间戳同步函数
  * @参数   addr：指定雷达的从机地址，当addr为0时，对总线上的所有雷达有效
  * @参数   std：指定的当前时间，单位ms
  * @返回值 数据传输状态，I2C_OK或I2C_ERROR
  */
uint8_t  MinipTimestampSync(uint8_t addr, uint32_t std)
{
	if(!sInitFlag)
	{
		return I2C_ERROR;
	}

	uint8_t cmd[8] = {0x5A, 8, 0x31};
	cmd[3] = std & 0xFF;
	cmd[4] = (std >> 8) & 0xFF;
	cmd[5] = (std >> 16) & 0xFF;
	cmd[6] = (std >> 24) & 0xFF;
	cmd[7] = 0;
	return i2c_write_func(addr, cmd, 8);
}


