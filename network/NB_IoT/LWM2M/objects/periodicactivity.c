/*
 * periodicactivity.c
 *
 *  Created on: 28 Oct 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#include "common.h"
#include "lwm2m.h"
#include "cbor.h"
#include "lwobject.h"
#include "sensor.h"
#include "periodicactivity.h"
#include "msg.h"

#define DBG_Print

void PRACT_Diag(PRACT_PractInstance_t _instance, PRACT_PractDiagCode_t _code, uint32_t _value)
{
	switch(_instance)
	{
		case GET_READING_PractInstance:
			DIAG_Code((NEXT_RUN_TIME_PractDiagCode== _code)?READING_NEXT_RUN_TIME_NbiotLwm2mDCode: READING_NEXT_DISPATCH_TIME_NbiotLwm2mDCode, _value);
			break;
		case GET_STATUS_PractInstance:
			DIAG_Code((NEXT_RUN_TIME_PractDiagCode== _code)?STATUS_NEXT_RUN_TIME_NbiotLwm2mDCode: STATUS_NEXT_DISPATCH_TIME_NbiotLwm2mDCode, _value);
			break;
		case QUERY_PractInstance:
			DIAG_Code((NEXT_RUN_TIME_PractDiagCode== _code)?QUERY_NEXT_RUN_TIME_NbiotLwm2mDCode: QUERY_NEXT_DISPATCH_TIME_NbiotLwm2mDCode, _value);
			break;
	}
}

uint16_t PRACT_RunActivity(LWOBJ_Obj_t *pLwObj, uint32_t _index, uint8_t *_record)
{
	uint16_t _recordLen= 0;

	switch(pLwObj->instance)
	{
		case GET_READING_PractInstance:
			{
				PRACT_GetReadingRecord_t *_pRecord= (PRACT_GetReadingRecord_t *)_record;
				_pRecord->index= _index;
				_pRecord->timestamp= (uint32_t) SYS_GetTimestampUTC_s();
				_pRecord->meterReading= PULSER_ValueInLiter_Get();
				_pRecord->meterConsumption= _pRecord->meterReading- PULSER_PrevValueInLiter_Get();
				PULSER_PrevValueInLiter_Set(_pRecord->meterReading);
				if(true== config.nbiot.lwm2m.useCellTemperature)
				{
					_pRecord->temperature= (int8_t)SENSOR_GetValue(CELLTEMPERATURE_Sensor);
				}
				else
				{
					_pRecord->temperature= (int8_t)SENSOR_GetValue(INTERNAL_TEMPERATURE_Sensor);
				}
				_pRecord->batteryLevel_percent= SENSOR_GetValue(CELLCAPACITY_PERCENTAGE_Sensor);
				_recordLen= sizeof(PRACT_GetReadingRecord_t);
			}
			break;

		case GET_STATUS_PractInstance:
			{
				PRACT_GetStatusRecord_t *_pRecord= (PRACT_GetStatusRecord_t *)_record;
				_pRecord->index= _index;
				_pRecord->timestamp= (uint32_t) SYS_GetTimestampUTC_s();
				_pRecord->stats= config.nbiot.stats;
				_recordLen= sizeof(PRACT_GetStatusRecord_t);
			}
			break;

		case QUERY_PractInstance:
			{
				PRACT_QueryRecord_t *_pRecord= (PRACT_QueryRecord_t *)_record;
				_pRecord->index= _index;
				_pRecord->timestamp= (uint32_t) SYS_GetTimestampUTC_s();

				MSG_t _msg= {0};
				_msg.taskId= MSG_TaskId;
				_msg.buffer= (uint8_t *)&(pLwObj->resource[SETTINGS_PractResource].value.opaque);
				_msg.bufferLen= pLwObj->resource[SETTINGS_PractResource].valueLen;
				_msg.sendResponse= true;
				_msg.responseTaskId= NBIOT_LWOBJ_TaskId;
				MSG_SyncTask(&_msg);
				_pRecord->datalen= (_msg.bufferLen<= PRACT_CFG_2_SETTINGS_RESOURCE_MAX_LEN)?_msg.bufferLen: PRACT_CFG_2_SETTINGS_RESOURCE_MAX_LEN;
				memcpy(_pRecord->data, _msg.buffer, _pRecord->datalen);
				_recordLen= sizeof(PRACT_QueryRecord_t);
			}
			break;

		default:
			break;
	}

	return _recordLen;
}

void PRACT_StopActivity(LWOBJ_Obj_t *pLwObj)
{
}

void PRACT_RefreshResources(LWOBJ_Obj_t *pLwObj)
{
	pLwObj->rte.prAct->startMask= UTILI_Mask_Decode(pLwObj->resource[START_MASK_PractResource].value.string);
	pLwObj->rte.prAct->recordDispatchStartMask= UTILI_Mask_Decode(pLwObj->resource[RECORD_DISPATCH_START_MASK_PractResource].value.string);

	if(WAIT_START_PractRunState< pLwObj->rte.prAct->runState)
	{
		/*already running, we need to adjust next run time*/
		pLwObj->rte.prAct->nextRunTime= UTILI_ComputeNextTime(SYS_GetTimestamp_s(), pLwObj->rte.prAct->startTime, pLwObj->resource[PERIODIC_INTERVAL_PractResource].value.integer);
		DBG_Print("NextRun [%d]: %s\r\n", pLwObj->instance, asctime(localtime(&(pLwObj->rte.prAct->nextRunTime))));
		PRACT_Diag(pLwObj->instance, NEXT_RUN_TIME_PractDiagCode, pLwObj->rte.prAct->nextRunTime);
	}
	else
	{
		/*re-start*/
		pLwObj->rte.prAct->runState= STOPPED_PractRunState;
	}

	if(WAIT_START_PractDispatchState< pLwObj->rte.prAct->dispatchState)
	{
		/*already running, we need to adjust next dispatch time*/
		pLwObj->rte.prAct->nextRecordDistpatchTime= UTILI_ComputeNextTime(SYS_GetTimestamp_s(), pLwObj->rte.prAct->recordDistpatchStartTime, pLwObj->resource[RECORD_DISPATCH_PERIODIC_INTERVAL_PractResource].value.integer);
		DBG_Print("NextDispatch [%d]: %s\r\n", pLwObj->instance, asctime(localtime(&(pLwObj->rte.prAct->nextRecordDistpatchTime))));
		PRACT_Diag(pLwObj->instance, NEXT_DISPATCH_TIME_PractDiagCode, pLwObj->rte.prAct->nextRecordDistpatchTime);
	}
	else
	{
		/*re-start*/
		pLwObj->rte.prAct->dispatchState= STOPPED_PractDispatchState;
	}

}

