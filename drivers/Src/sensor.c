/******************************************************************************
 * File:        sensor.c
 * Author:      Firmware Team
 * Created:     03-10-2025
 * Last Update: -
 *
 * Description:
 *   This file implements the main sensor management module. It acts as a
 *   facade, aggregating data and functionality from various underlying sensor
 *   drivers (internal, position, digital, battery, flow). It provides a
 *   unified interface for initializing sensors, retrieving values, managing
 *   status flags, and handling TLV-based communication for sensor data.
 *
 * Notes:
 *   - This module coordinates multiple sensor drivers to provide a consolidated
 *     view of the system's state.
 *
 * To Do:
 *   - The SENSOR_GetFlag function contains debug print statements that should
 *     be removed in a production build.
 *
 ******************************************************************************/

#include "common.h"
#include "sensor.h"
#include "internalsensor.h"
#include "positionsensor.h"
#include "digitalsensor.h"
#include "batterysensor.h"
#include "flowsensor.h"
#include "utili.h"
#include "bc66handler.h"
#include "nfctag.h"
#include "m95m01.h"

static SENSOR_t *pConfig;

float SENSOR_GetValue(SENSOR_Sensor_t _sensorType)
{
	float _value= -1;

	switch(_sensorType)
	{
		case PBMAGCOUNT_Sensor:
			_value= DIGISENSOR_PBMag_GetCount();
			break;
		case TAMPERINCOUNT_Sensor:
			_value= DIGISENSOR_TamperIN_GetCount();
			break;
		case RSTAMPERCOUNT_Sensor:
			_value= DIGISENSOR_RSTamper_GetCount();
			break;
		case INTERNAL_VOLTAGE_Sensor:
			_value= INTSENSOR_Voltage_Get();
			break;
		case INTERNAL_MINVOLTAGE_Sensor:
			_value= INTSENSOR_Voltage_GetMin();
			break;
		case INTERNAL_MAXVOLTAGE_Sensor:
			_value= INTSENSOR_Voltage_GetMax();
			break;
		case INTERNAL_TEMPERATURE_Sensor:
			_value= INTSENSOR_Temperature_Get()/ 1.0;
			break;
		case INTERNAL_MINTEMPERATURE_Sensor:
			_value= INTSENSOR_Temperature_GetMin()/ 1.0;
			break;
		case INTERNAL_MAXTEMPERATURE_Sensor:
			_value= INTSENSOR_Temperature_GetMax()/ 1.0;
			break;
		case CELLCAPACITY_Sensor:
			_value= BATTSENSOR_GetCapacity();
			break;
		case AVECURRENT_Sensor:
			_value= BATTSENSOR_GetAvgCurrent();
			break;
		case VCELL_Sensor:
			_value= BATTSENSOR_GetVCell();
			break;
		case VCELLAVG_Sensor:
			_value= BATTSENSOR_GetAvgVCell();
			break;
		case POSITION_X_Sensor:
			_value= POSSENSOR_X_Get();
			break;
		case POSITION_Y_Sensor:
			_value= POSSENSOR_Y_Get();
			break;
		case POSITION_Z_Sensor:
			_value= POSSENSOR_Z_Get();
			break;
		case TILTCOUNT_Sensor:
			_value= POSSENSOR_Tilt_GetCount();
			break;
		case PIPE_FLOWRATE_Sensor:
			_value= FLOWSENSOR_Get(FLOWRATE_FlowStats);
			break;
		case PIPE_MINFLOWRATE_Sensor:
			_value= FLOWSENSOR_Get(FLOWRATEMIN_FlowStats);
			break;
		case PIPE_MAXFLOWRATE_Sensor:
			_value= FLOWSENSOR_Get(FLOWRATEMAX_FlowStats);
			break;
		case PIPE_HOURLYFLOWRATE_Sensor:
			_value= FLOWSENSOR_Get(FLOWRATEHOURLY_FlowStats);
			break;
		case PIPE_BACKFLOW_Sensor:
			_value= FLOWSENSOR_Get(BACKFLOW_FlowStats);
			break;
		case PIPE_MAXBACKFLOW_Sensor:
			_value= FLOWSENSOR_Get(BACKFLOWMAX_FlowStats);
			break;
		case MINCURRENT_Sensor:
			_value= BATTSENSOR_GetMinCurrent();
			break;
		case MAXCURRENT_Sensor:
			_value= BATTSENSOR_GetMaxCurrent();
			break;
		case CELLCAPACITY_PERCENTAGE_Sensor:
			_value= BATTSENSOR_GetCapacityPercentage();
			break;
		case PIPE_BACKFLOW_FLAG_Sensor:
			_value= FLOWSENSOR_Get(BACKFLOWFLAG_FlowStats);
			break;
		case PIPE_BURST_FLAG_Sensor:
			_value= FLOWSENSOR_Get(BURSTFLAG_FlowStats);
			break;
		case PIPE_NOFLOW_FLAG_Sensor:
			_value= FLOWSENSOR_Get(NOFLOWFLAG_FlowStats);
			break;
		case PIPE_LEAKAGE_FLAG_Sensor:
			_value= FLOWSENSOR_Get(LEAKAGEFLAG_FlowStats);
			break;
		case PULSER_FAULT_Sensor:
			_value= (true== TRACSENS_ErrorDetected())? 1: 0;
			break;
		case CELLQH_Sensor:
			_value= BATTSENSOR_GetTotalQH()/ 1.0;
			//_value= ((int16_t)BATTSENSOR_GetRawQH())/ 1.0;/*signed int16*/
			break;
		case CELLTIMER_Sensor:
			_value= BATTSENSOR_GetTimer();
			break;
		case UNINTENTITONAL_MODEM_RESET_FLAG_Sensor:
			_value= (true== BC66HANDLER_GetUnintentionalResetFlag())? 1: 0;
			break;
		case NFC_ACCESS_FLAG_Sensor:
			_value= (true== NFCTAG_GetNFCAccessFlag())? 1: 0;
			break;
		case NFC_INVALID_ACCESS_FLAG_Sensor:
			_value= (true== NFCTAG_GetNFCInvalidAccessFlag())? 1: 0;
			break;
		case EEPROM_ACCESS_ERROR_FLAG_Sensor:
			_value= (true== M95M01_GetM95M01AccessErrorFlag())? 1: 0;
			break;
		case CELLTEMPERATURE_Sensor:
			_value= BATTSENSOR_GetTemperature();
		default:
			break;
	}

	//DBG_Print("sensor %d, value:%u.\r\n", (uint32_t)_sensorType, (uint32_t)_value);
	return _value;
}

