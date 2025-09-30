/*
 * meterlog.c
 *
 *  Created on: 13 Jan 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#include "common.h"
#include "devicelog.h"
#include "m95m01.h"
#include "sensor.h"
#include "pulser.h"
#include "rtc.h"
#include "rtcalarm.h"
#include <time.h>

static void DEVICELOG_Construct(DEVICELOG_Log_t *_log);
static void DEVICELOG_PeriodicLogTask(void);
static void DEVICELOG_TamperLogTask(void);

__IO static uint8_t pucWriteBuf[DEVICELOG_CFG_WRITE_BUFF_SIZE];/*Logs are stored in RAM for faster access. When overflowed logs are pushed to EEPROM*/

static bool bPeriodicLogEnabled= false;
static bool bTamperLogEnabled= false;

static uint8_t ucPrevStatusCode;
static bool bWaitForStartTick= true;
static bool bIsArmed= false;

static RTC_TickType_t eTickType= HOUR_TickType;
static RTC_AlarmStartMarker_t eStartTime;
static uint32_t ulTickSize= 1;
static uint32_t ulTickSize_s= 0;

__IO static DEVICELOG_Log_t	eDeviceLog;
__IO static DEVICELOG_Rte_t *pRte;


__IO static M95M01_Transaction_t		eNoTrx= {
								.type=  M95M01_TYPE_WRITE,
								.state.write= M95M01_WRITE_STATE_IDLE,
								.partition= M95M01_PARTITION_NONE,
								.dataPtr= NULL,
								.dataLen= 0,
								.dataAddress= 0,
								.timeout= 0,
								.retryCounter= 0,
								.status= M95M01_STATUS_SUCCESS,
								.transactionInQueue= false,
								.transactionInProgress= false,
								.transactionCompleted= false,
								.TransactionCompletedCb= NULL,
							};
/*sTrx is use to perform Periodic and Tamper log transaction(DeviceLog Application)*/
__IO static DEVICELOG_Trx_t sTrx= {
		.M95M01Trx.timeout= DEVICELOG_CFG_EEPROM_TIMEOUT,
		.M95M01Trx.retryCounter= DEVICELOG_CFG_MAX_RETRY_COUNT,
		.M95M01Trx.TransactionCompletedCb= DEVICELOG_AppWrite_Callback,
};
//M95M01_Transaction_t 	eM95M01AppTrx= {   		.timeout= DEVICELOG_CFG_EEPROM_TIMEOUT,
//												.retryCounter= DEVICELOG_CFG_MAX_RETRY_COUNT,
//												.TransactionCompletedCb= DEVICELOG_AppWrite_Callback};
M95M01_Transaction_t 	*pM95M01WriteTrx= &eNoTrx;
M95M01_Transaction_t 	*pM95M01ReadTrx= &eNoTrx;

static void DEVICELOG_Construct(DEVICELOG_Log_t *_log)
{
	_log->timestamp= UTILI_sTimeStamp();
	_log->meterReading= PULSER_ValueInLiter_Get();
	_log->pbmagCount= (SENSOR_GetValue(PBMAGCOUNT_Sensor)> 0xFF? 0xFF: SENSOR_GetValue(PBMAGCOUNT_Sensor));
	_log->tamperInCount= (SENSOR_GetValue(TAMPERINCOUNT_Sensor)> 0xFF? 0xFF: SENSOR_GetValue(TAMPERINCOUNT_Sensor));
	_log->rsTamperCount= (SENSOR_GetValue(RSTAMPERCOUNT_Sensor)> 0xFF? 0xFF: SENSOR_GetValue(RSTAMPERCOUNT_Sensor));
	_log->voltage= SENSOR_GetValue(INTERNAL_VOLTAGE_Sensor);
	_log->temperature= SENSOR_GetValue(INTERNAL_TEMPERATURE_Sensor);
	_log->cellCapacity= SENSOR_GetValue(CELLCAPACITY_Sensor);
	_log->aveCurrent= SENSOR_GetValue(AVECURRENT_Sensor);
	_log->positionX= SENSOR_GetValue(POSITION_X_Sensor);
	_log->positionY= SENSOR_GetValue(POSITION_Y_Sensor);
	_log->positionZ= SENSOR_GetValue(POSITION_Z_Sensor);
	_log->flowRate= SENSOR_GetValue(PIPE_HOURLYFLOWRATE_Sensor);
	_log->backflowReading= SENSOR_GetValue(PIPE_BACKFLOW_Sensor);
	_log->statusByte= SENSORS_GetStatusCode();
}

