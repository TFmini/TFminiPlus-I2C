#include "bsp_i2c.h"
#include "cmsis_os.h"

static void I2cResetBus(I2C_HandleTypeDef *hi2c, GPIO_TypeDef *GPIOx, uint16_t SCL_Pin, uint16_t SDA_Pin)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	hi2c->Instance->CR1 &= ~I2C_CR1_PE;

	HAL_GPIO_DeInit(GPIOx,  SCL_Pin | SDA_Pin);
    GPIO_InitStruct.Pin = SCL_Pin | SDA_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);

	GPIOx->ODR |= (SCL_Pin | SDA_Pin);
	GPIOx->ODR &= ~SDA_Pin;
	GPIOx->ODR &= ~SCL_Pin;
	GPIOx->ODR |=  SCL_Pin;
	GPIOx->ODR |=  SDA_Pin;

	HAL_GPIO_DeInit(GPIOx,  SCL_Pin | SDA_Pin);
	GPIO_InitStruct.Pin = SCL_Pin | SDA_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);

	hi2c->Instance->CR1 |= I2C_CR1_SWRST;
	hi2c->Instance->CR1 &= ~I2C_CR1_SWRST;
	HAL_I2C_Init(hi2c);
	hi2c->Instance->CR1 |= I2C_CR1_PE;
}

void BspI2cResetBus(void)
{
	I2cResetBus(I2C_MASTER, GPIOB, GPIO_PIN_10, GPIO_PIN_11);
}
