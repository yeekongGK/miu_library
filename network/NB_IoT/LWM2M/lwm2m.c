/*
 * nbiotlwm2m.c
 *
 *  Created on: 14 Feb 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#include "common.h"
#include "lwm2m.h"
#include "bc66link.h"
#include "logger.h"
#include "printf.h"
#include "sensor.h"
#include "periodicactivity.h"
#include "datamonitoring.h"
#include "softwaremgt.h"

#define DBG_Print

static LWM2M_t *pConfig;
__IO static BC66LINK_Transaction_t eBc66Trx= {
		.type= LWM2M_Bc66LinkTransactionType,
		.timeout_ms= 90000,
		.TransactionCompletedCb= NULL,
};
__IO static LOGGER_Transaction_t eLoggerTrx;
__IO static char pucValueBuffer[LWM2M_CFG_MAX_REQUEST_VALUE_LEN];
__IO static LWM2M_LWM2M_Request_t eRequest[LWM2M_CFG_MAX_REQUEST];
__IO static uint8_t ucRequestHead= 0;
__IO static uint8_t ucRequestTail= 0;


static bool bIsConfigured= false;
static bool bIsRegistered= false;
static bool bRegisterIsRequested= true;
static bool bPostRegisterIsRequested= false;

__IO static uint32_t ulNotifyRetryTime= 0;

static void LWM2M_SetTimeout_s(uint32_t _seconds)
{
	if(0!= _seconds)
	{
		SYS_Sleep(NBIOT_LWM2M_TaskId, (_seconds* 1000)- 100);
	}
}

static bool LWM2M_IsTimeout(void)
{
	return (true== SYS_IsAwake(NBIOT_LWM2M_TaskId));
}

void LWM2M_Diag(DIAG_DCode_t _dcode, uint32_t _objectId, uint32_t _instanceId, uint32_t _resourceId, uint32_t _param, uint32_t _status)
{
	DIAG_Code(_dcode,
			(_status<< 28)|
			(_param<< 24)|
			(_resourceId<< 20)|
			(_instanceId<< 16)|
			((_objectId)& 0x0000FFFF));
}

void LWM2M_ClearPendingRequest(void)
{
	ucRequestHead= ucRequestTail= 0;
	eRequest[ucRequestTail].resourceIndex= eRequest[ucRequestTail].resourceCount= 1;
}

void LWM2M_Recovery(void *_param)
{
	BC66LINK_Lwm2m_Recovery_t *_recovery= (BC66LINK_Lwm2m_Recovery_t *)_param;

	DBG_Print("LWM2M Recovery.\r\n");

	if(0!= _recovery->state)/*fail, need to configure and re-register*/
	{
		bIsRegistered= false;
		bIsConfigured= false;
		LWM2M_RequestRegister();
		DBG_Print("LWM2M Recovery triggered.\r\n");
	}
}

void LWM2M_Register(void *_param)
{
	BC66LINK_Lwm2m_Register_t *_register= (BC66LINK_Lwm2m_Register_t *)_param;
	bIsRegistered= (0== _register->state)? true: false;
	bPostRegisterIsRequested=  true;
}

bool LWM2M_IsRegistered(void)
{
	return bIsRegistered;
}

void DTMON_ReRegisterCallback(void);/*to invoke reboot alarm*/
void LWM2M_RequestRegister(void)
{
	bRegisterIsRequested= true;
	LWM2M_ClearPendingRequest();/*remove whatever pending, e.g. read, write observe etc*/
}

LWOBJ_Resource_t *LWM2M_GetResource(LWOBJ_Obj_t *_lwObj, uint16_t _resourceNo)
{
	switch(_lwObj->type)
	{
		case SWMGT_ObjType:
			return &(pConfig->swmgt.resource[_resourceNo]);
		case PRACT_ObjType:
			return &(pConfig->pract.resource[_lwObj->instance][_resourceNo]);
		case DTMON_ObjType:
			return &(pConfig->dtmon.resource[_lwObj->instance][_resourceNo]);
		default:
			return NULL;
	}
}

void LWM2M_WriteValues(LWOBJ_Obj_t *_lwObj, uint16_t _resourceNo, LWOBJ_ResourceType_t _valueType, char *_value, uint16_t _valueLen)
{
	 _lwObj->written= true;

	switch(_lwObj->resource[_resourceNo].attr.type)
	{
		case STRING_ResourceType:
			memcpy(_lwObj->resource[_resourceNo].value.string, _value, _valueLen);
			_lwObj->resource[_resourceNo].valueLen= _valueLen;
			break;
		case OPAQUE_ResourceType:
			memcpy(_lwObj->resource[_resourceNo].value.opaque, _value, _valueLen);
			_lwObj->resource[_resourceNo].valueLen= _valueLen;
			break;
		case BOOLEAN_ResourceType:
			if(OPAQUE_ResourceType== _valueType)
			{
				_lwObj->resource[_resourceNo].value.boolean= (0== _value[0])?false: true;
			}
			else
			{
				_lwObj->resource[_resourceNo].value.boolean= (0== atoi(_value))? false: true;
			}
			break;
		case INTEGER_ResourceType:
			if(OPAQUE_ResourceType== _valueType)
			{
				_lwObj->resource[_resourceNo].value.integer= 0;
				for(int i= 0; i<_valueLen; i++)
				{
					_lwObj->resource[_resourceNo].value.integer|= (_value[((_valueLen- 1)- i)]<< (i* 8));
				}
			}
			else
			{
				_lwObj->resource[_resourceNo].value.integer= atoi(_value);
			}
			break;
		case FLOAT_ResourceType:
			if(OPAQUE_ResourceType== _valueType)
			{
				uint8_t _temp[8]= {0};
				double *_p= (double *)_temp;
				for(int i= 0; i<_valueLen; i++)
				{
					_temp[i]= _value[(_valueLen- 1)- i];
				}
				_lwObj->resource[_resourceNo].value.single= (float)(*_p);
			}
			else
			{
				_lwObj->resource[_resourceNo].value.single= atof(_value);
			}
			break;
		case OBJLINK_ResourceType:
		case NONE_ResourceType:
			 _lwObj->written= false;
			break;
	}
}