void DEVICELOG_AppWrite_Callback(void)
{
	if(DeviceLog_TrxType== sTrx.type)
	{
		pRte->logCount+= 1;
		pRte->periodicLogCount+= 1;

		/*don't want to log this too frequent*/
		uint32_t _spacer= 1;
		switch(config.nbiot.gkcoap.logTickType)
		{
			case 0:
				_spacer= 600;
				break;
			case 1:
				_spacer= 60;
				break;
			case 2:
				_spacer= 6;
				break;

		}
		if(1== (pRte->periodicLogCount% _spacer))
		{
			DIAG_Code(METER_LOG_MeterLogDCode, pRte->periodicLogCount);
		}
	}
	else if(TamperLog_TrxType== sTrx.type)
	{
		pRte->logCount+= 1;
		pRte->tamperLogCount+= 1;
	}

	//DBG_Print("DEVICELOG_pRte->logCount: %d. PeriodicLog:% d. TamperLog:% d.\r\n", pRte->logCount, pRte->periodicLogCount, pRte->tamperLogCount);

    pRte->logsInEEPROMCeilingIndex+= 1;
    pRte->logsInEEPROMFloorIndex= ((pRte->logsInEEPROMCeilingIndex> DEVICELOG_CFG_MAX_LOGS_IN_EEPROM)? pRte->logsInEEPROMCeilingIndex- DEVICELOG_CFG_MAX_LOGS_IN_EEPROM: 0);
    pRte->currentEEPROMAddress= (pRte->currentEEPROMAddress+ sizeof(DEVICELOG_Log_t)); /*overflow is automatically rollover in eeprom job*/
}

ErrorStatus DEVICELOG_WriteLog(M95M01_Transaction_t *_trx, DEVICELOG_Log_t _log)
{
	return DEVICELOG_WriteLogs(_trx, &_log, 1);
}

ErrorStatus DEVICELOG_WriteLogs(M95M01_Transaction_t *_trx, DEVICELOG_Log_t _log[], uint16_t _logCount)
{
	if(
		(true== pM95M01WriteTrx->transactionInProgress)
		||(true== pM95M01WriteTrx->transactionInQueue)
	)
	{
		return ERROR;
	}

	uint16_t _len= (_logCount* sizeof(DEVICELOG_Log_t));
	if(_len> sizeof(pucWriteBuf))
	{
		return ERROR;
	}
	else
	{
		memcpy(pucWriteBuf, _log, _len);
	}

	pM95M01WriteTrx= _trx;
	pM95M01WriteTrx->type= M95M01_TYPE_WRITE;
	pM95M01WriteTrx->partition= M95M01_PARTITION_DEVICELOG;
	pM95M01WriteTrx->dataPtr= (uint8_t*)pucWriteBuf;
	pM95M01WriteTrx->dataLen= _len;
	pM95M01WriteTrx->dataAddress= pRte->currentEEPROMAddress;
	pM95M01WriteTrx->retryCounter= DEVICELOG_CFG_MAX_RETRY_COUNT;

    return M95M01_Enqueue(pM95M01WriteTrx );
}

