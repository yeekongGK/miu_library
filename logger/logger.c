/******************************************************************************
 * File:        logger.c
 * Author:      Firmware Team
 * Created:     03-10-2025
 * Last Update: -
 *
 * Description:
 *   This file implements the main logger module, which acts as a transactional
 *   layer for various logging subsystems like device and radio logs. It manages
 *   a queue of logging requests, handles the interaction with the M95M01
 *   EEPROM driver for persistent storage, and provides a TLV interface for
 *   external control and data retrieval.
 *
 * Notes:
 *   - It uses a transaction queue to handle asynchronous logging operations.
 *
 * To Do:
 *   - The `LOGGER_Test` function, which performs synchronous blocking
 *     operations, could be improved to be non-blocking or used only in a
 *     specific test mode.
 *
 ******************************************************************************/

#include "common.h"
#include "logger.h"
#include "m95m01.h"
#include "radiolog.h"
#include "devicelog.h"

static void LOGGER_EndTransaction(bool _transactionStatusOK);

static LOG_t *pConfig;
static LOGGER_Transaction_t	eNoTrx= {			.transactionType= TRANSACTION_TYPE_DEVICELOG_READ,
												.logBuffer= NULL,
												.logIndex= 0,
												.logCount= 0,
												.transactionInProgress= false,
												.transactionStatusOK= false,
												.TransactionCompletedCb= NULL,	};
LOGGER_Transaction_t 	*eLoggerTrxQueue[LOGGER_CFG_MAX_TRX_QUEUE];
LOGGER_Transaction_t 	*eCurrentLoggerTrx= &eNoTrx;
uint8_t 				ucLoggerTrxQueueDepth= 0;
uint8_t 				ucLoggerTrxDequeueIndex= 0;
M95M01_Transaction_t 	eM95M01Trx= { 			.timeout= LOGGER_CFG_EEPROM_TIMEOUT_MS,
												.retryCounter= LOGGER_CFG_MAX_RETRY_COUNT,
												.TransactionCompletedCb= NULL};
DEVICELOG_Log_t 		eMeterLog;
RADIOLOG_Log_t 			eRadioLog;

static uint32_t ulStatusChangedLogCount= 0;

uint8_t LOGGER_QueueDepth(void)
{
	return ucLoggerTrxQueueDepth;
}

ErrorStatus LOGGER_Enqueue(LOGGER_Transaction_t *_trx)
{
	/*TODO: boundary check for logs - so far logs task has no error checking :(*/
	if(LOGGER_CFG_MAX_TRX_QUEUE!= ucLoggerTrxQueueDepth)
	{
		_trx->transactionInQueue= true;
		_trx->transactionInProgress= false;
		_trx->transactionStatusOK= false;
		_trx->transactionCompleted= false;
		eLoggerTrxQueue[(ucLoggerTrxDequeueIndex+ ucLoggerTrxQueueDepth)% LOGGER_CFG_MAX_TRX_QUEUE]= _trx;
		ucLoggerTrxQueueDepth++;
		return SUCCESS;
	}

	return ERROR;
}

LOGGER_Transaction_t *LOGGER_Dequeue(void)
{
	LOGGER_Transaction_t *_trx= NULL;
	if(0!= ucLoggerTrxQueueDepth)
	{
		_trx= eLoggerTrxQueue[ucLoggerTrxDequeueIndex];
		_trx->transactionInQueue= false;
		ucLoggerTrxQueueDepth--;
		ucLoggerTrxDequeueIndex= (ucLoggerTrxDequeueIndex+ 1)% LOGGER_CFG_MAX_TRX_QUEUE;
	}
	return _trx;
}

static void LOGGER_EndTransaction(bool _transactionStatusOK)
{
	eCurrentLoggerTrx->transactionInProgress= false;
	eCurrentLoggerTrx->transactionStatusOK= _transactionStatusOK;
	eCurrentLoggerTrx->transactionCompleted= true;
//	if(NULL!= eCurrentLoggerTrx->TransactionCompletedCb)
//	{
//		eCurrentLoggerTrx->TransactionCompletedCb();
//	}
	eCurrentLoggerTrx= &eNoTrx;
}

DEVICELOG_StartType_t LOGGER_DeviceLog_StartPeriodic(DEVICELOG_StartType_t _startType, RTC_TickType_t _tickType, uint32_t _tickSize, RTC_AlarmStartMarker_t _startTime)
{
	return DEVICELOG_StartLog(_startType, _tickType, _tickSize, _startTime);
}