void LWM2M_FillValues(LWOBJ_Obj_t *_lwObj, uint16_t _resourceNo, char *_buffer, uint16_t *_len)
{
	*_len= 0;
	switch(_lwObj->resource[_resourceNo].attr.type)
	{
		case STRING_ResourceType:
			sprintf(_buffer, "%s", _lwObj->resource[_resourceNo].value.string);
			*_len= strlen(_buffer);

			if(0== (*_len))/*cannot send 0 len babi tol*/
			{
				(*_len)= 1;
				_buffer[0]= '0';
				_buffer[1]= '\0';
			}
			break;

		case OPAQUE_ResourceType:
			switch(_lwObj->type)
			{
				case PRACT_ObjType:
					switch(_resourceNo)
					{
						case RECORD_PractResource:
						{
							char *_pBuf= _buffer;
							cbor_stream_t _cborstream;
							uint16_t _index= _lwObj->resource[RECORD_READ_PractResource].value.integer;
							uint16_t _count= _lwObj->resource[RECORD_HEAD_PractResource].value.integer- _index;

							switch(_lwObj->instance)
							{
								case GET_READING_PractInstance:
									{

										uint8_t _cborbuf[PRACT_CFG_0_RECORD_CBOR_BYTE_LEN];
										while((_count* PRACT_CFG_0_RECORD_CBOR_BYTE_LEN)> (LWM2M_CFG_MAX_REQUEST_VALUE_LEN/ 2/*div by to since need to convert to hexstring*/))
										{
											_count--;
										}

										*_len= 0;
										for(int i= 0; i< _count; i++)
										{
											PRACT_GetReadingRecord_t _record[1];
											LOGGER_ReadSync(&(_lwObj->rte.prAct->log), _index+ i, 1, _record);
											//DBG_Print("LOGGER_ReadSync reading %d, read %d \r\n", _index+ i, _record->index);

											cbor_init(&_cborstream, _cborbuf, PRACT_CFG_0_RECORD_CBOR_BYTE_LEN+ 1);/*+1, might be  a bug on cbor lib*/
											cbor_serialize_array(&_cborstream, 6);
											cbor_serialize_int(&_cborstream, _record->index);
											cbor_serialize_int(&_cborstream, _record->timestamp);
											cbor_serialize_float(&_cborstream, _record->meterReading);
											cbor_serialize_float(&_cborstream, _record->meterConsumption);
											cbor_serialize_int(&_cborstream, (uint32_t)(_record->temperature));
											cbor_serialize_int(&_cborstream, (uint32_t)(_record->batteryLevel_percent));

											UTILI_BytesToHexString(_cborbuf, _cborstream.pos, _pBuf);
											_pBuf+= (_cborstream.pos* 2);
											*_len+= _cborstream.pos;
										}
										_lwObj->rte.prAct->recordDispatched= _count;
									}
									break;

								case GET_STATUS_PractInstance:
									{
										uint8_t _cborbuf[PRACT_CFG_1_RECORD_CBOR_BYTE_LEN];
										while((_count* PRACT_CFG_1_RECORD_CBOR_BYTE_LEN)> (LWM2M_CFG_MAX_REQUEST_VALUE_LEN/ 2/*div by two since need to convert to hexstring*/))
										{
											_count--;
										}

										*_len= 0;
										for(int i= 0; i< _count; i++)
										{
											PRACT_GetStatusRecord_t _record[1];
											uint8_t _recordHeaderSize= sizeof(uint32_t)+ sizeof(uint32_t);
											LOGGER_ReadSync(&(_lwObj->rte.prAct->log), _index+ i, 1, _record);

											cbor_init(&_cborstream, _cborbuf, PRACT_CFG_1_RECORD_CBOR_BYTE_LEN+ 1);/*+1, might be  a bug on cbor lib*/
											cbor_serialize_array(&_cborstream, 38);
											cbor_serialize_int(&_cborstream, _record->index);
											cbor_serialize_int(&_cborstream, _record->timestamp);
											cbor_serialize_int(&_cborstream, _record->stats.noOfTransmission);
											cbor_serialize_int(&_cborstream, _record->stats.noOfFailedTransmission);
											cbor_serialize_int(&_cborstream, _record->stats.noOfAttach);
											cbor_serialize_int(&_cborstream, _record->stats.noOfDisattach);
											cbor_serialize_int(&_cborstream, _record->stats.noOfSimError);
											cbor_serialize_int(&_cborstream, _record->stats.latency_ms);
											cbor_serialize_int(&_cborstream, _record->stats.aveLatency_ms);
											cbor_serialize_int(&_cborstream, _record->stats.minLatency_ms);
											cbor_serialize_int(&_cborstream, _record->stats.maxLatency_ms);
											cbor_serialize_int(&_cborstream, _record->stats.pingLatency_ms);
											cbor_serialize_int(&_cborstream, _record->stats.failsafeRebootCount);
											cbor_serialize_int(&_cborstream, _record->stats.PVDRebootCount);
											cbor_serialize_int(&_cborstream, _record->stats.BORRebootCount);
											cbor_serialize_int(&_cborstream, _record->stats.rsrp);
											cbor_serialize_int(&_cborstream, _record->stats.aveRsrp);
											cbor_serialize_int(&_cborstream, _record->stats.minRsrp);
											cbor_serialize_int(&_cborstream, _record->stats.maxRsrp);
											cbor_serialize_int(&_cborstream, _record->stats.rssi);
											cbor_serialize_int(&_cborstream, _record->stats.aveRssi);
											cbor_serialize_int(&_cborstream, _record->stats.minRssi);
											cbor_serialize_int(&_cborstream, _record->stats.maxRssi);
											cbor_serialize_int(&_cborstream, _record->stats.sinr);
											cbor_serialize_int(&_cborstream, _record->stats.aveSinr);
											cbor_serialize_int(&_cborstream, _record->stats.minSinr);
											cbor_serialize_int(&_cborstream, _record->stats.maxSinr);
											cbor_serialize_int(&_cborstream, _record->stats.rsrq);
											cbor_serialize_int(&_cborstream, _record->stats.aveRsrq);
											cbor_serialize_int(&_cborstream, _record->stats.minRsrq);
											cbor_serialize_int(&_cborstream, _record->stats.maxRsrq);
											cbor_serialize_int(&_cborstream, _record->stats.txPower);
											cbor_serialize_int(&_cborstream, _record->stats.aveTxPower);
											cbor_serialize_int(&_cborstream, _record->stats.minTxPower);
											cbor_serialize_int(&_cborstream, _record->stats.maxTxPower);
											cbor_serialize_int(&_cborstream, _record->stats.ceMode);
											cbor_serialize_int(&_cborstream, _record->stats.ecl);
											cbor_serialize_int(&_cborstream, _record->stats.battVoltage_mV);

											UTILI_BytesToHexString(_cborbuf, _cborstream.pos, _pBuf);
											_pBuf+= (_cborstream.pos* 2);
											*_len+= _cborstream.pos;
										}
										_lwObj->rte.prAct->recordDispatched= _count;
									}
									break;

								case QUERY_PractInstance:
									{
										uint8_t _cborbuf[PRACT_CFG_2_RECORD_CBOR_BYTE_LEN];
										while((_count* PRACT_CFG_2_RECORD_CBOR_BYTE_LEN)> (LWM2M_CFG_MAX_REQUEST_VALUE_LEN/ 2/*div by two since need to convert to hexstring*/))
										{
											_count--;
										}

										*_len= 0;
										for(int i= 0; i< _count; i++)
										{
											PRACT_QueryRecord_t _record[1];
											uint8_t _recordHeaderSize= sizeof(uint32_t)+ sizeof(uint32_t);
											LOGGER_ReadSync(&(_lwObj->rte.prAct->log), _index+ i, 1, _record);

											cbor_init(&_cborstream, _cborbuf, PRACT_CFG_2_RECORD_CBOR_BYTE_LEN+ 1);/*+1, might be  a bug on cbor lib*/
											cbor_serialize_array(&_cborstream, 3);
											cbor_serialize_int(&_cborstream, _record->index);
											cbor_serialize_int(&_cborstream, _record->timestamp);
											cbor_serialize_byte_stringl(&_cborstream, ((const char *)_record)+ _recordHeaderSize, sizeof(PRACT_QueryRecord_t)- _recordHeaderSize);

											UTILI_BytesToHexString(_cborbuf, _cborstream.pos, _pBuf);
											_pBuf+= (_cborstream.pos* 2);
											*_len+= _cborstream.pos;
										}
										_lwObj->rte.prAct->recordDispatched= _count;
									}
									break;

								default:
									break;
							}
						}
						break;
					}
					break;

				case DTMON_ObjType:
					switch(_resourceNo)
					{
						case SETTINGS_DtmonResource:
						case DATA_DtmonResource:
						case REFERENCE_A_DtmonResource:
						case REFERENCE_B_DtmonResource:
						case REFERENCE_C_DtmonResource:
							UTILI_BytesToHexString(_lwObj->resource[_resourceNo].value.opaque, _lwObj->resource[_resourceNo].valueLen, _buffer);
							*_len=  _lwObj->resource[_resourceNo].valueLen;
							break;

						case RESULTS_DtmonResource:
						{
							char *_pBuf= _buffer;
							cbor_stream_t _cborstream;
							uint8_t _cborbuf[DTMON_CFG_RESULTS_CBOR_BYTE_LEN];
							uint16_t _count= _lwObj->rte.dtMon->log.elementCount;
							while((_count* DTMON_CFG_RESULTS_CBOR_BYTE_LEN)> (LWM2M_CFG_MAX_REQUEST_VALUE_LEN/ 2/*div by 2 since need to convert to hexstring*/))
							{
								_count--;
							}

							*_len= 0;
							for(int i= 0; i< _count; i++)
							{
								DTMON_Result_t _result[1];
								LOGGER_ReadSync(&(_lwObj->rte.dtMon->log), i, 1, _result);

								cbor_init(&_cborstream, _cborbuf, DTMON_CFG_RESULTS_CBOR_BYTE_LEN+ 1);/*+1, might be  a bug on cbor lib*/
								cbor_serialize_array(&_cborstream, 2);
								cbor_serialize_int(&_cborstream, _result->index);
								cbor_serialize_int(&_cborstream, _result->timestamp);
								cbor_serialize_int(&_cborstream, _result->value);

								UTILI_BytesToHexString(_cborbuf, _cborstream.pos, _pBuf);
								_pBuf+= (_cborstream.pos* 2);
								*_len+= _cborstream.pos;
							}
							break;
						}
					}
					break;

				default:
					break;
			}

			if(0== (*_len))/*cannot send 0 len babi tol*/
			{
				(*_len)= 1;
				_buffer[0]= _buffer[1]= '0';
				_buffer[2]= '\0';
			}
			break;

		case BOOLEAN_ResourceType:
			sprintf(_buffer, "%d", _lwObj->resource[_resourceNo].value.boolean);
			*_len= strlen(_buffer);
			break;

		case INTEGER_ResourceType:
			sprintf(_buffer, "%d", _lwObj->resource[_resourceNo].value.integer);
			*_len= strlen(_buffer);
			break;

		case FLOAT_ResourceType:
			sprintf(_buffer, "%f", _lwObj->resource[_resourceNo].value.single);
			*_len= strlen(_buffer);
			break;

		case OBJLINK_ResourceType:
			break;

		case NONE_ResourceType:
			break;
	}
}