ErrorStatus DEVICELOG_ReadLogs(M95M01_Transaction_t *_trx, uint32_t _logIndex, uint32_t _logCount, DEVICELOG_Log_t *_logBuf)
{
	if(
		(true== pM95M01ReadTrx->transactionInProgress)
		||(true== pM95M01ReadTrx->transactionInQueue)
	)
	{
		return ERROR;
	}

	pM95M01ReadTrx= _trx;
	pM95M01ReadTrx->type= M95M01_TYPE_READ;
	pM95M01ReadTrx->partition= M95M01_PARTITION_DEVICELOG;
	pM95M01ReadTrx->dataPtr= (uint8_t *) _logBuf;
	pM95M01ReadTrx->dataLen= (uint16_t)(_logCount* sizeof(DEVICELOG_Log_t));
	pM95M01ReadTrx->dataAddress= (_logIndex* sizeof(DEVICELOG_Log_t));

    return M95M01_Enqueue(_trx );
}

uint32_t DEVICELOG_GetLogCount(void)
{
	return pRte->logCount;
}

uint32_t DEVICELOG_GetLogFloor(void)
{
	return pRte->logsInEEPROMFloorIndex;
}

uint32_t DEVICELOG_GetPeriodicLogCount(void)
{
	return pRte->periodicLogCount;
}

uint32_t DEVICELOG_GetTamperLogCount(void)
{
	return pRte->tamperLogCount;
}

void DEVICELOG_EnablePeriodicLog(bool _enable)
{
	bPeriodicLogEnabled= _enable;
}

void DEVICELOG_EnableTamperLog(bool _enable)
{
	bTamperLogEnabled= _enable;
	if(true== bTamperLogEnabled)
	{
		ucPrevStatusCode= SENSORS_GetStatusCode();
	}
}

static uint32_t DEVICELOG_TickSizeToTickSize_s(DEVICELOG_TickType_t _tickType, uint32_t _tickSize)
{
	switch(_tickType)
	{
		case SECOND_TickType:
			return _tickSize;
		case MINUTE_TickType:
			return 60* _tickSize;
		case HOUR_TickType:
			return 60* 60* _tickSize;
		case DAY_TickType:
			return 60* 60* 24* _tickSize;
		case MONTH_TickType:
			return 60* 60* 24* 30* _tickSize;
	}

	return 0;
}

DEVICELOG_StartType_t DEVICELOG_ReStartLog(DEVICELOG_StartType_t _startType, RTC_TickType_t _tickType, uint32_t _tickSize, RTC_AlarmStartMarker_t _startTime, uint32_t _savedTick)
{
	RTCALARM_A_Enable(_tickType, _startTime);

	if(IMMEDIATE_StartType== _startType)
	{
		pRte->nextTick= RTCALARM_A_GetTick()+ 0x01;/*start immediately*/
	}
	else if(WAITSTARTTIME_StartType== _startType)
	{
		if(0== _savedTick)
		{
			_startType= DEVICELOG_StartLog(_startType, _tickType, _tickSize, _startTime);
		}
		RTCALARM_A_SetTick(_savedTick);
	}
	bWaitForStartTick= false;

	return _startType;
}

