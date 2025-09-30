/*
 * datamonitoring.c
 *
 *  Created on: 2 Nov 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

//#include "common.h"
#include "lwm2m.h"
#include "lwobject.h"
#include "sensor.h"
#include "datamonitoring.h"
#include "cbor.h"

#define DBG_Print

void DTMON_Diag(DTMON_DtmonInstance_t _instance, DTMON_DtmonDiagCode_t _code, uint32_t _value)
{
	switch(_instance)
	{
		case ALARMS_DtmonInstance:
			DIAG_Code((NEXT_SAMPLE_TIME_DtmonDiagCode== _code)?ALARM_NEXT_SAMPLE_TIME_NbiotLwm2mDCode: ALARM_STOP_SAMPLE_TIME_NbiotLwm2mDCode, _value);
			break;
	}
}

size_t DTMON_DecodeCBORFloat(const cbor_stream_t *_stream, size_t _offset, float *_val)
{
	size_t _ret= 0;

	_ret= cbor_deserialize_float(_stream, _offset, _val);
	if(0== _ret)
	{
		_ret= cbor_deserialize_float_half(_stream, _offset, _val);

		if(0== _ret)
		{
			uint32_t _temp;
			_ret= cbor_deserialize_int(_stream, _offset, (int *)&_temp);
			if(0!= _ret)
			{
				*_val= _temp/ 1.0;
			}
		}
	}
	return _ret;
}

uint16_t DTMON_EncodeCBORDataResource(float _data, float _nrtBackflow, uint8_t *_cborBuffer)
{
	cbor_stream_t _stream;
	uint8_t *_buf= _cborBuffer;
	uint16_t _size= DTMON_CFG_DATA_RESOURCE_LEN;
	uint32_t _timestamp= (uint32_t) SYS_GetTimestampUTC_s();
	float _meterReading= PULSER_ValueInLiter_Get();

	cbor_init(&_stream, _buf, _size+ 1);/*+1, might be  a bug on cbor lib*/

	cbor_serialize_array(&_stream, 4);
	cbor_serialize_float(&_stream, _data);
	cbor_serialize_float(&_stream, _nrtBackflow);
	cbor_serialize_int(&_stream, _timestamp);
	cbor_serialize_float(&_stream, _meterReading);

	return _stream.pos;
}

float DTMON_DecodeCBORDataResource(uint8_t *_cborResource, uint16_t _cborResourceLen)
{
	float _data;
	cbor_stream_t _stream= {_cborResource, _cborResourceLen, 0};

	if(0== cbor_deserialize_float(&_stream, 0, &_data))
	{
		cbor_deserialize_float_half(&_stream, 0, &_data);
	}

	return _data;
}

uint16_t DTMON_EncodeCBORSettingsResource(DTMON_DtmonInstance_t _instance, DTMON_Settings_t *_settings, uint8_t *_cborBuffer)
{
	cbor_stream_t _stream;
	uint8_t *_buf= _cborBuffer;
	uint16_t _size= DTMON_CFG_SETTINGS_RESOURCE_MAX_LEN;

	cbor_init(&_stream, _buf, _size+ 1);/*+1, might be  a bug on cbor lib*/

	switch(_instance)
	{
		case ALARMS_DtmonInstance:
		{
			/*we need to sync with sensors settings*/
			_settings->alarms.backflowThreshold_l= config.sensors.flow.backflowThreshold;
			_settings->alarms.burstThreshold_lph= config.sensors.flow.burstThreshold;
			_settings->alarms.burstSamplingPeriod_s= config.sensors.flow.burstSamplingPeriod_s;
			_settings->alarms.noflowSamplingPeriod_s= config.sensors.flow.noflowSamplingPeriod_s;
			_settings->alarms.leakageThreshold_lph= config.sensors.flow.leakageThreshold;
			_settings->alarms.leakageSamplingPeriod_s= config.sensors.flow.leakageSamplingPeriod_s;

			cbor_serialize_array(&_stream, 9);
			cbor_serialize_int(&_stream, _settings->alarms.activePeriod_s);
			cbor_serialize_int(&_stream, _settings->alarms.backflowThreshold_l);
			cbor_serialize_float(&_stream, _settings->alarms.burstThreshold_lph);
			cbor_serialize_int(&_stream, _settings->alarms.burstSamplingPeriod_s);
			cbor_serialize_int(&_stream, _settings->alarms.noflowSamplingPeriod_s);
			cbor_serialize_float(&_stream, _settings->alarms.leakageThreshold_lph);
			cbor_serialize_int(&_stream, _settings->alarms.leakageSamplingPeriod_s);
			cbor_serialize_int(&_stream, _settings->alarms.lowBatteryLevel_percent);
			cbor_serialize_int(&_stream, _settings->alarms.nrtBackflowSamplingPeriod_s);
			break;
		}

		default:
			break;
	}
	return _stream.pos;
}