void LWM2M_WriteRequest(void *_param)
{
	BC66LINK_Lwm2m_Request_t *_request= (BC66LINK_Lwm2m_Request_t *)_param;
	uint8_t _requestStatus= 0;
	if(eRequest[ucRequestHead].resourceIndex!= eRequest[ucRequestHead].resourceCount)/*still have unfinished business, ignore*/
	{
		_requestStatus= 1;
	}
	else
	{
		eRequest[ucRequestHead].lwm2mType= WRITE_RESPONSE_Bc66LinkLwm2m;
		eRequest[ucRequestHead].messageId= _request->messageId;
		eRequest[ucRequestHead].objectId= _request->objectId;
		eRequest[ucRequestHead].instanceId= _request->instanceId;
		eRequest[ucRequestHead].resourceIndex= 0;
		eRequest[ucRequestHead].resourceCount= 1;
		eRequest[ucRequestHead].resourceIds[eRequest[ucRequestHead].resourceIndex]= _request->resourceId;
		eRequest[ucRequestHead].valueType= _request->valueType;
		eRequest[ucRequestHead].len= _request->len;
		memcpy(eRequest[ucRequestHead].value, _request->value, _request->len);
		eRequest[ucRequestHead].index= _request->index;

		ucRequestHead= (++ucRequestHead)% LWM2M_CFG_MAX_REQUEST;
	}
	LWM2M_Diag(REQUEST_NbiotLwm2mDCode, _request->objectId, _request->instanceId, _request->resourceId,
			WRITE_RESPONSE_Bc66LinkLwm2m, _requestStatus);
}

void LWM2M_ReadRequest(void *_param)
{
	BC66LINK_Lwm2m_Request_t *_request= (BC66LINK_Lwm2m_Request_t *)_param;
	uint8_t _requestStatus= 0;
	if(eRequest[ucRequestHead].resourceIndex!= eRequest[ucRequestHead].resourceCount)/*still have unfinished business, ignore*/
	{
		_requestStatus= 1;
	}
	else
	{
		eRequest[ucRequestHead].lwm2mType= READ_RESPONSE_Bc66LinkLwm2m;
		eRequest[ucRequestHead].messageId= _request->messageId;
		eRequest[ucRequestHead].objectId= _request->objectId;
		eRequest[ucRequestHead].instanceId= _request->instanceId;
		eRequest[ucRequestHead].resourceIndex= 0;
		eRequest[ucRequestHead].resourceCount= 1;
		eRequest[ucRequestHead].resourceIds[eRequest[ucRequestHead].resourceIndex]= _request->resourceId;

		ucRequestHead= (++ucRequestHead)% LWM2M_CFG_MAX_REQUEST;
	}
	LWM2M_Diag(REQUEST_NbiotLwm2mDCode, _request->objectId, _request->instanceId, _request->resourceId,
			READ_RESPONSE_Bc66LinkLwm2m, _requestStatus);
}