void LOGGER_DeviceLog_StartTamperLog(bool _start)
{

}

uint32_t LOGGER_DeviceLog_GetLogCount(void)
{
	return DEVICELOG_GetLogCount();
}

uint32_t LOGGER_DeviceLog_GetLogFloor(void)
{
	return DEVICELOG_GetLogFloor();
}

uint32_t LOGGER_DeviceLog_GetPeriodicLogCount(void)
{
	return DEVICELOG_GetPeriodicLogCount();
}

uint32_t LOGGER_DeviceLog_GetTamperLogCount(void)
{
	return DEVICELOG_GetTamperLogCount();
}

ErrorStatus LOGGER_InitSync(LOGGER_SyncRte_t *_rte, uint8_t _partition, uint16_t _elementSize, uint16_t _elementMax)
{
	_rte->partition= _partition;
	_rte->head= 0;
	_rte->elementCount= 0;
	_rte->elementSize= _elementSize;
	_rte->elementMax= _elementMax;
}

ErrorStatus LOGGER_WriteSync(LOGGER_SyncRte_t *_rte, void *_element)
{
	M95M01_Transaction_t _M95M01Trx;

	_M95M01Trx.type= M95M01_TYPE_WRITE;
	_M95M01Trx.partition= _rte->partition;
	_M95M01Trx.dataPtr= (uint8_t*)_element;
	_M95M01Trx.dataLen= _rte->elementSize;
	_M95M01Trx.dataAddress= _rte->head* _rte->elementSize;
	_M95M01Trx.timeout= LOGGER_CFG_EEPROM_TIMEOUT_MS;
	_M95M01Trx.retryCounter= LOGGER_CFG_MAX_RETRY_COUNT;
	_M95M01Trx.TransactionCompletedCb= NULL;

	uint64_t _timeout= 1500+ SYS_GetTimestamp_ms();
    while(ERROR== M95M01_Enqueue(&_M95M01Trx))
    {
		M95M01_Transaction();
		if(_timeout<= SYS_GetTimestamp_ms())
		{
			return ERROR;
		}
    }

	while(false== _M95M01Trx.transactionCompleted)
	{
		M95M01_Transaction();
		if(_timeout<= SYS_GetTimestamp_ms())
		{
			return ERROR;
		}
	}

    _rte->head= (_rte->head+ 1)% _rte->elementMax;
    _rte->elementCount= (_rte->elementCount== _rte->elementMax)? _rte->elementMax: _rte->elementCount+ 1;
	//DBG_Print("LOGGER_WriteSync %d \r\n",_rte->head);

	return SUCCESS;
}

ErrorStatus LOGGER_ReadSync(LOGGER_SyncRte_t *_rte, uint16_t _index, uint16_t _count,  uint8_t *_buf)
{
	M95M01_Transaction_t _M95M01Trx;

	_index%= _rte->elementMax;

	_M95M01Trx.type= M95M01_TYPE_READ;
	_M95M01Trx.partition= _rte->partition;
	_M95M01Trx.dataPtr= _buf;
	_M95M01Trx.dataLen= _count* _rte->elementSize;
	_M95M01Trx.dataAddress= _index* _rte->elementSize;
	_M95M01Trx.timeout= LOGGER_CFG_EEPROM_TIMEOUT_MS;
	_M95M01Trx.retryCounter= LOGGER_CFG_MAX_RETRY_COUNT;
	_M95M01Trx.TransactionCompletedCb= NULL;

    while(ERROR== M95M01_Enqueue(&_M95M01Trx))
    {
		M95M01_Transaction();
    }

	while(false== _M95M01Trx.transactionCompleted)
	{
		M95M01_Transaction();
	}

    if(M95M01_STATUS_SUCCESS!=  _M95M01Trx.status)
    {
		DBG_Print("_M95M01Trx Failed !!!!\r\n");
    	return ERROR;
    }

	return SUCCESS;
}

void LOGGER_SaveContext(void)
{
}

void LOGGER_RestoreContext(void)
{
}

