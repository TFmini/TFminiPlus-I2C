/**
  ******************************************************************************
  * @文件    frame.h
  * @作者    ZhuXiongwei
  * @描述    frame模块的接口头文件
  * 
  * frame模块提供3类接口：基础接口、扩展接口、调试接口。
  * 基础接口：使用数据帧的默认格式，即 | 0x44 | len | id | payload | checksum |
  * 扩展接口：支持删除frame模块、修改帧头、去除帧头、去除校验、修改为crc32校验等扩展
  * 		 功能。如果用户提供获取系统时钟的接口，可以使frame模块具备搜索超时功能。
  * 调试接口：为方便用户使用此模块时的调试需求开放的接口。
  ******************************************************************************
  */ 

#ifndef _FRAME_H
#define _FRAME_H

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdlib.h>
#include <stdint.h>

/** 
  * @描述  frame模块的句柄，用户使用frame模块前，必须先通过Frame_New函数获取一个有效的frame句柄。
  */
typedef struct __Frame_HandleStruct *Frame_HandlePtr;

/** 
  * @描述  获取系统时钟的函数原型
  */
typedef uint32_t (*GetTickFuncPtr)(void);

/** 
  * @描述  frame模块的状态枚举
  */
typedef enum
{
	FRAME_OK    = 0,         /*!< 状态正常或得到后效数据帧     */
	FRAME_LESS,              /*!< 没有从FIFO中搜索到有效数据帧 */
	FRAME_FIFO_OVERFLOW,     /*!< FIFO溢出                   */
	FRAME_NULL_PTR           /*!< 不是有效的frame句柄         */
}Frame_StatusEnum;

/** 
  * @描述  帧校验类型枚举
  */
typedef enum
{
	FRAME_CHECK_NONE = 0,
	FRAME_CHECK_SUM,
	FRAME_CHECK_CRC32
}Frame_CheckTypeEnum;

/**
  ******************************************************************************
  * 基础接口
  ******************************************************************************
  */

/**
  * @brief  为一个新的frame模块申请内存，返回frame句柄。
  * @param  fifo_size:      循环FIFO的大小 
  * @param  frame_buf:      有效数据帧的存放地址，由用户自行申请内存，将内存指针传入此函数。frame模块搜索
  * 					    到有效数据帧后，将其从FIFO读出到frame_buf所指向的内存，供用户进一步解析。
  * @param  frame_buf_size: frame_buf指向的内存空间的大小，该大小应该由最长数据帧的长度决定，最大为256。
  * @retval frame句柄
  */
extern Frame_HandlePtr  Frame_New(uint32_t fifo_size, uint8_t *frame_buf, uint16_t frame_buf_size);

/**
  * @brief  将通信端口收到的数据写入frame模块的循环FIFO。
  * @param  hframe:  frame句柄。 
  * @param  buf:     接收端口缓存地址，或用户自定义的内存地址，该地址存放了通信端口收到的数据
  * @param  size:    写入数据的大小
  * @retval frame状态
  */
extern Frame_StatusEnum Frame_WriteFifo(Frame_HandlePtr hframe, uint8_t *buf, uint32_t size);

/**
  * @brief  从循环FIFO中搜索数据帧，所搜到有效数据帧时，将有效数据帧复制到Frame_New接口的frame_buf参数指向的内存
  *         同时返回FRAME_OK。如果此时FIFO中累积了多个有效数据帧，此函数的一次调用将只获取一个有效数据帧。
  * @param  hframe:  frame句柄。 
  * @retval frame状态
  */
extern Frame_StatusEnum Frame_Search(Frame_HandlePtr hframe);

/**
  ******************************************************************************
  * 扩展接口
  ******************************************************************************
  */

/**
  * @brief  销毁frame模块，释放内存
  * @param  hframe:  frame句柄。 
  * @retval 无
  */
extern void Frame_Delete(Frame_HandlePtr hframe);

/**
  * @brief  修改数据帧帧头，默认帧头为 0x44('D')
  * @param  hframe:  frame句柄。 
  * @param  head:    将要生效的帧头。 
  * @retval 无
  */
extern void Frame_SetHead(Frame_HandlePtr hframe, uint8_t head);

/**
  * @brief  修改帧校验类型
  * @param  hframe:     frame句柄。 
  * @param  check_type: 帧校验类型，详见 Frame_CheckTypeEnum 定义。
  * @retval 无
  */
extern void Frame_SetCheckArith(Frame_HandlePtr hframe, Frame_CheckTypeEnum check_type);

/**
  * @brief  去除帧头
  * @param  hframe:     frame句柄。 
  * @retval 无
  */
extern void Frame_DiscardHead(Frame_HandlePtr hframe);

/**
  * @brief  注册获取系统时钟的函数，用于使能frame模块的帧搜索超时机制。
  *         应对的情况：当有异常数据被写入FIFO，且其中的帧长字节为一个很大的值，此时将导致
  *         frame模块持续等待足够的数据写入FIFO才进行下一步判断，而后续的有效数据帧无法被检测到。
  * @param  hframe:   frame句柄。 
  * @param  func:     获取系统时钟的函数指针。
  * @param  timeout:  超时时间，单位与func的返回值一致。
  * @retval 无
  */
extern void Frame_RegGetTickFunc(Frame_HandlePtr hframe, GetTickFuncPtr func, uint32_t timeout);


/**
  ******************************************************************************
  * 调试接口
  ******************************************************************************
  */

/**
  * @brief  获取FIFO中已写入的数据大小。
  * @param  hframe: frame句柄。 
  * @retval FIFO中已写入的数据大小
  */
extern uint32_t Frame_GetFifoDataSize(Frame_HandlePtr hframe);

/**
  * @brief  直接将FIFO中的数据拷贝到frame_buf指向的内存，如果FIFO中的数据量大于frame_buf_size，
  *         最多拷贝frame_buf_size大小的数据。
  * @param  hframe: frame句柄。 
  * @retval 实际拷贝的数据量
  */
extern uint32_t Frame_CopyFifoDataToFrameBuf(Frame_HandlePtr hframe);

/**
  * @brief  计算校验和。
  * @param  data: 数据内存指针。 
  * @param  size: 数据大小。 
  * @retval 校验和
  */
extern uint8_t Frame_CalSum(uint8_t *data, uint32_t size);

/**
  * @brief  计算crc32校验值，支持非4字节对齐的数据计算。
  * @param  data: 数据内存指针。 
  * @param  size: 数据大小。 
  * @retval crc32校验值
  */
extern uint32_t Frame_CalCrc32(uint8_t *data, uint32_t size);

#ifdef __cplusplus
}
#endif
#endif