void PRACT_NotifyResource(LWOBJ_Obj_t *pLwObj, uint8_t _resourceNo, bool _force)
{
	if((false== _force) && (false== pLwObj->resource[_resourceNo].observe))
	{
		return;
	}
	else
	{
		pLwObj->resource[_resourceNo].notifyState= DO_NOTIFY_NotifyState;
	}
}

void PRACT_Init(LWOBJ_Obj_t *pLwObj)
{
	int _instance= pLwObj->instance;

	/*read only*/
	pLwObj->resource[NAME_PractResource].value.string= (char *)PRACT_TEXT[_instance][0];
	pLwObj->resource[NAME_PractResource].valueLen= strlen(PRACT_TEXT[_instance][0]);
	pLwObj->resource[DESC_PractResource].value.string= (char *)PRACT_TEXT[_instance][1];
	pLwObj->resource[DESC_PractResource].valueLen= strlen(PRACT_TEXT[_instance][1]);
	/*read write*/
	pLwObj->resource[START_MASK_PractResource].valueLen= strlen(pLwObj->resource[START_MASK_PractResource].value.string);
	pLwObj->resource[RECORD_DISPATCH_START_MASK_PractResource].valueLen= strlen(pLwObj->resource[RECORD_DISPATCH_START_MASK_PractResource].value.string);

	PRACT_RefreshResources(pLwObj);
}