void LOGGER_TLVRequest(TLV_t *_tlv)
{
	_tlv->rv[0]= SUCCESS;
	_tlv->rl= 1;

	switch(_tlv->t)
	{
		case EEPROM_CHECK_LoggerTLVTag:
			{
				_tlv->rv[_tlv->rl++]= LOGGER_Test();
			}
			break;

		case GET_PERIODIC_LOG_LoggerTLVTag:
			{
				_tlv->rv[_tlv->rl++]= pConfig->device.deviceLogEnabled;
				_tlv->rv[_tlv->rl++]= (uint8_t)pConfig->device.startType;
				_tlv->rv[_tlv->rl++]= pConfig->device.startMarker.date;
				_tlv->rv[_tlv->rl++]= pConfig->device.startMarker.hour;
				_tlv->rv[_tlv->rl++]= pConfig->device.startMarker.minute;
				_tlv->rv[_tlv->rl++]= pConfig->device.startMarker.second;
				_tlv->rv[_tlv->rl++]= (uint8_t)pConfig->device.tickType;
				memcpy(_tlv->rv+ _tlv->rl, (uint8_t *)&(pConfig->device.tickSize), 4);
				_tlv->rl+= 4;
			}
			break;

		case SET_PERIODIC_LOG_LoggerTLVTag:
			{
				pConfig->device.deviceLogEnabled= _tlv->v[0];
				pConfig->device.startType= (DEVICELOG_StartType_t)_tlv->v[1];
				pConfig->device.startMarker.date= _tlv->v[2];
				pConfig->device.startMarker.hour= _tlv->v[3];
				pConfig->device.startMarker.minute= _tlv->v[4];
				pConfig->device.startMarker.second= _tlv->v[5];
				pConfig->device.tickType= (DEVICELOG_TickType_t)_tlv->v[6];
				memcpy((uint8_t *)&(pConfig->device.tickSize), &( _tlv->v[7]), 4);

				DEVICELOG_EnablePeriodicLog(pConfig->device.deviceLogEnabled);
				DEVICELOG_StartLog(pConfig->device.startType, pConfig->device.tickType, pConfig->device.tickSize, pConfig->device.startMarker);
			}
			break;

		case GET_TAMPER_LOG_LoggerTLVTag:
			{
				_tlv->rv[_tlv->rl++]= pConfig->device.tamperLogEnabled;
			}
			break;

		case SET_TAMPER_LOG_LoggerTLVTag:
			{
				pConfig->device.tamperLogEnabled= _tlv->v[0];
				DEVICELOG_EnableTamperLog(pConfig->device.tamperLogEnabled);
			}
			break;

		case GET_DEVICE_LOG_LoggerTLVTag:
			{
				uint16_t _index= 0;
				LOGGER_Transaction_t _trx= {0};

				_index+= UTILI_Array_Copy16_Ptr((uint8_t *)&_trx.logIndex, _tlv->v+ _index);
				_index+= UTILI_Array_Copy16_Ptr((uint8_t *)&_trx.logCount, _tlv->v+ _index);
				_trx.transactionType= TRANSACTION_TYPE_DEVICELOG_READ;
				_trx.logBuffer= _tlv->rv+ _tlv->rl;
				_trx.TransactionCompletedCb= NULL;
				if(SUCCESS== LOGGER_Enqueue(&_trx))
				{
					while(false== _trx.transactionCompleted)
					{
						LOGGER_Task();/*sue me for doing this here :p*/
					}
					_tlv->rl+= _trx.logCount* sizeof(DEVICELOG_Log_t);
				}
				else
				{
					_tlv->rv[0]= ERROR;
				}
			}
			break;

		default:
			_tlv->rv[0]= ERROR;
			break;
	}
}

void LOGGER_Init(LOG_t *_config)
{
	pConfig= _config;

	DEVICELOG_Init();
	//LOGGER_Test();

	DEVICELOG_EnablePeriodicLog(pConfig->device.deviceLogEnabled);
	DEVICELOG_EnableTamperLog(pConfig->device.tamperLogEnabled);

	//RTC_AlarmStartMarker_t _startTime= {.second= 0x00, .minute= 0x00, .hour= 0xFF, .date= 0xFF};
	//DEVICELOG_StartLog(Hour, 1, _startTime);
	//RTC_AlarmStartMarker_t _startTime= {.second= 0x00, .minute= 0xFF, .hour= 0xFF, .date= 0xFF};
	//DEVICELOG_StartLog(Second, 30, _startTime);
	//DEVICELOG_StartLog(NOTSTARTED_StartType, pConfig->device.tickType, pConfig->device.tickSize, pConfig->device.startMarker);
}

