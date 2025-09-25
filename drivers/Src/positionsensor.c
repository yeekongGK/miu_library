/*
 * positionsensor.c
 *
 *  Created on: 16 Jan 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#include "common.h"
#include "positionsensor.h"
#include "lis2dh12.h"

__IO ITStatus POSSENSOR_Tilt_Interrupt= RESET;
static uint32_t POSSENSOR_Tilt_Counter= 0;
static uint32_t POSSENSOR_Tilt_TotalCounter= 0;

static bool bTiltFlag= false;/*this flag has to be manuallay cleared*/

void POSSENSOR_XYZ_Get(uint16_t *_x, uint16_t *_y, uint16_t *_z)
{
	uint8_t _regValues[6];

	for(int i= 0; i< 6; i++)
	{
		LIS2DH12_Register_ReadSingle(0x28+ i, _regValues+ i);
	}

	_x[0]= (_regValues[0]<< 8)| _regValues[1];
	_y[0]= (_regValues[2]<< 8)| _regValues[3];
	_z[0]= (_regValues[4]<< 8)| _regValues[5];
}

uint16_t POSSENSOR_X_Get(void)
{
	uint8_t _regValues[2];

	for(int i= 0; i< 2; i++)
	{
		LIS2DH12_Register_ReadSingle(0x28+ i, _regValues+ i);
	}

	return ((_regValues[0]<< 8)| _regValues[1]);
}

uint16_t POSSENSOR_Y_Get(void)
{
	uint8_t _regValues[2];

	for(int i= 0; i< 2; i++)
	{
		LIS2DH12_Register_ReadSingle(0x28+ 2+ i, _regValues+ i);
	}

	return ((_regValues[0]<< 8)| _regValues[1]);
}

uint16_t POSSENSOR_Z_Get(void)
{
	uint8_t _regValues[2];

	for(int i= 0; i< 2; i++)
	{
		LIS2DH12_Register_ReadSingle(0x28+ 2+ 2+ i, _regValues+ i);
	}

	return ((_regValues[0]<< 8)| _regValues[1]);
}

ErrorStatus POSSENSOR_6DExtended_Setup(uint8_t _armedThreshold, uint8_t _disarmedThreshold)
{
	LIS2DH12_Register_WriteSingle(0x24, 0b00001000);/*latch int1*/
	LIS2DH12_Register_WriteSingle(0x22, 0x40);
	LIS2DH12_Register_WriteSingle(0x23, 0x00);
	LIS2DH12_Register_WriteSingle(0x30, 0x7f);
	LIS2DH12_Register_WriteSingle(0x32, _armedThreshold);
	LIS2DH12_Register_WriteSingle(0x33, _disarmedThreshold);
	//LIS2DH12_Register_WriteSingle(0x32, LIS2DH12_6D_THRESHOLD);
	//LIS2DH12_Register_WriteSingle(0x33, 0x06);
	LIS2DH12_Register_WriteSingle(0x20, 0x3f);
	UTILI_usDelay(1000); /* Settling time 1 ms */

	return SUCCESS;
}

void POSSENSOR_6DExtended_Calibrate(uint8_t _armedThreshold, uint8_t _disarmedThreshold)
{
	int8_t _Acc;
	bool _isNegative;
	uint8_t _interruptMask= 0x7F;/*enable all 6D interrupt*/

	for(int i= 0; i< 3; i++)
	{
		_isNegative= false;
		LIS2DH12_Register_ReadSingle(0x28+ (i* 2)+ 1, (uint8_t*)(&_Acc));/*read OUT_AXIS_H of particular axis*/
		if(0> _Acc)
		{
			_isNegative= true;
			_Acc*= -1;/*take absolute value(remove sign)*/
		}
		if((_Acc+ _disarmedThreshold)>= _armedThreshold)
		{
			/*not enough margin, need to disable that particular axis interrupt*/
			if(false== _isNegative)
			{
				_interruptMask&= ~(0x1<<((i* 2)+ 1));
			}
			else
			{
				_interruptMask&= ~(0x1<<((i* 2)));
			}
		}
	}
	LIS2DH12_Register_WriteSingle(0x30, _interruptMask);
}

