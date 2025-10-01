/*
 * alarm.c
 *
 *  Created on: 4 Oct 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

//#include "common.h"
#include "main.h"
#include "sys.h"
#include "alarm.h"

static ALARM_t *pConfig;
static ALARM_AlarmObject_t *pAlarm;
static SYS_TaskState_t eTaskState= SLEEP_TaskState;
static ALARM_AlarmOpState_t eState= RUN_AlarmOpState;

static bool ALARM_Compare(ALARM_AlarmThreshold_t _thresholdType, float _value, float _threshold)
{
	bool _result;
	switch(_thresholdType)
	{
		case NONE_AlarmThreshold:
			_result= true;
			break;

		case ABOVE_AlarmThreshold:
			_result= (_value> _threshold)? true: false;
			break;

		case ABOVE_AND_EQUAL_AlarmThreshold:
			_result= (_value>= _threshold)? true: false;
			break;

		case UNDER_AlarmThreshold:
			_result= (_value< _threshold)? true: false;
			break;

		case UNDER_AND_EQUAL_AlarmThreshold:
			_result= (_value<= _threshold)? true: false;
			break;

		case EQUAL_AlarmThreshold:
			_result= (_value== _threshold)? true: false;
			break;

		case AND_AlarmThreshold:
			_result= (_value&& _threshold)? true: false;
			break;

		case OR_AlarmThreshold:
			_result= (_value|| _threshold)? true: false;
			break;

		default:
			_result= false;
			break;
	}

	return _result;
}

uint32_t ALARM_GetActiveBitmap(void)
{
	uint32_t _flag= 0;

	for(int i= 0; i< MAX_Alarm; i++)
	{
		if(true== pAlarm[i].isActive)
		{
			_flag|= (1UL<< i);
		}
	}
	return _flag;
}

ALARM_AlarmObject_t ALARM_Get(ALARM_Alarm_t _type)
{
	return pAlarm[_type];
}

void ALARM_TLVRequest(TLV_t *_tlv)
{
	_tlv->rv[0]= SUCCESS;
	_tlv->rl= 1;

	switch(_tlv->t)
	{
		case GET_BACKOFF_PERIOD_AlarmTLVTag:
			{
				memcpy(_tlv->rv+ _tlv->rl, (uint8_t *)&(pConfig->backOffPeriod_ms), 4);
				_tlv->rl+= 4;
			}
			break;

		case SET_BACKOFF_PERIOD_AlarmTLVTag:
			{
				memcpy((uint8_t *)&(pConfig->backOffPeriod_ms), &( _tlv->v[0]), 4);
			}
			break;

		case GET_OBJECT_AlarmTLVTag:
			{
				int i= _tlv->v[0];

				_tlv->rv[_tlv->rl++]= (uint8_t)pAlarm[i].enabled;
				_tlv->rv[_tlv->rl++]= (uint8_t)pAlarm[i].sensor;
				_tlv->rv[_tlv->rl++]= (uint8_t)pAlarm[i].threshold1Type;
				memcpy(_tlv->rv+ _tlv->rl, (uint8_t *)&(pAlarm[i].threshold1), 4);
				_tlv->rl+= 4;
				_tlv->rv[_tlv->rl++]= (uint8_t)pAlarm[i].threshold12Type;
				_tlv->rv[_tlv->rl++]= (uint8_t)pAlarm[i].threshold2Type;
				memcpy(_tlv->rv+ _tlv->rl, (uint8_t *)&(pAlarm[i].threshold2), 4);
				_tlv->rl+= 4;
				memcpy(_tlv->rv+ _tlv->rl, (uint8_t *)&(pAlarm[i].debouncePeriod_ms), 4);
				_tlv->rl+= 4;
				memcpy(_tlv->rv+ _tlv->rl, (uint8_t *)&(pAlarm[i].stayActivePeriod_ms), 4);
				_tlv->rl+= 4;
			}
			break;

		case GET_ALL_OBJECTS_AlarmTLVTag:
			{
				for(int i= 0; i< MAX_Alarm; i++)
				{
					_tlv->rv[_tlv->rl++]= (uint8_t)pAlarm[i].enabled;
					_tlv->rv[_tlv->rl++]= (uint8_t)pAlarm[i].sensor;
					_tlv->rv[_tlv->rl++]= (uint8_t)pAlarm[i].threshold1Type;
					memcpy(_tlv->rv+ _tlv->rl, (uint8_t *)&(pAlarm[i].threshold1), 4);
					_tlv->rl+= 4;
					_tlv->rv[_tlv->rl++]= (uint8_t)pAlarm[i].threshold12Type;
					_tlv->rv[_tlv->rl++]= (uint8_t)pAlarm[i].threshold2Type;
					memcpy(_tlv->rv+ _tlv->rl, (uint8_t *)&(pAlarm[i].threshold2), 4);
					_tlv->rl+= 4;
					memcpy(_tlv->rv+ _tlv->rl, (uint8_t *)&(pAlarm[i].debouncePeriod_ms), 4);
					_tlv->rl+= 4;
					memcpy(_tlv->rv+ _tlv->rl, (uint8_t *)&(pAlarm[i].stayActivePeriod_ms), 4);
					_tlv->rl+= 4;
				}
			}
			break;

		case SET_OBJECT_AlarmTLVTag:
			{
				int i= _tlv->v[0];

				pAlarm[i].enabled= (bool)_tlv->v[1];
				pAlarm[i].sensor= (SENSOR_Sensor_t)_tlv->v[2];
				pAlarm[i].threshold1Type= (ALARM_AlarmThreshold_t)_tlv->v[3];
				memcpy((uint8_t *)&(pAlarm[i].threshold1), &( _tlv->v[4]), 4);
				pAlarm[i].threshold12Type= (ALARM_AlarmThreshold_t)_tlv->v[3];
				pAlarm[i].threshold2Type= (ALARM_AlarmThreshold_t)_tlv->v[3];
				memcpy((uint8_t *)&(pAlarm[i].threshold2), &( _tlv->v[4]), 4);
				memcpy((uint8_t *)&(pAlarm[i].debouncePeriod_ms), &( _tlv->v[4]), 4);
				memcpy((uint8_t *)&(pAlarm[i].stayActivePeriod_ms), &( _tlv->v[4]), 4);
			}
			break;

		default:
			_tlv->rv[0]= ERROR;
			break;
	}
}

void ALARM_Init(ALARM_t *_config)
{
	pConfig= _config;

	pAlarm= pConfig->object;
}

void ALARM_Task(void)
{
	static int i= 0;

	if(RUN_AlarmOpState== eState)
	{
		for(; i< MAX_Alarm; )
		{
			if(true== pAlarm[i].enabled)
			{
				bool _result1= ALARM_Compare(pAlarm[i].threshold1Type, SENSOR_GetValue(pAlarm[i].sensor), pAlarm[i].threshold1);
				bool _result2= ALARM_Compare(pAlarm[i].threshold2Type, SENSOR_GetValue(pAlarm[i].sensor), pAlarm[i].threshold2);
				pAlarm[i].isArmed= ALARM_Compare(pAlarm[i].threshold12Type, _result1/ 1.0, _result2/ 1.0);

				switch(pAlarm[i].state)
				{
					case IDLE_AlarmObjectState:
						if(true== pAlarm[i].isArmed)
						{
							pAlarm[i].timestamp= SYS_GetTick_ms();/*start sampling*/
							pAlarm[i].state= ARMED_AlarmObjectState;
						}
						break;

					case ARMED_AlarmObjectState:
						if(true== pAlarm[i].isArmed)/*if still, armed wait for debouncing*/
						{
							if((SYS_GetTick_ms()- pAlarm[i].timestamp)>= pAlarm[i].debouncePeriod_ms)
							{
								pAlarm[i].isActive= true;
								pAlarm[i].timestamp= SYS_GetTick_ms();
								pAlarm[i].state= ACTIVATED_AlarmObjectState;
								//DBG_Print("pAlarm[%d] activated. StatusCode:%d\r\n", i, SENSORS_GetStatusCode());
							}
						}
						else/*no longer armed, clear everything*/
						{
							pAlarm[i].state= IDLE_AlarmObjectState;
						}
						break;

					case ACTIVATED_AlarmObjectState:
						if((SYS_GetTick_ms()- pAlarm[i].timestamp)>= pAlarm[i].stayActivePeriod_ms)/*check for stay active period*/
						{
							SENSOR_ClearValue(pAlarm[i].sensor);
							pAlarm[i].state= CLEARED_AlarmObjectState;
							//DBG_Print("pAlarm[%d] cleared. StatusCode:%d\r\n", i, SENSORS_GetStatusCode());
						}
						break;

					default:
						pAlarm[i].state= IDLE_AlarmObjectState;
						break;
				}
			}
			break;
		}

		if(i++== MAX_Alarm)
		{
			i= 0;
			SYS_Sleep(ALARM_TaskId, pConfig->backOffPeriod_ms);
			eState= BACKOFF_AlarmOpState;
			eTaskState= SLEEP_TaskState;
		}
		else
		{
			eTaskState= RUN_TaskState;
		}
	}
	else if(BACKOFF_AlarmOpState== eState)
	{
		if(true== SYS_IsAwake(ALARM_TaskId))
		{
			eState= RUN_AlarmOpState;
			eTaskState= RUN_TaskState;
		}
	}
}

uint8_t ALARM_TaskState(void)
{
	return eTaskState;
}