void DTMON_DecodeCBORSettingsResource(DTMON_DtmonInstance_t _instance, uint8_t *_cborBuffer, DTMON_Settings_t *_settings)
{
	cbor_stream_t _stream;
	uint8_t *_buf= _cborBuffer;
	uint16_t _size= DTMON_CFG_SETTINGS_RESOURCE_MAX_LEN;

	cbor_init(&_stream, _buf, _size+ 1);/*+1, might be  a bug on cbor lib*/

	switch(_instance)
	{
		case ALARMS_DtmonInstance:
		{
			size_t _arraySize= 0;
			size_t _offset= 0;
			_offset+= cbor_deserialize_array(&_stream, _offset, &_arraySize);
			if(9== _arraySize)
			{
				_offset+= cbor_deserialize_int(&_stream, _offset, (int *)&(_settings->alarms.activePeriod_s));
				_offset+= cbor_deserialize_int(&_stream, _offset, (int *)&(_settings->alarms.backflowThreshold_l));
				_offset+= DTMON_DecodeCBORFloat(&_stream, _offset, &(_settings->alarms.burstThreshold_lph));
				_offset+= cbor_deserialize_int(&_stream, _offset, (int *)&(_settings->alarms.burstSamplingPeriod_s));
				_offset+= cbor_deserialize_int(&_stream, _offset, (int *)&(_settings->alarms.noflowSamplingPeriod_s));
				_offset+= DTMON_DecodeCBORFloat(&_stream, _offset, &(_settings->alarms.leakageThreshold_lph));
				_offset+= cbor_deserialize_int(&_stream, _offset, (int *)&(_settings->alarms.leakageSamplingPeriod_s));
				_offset+= cbor_deserialize_int(&_stream, _offset, (int *)&(_settings->alarms.lowBatteryLevel_percent));
				_offset+= cbor_deserialize_int(&_stream, _offset, (int *)&(_settings->alarms.nrtBackflowSamplingPeriod_s));

				/*we need to sync with sensors settings*/
				config.sensors.flow.backflowThreshold= _settings->alarms.backflowThreshold_l;
				config.sensors.flow.burstThreshold= _settings->alarms.burstThreshold_lph;
				config.sensors.flow.burstSamplingPeriod_s= _settings->alarms.burstSamplingPeriod_s;
				config.sensors.flow.noflowSamplingPeriod_s= _settings->alarms.noflowSamplingPeriod_s;
				config.sensors.flow.leakageThreshold= _settings->alarms.leakageThreshold_lph;
				config.sensors.flow.leakageSamplingPeriod_s= _settings->alarms.leakageSamplingPeriod_s;
				FLOWSENSOR_Init(&(config.sensors.flow));

				config.sensors.battery.lowThreshold= _settings->alarms.lowBatteryLevel_percent;
			}
			break;
		}

		default:
			break;
	}
}

