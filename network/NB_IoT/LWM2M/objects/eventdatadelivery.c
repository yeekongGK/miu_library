/*
 * eventdatdelivery.c
 *
 *  Created on: 18 Sep 2022
 *      Author: muhammad.ahmad@georgekent.net
 */

#include "common.h"
#include "lwm2m.h"
#include "cbor.h"
#include "lwobject.h"
#include "eventdatadelivery.h"

#define DBG_Print

/*NOTE:
 * This object is created just to accommodate PUB-GovTech Decada integration,
 * particularly non-real time backflow alarm.
 * Thus writing to resource not supported and object usage is not proper*/

float DTMON_DecodeCBORDataResource(uint8_t *_cborResource, uint16_t _cborResourceLen);

uint16_t EDD_EncodeCBOREvent(EDD_EventPayload_t* _payload, uint8_t *_cborBuffer)
{
	cbor_stream_t _stream;
	uint8_t *_buf= _cborBuffer;
	uint16_t _size= 256;
	cbor_init(&_stream, _buf, _size+ 1);/*+1, might be  a bug on cbor lib*/

	switch(_payload->code)
	{
		case EMPTY_PIPE_EDDAlarmCode:
			cbor_serialize_array(&_stream, 3);
			cbor_serialize_int(&_stream, _payload->code);
			cbor_serialize_int(&_stream, _payload->type);
			cbor_serialize_array(&_stream, 2);
			cbor_serialize_array(&_stream, 2);
			cbor_serialize_int(&_stream, _payload->timestamp1);
			cbor_serialize_int(&_stream, _payload->alarm1);
			cbor_serialize_array(&_stream, 2);
			cbor_serialize_int(&_stream, _payload->timestamp2);
			cbor_serialize_int(&_stream, _payload->alarm2);
			break;
		case HIGH_TEMP_EDDAlarmCode:
			cbor_serialize_array(&_stream, 3);
			cbor_serialize_int(&_stream, _payload->code);
			cbor_serialize_int(&_stream, _payload->type);
			cbor_serialize_array(&_stream, 2);
			cbor_serialize_array(&_stream, 3);
			cbor_serialize_int(&_stream, _payload->timestamp1);
			cbor_serialize_int(&_stream, _payload->alarm1);
			cbor_serialize_int(&_stream, _payload->value1);
			cbor_serialize_array(&_stream, 3);
			cbor_serialize_int(&_stream, _payload->timestamp2);
			cbor_serialize_int(&_stream, _payload->alarm2);
			cbor_serialize_int(&_stream, _payload->value2);
			break;
		case REVERSE_FLOW_NRT_EDDAlarmCode:
			cbor_serialize_array(&_stream, 3);
			cbor_serialize_int(&_stream, _payload->code);
			cbor_serialize_int(&_stream, _payload->type);
			cbor_serialize_array(&_stream, 2);
			cbor_serialize_int(&_stream, _payload->timestamp1);
			cbor_serialize_int(&_stream, _payload->value1);
			break;
		default:
			cbor_serialize_array(&_stream, 3);
			cbor_serialize_int(&_stream, _payload->code);
			cbor_serialize_int(&_stream, _payload->type);
			cbor_serialize_array(&_stream, 2);
			cbor_serialize_int(&_stream, _payload->timestamp1);
			cbor_serialize_int(&_stream, _payload->alarm1);
			break;
	}

	return _stream.pos;
}

void EDD_RefreshResources(LWOBJ_Obj_t *pLwObj)
{
}

void EDD_NotifyResource(LWOBJ_Obj_t *pLwObj, uint8_t _resourceNo, bool _force)
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

void EDD_Init(LWOBJ_Obj_t *pLwObj)
{
	BC66LINK_Init();
}