void LWM2M_ObserveRequest(void *_param)
{
	BC66LINK_Lwm2m_Request_t *_request= (BC66LINK_Lwm2m_Request_t *)_param;
	uint8_t _requestStatus= 0;
	if(eRequest[ucRequestHead].resourceIndex!= eRequest[ucRequestHead].resourceCount)/*still have unfinished business, ignore*/
	{
		_requestStatus= 1;
	}
	else
	{
		eRequest[ucRequestHead].lwm2mType= OBSERVE_RESPONSE_Bc66LinkLwm2m;
		eRequest[ucRequestHead].messageId= _request->messageId;
		eRequest[ucRequestHead].objectId= _request->objectId;
		eRequest[ucRequestHead].instanceId= _request->instanceId;
		eRequest[ucRequestHead].resourceIndex= 0;
		eRequest[ucRequestHead].resourceCount= 1;
		eRequest[ucRequestHead].resourceIds[eRequest[ucRequestHead].resourceIndex]= _request->resourceId;
		eRequest[ucRequestHead].flag= _request->flag;

		ucRequestHead= (++ucRequestHead)% LWM2M_CFG_MAX_REQUEST;
	}
	LWM2M_Diag(REQUEST_NbiotLwm2mDCode, _request->objectId, _request->instanceId, _request->resourceId,
			OBSERVE_RESPONSE_Bc66LinkLwm2m, _requestStatus);
}

void LWM2M_ExecuteRequest(void *_param)
{
	BC66LINK_Lwm2m_Request_t *_request= (BC66LINK_Lwm2m_Request_t *)_param;
	uint8_t _requestStatus= 0;
	if(eRequest[ucRequestHead].resourceIndex!= eRequest[ucRequestHead].resourceCount)/*still have unfinished business, ignore*/
	{
		_requestStatus= 1;
	}
	else
	{
		eRequest[ucRequestHead].lwm2mType= EXECUTE_RESPONSE_Bc66LinkLwm2m;
		eRequest[ucRequestHead].messageId= _request->messageId;
		eRequest[ucRequestHead].objectId= _request->objectId;
		eRequest[ucRequestHead].instanceId= _request->instanceId;
		eRequest[ucRequestHead].resourceIndex= 0;
		eRequest[ucRequestHead].resourceCount= 1;
		eRequest[ucRequestHead].resourceIds[eRequest[ucRequestHead].resourceIndex]= _request->resourceId;

		ucRequestHead= (++ucRequestHead)% LWM2M_CFG_MAX_REQUEST;
	}
	LWM2M_Diag(REQUEST_NbiotLwm2mDCode, _request->objectId, _request->instanceId, _request->resourceId,
			EXECUTE_RESPONSE_Bc66LinkLwm2m, _requestStatus);
}

void LWM2M_ProcessWriteRequest(void)
{
	eBc66Trx.lwm2m.WriteResponse.messageId= eRequest[ucRequestTail].messageId;
	eBc66Trx.lwm2m.WriteResponse.objectId= eRequest[ucRequestTail].objectId;
	eBc66Trx.lwm2m.WriteResponse.instanceId= eRequest[ucRequestTail].instanceId;
	eBc66Trx.lwm2m.WriteResponse.resourceId= eRequest[ucRequestTail].resourceIds[eRequest[ucRequestTail].resourceIndex];
	eBc66Trx.lwm2m.WriteResponse.result= NOTFOUND_Bc66LinkLwm2mRequestResult;

	for(int i= 0; i< MAX_ObjName; i++)
	{
		if((eRequest[ucRequestTail].objectId== pConfig->lwObj[i].id)&& (eRequest[ucRequestTail].instanceId== pConfig->lwObj[i].instance))
		{
			for(int j= 0; j< pConfig->lwObj[i].resourceCount; j++)
			{
				if(eRequest[ucRequestTail].resourceIds[eRequest[ucRequestTail].resourceIndex]== pConfig->lwObj[i].resource[j].attr.id)
				{
					if(((READWRITE_ResourceOperation!= pConfig->lwObj[i].resource[j].attr.operation)&& (WRITE_ResourceOperation!= pConfig->lwObj[i].resource[j].attr.operation))
							|| ((STRING_ResourceType== pConfig->lwObj[i].resource[j].attr.type)&& (eRequest[ucRequestTail].len> pConfig->lwObj[i].resource[j].valueMaxLen))
							|| ((OPAQUE_ResourceType== pConfig->lwObj[i].resource[j].attr.type)&& (eRequest[ucRequestTail].len> pConfig->lwObj[i].resource[j].valueMaxLen))
							|| ((eRequest[ucRequestTail].valueType!= pConfig->lwObj[i].resource[j].attr.type)&& (eRequest[ucRequestTail].valueType!= OPAQUE_ResourceType))
					)
					{
						eBc66Trx.lwm2m.WriteResponse.result= BADREQUEST_Bc66LinkLwm2mRequestResult;
						break;
					}
					else
					{
						LWM2M_WriteValues(&pConfig->lwObj[i], j, eRequest[ucRequestTail].valueType, (char *) eRequest[ucRequestTail].value, eRequest[ucRequestTail].len);
						eBc66Trx.lwm2m.WriteResponse.result= CHANGED_Bc66LinkLwm2mRequestResult;
						break;
					}
				}
			}

			break;
		}
	}

	eBc66Trx.lwm2mType= eRequest[ucRequestTail].lwm2mType;
	if(SUCCESS== BC66LINK_Enqueue(&eBc66Trx))
	{
		eRequest[ucRequestTail].resourceIndex++;
	}
	LWOBJ_WakeupTask();/*wakeup to process newly written data*/
}

