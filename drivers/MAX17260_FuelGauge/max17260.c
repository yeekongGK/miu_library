/*
 * max17260.c
 *
 *  Created on: 19 Jan 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#include "common.h"
#include "max17260.h"
#include "i2c1.h"

static ErrorStatus MAX17260_Register_Write(uint8_t _reg, uint8_t *_pBuffer, uint16_t _size);
static ErrorStatus MAX17260_Register_Read(uint8_t _reg, uint8_t *_pBuffer, uint16_t _size);

__IO ITStatus MAX17260_Interrupt_ALRT= RESET;
void (*MAX17260_Callback_ALRT)(void)= NULL;

static ErrorStatus MAX17260_Register_Write(uint8_t _reg, uint8_t *_pBuffer, uint16_t _size)
{
	if(HAL_OK!= I2C1_MemWrite(MAX17260_CFG_I2C_ADDRESS, (uint16_t)_reg, I2C_MEMADD_SIZE_8BIT, _pBuffer, _size, MAX17260_CFG_I2C_TIMEOUT_MAX))
	{
		if(HAL_OK!= I2C1_MemWrite(MAX17260_CFG_I2C_ADDRESS, (uint16_t)_reg, I2C_MEMADD_SIZE_8BIT, _pBuffer, _size, MAX17260_CFG_I2C_TIMEOUT_MAX))
		{
			if(HAL_OK!= I2C1_MemWrite(MAX17260_CFG_I2C_ADDRESS, (uint16_t)_reg, I2C_MEMADD_SIZE_8BIT, _pBuffer, _size, MAX17260_CFG_I2C_TIMEOUT_MAX))
			{
				I2C1_DeInit();
				I2C1_Init();
				return ERROR;
			}
		}
	}

	return SUCCESS;
}

static ErrorStatus MAX17260_Register_Read(uint8_t _reg, uint8_t *_pBuffer, uint16_t _size)
{
	if(HAL_OK!= I2C1_MemRead(MAX17260_CFG_I2C_ADDRESS, (uint16_t)_reg, I2C_MEMADD_SIZE_8BIT, _pBuffer, _size, MAX17260_CFG_I2C_TIMEOUT_MAX))
	{
		if(HAL_OK!= I2C1_MemRead(MAX17260_CFG_I2C_ADDRESS, (uint16_t)_reg, I2C_MEMADD_SIZE_8BIT, _pBuffer, _size, MAX17260_CFG_I2C_TIMEOUT_MAX))
		{
			if(HAL_OK!= I2C1_MemRead(MAX17260_CFG_I2C_ADDRESS, (uint16_t)_reg, I2C_MEMADD_SIZE_8BIT, _pBuffer, _size, MAX17260_CFG_I2C_TIMEOUT_MAX))
			{
				I2C1_DeInit();
				I2C1_Init();
				return ERROR;
			}
		}
	}

	return SUCCESS;
}

ErrorStatus MAX17260_Register_WriteSingle(uint8_t _reg, uint16_t _value)
{
	return MAX17260_Register_Write(_reg, (uint8_t *)(&_value), 2);
}

ErrorStatus MAX17260_Register_ReadSingle(uint8_t _reg, uint16_t *_value)
{
	*_value= 0;
	return MAX17260_Register_Read(_reg, (uint8_t *)_value, 2);
}

uint16_t MAX17260_Register_ReadSingleFast(uint8_t _reg)
{
	uint16_t _value= 0xFFFF;
	MAX17260_Register_Read(_reg, (uint8_t *)(&_value), 2);
	return _value;
}

void MAX17260_Callback_Set(void* _callback)
{
	MAX17260_Callback_ALRT= _callback;
}

void MAX17260_Init(void)
{
	static bool isInitialized= false;

	if(true== isInitialized)
	{
		return;
	}

	isInitialized= true;

	I2C1_Init();

	LL_EXTI_InitTypeDef EXTI_InitStruct;

	/*ALRT PIN*/
	SYS_EnablePortClock(FUEL_GAUGE_ALRT_GPIO_Port);

	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
	LL_SYSCFG_SetEXTISource(FUEL_GAUGE_ALRT_SYSCFG_EXTI_Port, FUEL_GAUGE_ALRT_SYSCFG_EXTI_Line);

	LL_GPIO_SetPinPull(FUEL_GAUGE_ALRT_GPIO_Port, FUEL_GAUGE_ALRT_Pin, LL_GPIO_PULL_NO);
	LL_GPIO_SetPinMode(FUEL_GAUGE_ALRT_GPIO_Port, FUEL_GAUGE_ALRT_Pin, LL_GPIO_MODE_INPUT);

	EXTI_InitStruct.Line_0_31= FUEL_GAUGE_ALRT_EXTI_Line;
	EXTI_InitStruct.LineCommand= ENABLE;
	EXTI_InitStruct.Mode= LL_EXTI_MODE_IT;
	EXTI_InitStruct.Trigger= LL_EXTI_TRIGGER_FALLING;
	LL_EXTI_Init(&EXTI_InitStruct);

	NVIC_SetPriority(FUEL_GAUGE_ALRT_EXTI_IRQn, SYS_CFG_SENSOR_PRIORITY);
	NVIC_EnableIRQ(FUEL_GAUGE_ALRT_EXTI_IRQn);
}

void MAX17260_Task(void)
{
	if(SET== MAX17260_Interrupt_ALRT)
	{
		MAX17260_Interrupt_ALRT= RESET;

		if(NULL!= MAX17260_Callback_ALRT)
		{
			MAX17260_Callback_ALRT();
		}
	}
}

uint8_t MAX17260_TaskState(void)
{
	return false;
}