void SENSOR_ClearValue(SENSOR_Sensor_t _sensorType)
{
	switch(_sensorType)
	{
		case PBMAGCOUNT_Sensor:
			DIGISENSOR_PBMag_ClearCount();
			DIGISENSOR_PBMag_SetFlag(false);
			break;
		case TAMPERINCOUNT_Sensor:
			DIGISENSOR_TamperIN_ClearCount();
			DIGISENSOR_TamperIN_SetFlag(false);
			break;
		case RSTAMPERCOUNT_Sensor:
			DIGISENSOR_RSTamper_ClearCount();
			DIGISENSOR_RSTamper_SetFlag(false);
			break;
		case TILTCOUNT_Sensor:
			POSSENSOR_Tilt_ClearCount();
			POSSENSOR_Tilt_SetFlag(false);
			break;
		case PIPE_BACKFLOW_FLAG_Sensor:
			FLOWSENSOR_SetFlag(BACKFLOWFLAG_FlowStats, false);
			break;
		case PIPE_BURST_FLAG_Sensor:
			FLOWSENSOR_SetFlag(BURSTFLAG_FlowStats, false);
			break;
		case PIPE_NOFLOW_FLAG_Sensor:
			FLOWSENSOR_SetFlag(NOFLOWFLAG_FlowStats, false);
			break;
		case PIPE_LEAKAGE_FLAG_Sensor:
			FLOWSENSOR_SetFlag(LEAKAGEFLAG_FlowStats, false);
			break;
		case PULSER_FAULT_Sensor:
			TRACSENS_ClearError();
			break;
		case UNINTENTITONAL_MODEM_RESET_FLAG_Sensor:
			BC66HANDLER_ClearUnintentionalResetFlag();
			break;
		case NFC_ACCESS_FLAG_Sensor:
			NFCTAG_ClearNFCAccessFlag();
			break;
		case NFC_INVALID_ACCESS_FLAG_Sensor:
			NFCTAG_ClearNFCInvalidAccessFlag();
			break;
		case EEPROM_ACCESS_ERROR_FLAG_Sensor:
			M95M01_ClearM95M01AccessErrorFlag();
			break;
		default:
			break;
	}
}