float DTMON_SampleData(LWOBJ_Obj_t *pLwObj)
{
	static bool bResetDetected= true; /*by default reset is detected*/
	float _data= -1;
	switch(pLwObj->instance)
	{
		case ALARMS_DtmonInstance:
			{
				time_t _currTime= SYS_GetTimestamp_s();
				static float _prevAlarmData= -1;
				if(
					(0!= pLwObj->rte.dtMon->settings.alarms.activePeriod_s)
					&&(0!= pLwObj->rte.dtMon->settings.alarms.rteClearTimestamp)
					&&(_currTime>= pLwObj->rte.dtMon->settings.alarms.rteClearTimestamp)
				)
				{
					pLwObj->rte.dtMon->settings.alarms.rteClearTimestamp= 0;
					SENSOR_ClearValue(PBMAGCOUNT_Sensor);
					SENSOR_ClearValue(TILTCOUNT_Sensor);
					SENSOR_ClearValue(TAMPERINCOUNT_Sensor);
					SENSOR_ClearValue(PULSER_FAULT_Sensor);
					SENSOR_ClearValue(CELLCAPACITY_PERCENTAGE_Sensor);
					SENSOR_ClearValue(PIPE_BACKFLOW_FLAG_Sensor);
					SENSOR_ClearValue(PIPE_BURST_FLAG_Sensor);
					SENSOR_ClearValue(PIPE_NOFLOW_FLAG_Sensor);
					SENSOR_ClearValue(PIPE_LEAKAGE_FLAG_Sensor);
					SENSOR_ClearValue(UNINTENTITONAL_MODEM_RESET_FLAG_Sensor);
					SENSOR_ClearValue(NFC_ACCESS_FLAG_Sensor);
					SENSOR_ClearValue(NFC_INVALID_ACCESS_FLAG_Sensor);
					SENSOR_ClearValue(EEPROM_ACCESS_ERROR_FLAG_Sensor);
				}

				//pLwObj->rte.dtMon->nrtBackflowValue= 0;
				if((0== pLwObj->rte.dtMon->backflowStartTime)&& (0!= SENSOR_GetValue(PIPE_BACKFLOW_FLAG_Sensor)))
				{
					pLwObj->rte.dtMon->backflowStartTime= _currTime;
				}
				else if(_currTime>= ((pLwObj->rte.dtMon->backflowStartTime)+ pLwObj->rte.dtMon->settings.alarms.nrtBackflowSamplingPeriod_s))
				{
					pLwObj->rte.dtMon->backflowStartTime= 0;
					pLwObj->rte.dtMon->nrtBackflowValue= SENSOR_GetValue(PIPE_BACKFLOW_Sensor);
				}

				uint32_t _bitmap= 0;
				_bitmap|= (((0!= SENSOR_GetValue(PBMAGCOUNT_Sensor))? 1: 0)<< 0);
				_bitmap|= (((0!= SENSOR_GetValue(TILTCOUNT_Sensor))? 1: 0)<< 1);
				_bitmap|= (((0!= SENSOR_GetValue(TAMPERINCOUNT_Sensor))? 1: 0)<< 2);
				_bitmap|= (((0!= SENSOR_GetValue(PULSER_FAULT_Sensor))? 1: 0)<< 3);
				_bitmap|= (((config.sensors.battery.lowThreshold>= SENSOR_GetValue(CELLCAPACITY_PERCENTAGE_Sensor))? 1: 0)<< 4);
				_bitmap|= (((0!= SENSOR_GetValue(PIPE_BACKFLOW_FLAG_Sensor))? 1: 0)<< 5);
				_bitmap|= (((0!= SENSOR_GetValue(PIPE_BURST_FLAG_Sensor))? 1: 0)<< 6);
				_bitmap|= (((0!= SENSOR_GetValue(PIPE_NOFLOW_FLAG_Sensor))? 1: 0)<< 7);
				_bitmap|= (((0!= SENSOR_GetValue(PIPE_LEAKAGE_FLAG_Sensor))? 1: 0)<< 8);
				_bitmap|= (((true== bResetDetected)? 1: 0)<< 9);/*treat as reboot alarm*/
				_bitmap|= (((0!= SENSOR_GetValue(UNINTENTITONAL_MODEM_RESET_FLAG_Sensor))? 1: 0)<< 9); /*treat as reboot alarm as well*/
				_bitmap|= (((0!= pLwObj->rte.dtMon->nrtBackflowValue)? 1: 0)<< 10);
				_bitmap|= (((0!= SENSOR_GetValue(NFC_ACCESS_FLAG_Sensor))? 1: 0)<< 11);
				_bitmap|= (((0!= SENSOR_GetValue(NFC_INVALID_ACCESS_FLAG_Sensor))? 1: 0)<< 12);
				_bitmap|= (((0!= SENSOR_GetValue(EEPROM_ACCESS_ERROR_FLAG_Sensor))? 1: 0)<< 13);

				bResetDetected= false;

				_data= _bitmap/ 1.0;
				if(_prevAlarmData!= _data)
				{
					/*only auto clear when there is changes from before*/
					_prevAlarmData= _data;
					pLwObj->rte.dtMon->settings.alarms.rteClearTimestamp= _currTime+ pLwObj->rte.dtMon->settings.alarms.activePeriod_s;
				}
			}
			break;
		default:
			break;
	}
	return _data;
}