time_t PRACT_Task(LWOBJ_Obj_t *pLwObj, time_t _currTime, uint64_t _currMask)
{
	time_t _awakeTime= _currTime+ LWOBJ_CFG_MAX_SLEEP_TIME_S;

	if(true== pLwObj->written)
	{
		pLwObj->written= false;
		PRACT_RefreshResources(pLwObj);
	}

	switch(pLwObj->rte.prAct->runState)
	{
		case STOPPED_PractRunState:
			pLwObj->rte.prAct->startTime= UTILI_Mask_GetMatchedTime(_currTime, pLwObj->rte.prAct->startMask);
			pLwObj->rte.prAct->runState= WAIT_START_PractRunState;
			DBG_Print("StartTime [%d]: %s\r\n", pLwObj->instance, asctime(localtime(&(pLwObj->rte.prAct->startTime))));
			PRACT_Diag(pLwObj->instance, NEXT_RUN_TIME_PractDiagCode, pLwObj->rte.prAct->startTime);

		case WAIT_START_PractRunState:
			if(_currTime< pLwObj->rte.prAct->startTime)
			{
				_awakeTime= UTILI_GetSmallerTime(_awakeTime, pLwObj->rte.prAct->startTime);
				break;
			}
			pLwObj->rte.prAct->nextRunTime= pLwObj->rte.prAct->startTime;
			pLwObj->rte.prAct->stopTime= pLwObj->rte.prAct->startTime+ pLwObj->resource[RUN_PERIOD_PractResource].value.integer;
			pLwObj->rte.prAct->runState= STARTED_PractRunState;
			DBG_Print("stopTime [%d]: %s\r\n", pLwObj->instance, asctime(localtime(&(pLwObj->rte.prAct->stopTime))));

		case STARTED_PractRunState:
			if(_currTime>= pLwObj->rte.prAct->nextRunTime)
			{
				pLwObj->rte.prAct->nextRunTime= UTILI_ComputeNextTime(_currTime, pLwObj->rte.prAct->startTime, pLwObj->resource[PERIODIC_INTERVAL_PractResource].value.integer);
				DBG_Print("PrAct Run [%d].\r\n", pLwObj->instance);

				/*append record*/
				uint8_t _record[sizeof(PRACT_GetStatusRecord_t)];/*use largest record for buffer*/
				PRACT_RunActivity(pLwObj, pLwObj->resource[RECORD_HEAD_PractResource].value.integer, _record);
				LOGGER_WriteSync(&(pLwObj->rte.prAct->log), (void *)&_record);
				pLwObj->resource[RECORD_HEAD_PractResource].value.integer++;
				//PRACT_NotifyResource(pLwObj, RECORD_PractResource, false);
				PRACT_NotifyResource(pLwObj, RECORD_HEAD_PractResource, false);

				uint32_t _diff= pLwObj->resource[RECORD_HEAD_PractResource].value.integer- pLwObj->resource[RECORD_TAIL_PractResource].value.integer;
				if(pLwObj->rte.prAct->log.elementMax< _diff)
				{
					pLwObj->resource[RECORD_TAIL_PractResource].value.integer++;
					PRACT_NotifyResource(pLwObj, RECORD_TAIL_PractResource, false);

					if(pLwObj->resource[RECORD_READ_PractResource].value.integer< pLwObj->resource[RECORD_TAIL_PractResource].value.integer)
					{
						pLwObj->resource[RECORD_READ_PractResource].value.integer= pLwObj->resource[RECORD_TAIL_PractResource].value.integer;
						PRACT_NotifyResource(pLwObj, RECORD_READ_PractResource, false);
					}
				}
			}
			_awakeTime= UTILI_GetSmallerTime(_awakeTime, pLwObj->rte.prAct->nextRunTime);

			if(_currTime>= pLwObj->rte.prAct->stopTime)
			{
				DBG_Print("PrAct Stop [%d]. \r\n", pLwObj->instance);
				PRACT_StopActivity(pLwObj);
				pLwObj->rte.prAct->runState= STOPPED_PractRunState;
				_awakeTime= _currTime+ LWOBJ_CFG_MIN_SLEEP_TIME_S;/*to immediately start next cycle to check for start condition*/
			}
			_awakeTime= UTILI_GetSmallerTime(_awakeTime, pLwObj->rte.prAct->stopTime);
			break;

		default:
			break;
	}

	switch(pLwObj->rte.prAct->dispatchState)
	{
		case STOPPED_PractDispatchState:
			pLwObj->rte.prAct->recordDistpatchStartTime= UTILI_Mask_GetMatchedTime(_currTime, pLwObj->rte.prAct->recordDispatchStartMask);
			pLwObj->rte.prAct->dispatchState= WAIT_START_PractDispatchState;
			DBG_Print("StartTime [%d]: %s\r\n", pLwObj->instance, asctime(localtime(&(pLwObj->rte.prAct->startTime))));
			PRACT_Diag(pLwObj->instance, NEXT_RUN_TIME_PractDiagCode, pLwObj->rte.prAct->startTime);

		case WAIT_START_PractDispatchState:
			if(_currTime< pLwObj->rte.prAct->recordDistpatchStartTime)
			{
				_awakeTime= UTILI_GetSmallerTime(_awakeTime, pLwObj->rte.prAct->recordDistpatchStartTime);
				break;
			}
			pLwObj->rte.prAct->nextRecordDistpatchTime= pLwObj->rte.prAct->recordDistpatchStartTime;
			pLwObj->rte.prAct->dispatchState= STARTED_PractDispatchState;

		case STARTED_PractDispatchState:
			if(_currTime>= pLwObj->rte.prAct->nextRecordDistpatchTime)
			{
				pLwObj->rte.prAct->nextRecordDistpatchTime= UTILI_ComputeNextTime(_currTime, pLwObj->rte.prAct->recordDistpatchStartTime, pLwObj->resource[RECORD_DISPATCH_PERIODIC_INTERVAL_PractResource].value.integer);
				DBG_Print("Dispatch,Next [%d]: %s\r\n", pLwObj->instance, asctime(localtime(&(pLwObj->rte.prAct->nextRecordDistpatchTime))));
				PRACT_Diag(pLwObj->instance, NEXT_DISPATCH_TIME_PractDiagCode, pLwObj->rte.prAct->nextRecordDistpatchTime);

				PRACT_NotifyResource(pLwObj, RECORD_PractResource, false);
			}
			_awakeTime= UTILI_GetSmallerTime(_awakeTime, pLwObj->rte.prAct->nextRecordDistpatchTime);

			break;

		default:
			break;
	}

	return _awakeTime;
}