time_t EDD_Task(LWOBJ_Obj_t *pLwObj, time_t _currTime, uint64_t _currMask)
{
	time_t _awakeTime= _currTime+ LWOBJ_CFG_MAX_SLEEP_TIME_S;

	if(true== pLwObj->written)
	{
		pLwObj->written= false;
		/*not support*/
	}

	/*
	 * 1. Check if there is event data from data monitoring object
	 * 2. Notify server*/

//	__IO uint32_t _currBitmap= (uint32_t) DTMON_DecodeCBORDataResource(
//			config.nbiot.lwm2m.lwObj[DTMON_ALARMS_ObjName].resource[REFERENCE_A_DtmonResource].value.opaque,
//			config.nbiot.lwm2m.lwObj[DTMON_ALARMS_ObjName].resource[REFERENCE_A_DtmonResource].valueLen
//			);
//	uint32_t _deltaBitmap= _currBitmap^ pLwObj->rte.edd->prevBitmap;
//
//	if((0!= _deltaBitmap)
//			&& (false!= pLwObj->resource[LATEST_EVENT_LOG_EDDResource].notify)/*not sure if server support array of payload, so we only transmit one at a time*/
//			)
//	{
//		EDD_EventPayload_t _payload;
//		_payload.code= 0;
//
//		if(0!= ((_deltaBitmap& (1<< 0)))|| (0!= (_deltaBitmap& (1<< 1)))|| (0!= (_deltaBitmap& (1<< 2))))/*TAMPER_EDDAlarmCode*/
//		{
//			_payload.code= TAMPER_EDDAlarmCode;
//			_payload.type= 2;
//			_payload.timestamp1= SYS_GetTimestampUTC_s();
//			_payload.alarm1= (0!= ((_currBitmap& (1<< 0)))|| (0!= (_currBitmap& (1<< 1)))|| (0!= (_currBitmap& (1<< 2))))? 1: 0;
//		}
//		else if(0!= (_deltaBitmap& (1<< 3)))/*PULSER_FAULT_Sensor*/
//		{
//		}
//		else if(0!= (_deltaBitmap& (1<< 4)))/*Low Batt*/
//		{
//			_payload.code= LOW_BATT_EDDAlarmCode;
//			_payload.type= 1;
//			_payload.timestamp1= SYS_GetTimestampUTC_s();
//			_payload.alarm1= (0!= (_currBitmap& (1<< 4)))? 1: 0;
//		}
//		else if(0!= (_deltaBitmap& (1<< 5)))/*back flow*/
//		{
//			_payload.code= REVERSE_FLOW_EDDAlarmCode;
//			_payload.type= 1;
//			_payload.timestamp1= SYS_GetTimestampUTC_s();
//			_payload.alarm1= 0== (_deltaBitmap& (1<< 8))? 0: 1;
//		}
//		else if(0!= (_deltaBitmap& (1<< 6)))/*burst*/
//		{
//		}
//		else if(0!= (_deltaBitmap& (1<< 7)))/*no flow*/
//		{
////			_payload.code= EMPTY_PIPE_EDDAlarmCode;
////			_payload.type= 2;
////			_payload.timestamp1= SYS_GetTimestampUTC_s();
////			_payload.alarm1= (0!= (_deltaBitmap& (1<< 7)))? 1: 0;
////			_payload.timestamp2= SYS_GetTimestampUTC_s();
////			_payload.alarm2= (0!= (_deltaBitmap& (1<< 7)))? 1: 0;
//		}
//		else if(0!= (_deltaBitmap& (1<< 8)))/*leakage*/
//		{
//			_payload.code= CUST_LEAKAGE_EDDAlarmCode;
//			_payload.type= 1;
//			_payload.timestamp1= SYS_GetTimestampUTC_s();
//			_payload.alarm1= (0!= (_currBitmap& (1<< 8)))? 1: 0;
//		}
//		else if(0) /*REVERSE_FLOW_NRT*/
//		{
//			_payload.code= REVERSE_FLOW_NRT_EDDAlarmCode;
//			_payload.type= 2;
//			_payload.timestamp1= SYS_GetTimestampUTC_s();
//			_payload.value1= (uint32_t)SENSOR_GetValue(PIPE_BACKFLOW_Sensor);
//		}
//		else if(0) /*REBOOT_EDDAlarmCode*/
//		{
//			_payload.code= REBOOT_EDDAlarmCode;
//			_payload.type= 3;
//			_payload.timestamp1= SYS_GetTimestampUTC_s();
//			_payload.value1= config.failsafe.rteResetLogCounter;
//		}
//
//		if(0!= _payload.code)
//		{
//			pLwObj->resource[LATEST_EVENT_LOG_EDDResource].valueLen=
//					EDD_EncodeCBOREvent(&_payload, pLwObj->resource[LATEST_EVENT_LOG_EDDResource].value.opaque);
//			EDD_NotifyResource(pLwObj, DATA_DtmonResource, false);
//		}
//	}

	return _awakeTime;
}