bool DTMON_CompareData(float _data, float _reference, DTMON_DtmonComparison_t _comparison)
{
	switch(_comparison)
	{
		case NONE_DtmonComparison:
			return false;

		case DATA_GREATER_THAN_REF_DtmonComparison:
			return (_data> _reference)? true: false;

		case DATA_GREATER_THAN_OR_EQUAL_TO_REF_DtmonComparison:
			return (_data>= _reference)? true: false;

		case DATA_LESS_THAN_REF_DtmonComparison:
			return (_data< _reference)? true: false;

		case DATA_LESS_THAN_OR_EQUAL_TO_REF_DtmonComparison:
			return (_data<= _reference)? true: false;

		case DATA_EQUALS_TO_REF_DtmonComparison:
			return (_data= _reference)? true: false;

		case DATA_NOT_EQUAL_TO_REF_DtmonComparison:
			return (_data!= _reference)? true: false;

		default:
			return false;
	}
}

void DTMON_RefreshResources(LWOBJ_Obj_t *pLwObj)
{
	DTMON_DecodeCBORSettingsResource(pLwObj->instance, pLwObj->resource[SETTINGS_DtmonResource].value.opaque, &(pLwObj->rte.dtMon->settings));
	pLwObj->rte.dtMon->samplingStartMask= UTILI_Mask_Decode(pLwObj->resource[SAMPLING_START_MASK_DtmonResource].value.string);

	if(WAIT_START_DtmonState< pLwObj->rte.dtMon->state)
	{
		/*already running, we need to adjust nextSample time*/
		pLwObj->rte.dtMon->nextSamplingTime= UTILI_ComputeNextTime(SYS_GetTimestamp_s(), pLwObj->rte.dtMon->startSamplingTime, pLwObj->resource[SAMPLING_INTERVAL_DtmonResource].value.integer);
	}
	else
	{
		/*re-start*/
		pLwObj->rte.dtMon->state= STOPPED_DtmonState;
	}

	if(STOPPED_DtmonState== pLwObj->rte.dtMon->state)
	{
		/*fill initial data*/
		float _data= DTMON_SampleData(pLwObj);
		pLwObj->resource[DATA_DtmonResource].valueLen= DTMON_EncodeCBORDataResource(_data,  pLwObj->rte.dtMon->nrtBackflowValue, pLwObj->resource[DATA_DtmonResource].value.opaque);
		//pLwObj->resource[DATA_DtmonResource].valueLen= DTMON_EncodeCBORDataResource(0b11111111111/ 1.0, 99999.999, pLwObj->resource[DATA_DtmonResource].value.opaque);
		pLwObj->rte.dtMon->prevData= _data;
	}
}

void DTMON_NotifyResource(LWOBJ_Obj_t *pLwObj, uint8_t _resourceNo, bool _force)
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

