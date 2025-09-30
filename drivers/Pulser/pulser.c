/*
 * pulsecntr.c
 *
 *  Created on: 13 Jan 2021
 *      Author: muhammad.ahamd@georgekent.net
 */

#include "common.h"
#include "pulser.h"

PULSER_t *pConfig;

uint32_t PULSER_NoCallback1(void);
void PULSER_NoCallback2(int32_t _value);
PULSER_CounterDirection_t PULSER_NoCallback3(void);
void PULSER_NoCallback4(void);
uint8_t PULSER_NoCallback5(void);

uint32_t (*PULSER_GetValue)(void)= PULSER_NoCallback1;
void (*PULSER_SetValue)(int32_t)= PULSER_NoCallback2;
PULSER_CounterDirection_t (*PULSER_GetDirection)(void)= PULSER_NoCallback3;
void (*PULSER_DoTask)(void)= PULSER_NoCallback4;
uint8_t (*PULSER_GetTaskState)(void)= PULSER_NoCallback5;

uint32_t PULSER_NoCallback1(void)
{
	return 0;
}

void PULSER_NoCallback2(int32_t _value)
{
}

PULSER_CounterDirection_t PULSER_NoCallback3(void)
{
	return FORWARD_CounterDirection;
}

void PULSER_NoCallback4(void)
{
}

uint8_t PULSER_NoCallback5(void)
{
	return SLEEP_TaskState;
}

PULSER_Mode_t PULSER_Mode_Get(void)
{
	return pConfig->mode;
}

uint32_t PULSER_Value_Get(void)
{
	return PULSER_GetValue();
}

void PULSER_Value_Set(int32_t _value)
{
	PULSER_SetValue(_value);
	LL_RTC_BAK_SetRegister(RTC, PULSER_VALUE_BKPReg, _value);
}

float PULSER_ValueInLiter_Get(void)
{
	return PULSER_ConvertToMeterReading(PULSER_Value_Get());
}

void PULSER_ValueInLiter_Set(float _value)
{
	PULSER_Value_Set(PULSER_ConvertToPulseValue(_value));
}

float PULSER_PrevValueInLiter_Get(void)
{
	return pConfig->rtePrevValue_liter;
}

void PULSER_PrevValueInLiter_Set(float _value)
{
	pConfig->rtePrevValue_liter= _value;
}

float PULSER_PrevValue2InLiter_Get(void)
{
	return pConfig->rtePrevValue2_liter;
}

void PULSER_PrevValue2InLiter_Set(float _value)
{
	pConfig->rtePrevValue2_liter= _value;
}

PULSER_CounterDirection_t PULSER_Direction(void)
{
	return PULSER_GetDirection();
}

float PULSER_ConvertToMeterReading(uint32_t _pulseValue)
{
	int32_t _pulse= (int32_t)(_pulseValue);
	float _readingInLiter= _pulse* pConfig->weight_liter;

	return _readingInLiter;
}

int32_t PULSER_ConvertToPulseValue(float _meterReading)
{
	int32_t _pulseValue= (int32_t)(_meterReading/ pConfig->weight_liter);

	return _pulseValue;
}