DEVICELOG_StartType_t DEVICELOG_StartLog(DEVICELOG_StartType_t _startType, RTC_TickType_t _tickType, uint32_t _tickSize, RTC_AlarmStartMarker_t _startTime)
{
	eTickType= _tickType;
	ulTickSize= _tickSize;
	eStartTime= _startTime;

	if((IMMEDIATE_StartType== _startType)|| ((0xFF== eStartTime.second)&& (0xFF== eStartTime.minute)&& (0xFF== eStartTime.hour)&& (0xFF== eStartTime.date)))
	{
		/*start now*/
		RTC_DateTime_GetBCD(
				&(eStartTime.date),
				NULL,
				NULL,
				&(eStartTime.hour),
				&(eStartTime.minute),
				&(eStartTime.second)
				);
		RTCALARM_A_Enable(eTickType, eStartTime);

		pRte->nextTick= RTCALARM_A_GetTick()+ 0x01;
		pRte->currentLogTimestamp= 0;
		bWaitForStartTick= false;

		_startType= IMMEDIATE_StartType;
	}
	else
	{
		/*start time is defined, strategize Alarm to adhere to the start time.*/
		if((0xFF== eStartTime.minute)&& (0xFF== eStartTime.hour)&& (0xFF== eStartTime.date))
		{
			RTCALARM_A_Enable(MINUTE_TickType, eStartTime);/*use Minute to wait for /ss start datetime.*/
		}
		else if((0xFF== eStartTime.hour)&& (0xFF== eStartTime.date))
		{
			RTCALARM_A_Enable(HOUR_TickType, eStartTime);/*use Hour to wait for mm/ss start datetime.*/
		}
		else if(0xFF== eStartTime.date)
		{
			RTCALARM_A_Enable(DAY_TickType, eStartTime);/*use Day to wait for hh/mm/ss start datetime.*/
		}
		else
		{
			RTCALARM_A_Enable(MONTH_TickType, eStartTime);/*use Month to wait for dd hh/mm/ss start datetime.*/
		}

		pRte->currentLogTimestamp= 0;
		pRte->nextTick= RTCALARM_A_GetTick()+ 0x01;
		bWaitForStartTick= true;

		_startType= WAITSTARTTIME_StartType;
	}

	ulTickSize_s= DEVICELOG_TickSizeToTickSize_s(eTickType, ulTickSize);
	bIsArmed= true;

	return _startType;
}

static void DEVICELOG_PeriodicLogSync(void)/*to sync log tick when RTC is synched*/
{
	static uint32_t _previousLogTick= 0;

	if(RTCALARM_A_GetTick()!= _previousLogTick)
	{
		if((1< ulTickSize)&& (0!= pRte->currentLogTimestamp))
		{
			uint32_t _timestamp= UTILI_sTimeStamp();
			uint32_t _expectedTimestamp= (pRte->currentLogTimestamp+ ulTickSize_s);
			bool _logTickMatched= (RTCALARM_A_GetTick()>= pRte->nextTick)? true: false;
			uint32_t _singleTickInSeconds= DEVICELOG_TickSizeToTickSize_s(eTickType, 1);

			if((_timestamp== _expectedTimestamp)&& (false== _logTickMatched))
			{
				DBG_Print("#stat:logg: timestamp matched but logtick not matched >\r\n");

				RTCALARM_A_SetTick(pRte->nextTick);
			}
			else if(((true== _logTickMatched)&& (_timestamp< _expectedTimestamp))|| (_timestamp<=pRte->currentLogTimestamp))
			{
				DBG_Print("#stat:logg: timestamp lesser than expected >\r\n");

				uint32_t _regressedTicks= ((_expectedTimestamp- _timestamp)/ _singleTickInSeconds);

				pRte->currentLogTimestamp= _timestamp- (_timestamp% ulTickSize_s);
				RTCALARM_A_SetTick(((_timestamp-pRte->currentLogTimestamp)/ _singleTickInSeconds)+ 1);
				if(0== (_regressedTicks% ulTickSize))
				{
					pRte->nextTick= RTCALARM_A_GetTick();/*the tick is actually at the log point. we can take this as valid log tick*/
				}
				else
				{
					pRte->nextTick= ulTickSize+ 1;
				}
			}
			else if(_timestamp> _expectedTimestamp)
			{
				DBG_Print("#stat:logg: timestamp larger than expected >\r\n");

				uint32_t _advanceTicks= ((_timestamp- _expectedTimestamp)/ _singleTickInSeconds);

				if(0== (_advanceTicks% ulTickSize))
				{
					RTCALARM_A_SetTick(pRte->nextTick);/*the tick is actually at the log point. we can take this as valid log tick*/
				}
				else
				{
					RTCALARM_A_SetTick(pRte->nextTick+ _advanceTicks);
					pRte->nextTick+= ((_advanceTicks/ ulTickSize)+ 1)* ulTickSize;
				}
				pRte->currentLogTimestamp= _timestamp- ((ulTickSize- (pRte->nextTick- RTCALARM_A_GetTick()))* _singleTickInSeconds);
			}
		}

		_previousLogTick= RTCALARM_A_GetTick();
	}
}