extern SENSOR_t *pConfig_SENS;
bool SENSOR_GetFlag(SENSOR_Sensor_t _sensorType)
{
	switch(_sensorType)
	{
		case PBMAGCOUNT_Sensor:
			pConfig_SENS->battery.rteQHMultiplier++;
			DBG_Print("PBMag %d\r\n",pConfig_SENS->battery.rteQHMultiplier);
			return DIGISENSOR_PBMag_GetFlag();
		case TAMPERINCOUNT_Sensor:
			pConfig_SENS->battery.rteQHMultiplier--;
			DBG_Print("TampIN %d\r\n",pConfig_SENS->battery.rteQHMultiplier);
			return DIGISENSOR_TamperIN_GetFlag();
		case RSTAMPERCOUNT_Sensor:
			DBG_Print("1Test %d\r\n", pConfig_SENS->battery.rtePrevQH);
			return DIGISENSOR_RSTamper_GetFlag();
		case TILTCOUNT_Sensor:
			pConfig_SENS->reserve[0] = 5;
			DBG_Print("2Test\r\n");
			return POSSENSOR_Tilt_GetFlag();
		case PIPE_BACKFLOW_FLAG_Sensor:
			return FLOWSENSOR_GetFlag(BACKFLOWFLAG_FlowStats);
		case PIPE_BURST_FLAG_Sensor:
			return FLOWSENSOR_GetFlag(BURSTFLAG_FlowStats);
		case PIPE_NOFLOW_FLAG_Sensor:
			return FLOWSENSOR_GetFlag(NOFLOWFLAG_FlowStats);
		case PIPE_LEAKAGE_FLAG_Sensor:
			return FLOWSENSOR_GetFlag(LEAKAGEFLAG_FlowStats);
		default:
			break;
	}

	return false;
}

void SENSOR_SetFlag(SENSOR_Sensor_t _sensorType, bool _flag)
{
	switch(_sensorType)
	{
		case PBMAGCOUNT_Sensor:
			if(false== _flag)
			{
				DIGISENSOR_PBMag_ClearCount();
			}
			DIGISENSOR_PBMag_SetFlag(_flag);
			break;
		case TAMPERINCOUNT_Sensor:
			if(false== _flag)
			{
				DIGISENSOR_TamperIN_ClearCount();
			}
			DIGISENSOR_TamperIN_SetFlag(_flag);
			break;
		case RSTAMPERCOUNT_Sensor:
			if(false== _flag)
			{
				DIGISENSOR_RSTamper_ClearCount();
			}
			DIGISENSOR_RSTamper_SetFlag(_flag);
			break;
		case TILTCOUNT_Sensor:
			if(false== _flag)
			{
				POSSENSOR_Tilt_ClearCount();
			}
			POSSENSOR_Tilt_SetFlag(_flag);
			break;
		case PIPE_BACKFLOW_FLAG_Sensor:
			FLOWSENSOR_SetFlag(BACKFLOWFLAG_FlowStats, _flag);
			break;
		case PIPE_BURST_FLAG_Sensor:
			FLOWSENSOR_SetFlag(BURSTFLAG_FlowStats, _flag);
			break;
		case PIPE_NOFLOW_FLAG_Sensor:
			FLOWSENSOR_SetFlag(NOFLOWFLAG_FlowStats, _flag);
			break;
		case PIPE_LEAKAGE_FLAG_Sensor:
			FLOWSENSOR_SetFlag(LEAKAGEFLAG_FlowStats, _flag);
			break;
		default:
			break;
	}
}

uint8_t SENSORS_GetStatusCode(void)
{
	/* 0: tilt
	 * 1: magnet tamper
	 * 2: backflow
	 * 3: low battery
	 * 4: normal tamper
	 * 5: burst pipe
	 * 6: empty pipe
	 * 7: leak
	 * */
	uint8_t _statusCode= 0;

	_statusCode|= (((true== POSSENSOR_Tilt_GetFlag())? 1: 0)<< 0);
	_statusCode|= (((true== DIGISENSOR_PBMag_GetFlag())? 1: 0)<< 1);
	_statusCode|= (((true== FLOWSENSOR_GetFlag(BACKFLOWFLAG_FlowStats))? 1: 0)<< 2);
	_statusCode|= (((pConfig->battery.lowThreshold>= SENSOR_GetValue(CELLCAPACITY_PERCENTAGE_Sensor))? 1: 0)<< 3);
	_statusCode|= (((true== DIGISENSOR_TamperIN_GetFlag())? 1: 0)<< 4);
	_statusCode|= (((true== FLOWSENSOR_GetFlag(BURSTFLAG_FlowStats))? 1: 0)<< 5);
	_statusCode|= (((true== FLOWSENSOR_GetFlag(NOFLOWFLAG_FlowStats))? 1: 0)<< 6);
	_statusCode|= (((true== FLOWSENSOR_GetFlag(LEAKAGEFLAG_FlowStats))? 1: 0)<< 7);

	return _statusCode;//(_statusCode& pConfig->statusEnabledMask);
}