void DTMON_Init(LWOBJ_Obj_t *pLwObj)
{
	int _instance= pLwObj->instance;

	/*read only*/
	pLwObj->resource[NAME_DtmonResource].value.string= (char *)DTMON_TEXT[_instance][0];
	pLwObj->resource[NAME_DtmonResource].valueLen= 	strlen(DTMON_TEXT[_instance][0]);
	pLwObj->resource[DESC_DtmonResource].value.string= (char *)DTMON_TEXT[_instance][1];
	pLwObj->resource[DESC_DtmonResource].valueLen= strlen(DTMON_TEXT[_instance][1]);
	/*read write*/
	pLwObj->resource[SETTINGS_DtmonResource].valueLen=
			DTMON_EncodeCBORSettingsResource(pLwObj->instance, &(pLwObj->rte.dtMon->settings), pLwObj->resource[SETTINGS_DtmonResource].value.opaque);

	//uint8_t _temp[]={0x88, 0x1A, 0x00, 0x01, 0x51, 0x80, 0x10, 0xF9, 0x69, 0xDC, 0x18, 0x1E, 0x19, 0x01, 0x2C, 0xF9, 0x4C, 0x00, 0x19, 0x01, 0x2C, 0x00};
	//uint8_t _temp[]={0x88, 0x1A, 0x00, 0x01, 0x51, 0x80, 0x10, 0x19, 0x0B, 0xB8, 0x18, 0x1E, 0x19, 0x01, 0x2C, 0x10, 0x19, 0x01, 0x2C, 0x00};
	//memcpy(pLwObj->resource[SETTINGS_DtmonResource].value.opaque, _temp, sizeof(_temp));
	//pLwObj->resource[SETTINGS_DtmonResource].valueLen= sizeof(_temp);

//	pLwObj->resource[SETTINGS_DtmonResource].value.string= pLwObj->placeHolder.dtMon.settings;
//	pLwObj->resource[SETTINGS_DtmonResource].valueLen= strlen(pLwObj->placeHolder.dtMon.settings);
//	pLwObj->resource[SETTINGS_DtmonResource].valueMaxLen= DTMON_CFG_SETTINGS_RESOURCE_MAX_LEN;
//	pLwObj->resource[SAMPLING_START_MASK_DtmonResource].value.string= pLwObj->placeHolder.dtMon.samplingStartMask;
	pLwObj->resource[SAMPLING_START_MASK_DtmonResource].valueLen= strlen(pLwObj->resource[SAMPLING_START_MASK_DtmonResource].value.string);
//	pLwObj->resource[SAMPLING_START_MASK_DtmonResource].valueMaxLen= DTMON_CFG_MASK_RESOURCE_LEN;
//	pLwObj->resource[DATA_DtmonResource].value.opaque= pLwObj->placeHolder.dtMon.data;
//	pLwObj->resource[DATA_DtmonResource].valueLen= DTMON_CFG_DATA_REF_RESOURCE_LEN;
//	pLwObj->resource[DATA_DtmonResource].valueMaxLen= DTMON_CFG_DATA_REF_RESOURCE_LEN;
//	pLwObj->resource[REFERENCE_A_DtmonResource].value.opaque= pLwObj->placeHolder.dtMon.referenceA;
//	pLwObj->resource[REFERENCE_A_DtmonResource].valueLen= DTMON_CFG_DATA_REF_RESOURCE_LEN;
//	pLwObj->resource[REFERENCE_A_DtmonResource].valueMaxLen= DTMON_CFG_DATA_REF_RESOURCE_LEN;
//	pLwObj->resource[REFERENCE_B_DtmonResource].value.opaque= pLwObj->placeHolder.dtMon.referenceB;
//	pLwObj->resource[REFERENCE_B_DtmonResource].valueLen= DTMON_CFG_DATA_REF_RESOURCE_LEN;
//	pLwObj->resource[REFERENCE_B_DtmonResource].valueMaxLen= DTMON_CFG_DATA_REF_RESOURCE_LEN;
//	pLwObj->resource[REFERENCE_C_DtmonResource].value.opaque= pLwObj->placeHolder.dtMon.referenceC;
//	pLwObj->resource[REFERENCE_C_DtmonResource].valueLen= DTMON_CFG_DATA_REF_RESOURCE_LEN;
//	pLwObj->resource[REFERENCE_C_DtmonResource].valueMaxLen= DTMON_CFG_DATA_REF_RESOURCE_LEN;

	DTMON_RefreshResources(pLwObj);
}

