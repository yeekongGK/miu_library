/*
 * sewcobject.c
 *
 *  Created on: 9 Mar 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#include "common.h"
#include "lwobject.h"
#include "sensor.h"

#define DBG_Print

LWOBJ_ValueType_t LWOBJ_ExecuteResource(LWOBJ_Obj_t *pLwObj, uint16_t _resourceId)
{
	switch(pLwObj->type)
	{
		case SWMGT_ObjType:
		{

		}
		break;

		case PRACT_ObjType:
		{

		}
		break;

		case DTMON_ObjType:
		{

		}
		break;

		default:
			break;
	}
}

void LWOBJ_WakeupTask(void)
{
	SYS_Sleep(NBIOT_LWOBJ_TaskId, 10);
	DBG_Print("\r\n LWOBJ_WakeupTask\r\n");
}

void LWOBJ_Init(void)
{
	for(int i= 0; i< MAX_ObjName; i++)
	{
		switch(config.nbiot.lwm2m.lwObj[i].type)
		{
			case SWMGT_ObjType:
				{
					SWMGT_Init(&(config.nbiot.lwm2m.lwObj[i]));
				}
				break;

			case PRACT_ObjType:
				{
					PRACT_Init(&(config.nbiot.lwm2m.lwObj[i]));
				}
				break;

			case DTMON_ObjType:
				{
					DTMON_Init(&(config.nbiot.lwm2m.lwObj[i]));
				}
				break;
		}

		for(int j= 0; j< config.nbiot.lwm2m.lwObj[i].resourceCount; j++)
		{
			LWM2M_GetResource(&(config.nbiot.lwm2m.lwObj[i]), j)->observe= false;
		}
	}
}

void LWOBJ_Task(void)
{
	if(true== SYS_IsAwake(NBIOT_LWOBJ_TaskId))
	{
		__IO time_t _currTime= SYS_GetTimestamp_s();
		__IO uint64_t _currMask= UTILI_Mask_Convert(_currTime);
		uint64_t _sleepPeriod_ms;
		time_t _awakeTime= _currTime+ LWOBJ_CFG_MAX_SLEEP_TIME_S;
		time_t _objAwakeTime= _currTime+ LWOBJ_CFG_MAX_SLEEP_TIME_S;

		for(int _index= 0; _index< MAX_ObjName; _index++)
		{
			switch(config.nbiot.lwm2m.lwObj[_index].type)
			{
				case SWMGT_ObjType:
					{
						_objAwakeTime= SWMGT_Task(&(config.nbiot.lwm2m.lwObj[_index]), _currTime, _currMask);
					}
					break;

				case PRACT_ObjType:
					{
						_objAwakeTime= PRACT_Task(&(config.nbiot.lwm2m.lwObj[_index]), _currTime, _currMask);
					}
					break;

				case DTMON_ObjType:
					{
						_objAwakeTime= DTMON_Task(&(config.nbiot.lwm2m.lwObj[_index]), _currTime, _currMask);
					}
					break;

				default:
					break;
			}

			_awakeTime= UTILI_GetSmallerTime(_awakeTime, _objAwakeTime);
		}

		if(_awakeTime<= SYS_GetTimestamp_s())
		{
			_sleepPeriod_ms= 100;/*obj wants to stay awake, minimum for 10ms*/
		}
		else
		{
			//_sleepPeriod_ms= (((_awakeTime*1000)+ 100)- SYS_GetTimestamp_ms());
			_sleepPeriod_ms= (((_awakeTime*1000)+ 9)- SYS_GetTimestamp_ms());
			DBG_Print("NBIOT_LWM2M sleep: %d. \r\n", (uint32_t)_sleepPeriod_ms);
		}
		SYS_Sleep(NBIOT_LWOBJ_TaskId, _sleepPeriod_ms);
	}
}

uint8_t LWOBJ_TaskState(void)
{
	return SLEEP_TaskState;
}