void SENSORS_SetStatusCode(uint8_t _value)
{
	if(0b00000001& _value) SENSOR_SetFlag(TILTCOUNT_Sensor, 0);
	if(0b00000010& _value) SENSOR_SetFlag(PBMAGCOUNT_Sensor, 0);
	if(0b00000100& _value) SENSOR_SetFlag(PIPE_BACKFLOW_FLAG_Sensor, 0);
	if(0b00001000& _value) SENSOR_SetFlag(CELLCAPACITY_PERCENTAGE_Sensor, 0);
	if(0b00010000& _value) SENSOR_SetFlag(TAMPERINCOUNT_Sensor, 0);
	if(0b00100000& _value) SENSOR_SetFlag(PIPE_BURST_FLAG_Sensor, 0);
	if(0b01000000& _value) SENSOR_SetFlag(PIPE_NOFLOW_FLAG_Sensor, 0);
	if(0b10000000& _value) SENSOR_SetFlag(PIPE_LEAKAGE_FLAG_Sensor, 0);
}

void SENSOR_HibernateBattSensor(bool _true)
{
	BATTSENSOR_Hibernate(_true, (true== _true)? LOW_BattSensorGain: HIGH_BattSensorGain);
	//BATTSENSOR_Hibernate(_true, LOW_BattSensorGain);
}

void SENSOR_TLVRequest(TLV_t *_tlv)
{
	_tlv->rv[0]= SUCCESS;
	_tlv->rl= 1;

	switch(_tlv->t)
	{
		case GET_VALUE_SensorTLVTag:
			{
				_tlv->rv[_tlv->rl++]= _tlv->v[0];

				float _value= SENSOR_GetValue(_tlv->v[0]);
				memcpy(_tlv->rv+ _tlv->rl, (uint8_t *)&_value, 4);
				_tlv->rl+= sizeof(float);
			}
			break;

		case GET_ALL_VALUES_SensorTLVTag:
			{
				for(int i= 0; i< MAX_Sensor; i++)
				{
					float _value= SENSOR_GetValue(i);
					memcpy(_tlv->rv+ _tlv->rl+ (i* 4), (uint8_t *)&_value, 4);
					_tlv->rl+= sizeof(float);
				}
			}
			break;

		case CLEAR_VALUE_SensorTLVTag:
			{
				SENSOR_ClearValue(_tlv->v[0]);
			}
			break;

		case GET_FLAG_SensorTLVTag:
			{
				_tlv->rv[_tlv->rl++]= _tlv->v[0];
				_tlv->rv[_tlv->rl++]= SENSOR_GetFlag(_tlv->v[0]);
			}
			break;

		case SET_FLAG_SensorTLVTag:
			{
				SENSOR_SetFlag(_tlv->v[0], _tlv->v[1]);
			}
			break;

		case GET_CONFIG_SensorTLVTag:
			{
				_tlv->rv[_tlv->rl++]= _tlv->v[0];
				switch(_tlv->v[0])
				{
//					case 0:
//						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->taskInterval_ms);
//						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->enableMask);
//						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->ADCSensorsSamplingInterval_ms);
//						_tlv->rv[_tlv->rl]=  pConfig->temperatureOffset;_tlv->rl++;
//						break;
//					case 1:
//						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->battery.RSense);
//						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->battery.designCapacity_Ah);
//						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->battery.lowThreshold);
//						break;
					case 2:
						_tlv->rv[_tlv->rl]=  pConfig->flow.enabled;_tlv->rl++;
						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->flow.samplingPeriod_s);
						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->flow.backflowThreshold);
						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->flow.burstSamplingPeriod_s);
						memcpy(_tlv->rv+ _tlv->rl, (uint8_t *)&(pConfig->flow.burstThreshold), 4);_tlv->rl+= 4;
						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->flow.noflowSamplingPeriod_s);
						memcpy(_tlv->rv+ _tlv->rl, (uint8_t *)&(pConfig->flow.leakageThreshold), 4);_tlv->rl+= 4;
						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->flow.leakageSamplingPeriod_s);
						break;
					default:
						_tlv->rv[0]= ERROR;
						return;
				}
			}
			break;

		case SET_CONFIG_SensorTLVTag:
			{
				uint16_t _index= 1;
				_tlv->rv[_tlv->rl]= _tlv->v[0];
				switch(_tlv->v[0])
				{
//					case 0:
//						_index+= UTILI_Array_Copy32_Ptr(&(pConfig->taskInterval_ms), _tlv->v+ _index);
//						_index+= UTILI_Array_Copy32_Ptr(&(pConfig->enableMask), _tlv->v+ _index);
//						_index+= UTILI_Array_Copy32_Ptr(&(pConfig->ADCSensorsSamplingInterval_ms), _tlv->v+ _index);
//						 pConfig->temperatureOffset=_tlv->v[_index]; _index++;
//						break;
//					case 1:
//						_index+= UTILI_Array_Copy32_Ptr(&(pConfig->battery.RSense), _tlv->v+ _index);
//						_index+= UTILI_Array_Copy32_Ptr(&(pConfig->battery.designCapacity_Ah), _tlv->v+ _index);
//						_index+= UTILI_Array_Copy32_Ptr(&(pConfig->battery.lowThreshold), _tlv->v+ _index);
//						break;
					case 2:
						pConfig->flow.enabled=_tlv->v[_index]; _index++;
						_index+= UTILI_Array_Copy32_Ptr(&(pConfig->flow.samplingPeriod_s), _tlv->v+ _index);
						_index+= UTILI_Array_Copy32_Ptr(&(pConfig->flow.backflowThreshold), _tlv->v+ _index);
						_index+= UTILI_Array_Copy32_Ptr(&(pConfig->flow.burstSamplingPeriod_s), _tlv->v+ _index);
						memcpy(&(pConfig->flow.burstThreshold), _tlv->v+ _index, 4);_index+= 4;
						_index+= UTILI_Array_Copy32_Ptr(&(pConfig->flow.noflowSamplingPeriod_s), _tlv->v+ _index);
						memcpy(&(pConfig->flow.leakageThreshold), _tlv->v+ _index, 4); _index+= 4;
						_index+= UTILI_Array_Copy32_Ptr(&(pConfig->flow.leakageSamplingPeriod_s), _tlv->v+ _index);
						FLOWSENSOR_Init(&(config.sensors.flow));
						break;
					default:
						_tlv->rv[0]= ERROR;
						return;
				}
			}
			break;

		default:
			_tlv->rv[0]= ERROR;
			break;
	}
}