time_t DTMON_Task(LWOBJ_Obj_t *pLwObj, time_t _currTime, uint64_t _currMask)
{
	time_t _awakeTime= _currTime+ LWOBJ_CFG_MAX_SLEEP_TIME_S;

	if(pLwObj->written)
	{
		pLwObj->written= false;
		DTMON_RefreshResources(pLwObj);
	}

	switch(pLwObj->rte.dtMon->state)
	{
		case STOPPED_DtmonState:
			pLwObj->rte.dtMon->startSamplingTime= UTILI_Mask_GetMatchedTime(_currTime, pLwObj->rte.dtMon->samplingStartMask);
			pLwObj->rte.dtMon->state= WAIT_START_DtmonState;
			//DBG_Print("dtMon.startSamplingTime: %s\r\n", asctime(localtime(&(pLwObj->rte.dtMon->startSamplingTime))));

		case WAIT_START_DtmonState:
			if(_currTime< pLwObj->rte.dtMon->startSamplingTime)
			{
				_awakeTime= UTILI_GetSmallerTime(_awakeTime,  pLwObj->rte.dtMon->startSamplingTime);
				break;
			}
			pLwObj->rte.dtMon->nextSamplingTime= pLwObj->rte.dtMon->startSamplingTime;
			pLwObj->rte.dtMon->stopSamplingTime= pLwObj->rte.dtMon->startSamplingTime+ pLwObj->resource[SAMPLING_RUN_PERIOD_DtmonResource].value.integer;
			pLwObj->rte.dtMon->state= STARTED_DtmonState;
			DTMON_Diag(pLwObj->instance, NEXT_SAMPLE_TIME_DtmonDiagCode, pLwObj->rte.dtMon->nextSamplingTime);
			DTMON_Diag(pLwObj->instance, STOP_TIME_DtmonDiagCode, pLwObj->rte.dtMon->stopSamplingTime);

		case STARTED_DtmonState:
			if(_currTime>= pLwObj->rte.dtMon->nextSamplingTime)
			{
				pLwObj->rte.dtMon->nextSamplingTime= UTILI_ComputeNextTime(_currTime, pLwObj->rte.dtMon->startSamplingTime, pLwObj->resource[SAMPLING_INTERVAL_DtmonResource].value.integer);
				DBG_Print("Dtmon Sampled\r\n");// currTime: %llu _currMask: %llu\r\n", _currTime, _currMask);

				__IO float _data= DTMON_SampleData(pLwObj);
				pLwObj->resource[DATA_DtmonResource].valueLen= DTMON_EncodeCBORDataResource(_data, pLwObj->rte.dtMon->nrtBackflowValue, pLwObj->resource[DATA_DtmonResource].value.opaque);
				//pLwObj->resource[DATA_DtmonResource].valueLen= DTMON_EncodeCBORDataResource(0b11111111111/ 1.0, 99999.999, pLwObj->resource[DATA_DtmonResource].value.opaque);
				if(_data!= pLwObj->rte.dtMon->prevData)
				{
					pLwObj->rte.dtMon->prevData= _data;
					DTMON_NotifyResource(pLwObj, DATA_DtmonResource, false);
				}

				__IO float _refA= DTMON_DecodeCBORDataResource(pLwObj->resource[REFERENCE_A_DtmonResource].value.opaque, pLwObj->resource[REFERENCE_A_DtmonResource].valueLen);
				__IO float _refB= DTMON_DecodeCBORDataResource(pLwObj->resource[REFERENCE_B_DtmonResource].value.opaque, pLwObj->resource[REFERENCE_B_DtmonResource].valueLen);
				__IO float _refC= DTMON_DecodeCBORDataResource(pLwObj->resource[REFERENCE_C_DtmonResource].value.opaque, pLwObj->resource[REFERENCE_C_DtmonResource].valueLen);
				__IO int _currResult= 0;
				if(true== DTMON_CompareData(_data, _refA, pLwObj->resource[COMPARISON_A_DtmonResource].value.integer))
				{
					_currResult|= 0b1;
				}
				if(true== DTMON_CompareData(_data, _refB, pLwObj->resource[COMPARISON_B_DtmonResource].value.integer))
				{
					_currResult|= 0b10;
				}
				if(true== DTMON_CompareData(_data, _refC, pLwObj->resource[COMPARISON_C_DtmonResource].value.integer))
				{
					_currResult|= 0b100;
				}
				if(_currResult!= pLwObj->rte.dtMon->prevResultValue)
				{
					pLwObj->rte.dtMon->prevResultValue= _currResult;
					/*append result*/
					DTMON_Result_t _result=  {pLwObj->rte.dtMon->resultIndex, (uint32_t) SYS_GetTimestampUTC_s(), _currResult};
					LOGGER_WriteSync(&(pLwObj->rte.dtMon->log), (void *)&_result);
					pLwObj->rte.dtMon->resultIndex++;
					DTMON_NotifyResource(pLwObj, RESULTS_DtmonResource, false);
				}
			}
			_awakeTime= UTILI_GetSmallerTime(_awakeTime,  pLwObj->rte.dtMon->nextSamplingTime);

			if(_currTime>= pLwObj->rte.dtMon->stopSamplingTime)
			{
				DBG_Print("Dtmon Stop. \r\n");

				pLwObj->rte.dtMon->state= STOPPED_DtmonState;

				_awakeTime= _currTime+ LWOBJ_CFG_MIN_SLEEP_TIME_S;/*to immediately start next cycle to check for start condition*/
			}
			_awakeTime= UTILI_GetSmallerTime(_awakeTime,  pLwObj->rte.dtMon->stopSamplingTime);

			break;
		default:
			break;
	}

	return _awakeTime;
}

