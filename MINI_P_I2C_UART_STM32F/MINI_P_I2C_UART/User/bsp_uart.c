#include <string.h>
#include "bsp_uart.h"
#include "i2c.h"
#include "frame.h"

#define UART_RX_BUF_SIZE 512

uint8_t uart_pc_rx_buf[UART_RX_BUF_SIZE];

extern Frame_HandlePtr hframe_pc;

void BspUartInit(void)
{
	__HAL_UART_ENABLE_IT(UART_PC, UART_IT_IDLE);
	HAL_UART_Receive_DMA(UART_PC, uart_pc_rx_buf, UART_RX_BUF_SIZE);
}

int stdout_putchar (int ch)
{
	HAL_UART_Transmit(UART_PC, (uint8_t*)&ch, 1, 10);
	return 0;
}

void BspUartRxIdleCallback(UART_HandleTypeDef *huart)
{
	uint32_t tmp_flag      = __HAL_UART_GET_FLAG     (huart, UART_FLAG_IDLE);
	uint32_t tmp_it_source = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_IDLE);
	uint32_t len;

	if ((RESET != tmp_flag) && (RESET != tmp_it_source))
	{
		__HAL_UART_CLEAR_IDLEFLAG(huart);
		huart->gState = (HAL_UART_STATE_BUSY_TX_RX == huart->gState) ? HAL_UART_STATE_BUSY_TX : HAL_UART_STATE_READY;

		__HAL_DMA_DISABLE(huart->hdmarx);
		len = UART_RX_BUF_SIZE - __HAL_DMA_GET_COUNTER(huart->hdmarx);
		Frame_WriteFifo(hframe_pc, uart_pc_rx_buf, len);
		huart->hdmarx->Instance->CNDTR = UART_RX_BUF_SIZE;
		__HAL_DMA_ENABLE(huart->hdmarx);
	}
}

/* 以下内容改写CUBE生成的库函数，避免因波特率异常导致的某些串口错误会直接关闭串口的正常功能 */

/**
  * @brief  Sends an amount of data in non blocking mode.
  * @param  huart: Pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval HAL status
  */
static HAL_StatusTypeDef UART_Transmit_IT(UART_HandleTypeDef *huart)
{
  uint16_t* tmp;
  
  /* Check that a Tx process is ongoing */
  if(huart->gState == HAL_UART_STATE_BUSY_TX)
  {
    if(huart->Init.WordLength == UART_WORDLENGTH_9B)
    {
      tmp = (uint16_t*) huart->pTxBuffPtr;
      huart->Instance->DR = (uint16_t)(*tmp & (uint16_t)0x01FF);
      if(huart->Init.Parity == UART_PARITY_NONE)
      {
        huart->pTxBuffPtr += 2U;
      }
      else
      {
        huart->pTxBuffPtr += 1U;
      }
    } 
    else
    {
      huart->Instance->DR = (uint8_t)(*huart->pTxBuffPtr++ & (uint8_t)0x00FF);
    }

    if(--huart->TxXferCount == 0U)
    {
      /* Disable the UART Transmit Complete Interrupt */
      __HAL_UART_DISABLE_IT(huart, UART_IT_TXE);

      /* Enable the UART Transmit Complete Interrupt */    
      __HAL_UART_ENABLE_IT(huart, UART_IT_TC);
    }
    return HAL_OK;
  }
  else
  {
    return HAL_BUSY;
  }
}

/**
  * @brief  Wraps up transmission in non blocking mode.
  * @param  huart: pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval HAL status
  */
static HAL_StatusTypeDef UART_EndTransmit_IT(UART_HandleTypeDef *huart)
{
  /* Disable the UART Transmit Complete Interrupt */    
  __HAL_UART_DISABLE_IT(huart, UART_IT_TC);
  
  /* Tx process is ended, restore huart->gState to Ready */
  huart->gState = HAL_UART_STATE_READY;
  HAL_UART_TxCpltCallback(huart);
  
  return HAL_OK;
}

/**
  * @brief  Receives an amount of data in non blocking mode 
  * @param  huart: pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval HAL status
  */
static HAL_StatusTypeDef UART_Receive_IT(UART_HandleTypeDef *huart)
{
  uint16_t* tmp;
  
  /* Check that a Rx process is ongoing */
  if(huart->RxState == HAL_UART_STATE_BUSY_RX) 
  {
    if(huart->Init.WordLength == UART_WORDLENGTH_9B)
    {
      tmp = (uint16_t*) huart->pRxBuffPtr;
      if(huart->Init.Parity == UART_PARITY_NONE)
      {
        *tmp = (uint16_t)(huart->Instance->DR & (uint16_t)0x01FF);
        huart->pRxBuffPtr += 2U;
      }
      else
      {
        *tmp = (uint16_t)(huart->Instance->DR & (uint16_t)0x00FF);
        huart->pRxBuffPtr += 1U;
      }
    }
    else
    {
      if(huart->Init.Parity == UART_PARITY_NONE)
      {
        *huart->pRxBuffPtr++ = (uint8_t)(huart->Instance->DR & (uint8_t)0x00FF);
      }
      else
      {
        *huart->pRxBuffPtr++ = (uint8_t)(huart->Instance->DR & (uint8_t)0x007F);
      }
    }

    if(--huart->RxXferCount == 0U)
    {
      /* Disable the IRDA Data Register not empty Interrupt */
      __HAL_UART_DISABLE_IT(huart, UART_IT_RXNE);

      /* Disable the UART Parity Error Interrupt */
      __HAL_UART_DISABLE_IT(huart, UART_IT_PE);
        /* Disable the UART Error Interrupt: (Frame error, noise error, overrun error) */
        __HAL_UART_DISABLE_IT(huart, UART_IT_ERR);

      /* Rx process is completed, restore huart->RxState to Ready */
      huart->RxState = HAL_UART_STATE_READY;

      HAL_UART_RxCpltCallback(huart);

      return HAL_OK;
    }
    return HAL_OK;
  }
  else
  {
    return HAL_BUSY;
  }
}