void PULSER_TLVRequest(TLV_t *_tlv)
{
	_tlv->rv[0]= SUCCESS;
	_tlv->rl= 1;

	switch(_tlv->t)
	{
		case GET_PARAMS_PulserTLVTag:
			{
				_tlv->rv[_tlv->rl++]= (uint8_t)pConfig->mode;
				memcpy(_tlv->rv+ _tlv->rl, (uint8_t *)&(pConfig->weight_liter), 4);
				_tlv->rl+= 4;
			}
			break;

		case SET_PARAMS_PulserTLVTag:
			{
				PULSER_Mode_t _prevMode= pConfig->mode;
				pConfig->mode= (PULSER_Mode_t)_tlv->v[0];
				memcpy((uint8_t *)&(pConfig->weight_liter), &( _tlv->v[1]), 4);
				if(_prevMode!= pConfig->mode)
				{
					//PULSER_Init(&(config.pulser));/*not sure consequence of this*/
					SYS_Request(SOFT_REBOOT_SysRequest);
				}
				//LCSENS_StaticCalibration();
			}
			break;

		case GET_ERRPATT_PARAMS_PulserTLVTag:
			{
				_tlv->rv[_tlv->rl++]= (uint8_t)pConfig->tracsens.enableErrorPatternCheck;
				_tlv->rv[_tlv->rl++]= (uint8_t)pConfig->tracsens.useCompensatedValue;
				memcpy(_tlv->rv+ _tlv->rl, (uint8_t *)&(pConfig->tracsens.errorPatternConfirmationCount), 2);
				_tlv->rl+= 2;
			}
			break;

		case SET_ERRPATT_PARAMS_PulserTLVTag:
			{
				pConfig->tracsens.enableErrorPatternCheck= (bool)_tlv->v[0];
				pConfig->tracsens.useCompensatedValue= (bool)_tlv->v[1];
				memcpy((uint8_t *)&(pConfig->tracsens.errorPatternConfirmationCount), &( _tlv->v[2]), 2);
			}
			break;

		case GET_ERRPATT_COUNT_PulserTLVTag:
			{
				memcpy(_tlv->rv+ _tlv->rl, (uint8_t *)&(pConfig->tracsens.rteErrorPatternCount), 4);
				_tlv->rl+= 4;
			}
			break;

		case SET_ERRPATT_COUNT_PulserTLVTag:
			{
				memcpy((uint8_t *)&(pConfig->tracsens.rteErrorPatternCount), &( _tlv->v[0]), 4);
			}
			break;

		case SET_ERRPATT_USE_COMPENSATED_VALUE_PulserTLVTag:
			{
				pConfig->tracsens.useCompensatedValue= (bool)_tlv->v[0];
			}
			break;

		case GET_PULSE_PulserTLVTag:
		{
			_tlv->rv[_tlv->rl++]= _tlv->v[0];
			switch(_tlv->v[0])
			{
				case 0:
					{
						float _reading= PULSER_ValueInLiter_Get();
						memcpy(_tlv->rv+ _tlv->rl, (uint8_t *)&(_reading), 4);
						_tlv->rl+= 4;
					}
					break;
				case 1:
					{
						int32_t _counter= PULSER_Value_Get();
						memcpy(_tlv->rv+ _tlv->rl, (uint8_t *)&(_counter), 4);
						_tlv->rl+= 4;
					}
					break;
				default:
					_tlv->rv[0]= ERROR;
					break;
			}
		}
		break;

		case SET_PULSE_PulserTLVTag:
		{
			_tlv->rv[_tlv->rl++]= _tlv->v[0];
			switch(_tlv->v[0])
			{
				case 0:
					{
						float _reading;
						memcpy((uint8_t *)&(_reading), &( _tlv->v[1]), 4);
						PULSER_ValueInLiter_Set(_reading);
					}
					break;
				case 1:
					{
						float _adjustment;
						memcpy((uint8_t *)&(_adjustment), &( _tlv->v[1]), 4);
						PULSER_ValueInLiter_Set(PULSER_ValueInLiter_Get()+ _adjustment);
					}
					break;
				case 2:
					{
						int32_t _counter;
						memcpy((uint8_t *)&(_counter), &( _tlv->v[1]), 4);
						PULSER_Value_Set(_counter);
					}
					break;
				case 3:
					{
						int32_t _adjustment;
						memcpy((uint8_t *)&(_adjustment), &( _tlv->v[1]), 4);
						PULSER_Value_Set(PULSER_Value_Get()+ _adjustment);
					}
					break;
				default:
					_tlv->rv[0]= ERROR;
					break;
			}
		}
		break;

		default:
			_tlv->rv[0]= ERROR;
			break;
	}
}

void PULSER_Save(void)
{
	LL_RTC_BAK_SetRegister(RTC, PULSER_VALUE_BKPReg, PULSER_Value_Get());
	LL_RTC_BAK_SetRegister(RTC, PULSER_ERRPATT_CNT_BKPReg, pConfig->tracsens.rteErrorPatternCount);/*this also need to be saved at the very last minute*/
}

void PULSER_Restore(void)
{
	uint32_t _pulserValue= LL_RTC_BAK_GetRegister(RTC, PULSER_VALUE_BKPReg);
	uint32_t _errPatternCount= LL_RTC_BAK_GetRegister(RTC, PULSER_ERRPATT_CNT_BKPReg);

	if(0!= _pulserValue)
	{
		pConfig->tracsens.rteLastSavedValue= _pulserValue;
		LL_RTC_BAK_SetRegister(RTC, PULSER_VALUE_BKPReg, 0);
	}

	if(0!= _errPatternCount)
	{
		pConfig->tracsens.rteErrorPatternCount= _errPatternCount;
		LL_RTC_BAK_SetRegister(RTC, PULSER_ERRPATT_CNT_BKPReg, 0);
	}

	PULSER_Value_Set(pConfig->tracsens.rteLastSavedValue);
}

void PULSER_Init(PULSER_t *_config)
{
	pConfig= _config;

	switch(pConfig->mode)
	{
		case TRACSENS_Mode:
		case TRACSENSi_Mode:
			PULSER_GetValue= TRACSENS_GetValue;
			PULSER_SetValue= TRACSENS_SetValue;
			PULSER_GetDirection= TRACSENS_GetDirection;
			//PULSER_DoTask= ;
			//PULSER_GetTaskState= ;

			TRACSENS_Init(&(pConfig->tracsens));
			TRACSENS_StartCounting();
			break;

		case LCSENS_Mode:
//			PULSER_GetValue= LCSENS_GetValue;
//			PULSER_SetValue= LCSENS_SetValue;
//			PULSER_GetDirection= LCSENS_GetDirection;
//			PULSER_DoTask= LCSENS_Task;
//			PULSER_GetTaskState= LCSENS_TaskState;
//
//			LCSENS_Init(&(pConfig->lcsens));
//			LCSENS_StartCounting();
//			break;

		case ELSTER_Mode:
			PULSER_GetValue= ELSTER_GetValue;
			PULSER_SetValue= ELSTER_SetValue;
			PULSER_GetDirection= ELSTER_GetDirection;

			ELSTER_Init(&(pConfig->elster));
			ELSTER_StartCounting();
			break;

		default:
			break;
	}

	PULSER_Restore();
}

void PULSER_Task(void)
{
	PULSER_DoTask();
}

uint8_t PULSER_TaskState(void)
{
	return PULSER_GetTaskState();
}