void SENSOR_Init(SENSOR_t *_config)
{
	pConfig= _config;

	POSSENSOR_Init();
	DIGISENSOR_Init();
	INTSENSOR_Init();
	BATTSENSOR_Init(pConfig);
	FLOWSENSOR_Init(&(pConfig->flow));
}

void SENSOR_Task(void)
{
	POSSENSOR_Task();
	DIGISENSOR_Task();
	INTSENSOR_Task();
	BATTSENSOR_Task();
	FLOWSENSOR_Task();

	if(true== SYS_IsAwake(SENSOR_TaskId))
	{
		__IO float _qh= SENSOR_GetValue(CELLQH_Sensor);/*QH need to be called periodically because we have to manually detect register overflow*/

//		uint16_t _x, _y, _z;
//		POSSENSOR_XYZ_Get(&_x, &_y, &_z);
//		DBG_Print("\r\n------------------------------------\r\n");
//		DBG_Print("-------------- Counter: %u, CurrentConsumed: %u mA.\r\n", BATTSENSOR_GetQH(), (uint32_t)(BATTSENSOR_GetCurrentUsed()* 1000));
//		DBG_Print("-------------- RepCap: %u, RepSOC: %u %.\r\n", BATTSENSOR_GetRepSOC(), BATTSENSOR_GetRepCap());
//		DBG_Print("-------------- VCell: %u mV, AvgVCell: %u mV.\r\n", (int32_t)BATTSENSOR_GetVCell(), (int32_t)BATTSENSOR_GetAvgVCell());
//		DBG_Print("-------------- Current: %d uA, AvgCurrent: %d uA.\r\n", (int32_t)BATTSENSOR_GetCurrent(), (int32_t)BATTSENSOR_GetAvgCurrent());
//		DBG_Print("-------------- BTemp: %d C, BAvgTemp: %d C.\r\n", (int32_t)BATTSENSOR_GetTemperature(), (int32_t)BATTSENSOR_GetAvgTemperature());
//		DBG_Print("-------------- IntTemp: %d C.\r\n", (int32_t)INTSENSOR_Temperature_Get());
//		DBG_Print("-------------- X: %u , Y: %u , Z: %u .\r\n", _x, _y, _z);
//		DBG_Print("------------------------------------\r\n");
		SYS_Sleep(SENSOR_TaskId, pConfig->taskInterval_ms);
	}
}

uint8_t SENSOR_TaskState(void)
{
	return (0
			| POSSENSOR_TaskState()
			| DIGISENSOR_TaskState()
			| INTSENSOR_TaskState()
			| BATTSENSOR_TaskState()
			| FLOWSENSOR_TaskState()
			);
}