void LOGGER_Task(void)
{
	DEVICELOG_Task();

	if(true== eM95M01Trx.transactionCompleted)
	{
		if(true== eCurrentLoggerTrx->transactionInProgress)
		{
			eM95M01Trx.transactionCompleted= false;
			LOGGER_EndTransaction((eM95M01Trx.status== M95M01_STATUS_SUCCESS)? true: false);
		}
	}

	/*Service read/ write from log app or other task*/
    if((0!= LOGGER_QueueDepth())
    	&& (false== eCurrentLoggerTrx->transactionInProgress))
    {
    	eCurrentLoggerTrx= LOGGER_Dequeue();
    	switch(eCurrentLoggerTrx->transactionType)
    	{
			case TRANSACTION_TYPE_DEVICELOG_WRITE:
				if(ERROR== DEVICELOG_WriteLogs(&eM95M01Trx, (DEVICELOG_Log_t *)(eCurrentLoggerTrx->logBuffer) , eCurrentLoggerTrx->logCount))
				{
					LOGGER_EndTransaction(false);
					break;
				}
				eCurrentLoggerTrx->transactionInProgress= true;
				break;

			case TRANSACTION_TYPE_DEVICELOG_READ:
				if(ERROR== DEVICELOG_ReadLogs(&eM95M01Trx, eCurrentLoggerTrx->logIndex , eCurrentLoggerTrx->logCount, (DEVICELOG_Log_t *)(eCurrentLoggerTrx->logBuffer)))
				{
					LOGGER_EndTransaction(false);
					break;
				}
				eCurrentLoggerTrx->transactionInProgress= true;
				break;

    		default:
    			SYS_FailureHandler();
    			break;
    	}
    }
}

uint8_t LOGGER_TaskState(void)
{
	return DEVICELOG_TaskState();
}

ErrorStatus LOGGER_Test(void)
{
	/*call this only once before program start to check eeprom*/
	LOGGER_Transaction_t _trx;
	uint8_t _logBufferDefault[1* sizeof(DEVICELOG_Log_t)];
	uint8_t _logBufferWrite[1* sizeof(DEVICELOG_Log_t)];
	uint8_t _logBufferRead[1* sizeof(DEVICELOG_Log_t)];
	uint32_t _logsInEEPROMCeilingIndex= pConfig->device.rte.logsInEEPROMCeilingIndex;

	pConfig->device.rte.logsInEEPROMCeilingIndex= 2;/*emulate no log in RAM*/

	/*1. read default data*/
	_trx.logIndex= 0;
	_trx.logCount= sizeof(_logBufferDefault)/ sizeof(DEVICELOG_Log_t);
	_trx.transactionType= 	TRANSACTION_TYPE_DEVICELOG_READ;
	_trx.logBuffer= 			_logBufferDefault;
	_trx.TransactionCompletedCb= NULL;
	LOGGER_Enqueue(&_trx);
	while(false== _trx.transactionCompleted)
	{
		LOGGER_Task();/*sue me for doing this here :p*/
	}

	/*2. set and write sample data*/
	int _multi= 0; for(int i=0; i< sizeof(_logBufferWrite); i++){ _logBufferWrite[i]= ++_multi* i; }
	_trx.transactionType= 	TRANSACTION_TYPE_DEVICELOG_WRITE;
	_trx.logBuffer= 			_logBufferWrite;
	LOGGER_Enqueue(&_trx);
	while(false== _trx.transactionCompleted)
	{
		LOGGER_Task();/*sue me for doing this here :p*/
	}

	/*3. read back and compare*/
	_trx.transactionType= 	TRANSACTION_TYPE_DEVICELOG_READ;
	_trx.logBuffer= 			_logBufferRead;
	LOGGER_Enqueue(&_trx);
	while(false== _trx.transactionCompleted)
	{
		LOGGER_Task();/*sue me for doing this here :p*/
	}
	for(int i= 0; i<sizeof(_logBufferWrite); i++)
	{
		if(_logBufferRead[i]!= _logBufferWrite[i])
		{
			pConfig->device.rte.logsInEEPROMCeilingIndex= _logsInEEPROMCeilingIndex;
			DBG_Print("LOGGER_Test_ERROR. \r\n");
			return ERROR;
		}
	}

	/*4. write back default data*/
	_trx.transactionType= 	TRANSACTION_TYPE_DEVICELOG_WRITE;
	_trx.logBuffer= 		_logBufferDefault;
	LOGGER_Enqueue(&_trx);
	while(false== _trx.transactionCompleted)
	{
		LOGGER_Task();/*sue me for doing this here :p*/
	}

	pConfig->device.rte.logsInEEPROMCeilingIndex= _logsInEEPROMCeilingIndex;

	DBG_Print("LOGGER_Test_SUCCESS. \r\n");
	return SUCCESS;
}
