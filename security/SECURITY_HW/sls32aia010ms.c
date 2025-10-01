/*
 * sls32aia010ms.c
 *
 *  Created on: 19 Jan 2021
 *      Author: muhammad.ahmad@georgekent.net
 *
 *
 *      PIC: CYK
 *      TODO:
 *      	* replace ioctrl
 */

//#include "common.h"
#include "sls32aia010ms.h"
#include "i2c.h"
//#include "ioctrl.h"

static ErrorStatus SLS32AIA_Register_Write(uint8_t _reg, uint8_t *_pBuffer, uint16_t _size);
static ErrorStatus SLS32AIA_Register_Read(uint8_t _reg, uint8_t *_pBuffer, uint16_t _size);

__IO ITStatus SLS32AIA_Interrupt_ALRT= RESET;
void (*SLS32AIA_Callback_ALRT)(void)= NULL;

static ErrorStatus SLS32AIA_Register_Write(uint8_t _reg, uint8_t *_pBuffer, uint16_t _size)
{
	if(HAL_OK!= I2C2_MemWrite(SLS32AIA_CFG_I2C_ADDRESS, (uint16_t)_reg, I2C_MEMADD_SIZE_8BIT, _pBuffer, _size, SLS32AIA_CFG_I2C_TIMEOUT_MAX))
	{
		if(HAL_OK!= I2C2_MemWrite(SLS32AIA_CFG_I2C_ADDRESS, (uint16_t)_reg, I2C_MEMADD_SIZE_8BIT, _pBuffer, _size, SLS32AIA_CFG_I2C_TIMEOUT_MAX))
		{
			if(HAL_OK!= I2C2_MemWrite(SLS32AIA_CFG_I2C_ADDRESS, (uint16_t)_reg, I2C_MEMADD_SIZE_8BIT, _pBuffer, _size, SLS32AIA_CFG_I2C_TIMEOUT_MAX))
			{
				I2C2_DeInit();
				I2C2_Init();
				return ERROR;
			}
		}
	}

	return SUCCESS;
}

//static ErrorStatus SLS32AIA_Register_Read(uint8_t _reg, uint8_t *_pBuffer, uint16_t _size)
//{
//	if(HAL_OK!= I2C2_MemRead(SLS32AIA_CFG_I2C_ADDRESS, (uint16_t)_reg, I2C_MEMADD_SIZE_8BIT, _pBuffer, _size, SLS32AIA_CFG_I2C_TIMEOUT_MAX))
//	{
//		if(HAL_OK!= I2C2_MemRead(SLS32AIA_CFG_I2C_ADDRESS, (uint16_t)_reg, I2C_MEMADD_SIZE_8BIT, _pBuffer, _size, SLS32AIA_CFG_I2C_TIMEOUT_MAX))
//		{
//			if(HAL_OK!= I2C2_MemRead(SLS32AIA_CFG_I2C_ADDRESS, (uint16_t)_reg, I2C_MEMADD_SIZE_8BIT, _pBuffer, _size, SLS32AIA_CFG_I2C_TIMEOUT_MAX))
//			{
////				I2C2_DeInit();
////				I2C2_Init();
//				return ERROR;
//			}
//		}
//	}
//
//	return SUCCESS;
//}