void POSSENSOR_SetupMode(LIS2DH12_Mode_Typedef _mode, uint8_t *_RWRegisters)
{
	switch(_mode)
	{
		case POSSENSOR_MODE_CUSTOM:
			LIS2DH12_Register_WriteAll(_RWRegisters);/*by default this registers array is based on LIS2DH12_SetupRemovalTilt and LIS2DH12_SetupFreeFall*/
		default:
			break;

		case POSSENSOR_MODE_6D_EXTENDED:
			LIS2DH12_Register_WriteDefault();
			POSSENSOR_6DExtended_Setup(config.sensors.position.intArmedThreshold, config.sensors.position.intDisarmedThreshold);
			POSSENSOR_6DExtended_Calibrate(config.sensors.position.intArmedThreshold, config.sensors.position.intDisarmedThreshold);
			LIS2DH12_INT1_Clear();
			break;
	}
}

void POSSENSOR_Callback_INT1(void)
{
	uint8_t _regValue= 0;
	LIS2DH12_Register_ReadSingle(0x31, &_regValue);/*read to clear if latch is enabled(reg 0x24)*/

	_regValue&= 0b00111111;
	switch(_regValue)
	{
		default:
			break;
	}

	//DBG_Print("#stat:>>>>>>>>>>>>>>>>>>>>>>>>>>>Tilt_detected\r\n");
	POSSENSOR_6DExtended_Calibrate(config.sensors.position.intArmedThreshold, config.sensors.position.intDisarmedThreshold);
	POSSENSOR_Tilt_Interrupt= SET;
}

void POSSENSOR_Callback_INT2(void)
{
	uint8_t _regValue= 0;
	LIS2DH12_Register_ReadSingle(0x35, &_regValue);/*read to clear if latch is enabled(reg 0x24)*/

	_regValue&= 0b00111111;
	switch(_regValue)
	{
		default:
			break;
	}
}

uint32_t POSSENSOR_Tilt_GetCount(void)
{
	return POSSENSOR_Tilt_TotalCounter- POSSENSOR_Tilt_Counter;
}

uint32_t POSSENSOR_Tilt_GetTotalCount(void)
{
	return POSSENSOR_Tilt_TotalCounter;
}

void POSSENSOR_Tilt_ClearCount(void)
{
	POSSENSOR_Tilt_Counter= POSSENSOR_Tilt_TotalCounter;
}

void  POSSENSOR_Tilt_ClearTotal(void)
{
	POSSENSOR_Tilt_TotalCounter= 0;
}

bool POSSENSOR_Tilt_GetFlag(void)
{
	return bTiltFlag;
}

void POSSENSOR_Tilt_SetFlag(bool _flag)
{
	bTiltFlag= _flag;
}

void POSSENSOR_Init(void)
{
	LIS2DH12_Init();
	LIS2DH12_Callback_SetINT1(POSSENSOR_Callback_INT1);
	LIS2DH12_Callback_SetINT2(POSSENSOR_Callback_INT2);
	//POSSENSOR_SetupMode(POSSENSOR_MODE_CUSTOM, config.sensors.acceleroRWRegisters);
	POSSENSOR_SetupMode(POSSENSOR_MODE_6D_EXTENDED, NULL);
}

void POSSENSOR_Task(void)
{
	LIS2DH12_Task();

	if(SET== POSSENSOR_Tilt_Interrupt)
	{
		POSSENSOR_Tilt_Interrupt= RESET;
		POSSENSOR_Tilt_TotalCounter++;
		bTiltFlag= true;
	}
}

uint8_t POSSENSOR_TaskState(void)
{
	if(true== LIS2DH12_TaskState())
	{
		return true;
	}
	return false;
}