static void DEVICELOG_PeriodicLogTask(void)
{
	DEVICELOG_PeriodicLogSync();

	if(RTCALARM_A_GetTick()< pRte->nextTick)
	{
		return;
	}

	pRte->currentLogTimestamp= UTILI_sTimeStamp();

	if(true== bWaitForStartTick)
	{
		/*at this point start tick has arrived. now reconfigure the alarm accordingly.*/
		RTCALARM_A_Enable(eTickType, eStartTime);
		bWaitForStartTick= false;
	}

	/*at this point we received new tick */
	if((false== sTrx.M95M01Trx.transactionInQueue)&& (false== sTrx.M95M01Trx.transactionInProgress))
	{
		DEVICELOG_Construct(&eDeviceLog);
		if(SUCCESS== DEVICELOG_WriteLog(&sTrx.M95M01Trx, eDeviceLog))/*successfully queued*/
		{
			sTrx.type= DeviceLog_TrxType;
			pRte->nextTick+= ulTickSize;
		}
	}
}

static void DEVICELOG_TamperLogTask(void)
{
	uint8_t _currStatusCode= SENSORS_GetStatusCode();
	if(_currStatusCode!= ucPrevStatusCode)
	{
		if(_currStatusCode> ucPrevStatusCode)/*only care for new flag set, not cleared*/
		{
			if((false== sTrx.M95M01Trx.transactionInQueue)&& (false== sTrx.M95M01Trx.transactionInProgress))
			{
				DEVICELOG_Construct(&eDeviceLog);
				if(SUCCESS== DEVICELOG_WriteLog(&sTrx.M95M01Trx, eDeviceLog))/*successfully queued*/
				{
					sTrx.type= TamperLog_TrxType;
					ucPrevStatusCode= _currStatusCode;
				}
			}
		}
		else
		{
			ucPrevStatusCode= _currStatusCode;
		}
	}
}

void DEVICELOG_Init(void)
{
	pRte= (DEVICELOG_Rte_t *)(&config.log.device.rte);
	M95M01_Init();
	RTCALARM_Init();
}

void DEVICELOG_Task(void)
{
	if(true== bPeriodicLogEnabled)
	{
		DEVICELOG_PeriodicLogTask();
	}

	if(true== bTamperLogEnabled)
	{
		DEVICELOG_TamperLogTask();
	}

	if(
		(true== pM95M01WriteTrx->transactionInProgress)
		||(true== pM95M01WriteTrx->transactionInQueue)
		||(true== pM95M01ReadTrx->transactionInProgress)
		||(true== pM95M01ReadTrx->transactionInQueue)
	)
	{
		M95M01_Transaction();

		if((true== pM95M01WriteTrx->transactionCompleted)/*&& (NULL!= pM95M01WriteTrx->TransactionCompletedCb)*/)
		{
			//pM95M01WriteTrx->TransactionCompletedCb();
			pM95M01WriteTrx= &eNoTrx;
		}

		if((true== pM95M01ReadTrx->transactionCompleted)/*&& (NULL!= pM95M01ReadTrx->TransactionCompletedCb)*/)
		{
			//pM95M01ReadTrx->TransactionCompletedCb();
			pM95M01ReadTrx= &eNoTrx;
		}
	}
}

uint8_t DEVICELOG_TaskState(void)
{
	if(
		(true== pM95M01WriteTrx->transactionInProgress)
		||(true== pM95M01WriteTrx->transactionInQueue)
		||(true== pM95M01ReadTrx->transactionInProgress)
		||(true== pM95M01ReadTrx->transactionInQueue)
	)
	{
		return M95M01_TransactionState();
	}

	return SLEEP_TaskState;
}
