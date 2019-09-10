/**
  ********************************************************************************
  * 此文件为tfmini plus I2C模式的样例程序，大部分代码用于与PC通信，为了调试或演示方便。
  * 用户在实际系统中使用时，只需要关注tfminip_i2c_driver.h文件提供的接口函数。
  ********************************************************************************
  */
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "usart.h"
#include "bsp_i2c.h"
#include <string.h>
#include "tfminip_i2c_driver.h"
#include "frame.h"

// 以下两个ID是为调试方便临时定义的指令，调试用
#define ID_ADA            (0x40)
#define ID_SCAN_BUS       (0x41)

// 以下ID是雷达通信协议中的部分指令，在这里发送给I2C主控板，代为转发，调试用
#define ID_SOFT_RESET     (0x02)
#define ID_SAMPLE_FREQ    (0x03)
#define ID_OUTPUT_EN      (0x07)
#define ID_I2C_SLAVE_ADDR (0x0B)
#define ID_SAVE_SETTINGS  (0x11)

typedef struct
{
	uint16_t rate;
	uint8_t  en;
}ConfigParaStruct;

// 用于处理PC下发的串口指令，调试用
uint8_t         frame_buf_pc[256];
Frame_HandlePtr hframe_pc;

MinipDevListStruct   dev_list;  // 记录当前I2C总线上的设备状态
ConfigParaStruct     config;    // 用于记录当前雷达的工作状态，调试用，建议将雷达的工作帧率作为循环读取I2C总线的频率

// 按照I2cWriteFuncPtr的形式，定义I2C写函数
uint8_t i2c_write(uint8_t addr, uint8_t *buf, uint32_t size)
{
	HAL_StatusTypeDef status;
	status = HAL_I2C_Master_Transmit(I2C_MASTER, addr << 1, buf, size, 100);
	if(HAL_OK == status)
	{
		return I2C_OK;
	}
	else
	{
		return I2C_ERROR;
	}
}

// 按照I2cReadFuncPtr的形式，定义I2C读函数
uint8_t i2c_read(uint8_t addr, uint8_t *buf, uint32_t size)
{
	HAL_StatusTypeDef status;
	status = HAL_I2C_Master_Receive(I2C_MASTER, addr << 1, buf, size, 100);
	if(HAL_OK == status)
	{
		return I2C_OK;
	}
	else
	{
		return I2C_ERROR;
	}
}

// I2C总线设备信息打印函数，调试用
void PrintDevList(void)
{
	MinipFirmwareVersion version;
	if(dev_list.num > 0)
	{
		printf("Dev list:\n");
		for(uint8_t n = 0; n < dev_list.num; n++)
		{
			MinipReadVersion(dev_list.addr_list[n], &version);
			printf("dev %2d addr = 0x%02x firmware version = V%d.%d.%d\n", n, dev_list.addr_list[n], version.major, version.minor, version.revision);
		}
	}
	else
	{
		printf("No minip on i2c bus\n");
	}
}

// 初始化配置函数
void DataStreamInit(void)
{
	hframe_pc    = Frame_New(256, frame_buf_pc, 256);
	Frame_SetHead(hframe_pc, 0x5A);
	Frame_SetCheckArith(hframe_pc, FRAME_CHECK_NONE);
	MinipI2cInit(i2c_write, i2c_read, BspI2cResetBus, (DelayMsFuncPtr)osDelay);
	dev_list = MinipI2cScanBus();
	PrintDevList();
	if(dev_list.num > 0)
	{
		MinipGetSampleRate(dev_list.addr_list[0], &config.rate);
		MinipGetStatus(dev_list.addr_list[0], &config.en);
	}
	else
	{
		config.rate = 1;
		config.en = 0;
	}
}

// 主任务函数
void StartUserTask(void const * argument)
{
  /* USER CODE BEGIN StartDataStreamProcTask */
  osDelay(1000);
  MinipDataStruct data;
  uint32_t PreviousWakeTime = osKernelSysTick();
  
  DataStreamInit();
  MinipTimestampSync(0, 0);
  /* Infinite loop */
  for(;;)
  {
	osDelayUntil(&PreviousWakeTime, 1000 / config.rate);

	if(config.en)
	{
		for(uint8_t n = 0; n < dev_list.num; n++)
		{
			// 读取雷达测距结果
			if(I2C_OK == MinipReadData(dev_list.addr_list[n], &data))
			{
				printf("[%d] dist=%5d amp=%5d tick=%12d      ", n, data.dist, data.amp, data.tick_ms);
			}
			else
			{
				dev_list = MinipI2cScanBus();
				PrintDevList();
			}
		}
		printf("\n");
	}
	
	// 以下解析和处理PC下发的串口指令，调试用
	if(FRAME_OK == Frame_Search(hframe_pc))
	{
		uint8_t id = frame_buf_pc[2];
		switch (id)
		{
			case ID_ADA:
				dev_list = MinipAddrDynamicAllocation(0x10);
				PrintDevList();
				break;
			case ID_SCAN_BUS:
				dev_list = MinipI2cScanBus();
				PrintDevList();
				break;
			case ID_SOFT_RESET:
				MinipSoftReset(0);
				HAL_NVIC_SystemReset();
				break;
			case ID_SAMPLE_FREQ:
				config.rate = (uint16_t)frame_buf_pc[3] + ((uint16_t)frame_buf_pc[4] << 8);
				if(config.rate < 1)
				{
					config.rate = 1;
				}
				else if(config.rate > 1000)
				{
					config.rate = 1000;
				}
				MinipSetSampleRate(0, config.rate);
				break;
			case ID_OUTPUT_EN:
				config.en = frame_buf_pc[3];
				if(config.en)
				{
					MinipEnable(0);
				}
				else
				{
					MinipDisable(0);
				}
				break;
			case ID_I2C_SLAVE_ADDR:
				MinipSetSlaveAddr(0, frame_buf_pc[3]);
				break;
			case ID_SAVE_SETTINGS:
				MinipSaveSettings(0);
				break;
			default:
				break;
		}
	}
  }
  /* USER CODE END StartDownstreamProcTask */
}