void LWM2M_ProcessReadRequest(void)
{
process_read_request_restart:
	eBc66Trx.lwm2m.ReadResponse.messageId= eRequest[ucRequestTail].messageId;
	eBc66Trx.lwm2m.ReadResponse.result= NOTFOUND_Bc66LinkLwm2mRequestResult;
	eBc66Trx.lwm2m.ReadResponse.objectId= eRequest[ucRequestTail].objectId;
	eBc66Trx.lwm2m.ReadResponse.instanceId= eRequest[ucRequestTail].instanceId;
	eBc66Trx.lwm2m.ReadResponse.resourceId= eRequest[ucRequestTail].resourceIds[eRequest[ucRequestTail].resourceIndex];
	eBc66Trx.lwm2m.ReadResponse.valueType= STRING_Bc66LinkLwm2mValueType;/*default for not found result*/
	sprintf((char *)pucValueBuffer, "0");/*default for not found result*/
	eBc66Trx.lwm2m.ReadResponse.len= 1;/*default for not found result*/
	eBc66Trx.lwm2m.ReadResponse.value= (char *)pucValueBuffer;
	eBc66Trx.lwm2m.ReadResponse.index= (eRequest[ucRequestTail].resourceCount- eRequest[ucRequestTail].resourceIndex)- 1;

	for(int i= 0; i< MAX_ObjName; i++)
	{
		if((eRequest[ucRequestTail].objectId== pConfig->lwObj[i].id)&& (eRequest[ucRequestTail].instanceId== pConfig->lwObj[i].instance))
		{
			if(-1== eRequest[ucRequestTail].resourceIds[eRequest[ucRequestTail].resourceIndex])/*read all resource*/
			{
				eRequest[ucRequestTail].resourceCount= 0;
				for(int j= 0; j< pConfig->lwObj[i].resourceCount; j++)
				{
					if((READ_ResourceOperation== pConfig->lwObj[i].resource[j].attr.operation)|| (READWRITE_ResourceOperation== pConfig->lwObj[i].resource[j].attr.operation))
					eRequest[ucRequestTail].resourceIds[eRequest[ucRequestTail].resourceCount++]= pConfig->lwObj[i].resource[j].attr.id;
				}
				goto process_read_request_restart;
			}
			else
			{
				for(int j= 0; j< pConfig->lwObj[i].resourceCount; j++)
				{
					if(eRequest[ucRequestTail].resourceIds[eRequest[ucRequestTail].resourceIndex]== pConfig->lwObj[i].resource[j].attr.id)
					{
						if((OBJLINK_ResourceType== pConfig->lwObj[i].resource[j].attr.type)
								|| (NONE_ResourceType== pConfig->lwObj[i].resource[j].attr.type))
						{
							eBc66Trx.lwm2m.ReadResponse.result= BADREQUEST_Bc66LinkLwm2mRequestResult;
							eBc66Trx.lwm2m.ReadResponse.len= 0;
						}
						else
						{
							eBc66Trx.lwm2m.ReadResponse.result= CONTENT_Bc66LinkLwm2mRequestResult;
							eBc66Trx.lwm2m.ReadResponse.valueType= (BC66LINK_Lwm2m_Value_Type_t)pConfig->lwObj[i].resource[j].attr.type;
							LWM2M_FillValues(&pConfig->lwObj[i], j, (char *) pucValueBuffer, &(eBc66Trx.lwm2m.ReadResponse.len));
						}

						break;
					}
				}
			}

			break;
		}
	}

	eBc66Trx.lwm2mType= eRequest[ucRequestTail].lwm2mType;
	if(SUCCESS== BC66LINK_Enqueue(&eBc66Trx))
	{
		eRequest[ucRequestTail].resourceIndex++;
	}
}

void LWM2M_ProcessObserveRequest(void)
{
process_observe_request_restart:
	eBc66Trx.lwm2m.ObserveResponse.messageId= eRequest[ucRequestTail].messageId;
	eBc66Trx.lwm2m.ObserveResponse.result= NOTFOUND_Bc66LinkLwm2mRequestResult;
	eBc66Trx.lwm2m.ObserveResponse.objectId= eRequest[ucRequestTail].objectId;
	eBc66Trx.lwm2m.ObserveResponse.instanceId= eRequest[ucRequestTail].instanceId;
	eBc66Trx.lwm2m.ObserveResponse.resourceId= eRequest[ucRequestTail].resourceIds[eRequest[ucRequestTail].resourceIndex];
	eBc66Trx.lwm2m.ObserveResponse.valueType= STRING_Bc66LinkLwm2mValueType;/*default for not found result*/
	sprintf((char *)pucValueBuffer, "0");/*default for not found result*/
	eBc66Trx.lwm2m.ObserveResponse.len= 1;/*default for not found result*/
	eBc66Trx.lwm2m.ObserveResponse.value= (char *)pucValueBuffer;
	eBc66Trx.lwm2m.ObserveResponse.index= (eRequest[ucRequestTail].resourceCount- eRequest[ucRequestTail].resourceIndex)- 1;

	for(int i= 0; i< MAX_ObjName; i++)
	{
		if((eRequest[ucRequestTail].objectId== pConfig->lwObj[i].id)&& (eRequest[ucRequestTail].instanceId== pConfig->lwObj[i].instance))
		{
			if(-1== eRequest[ucRequestTail].resourceIds[eRequest[ucRequestTail].resourceIndex])/*read all resource*/
			{
				eRequest[ucRequestTail].resourceCount= 0;
				for(int j= 0; j< pConfig->lwObj[i].resourceCount; j++)
				{
					if((READ_ResourceOperation== pConfig->lwObj[i].resource[j].attr.operation)|| (READWRITE_ResourceOperation== pConfig->lwObj[i].resource[j].attr.operation))
					eRequest[ucRequestTail].resourceIds[eRequest[ucRequestTail].resourceCount++]= pConfig->lwObj[i].resource[j].attr.id;
				}
				goto process_observe_request_restart;
			}
			else
			{
				for(int j= 0; j< pConfig->lwObj[i].resourceCount; j++)
				{
					if(eRequest[ucRequestTail].resourceIds[eRequest[ucRequestTail].resourceIndex]== pConfig->lwObj[i].resource[j].attr.id)
					{
						if((OBJLINK_ResourceType== pConfig->lwObj[i].resource[j].attr.type)
								|| (NONE_ResourceType== pConfig->lwObj[i].resource[j].attr.type))
						{
							eBc66Trx.lwm2m.ObserveResponse.result= BADREQUEST_Bc66LinkLwm2mRequestResult;
							eBc66Trx.lwm2m.ObserveResponse.len= 0;
						}
						else
						{
							eBc66Trx.lwm2m.ObserveResponse.result= CONTENT_Bc66LinkLwm2mRequestResult;
							eBc66Trx.lwm2m.ObserveResponse.valueType= (BC66LINK_Lwm2m_Value_Type_t)pConfig->lwObj[i].resource[j].attr.type;
							pConfig->lwObj[i].resource[j].observe= (0== eRequest[ucRequestTail].flag)? true: false;
							pConfig->lwObj[i].resource[j].notifyState= (true== pConfig->lwObj[i].resource[j].observe)? DO_NOTIFY_NotifyState: IDLE_NotifyState;/*for PRACT, it's best to flush everything, jic modem reboot in the middle of previous notify*/
							LWM2M_FillValues(&pConfig->lwObj[i], j, (char *) pucValueBuffer, &(eBc66Trx.lwm2m.ObserveResponse.len));
						}

						break;
					}
				}
			}

			break;
		}
	}

	eBc66Trx.lwm2mType= eRequest[ucRequestTail].lwm2mType;
	if(SUCCESS== BC66LINK_Enqueue(&eBc66Trx))
	{
		eRequest[ucRequestTail].resourceIndex++;
	}
}

