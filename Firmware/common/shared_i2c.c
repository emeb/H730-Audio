/*
 * shared_i2c.c - shared I2C bus basic routines
 * 07-12-19 E. Brombaugh
 */
 
#include "shared_i2c.h"

/* ST's crazy I2C timing register for 100kHz (from CubeMX) */
#define I2Cx_TIMING ((uint32_t)0x10c0ecff)  

I2C_HandleTypeDef i2c_handler;

/*
 * initialize shared I2C bus
 */
void shared_i2c_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable I2C1 GPIO clock */
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/* Hook up I2C1 to PB9/8 */
	GPIO_InitStructure.Pin =  GPIO_PIN_9|GPIO_PIN_8;
	GPIO_InitStructure.Mode = GPIO_MODE_AF_OD;
	GPIO_InitStructure.Speed = GPIO_SPEED_MEDIUM;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	GPIO_InitStructure.Alternate = GPIO_AF4_I2C1;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* reset the I2C bus */
	shared_i2c_reset();
}

void shared_i2c_reset(void)
{
	/* Enable the I2C1 peripheral clock & reset it */
	__HAL_RCC_I2C1_CLK_ENABLE();
	__HAL_RCC_I2C1_FORCE_RESET();
	__HAL_RCC_I2C1_RELEASE_RESET();
	
	/* point at the right interface */
	i2c_handler.Instance              = I2C1;
	
	/* De-initialize the I2C communication bus */
	HAL_I2C_DeInit(&i2c_handler);

	/* I2C1 peripheral configuration */
	i2c_handler.Init.Timing           = I2Cx_TIMING;
	i2c_handler.Init.OwnAddress1      = 0;
	i2c_handler.Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
	i2c_handler.Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
	i2c_handler.Init.OwnAddress2      = 0;
	i2c_handler.Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
	i2c_handler.Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;

	/* Init the I2C */
	HAL_I2C_Init(&i2c_handler);
}