static ErrorStatus SLS32AIA_Register_Read(uint8_t _reg, uint8_t *_pBuffer, uint16_t _size)
{
	if(HAL_OK!= I2C2_Write(SLS32AIA_CFG_I2C_ADDRESS, &_reg, 1, SLS32AIA_CFG_I2C_TIMEOUT_MAX))
	{
		//DBG_PrintLine("I2C2_Write FAILED 1");
		if(HAL_OK!= I2C2_Write(SLS32AIA_CFG_I2C_ADDRESS, &_reg, 1, SLS32AIA_CFG_I2C_TIMEOUT_MAX))
		{
			//DBG_PrintLine("I2C2_Write FAILED 2");
			if(HAL_OK!= I2C2_Write(SLS32AIA_CFG_I2C_ADDRESS, &_reg, 1, SLS32AIA_CFG_I2C_TIMEOUT_MAX))
			{
				//DBG_PrintLine("I2C2_Write FAILED 3");
//				I2C2_DeInit();
//				I2C2_Init();
//				return ERROR;
				if(HAL_OK!= I2C2_Write(SLS32AIA_CFG_I2C_ADDRESS, &_reg, 1, SLS32AIA_CFG_I2C_TIMEOUT_MAX))
				{
					//DBG_PrintLine("I2C2_Write FAILED 4");
	//				I2C2_DeInit();
	//				I2C2_Init();
	//				return ERROR;
					if(HAL_OK!= I2C2_Write(SLS32AIA_CFG_I2C_ADDRESS, &_reg, 1, SLS32AIA_CFG_I2C_TIMEOUT_MAX))
					{
		//				I2C2_DeInit();
		//				I2C2_Init();
		//				return ERROR;
						if(HAL_OK!= I2C2_Write(SLS32AIA_CFG_I2C_ADDRESS, &_reg, 1, SLS32AIA_CFG_I2C_TIMEOUT_MAX))
						{
			//				I2C2_DeInit();
			//				I2C2_Init();
			//				return ERROR;
							if(HAL_OK!= I2C2_Write(SLS32AIA_CFG_I2C_ADDRESS, &_reg, 1, SLS32AIA_CFG_I2C_TIMEOUT_MAX))
							{
				//				I2C2_DeInit();
				//				I2C2_Init();
				//				return ERROR;
								if(HAL_OK!= I2C2_Write(SLS32AIA_CFG_I2C_ADDRESS, &_reg, 1, SLS32AIA_CFG_I2C_TIMEOUT_MAX))
								{
					//				I2C2_DeInit();
					//				I2C2_Init();
					//				return ERROR;
									if(HAL_OK!= I2C2_Write(SLS32AIA_CFG_I2C_ADDRESS, &_reg, 1, SLS32AIA_CFG_I2C_TIMEOUT_MAX))
									{
						//				I2C2_DeInit();
						//				I2C2_Init();
						//				return ERROR;

										//DBG_PrintLine("I2C2_Write FAILED");
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if(HAL_OK!= I2C2_Read(SLS32AIA_CFG_I2C_ADDRESS, _pBuffer, _size, SLS32AIA_CFG_I2C_TIMEOUT_MAX))
	{
		//DBG_PrintLine("I2C2_Read FAILED 1");
		if(HAL_OK!= I2C2_Read(SLS32AIA_CFG_I2C_ADDRESS, _pBuffer, _size, SLS32AIA_CFG_I2C_TIMEOUT_MAX))
		{
			//DBG_PrintLine("I2C2_Read FAILED 2");
			if(HAL_OK!= I2C2_Read(SLS32AIA_CFG_I2C_ADDRESS, _pBuffer, _size, SLS32AIA_CFG_I2C_TIMEOUT_MAX))
			{
				//DBG_PrintLine("I2C2_Read FAILED");
				I2C2_DeInit();
				I2C2_Init();
				return ERROR;
			}
		}
	}

	//DBG_PrintLine("I2C2 Write-Read SUCCESS");
	return SUCCESS;
}

ErrorStatus SLS32AIA_Register_WriteSingle(uint8_t _reg, uint16_t _value)
{
	return SLS32AIA_Register_Write(_reg, (uint8_t *)(&_value), 2);
}

ErrorStatus SLS32AIA_Register_ReadSingle(uint8_t _reg, uint16_t *_value)
{
	*_value= 0;
	return SLS32AIA_Register_Read(_reg, (uint8_t *)_value, 2);
}

void SLS32AIA_Callback_Set(void* _callback)
{
	SLS32AIA_Callback_ALRT= _callback;
}

void SLS32AIA_Reset(void)
{
//	IOCTRL_SEReset_Enable( false);
	UTILI_usDelay( SLS32AIA_CFG_RST_LOW_TIME_US);
//	IOCTRL_SEReset_Enable( true);
	UTILI_usDelay( SLS32AIA_CFG_STARTUP_TIME_US);
}

void SLS32AIA_Init(void)
{
//	IOCTRL_SEReset_Init(true);
//	IOCTRL_SEOnOff_Init(false);
//	IOCTRL_SEOnOff_Enable(true);
	I2C2_Init();

	SLS32AIA_Reset();

	uint16_t _value= 0x0f;
	SLS32AIA_Register_ReadSingle(0x82, &_value);
	SLS32AIA_Register_ReadSingle(0x82, &_value);

//	IOCTRL_SEOnOff_Enable(false);
}

void SLS32AIA_Task(void)
{
	I2C2_Task();
}

uint8_t SLS32AIA_TaskState(void)
{
	return false;
}