void LWM2M_ProcessExecuteRequest(void)
{
	eBc66Trx.lwm2m.ExecuteResponse.messageId= eRequest[ucRequestTail].messageId;
	eBc66Trx.lwm2m.ExecuteResponse.objectId= eRequest[ucRequestTail].objectId;
	eBc66Trx.lwm2m.ExecuteResponse.instanceId= eRequest[ucRequestTail].instanceId;
	eBc66Trx.lwm2m.ExecuteResponse.resourceId= eRequest[ucRequestTail].resourceIds[eRequest[ucRequestTail].resourceIndex];
	eBc66Trx.lwm2m.ExecuteResponse.result= NOTFOUND_Bc66LinkLwm2mRequestResult;

	for(int i= 0; i< MAX_ObjName; i++)
	{
		if((eRequest[ucRequestTail].objectId== pConfig->lwObj[i].id)&& (eRequest[ucRequestTail].instanceId== pConfig->lwObj[i].instance))
		{
			for(int j= 0; j< pConfig->lwObj[i].resourceCount; j++)
			{
				if(eRequest[ucRequestTail].resourceIds[eRequest[ucRequestTail].resourceIndex]== pConfig->lwObj[i].resource[j].attr.id)
				{
					pConfig->lwObj[i].resource[j].value.execute(&(pConfig->lwObj[i]), pConfig->lwObj[i].resource[j].attr.id);/*execute*/
					eBc66Trx.lwm2m.ExecuteResponse.result= CHANGED_Bc66LinkLwm2mRequestResult;
					break;
				}
			}

			break;
		}
	}

	eBc66Trx.lwm2mType= eRequest[ucRequestTail].lwm2mType;
	if(SUCCESS== BC66LINK_Enqueue(&eBc66Trx))
	{
		eRequest[ucRequestTail].resourceIndex++;
	}
}

ErrorStatus LWM2M_SendNotify(uint8_t _obj, uint8_t _rsc)
{
	if((true== pConfig->reregisterAfterPSM)&& (true== BC66LINK_IsInPSM())&& (false== bRegisterIsRequested))/*for some server, connection is lost after PSM (perhaps due to diff port when reconnect)*/
	{
		bRegisterIsRequested= true;
	}
	else
	{
		eBc66Trx.lwm2mType= NOTIFY_Bc66LinkLwm2m;
		eBc66Trx.lwm2m.Notify.lifetime= pConfig->currentConnection.lifetime_s;
		eBc66Trx.lwm2m.Notify.objectId= pConfig->lwObj[_obj].id;
		eBc66Trx.lwm2m.Notify.instanceId= pConfig->lwObj[_obj].instance;
		eBc66Trx.lwm2m.Notify.resourceId= pConfig->lwObj[_obj].resource[_rsc].attr.id;
		eBc66Trx.lwm2m.Notify.valueType= (BC66LINK_Lwm2m_Value_Type_t)pConfig->lwObj[_obj].resource[_rsc].attr.type;
		eBc66Trx.lwm2m.Notify.value= (char *)pucValueBuffer;
		eBc66Trx.lwm2m.Notify.index= 0;/*TODO: utilise this rather than sending separate packets*/
		eBc66Trx.lwm2m.Notify.ack= pConfig->notifyACK;
		eBc66Trx.lwm2m.Notify.raiFlag= pConfig->notifyRAI;
		eBc66Trx.lwm2m.Notify.objectIndex= _obj;
		eBc66Trx.lwm2m.Notify.resourceIndex= _rsc;

		LWM2M_FillValues(&pConfig->lwObj[_obj], _rsc, (char *) pucValueBuffer, &(eBc66Trx.lwm2m.Notify.len));

		if(SUCCESS== BC66LINK_Enqueue(&eBc66Trx))
		{
			return SUCCESS;
		}
	}

	return ERROR;
}

void LWM2M_ProcessNotify(void)
{
	static uint8_t _i= 0;
	static uint8_t _j= 0;

	for(; _i< MAX_ObjName; _i++)
	{
		for(; _j< pConfig->lwObj[_i].resourceCount; _j++)
		{
			if((FIRST_RETRY_NotifyState== pConfig->lwObj[_i].resource[_j].notifyState)
					||(SECOND_RETRY_NotifyState== pConfig->lwObj[_i].resource[_j].notifyState))
			{
				if(SYS_GetTimestamp_s()> ulNotifyRetryTime)
				{
					if(SUCCESS== LWM2M_SendNotify(_i, _j))
					{
						pConfig->lwObj[_i].resource[_j].notifyState= (FIRST_RETRY_NotifyState== pConfig->lwObj[_i].resource[_j].notifyState)? SECOND_RETRY_NotifyState: RETRY_DONE_NotifyState;
						return;/*exit, we do one by one*/
					}
				}
			}

			if(DO_NOTIFY_NotifyState== pConfig->lwObj[_i].resource[_j].notifyState)
			{
				if(SUCCESS== LWM2M_SendNotify(_i, _j))
				{
					pConfig->lwObj[_i].resource[_j].notifyState= IDLE_NotifyState;
					_j++;
				}
				return;/*exit, we do one by one*/
			}
		}
		_j= 0;
	}
	_i= 0;

	return;
}

void LWM2M_ClearObserve(void)
{
	for(int i= 0; i< MAX_ObjName; i++)
	{
		for(int j= 0; j< pConfig->lwObj[i].resourceCount; j++)
		{
			pConfig->lwObj[i].resource[j].observe= false;
			pConfig->lwObj[i].resource[j].notifyState= IDLE_NotifyState;
		}
	}

	return;
}