void User_HAL_UART_IRQHandler(UART_HandleTypeDef *huart)
{
   uint32_t isrflags   = READ_REG(huart->Instance->SR);
   uint32_t cr1its     = READ_REG(huart->Instance->CR1);
   uint32_t cr3its     = READ_REG(huart->Instance->CR3);
   uint32_t errorflags = 0x00U;
//   uint32_t dmarequest = 0x00U;

  /* If no error occurs */
  errorflags = (isrflags & (uint32_t)(USART_SR_PE | USART_SR_FE | USART_SR_ORE | USART_SR_NE));
  if(errorflags == RESET)
  {
    /* UART in mode Receiver -------------------------------------------------*/
    if(((isrflags & USART_SR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET))
    {
      UART_Receive_IT(huart);
      return;
    }
  }

  /* If some errors occur */
//   if((errorflags != RESET) && (((cr3its & USART_CR3_EIE) != RESET) || ((cr1its & (USART_CR1_RXNEIE | USART_CR1_PEIE)) != RESET)))
//   {
//     /* UART parity error interrupt occurred ----------------------------------*/
//     if(((isrflags & USART_SR_PE) != RESET) && ((cr1its & USART_CR1_PEIE) != RESET))
//     {
//       huart->ErrorCode |= HAL_UART_ERROR_PE;
//     }

//     /* UART noise error interrupt occurred -----------------------------------*/
//     if(((isrflags & USART_SR_NE) != RESET) && ((cr3its & USART_CR3_EIE) != RESET))
//     {
//       huart->ErrorCode |= HAL_UART_ERROR_NE;
//     }

//     /* UART frame error interrupt occurred -----------------------------------*/
//     if(((isrflags & USART_SR_FE) != RESET) && ((cr3its & USART_CR3_EIE) != RESET))
//     {
//       huart->ErrorCode |= HAL_UART_ERROR_FE;
//     }

//     /* UART Over-Run interrupt occurred --------------------------------------*/
//     if(((isrflags & USART_SR_ORE) != RESET) && ((cr3its & USART_CR3_EIE) != RESET))
//     { 
//       huart->ErrorCode |= HAL_UART_ERROR_ORE;
//     }

//     /* Call UART Error Call back function if need be --------------------------*/
//     if(huart->ErrorCode != HAL_UART_ERROR_NONE)
//     {
//       /* UART in mode Receiver -----------------------------------------------*/
//       if(((isrflags & USART_SR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET))
//       {
//         UART_Receive_IT(huart);
//       }

//       /* If Overrun error occurs, or if any error occurs in DMA mode reception,
//          consider error as blocking */
//       dmarequest = HAL_IS_BIT_SET(huart->Instance->CR3, USART_CR3_DMAR);
//       if(((huart->ErrorCode & HAL_UART_ERROR_ORE) != RESET) || dmarequest)
//       {
//         /* Blocking error : transfer is aborted
//            Set the UART state ready to be able to start again the process,
//            Disable Rx Interrupts, and disable Rx DMA request, if ongoing */
//         UART_EndRxTransfer(huart);

//         /* Disable the UART DMA Rx request if enabled */
//         if(HAL_IS_BIT_SET(huart->Instance->CR3, USART_CR3_DMAR))
//         {
//           CLEAR_BIT(huart->Instance->CR3, USART_CR3_DMAR);

//           /* Abort the UART DMA Rx channel */
//           if(huart->hdmarx != NULL)
//           {
//             /* Set the UART DMA Abort callback : 
//                will lead to call HAL_UART_ErrorCallback() at end of DMA abort procedure */
//             huart->hdmarx->XferAbortCallback = UART_DMAAbortOnError;
//             if(HAL_DMA_Abort_IT(huart->hdmarx) != HAL_OK)
//             {
//               /* Call Directly XferAbortCallback function in case of error */
//               huart->hdmarx->XferAbortCallback(huart->hdmarx);
//             }
//           }
//           else
//           {
//             /* Call user error callback */
//             HAL_UART_ErrorCallback(huart);
//           }
//         }
//         else
//         {
//           /* Call user error callback */
//           HAL_UART_ErrorCallback(huart);
//         }
//       }
//       else
//       {
//         /* Non Blocking error : transfer could go on. 
//            Error is notified to user through user error callback */
//         HAL_UART_ErrorCallback(huart);
//         huart->ErrorCode = HAL_UART_ERROR_NONE;
//       }
//     }
//     return;
//   } /* End if some error occurs */

  /* UART in mode Transmitter ------------------------------------------------*/
  if(((isrflags & USART_SR_TXE) != RESET) && ((cr1its & USART_CR1_TXEIE) != RESET))
  {
    UART_Transmit_IT(huart);
    return;
  }
  
  /* UART in mode Transmitter end --------------------------------------------*/
  if(((isrflags & USART_SR_TC) != RESET) && ((cr1its & USART_CR1_TCIE) != RESET))
  {
    UART_EndTransmit_IT(huart);
    return;
  }
}