void LWM2M_NotifyCallback(bool _isSuccessful, uint8_t _ackStatus, LWOBJ_Obj_t *_lwObj, uint16_t _resourceNo)
{
	switch(_lwObj->type)
	{
		case PRACT_ObjType:
			switch(_resourceNo)
			{
				case RECORD_PractResource:
					if((true== _isSuccessful)&& (0== _ackStatus))
					{
						/*increment Read index*/
						_lwObj->resource[RECORD_READ_PractResource].value.integer+= _lwObj->rte.prAct->recordDispatched;
						_lwObj->rte.prAct->recordDispatched= 0;
						if(true== _lwObj->resource[RECORD_READ_PractResource].observe)
						{
							_lwObj->resource[RECORD_READ_PractResource].notifyState= DO_NOTIFY_NotifyState;
						}

						/*check if we still need to notify(we might truncate previously because of buffer size)*/
						if(_lwObj->resource[RECORD_READ_PractResource].value.integer< _lwObj->resource[RECORD_HEAD_PractResource].value.integer)
						{
							if(true== _lwObj->resource[RECORD_PractResource].observe)
							{
								_lwObj->resource[RECORD_PractResource].notifyState= DO_NOTIFY_NotifyState;
							}
						}
					}
					break;
			}
			break;

		case DTMON_ObjType:
			switch(_resourceNo)
			{
				case DATA_DtmonResource:
				case REFERENCE_A_DtmonResource:
				case REFERENCE_B_DtmonResource:
				case REFERENCE_C_DtmonResource:
				case RESULTS_DtmonResource:
					break;
			}
			break;

		default:
			break;
	}

	if(9== _ackStatus)
	{
		/*we need to reset observe flag*/
		_lwObj->resource[_resourceNo].observe= false;
	}
	else if(1== _ackStatus)
	{
		/*notify rejected, we may need to re-register*/
		/*this won't do it cos of auto register*///LWM2M_RequestRegister();
		BC66LINK_ResetModem();/*Quickfix: reboot modem to force registration*/
	}
	else if((false== _isSuccessful)|| (0!= _ackStatus))/*other issue, timeout or link issue*/
	{
		/*need to backoff and resend*/
		if(IDLE_NotifyState== _lwObj->resource[_resourceNo].notifyState)
		{
			_lwObj->resource[_resourceNo].notifyState= FIRST_RETRY_NotifyState;
		}
		else if(RETRY_DONE_NotifyState== _lwObj->resource[_resourceNo].notifyState)
		{
			_lwObj->resource[_resourceNo].notifyState= IDLE_NotifyState;
		}

		ulNotifyRetryTime= SYS_GetTimestamp_s()+ UTILI_GetRandom(config.nbiot.lwm2m.notifyRetryBackoffMin_s, config.nbiot.lwm2m.notifyRetryBackoffMax_s);
	}
}

void SWMGT_Activate(LWOBJ_Obj_t *pLwObj);
void LWM2M_ExecuteCallback(bool _isSuccessful, LWOBJ_Obj_t *_lwObj, uint16_t _resourceNo)
{
	switch(_lwObj->type)
	{
		case SOFTWARE_MANAGEMENT_ObjName:
			switch(_resourceNo)
			{
				case ACTIVATE_SwMgtResource:
					if(true== _isSuccessful)
					{
						SWMGT_Activate(_lwObj);
					}
					break;
			}
			break;

		default:
			break;
	}
}

void LWM2M_CustomInit(void)
{
	if(true== pConfig->diversifyPractDispatchMask)
	{
		pConfig->diversifyPractDispatchMask= false;/*this is a one time thing, as default mask value.*/

		for(int i= 0; i< MAX_ObjName; i++)
		{
			if(PRACT_ObjType== pConfig->lwObj[i].type)
			{
				/*diversify based on serial number*/
				char _sNoString[5];
				memcpy(_sNoString, config.system.meterSerialNo+ (strlen(config.system.meterSerialNo)- 4), 4);
				if((_sNoString[3]< 0x30)|| (_sNoString[3]> 0x39))/*last digit may not be a number*/
				{
					memcpy(_sNoString, config.system.meterSerialNo+ (strlen(config.system.meterSerialNo)- 5), 4);
				}
				_sNoString[4]= '\0';
				__IO uint32_t _seconds= atoi(_sNoString);
				_seconds= (_seconds* pConfig->practDispatchMaskInterval)% pConfig->practDispatchMaskPeriod;
				_seconds+= pConfig->practDispatchMaskOffset;
				uint8_t _hour= _seconds/ 3600;
				_seconds-= (_hour* 3600);
				uint8_t _minute= _seconds/ 60;
				_seconds-= (_minute* 60);

				/*write back to mask*/
				_hour= UTILI_BinToBCD(_hour);
				_minute= UTILI_BinToBCD(_minute);
				_seconds= UTILI_BinToBCD(_seconds);
				UTILI_BytesToHexString(&_hour, 1, pConfig->lwObj[i].resource[RECORD_DISPATCH_START_MASK_PractResource].value.string+ 6);
				UTILI_BytesToHexString(&_minute, 1, pConfig->lwObj[i].resource[RECORD_DISPATCH_START_MASK_PractResource].value.string+ 8);
				UTILI_BytesToHexString(&_seconds, 1, pConfig->lwObj[i].resource[RECORD_DISPATCH_START_MASK_PractResource].value.string+ 10);
			}
		}
	}
}

void LWM2M_Init(LWM2M_t *_config)
{
	pConfig= _config;

	LWM2M_CustomInit();
	LWOBJ_Init();

	BC66LINK_LWM2M_SetWriteRequestCallback(LWM2M_WriteRequest);
	BC66LINK_LWM2M_SetReadRequestCallback(LWM2M_ReadRequest);
	BC66LINK_LWM2M_SetObserveRequestCallback(LWM2M_ObserveRequest);
	BC66LINK_LWM2M_SetExecuteRequestCallback(LWM2M_ExecuteRequest);
	BC66LINK_LWM2M_SetRecoveryCallback(LWM2M_Recovery);
	BC66LINK_LWM2M_SetRegisterCallback(LWM2M_Register);


//	RTC_AlarmStartMarker_t _marker;
//	_marker.date=  0xFF;
//	_marker.hour=  0xFF;
//	_marker.minute=  0xFF;
//	_marker.second=  0xFF;
//	DEVICELOG_EnablePeriodicLog(true);
//	DEVICELOG_StartLog(NOTSTARTED_StartType, 0, 10, _marker);
}

void LWM2M_Task(void)
{
	static bool _isIpso= true;
	static uint8_t _ipsoObjAddedCount= 0;
	static uint8_t _lwObjAddedCount= 0;

	LWOBJ_Task();

	if((true== eBc66Trx.transactionInQueue)|| (true== eBc66Trx.transactionInProgress))
	{
		return;
	}

	if(true== eBc66Trx.transactionCompleted)
	{
		eBc66Trx.transactionCompleted= false;

		if((CONFIGURE_Bc66LinkLwm2m== eBc66Trx.lwm2mType)&& (SUCCESS_TransactionStatus== eBc66Trx.status))
		{
			bIsConfigured= true;
		}
		else if((ADD_OBJECT_Bc66LinkLwm2m== eBc66Trx.lwm2mType)&& (SUCCESS_TransactionStatus== eBc66Trx.status))
		{
			if(_isIpso) _ipsoObjAddedCount++;
			else _lwObjAddedCount++;
		}
		else if(REGISTER_Bc66LinkLwm2m== eBc66Trx.lwm2mType)
		{
			if(SUCCESS_TransactionStatus== eBc66Trx.status)
			{
				if(true== bRegisterIsRequested)
				{
					bRegisterIsRequested= false;
				}
			}
			else
			{
				/*Register failed, backoff mechanism*/
				bRegisterIsRequested= false;
			}
		}
		else if((DEREGISTER_Bc66LinkLwm2m== eBc66Trx.lwm2mType))
		{
			if(SUCCESS_TransactionStatus== eBc66Trx.status)
			{
				bIsConfigured= false;
				bIsRegistered= false;
			}
			else
			{
				if(true== bRegisterIsRequested)
				{
					bRegisterIsRequested= false;/*normally this is triggered manually from user, they need to trigger registration again*/
				}
			}
		}
		else if(NOTIFY_Bc66LinkLwm2m== eBc66Trx.lwm2mType)
		{
			LWM2M_NotifyCallback((SUCCESS_TransactionStatus== eBc66Trx.status)? true: false, eBc66Trx.lwm2m.Notify.ackStatus,
					&(pConfig->lwObj[eBc66Trx.lwm2m.Notify.objectIndex]),
					eBc66Trx.lwm2m.Notify.resourceIndex);
		}
		else if(EXECUTE_RESPONSE_Bc66LinkLwm2m== eBc66Trx.lwm2mType)
		{
			LWM2M_ExecuteCallback((SUCCESS_TransactionStatus== eBc66Trx.status)? true: false,
					&(pConfig->lwObj[eBc66Trx.lwm2m.ExecuteResponse.objectId]),
					eBc66Trx.lwm2m.ExecuteResponse.resourceId);
		}
	}

	/*Check if we can already transmit or not.*/
	if(false== BC66LINK_TransmissionIsReady())
	{
		/*TODO: intermittent network disruption handling required, for example device is de-attached halfway.*/
		return;
	}

	if(false== bIsRegistered)
	{
		if(true== LWM2M_IsTimeout())/*Check if Start Time adherence is required.*/
		{
			bRegisterIsRequested= true;
			LWM2M_SetTimeout_s(pConfig->registerBackoff_s);
		}
	}

	/*Check if we need to first Register.*/
	if(MAX_ObjName> _lwObjAddedCount)
	{
		_isIpso= false;
		eBc66Trx.lwm2mType= ADD_OBJECT_Bc66LinkLwm2m;
		eBc66Trx.lwm2m.AddObject.objectId= pConfig->lwObj[_lwObjAddedCount].id;
		eBc66Trx.lwm2m.AddObject.instanceId= pConfig->lwObj[_lwObjAddedCount].instance;
		eBc66Trx.lwm2m.AddObject.noOfResource= pConfig->lwObj[_lwObjAddedCount].resourceCount;
		for(int i= 0; i< pConfig->lwObj[_lwObjAddedCount].resourceCount; i++)
		{
			eBc66Trx.lwm2m.AddObject.resourceId[i]= pConfig->lwObj[_lwObjAddedCount].resource[i].attr.id;
		}

		BC66LINK_Enqueue(&eBc66Trx);
	}
	else if(false== bIsConfigured)
	{
		eBc66Trx.lwm2mType= CONFIGURE_Bc66LinkLwm2m;
		eBc66Trx.lwm2m.Configure.enableBootstrap= pConfig->enableBootstrap;
		eBc66Trx.lwm2m.Configure.serverIP= pConfig->defaultConnection.serverIP;
		eBc66Trx.lwm2m.Configure.serverPort= pConfig->defaultConnection.serverPort;
		eBc66Trx.lwm2m.Configure.endpointName= pConfig->defaultConnection.endpointName;
		eBc66Trx.lwm2m.Configure.lifetime= pConfig->defaultConnection.lifetime_s;
		eBc66Trx.lwm2m.Configure.securityMode= pConfig->defaultConnection.securityMode;
		eBc66Trx.lwm2m.Configure.pskId= pConfig->defaultConnection.pskId;
		eBc66Trx.lwm2m.Configure.psk= pConfig->defaultConnection.psk;

		BC66LINK_Enqueue(&eBc66Trx);
	}
	else if(true== bRegisterIsRequested)
	{
		if(false== bIsRegistered)/*haven't register*/
		{
			LWM2M_ClearObserve();/*observe/notify are not supposed to run on new registration*/

			UTILI_Array_CopyString(pConfig->currentConnection.serverIP, pConfig->defaultConnection.serverIP);
			pConfig->currentConnection.serverPort= pConfig->defaultConnection.serverPort;
			UTILI_Array_CopyString(pConfig->currentConnection.endpointName, pConfig->defaultConnection.endpointName);
			pConfig->currentConnection.lifetime_s= pConfig->defaultConnection.lifetime_s;
			pConfig->currentConnection.securityMode= pConfig->defaultConnection.securityMode;
			UTILI_Array_CopyString(pConfig->currentConnection.pskId, pConfig->defaultConnection.pskId);
			UTILI_Array_CopyString(pConfig->currentConnection.psk, pConfig->defaultConnection.psk);

			eBc66Trx.lwm2mType= REGISTER_Bc66LinkLwm2m;

			BC66LINK_Enqueue(&eBc66Trx);
		}
		else/*have registered, user request re-register*/
		{
			eBc66Trx.lwm2mType= DEREGISTER_Bc66LinkLwm2m;

			BC66LINK_Enqueue(&eBc66Trx);
		}
	}
	else if(true== bPostRegisterIsRequested)
	{
		bPostRegisterIsRequested= false;
		eBc66Trx.lwm2mType= POSTREGISTER_Bc66LinkLwm2m;
		eBc66Trx.lwm2m.PostRegister.pingServerIP= pConfig->currentConnection.serverIP;

		BC66LINK_Enqueue(&eBc66Trx);
	}
	else if(eRequest[ucRequestTail].resourceIndex!= eRequest[ucRequestTail].resourceCount)/*still have unfinished business, ignore*/
	{
		switch(eRequest[ucRequestTail].lwm2mType)
		{
			case WRITE_RESPONSE_Bc66LinkLwm2m:
				LWM2M_ProcessWriteRequest();
				break;
			case READ_RESPONSE_Bc66LinkLwm2m:
				LWM2M_ProcessReadRequest();
				break;
			case EXECUTE_RESPONSE_Bc66LinkLwm2m:
				LWM2M_ProcessExecuteRequest();
				break;
			case OBSERVE_RESPONSE_Bc66LinkLwm2m:
				LWM2M_ProcessObserveRequest();
				break;
			default:
				break;
		}

		if(eRequest[ucRequestTail].resourceIndex== eRequest[ucRequestTail].resourceCount)
		{
			ucRequestTail= (++ucRequestTail)% LWM2M_CFG_MAX_REQUEST;
		}
	}
	else
	{
		LWM2M_ProcessNotify();
	}


	/*Check if we need to response to object query*/

	/*Check if we need to update the object.*/

}

uint8_t LWM2M_TaskState(void)
{
	return (BC66LINK_TaskState()| LWOBJ_TaskState());
}
