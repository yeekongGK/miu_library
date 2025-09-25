/*
 * sensor.h
 *
 *  Created on: 18 Jan 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef SENSOR_SENSOR_H_
#define SENSOR_SENSOR_H_

#include "main.h"
#include "flowsensor.h"

#define SENSOR_CFG_VERSION			0x01

typedef enum
{
	PBMAGCOUNT_Sensor= 0,
	TAMPERINCOUNT_Sensor,
	RSTAMPERCOUNT_Sensor,
	INTERNAL_VOLTAGE_Sensor,
	INTERNAL_MINVOLTAGE_Sensor,
	INTERNAL_MAXVOLTAGE_Sensor,
	INTERNAL_TEMPERATURE_Sensor,
	INTERNAL_MINTEMPERATURE_Sensor,
	INTERNAL_MAXTEMPERATURE_Sensor,
	CELLCAPACITY_Sensor,
	AVECURRENT_Sensor,
	VCELL_Sensor,
	VCELLAVG_Sensor,
	POSITION_X_Sensor,
	POSITION_Y_Sensor,
	POSITION_Z_Sensor,
	TILTCOUNT_Sensor,
	PIPE_FLOWRATE_Sensor,
	PIPE_MINFLOWRATE_Sensor,
	PIPE_MAXFLOWRATE_Sensor,
	PIPE_HOURLYFLOWRATE_Sensor,
	PIPE_BACKFLOW_Sensor,
	PIPE_MAXBACKFLOW_Sensor,
	MINCURRENT_Sensor,
	MAXCURRENT_Sensor,
	CELLCAPACITY_PERCENTAGE_Sensor,
	PIPE_BACKFLOW_FLAG_Sensor,
	PIPE_BURST_FLAG_Sensor,
	PIPE_NOFLOW_FLAG_Sensor,
	PIPE_LEAKAGE_FLAG_Sensor,
	PULSER_FAULT_Sensor,
	CELLQH_Sensor,
	CELLTIMER_Sensor,
	UNINTENTITONAL_MODEM_RESET_FLAG_Sensor,
	NFC_ACCESS_FLAG_Sensor,
	NFC_INVALID_ACCESS_FLAG_Sensor,
	EEPROM_ACCESS_ERROR_FLAG_Sensor,
	CELLTEMPERATURE_Sensor,
	MAX_Sensor,
}SENSOR_Sensor_t;

typedef enum
{
	SENSOR_OBJECT_GENERIC,
	SENSOR_OBJECT_TEMPERATURE,
	SENSOR_OBJECT_ACCELEROMETER,
	SENSOR_OBJECT_VOLTAGE,
	SENSOR_OBJECT_CURRENT,
	SENSOR_OBJECT_PUSHBUTTON,
}SENSOR_ObjectType_t;

typedef enum
{
	SENSOR_OBJECT_DATATYPE_FLOAT,
	SENSOR_OBJECT_DATATYPE_INTEGER,
	SENSOR_OBJECT_DATATYPE_STRING,
	SENSOR_OBJECT_DATATYPE_BOOL,
	SENSOR_OBJECT_DATATYPE_TIME,
}SENSOR_ObjectDataType_t;

typedef struct
{
	uint16_t id;
	SENSOR_Sensor_t sensor;
	SENSOR_ObjectDataType_t dataType;
	union
	{
		float valueF;
		uint32_t valueI;
		const char* valueS;
		bool valueB;
		uint32_t valueT;
	};
}SENSOR_ResourceValue_t;

typedef struct
{
	uint16_t id;
	SENSOR_Sensor_t sensor;
	float value;
}SENSOR_ResourceValueInteger_t;

typedef struct
{
	uint16_t id;
	SENSOR_Sensor_t sensor;
	float value;
}SENSOR_ResourceValueBool_t;

typedef struct
{
	uint16_t id;
	float value;
}SENSOR_ResourceFloat_t;

typedef struct
{
	uint16_t id;
	float value;
}SENSOR_ResourceRange_t;

typedef struct
{
	uint16_t id;
	uint32_t value;
}SENSOR_ResourceInteger_t;

typedef struct
{
	uint16_t id;
	uint32_t value;
}SENSOR_ResourceTime_t;

typedef struct
{
	uint16_t id;
	const char* value;
}SENSOR_ResourceString_t;

typedef struct
{
	SENSOR_ResourceValue_t value;
	SENSOR_ResourceString_t units;
	SENSOR_ResourceValue_t minValue;
	SENSOR_ResourceValue_t maxValue;
	SENSOR_ResourceRange_t rangeMin;
	SENSOR_ResourceRange_t rangeMax;
	SENSOR_ResourceString_t applicationType;
	SENSOR_ResourceString_t sensorType;
	bool resetMinMax;
	SENSOR_ResourceTime_t timestamp;
	SENSOR_ResourceFloat_t fracTimestamp;
	SENSOR_ResourceInteger_t qualityIndicator;
	SENSOR_ResourceInteger_t qualityLevel;
}SENSOR_ObjectGeneric_t;

typedef struct
{
	SENSOR_ResourceValue_t value;
	SENSOR_ResourceValue_t minValue;
	SENSOR_ResourceValue_t maxValue;
	SENSOR_ResourceRange_t rangeMin;
	SENSOR_ResourceRange_t rangeMax;
	SENSOR_ResourceString_t units;
	bool resetMinMax;
	SENSOR_ResourceString_t applicationType;
	SENSOR_ResourceTime_t timestamp;
	SENSOR_ResourceFloat_t fracTimestamp;
	SENSOR_ResourceInteger_t qualityIndicator;
	SENSOR_ResourceInteger_t qualityLevel;
}SENSOR_ObjectTemperature_t;

typedef struct
{
	SENSOR_ResourceValue_t valueX;
	SENSOR_ResourceValue_t valueY;
	SENSOR_ResourceValue_t valueZ;
	SENSOR_ResourceString_t units;
	SENSOR_ResourceRange_t rangeMin;
	SENSOR_ResourceRange_t rangeMax;
	SENSOR_ResourceString_t applicationType;
	SENSOR_ResourceTime_t timestamp;
	SENSOR_ResourceFloat_t fracTimestamp;
	SENSOR_ResourceInteger_t qualityIndicator;
	SENSOR_ResourceInteger_t qualityLevel;
}SENSOR_ObjectAccelerometer_t;

typedef struct
{
	SENSOR_ResourceValue_t value;
	SENSOR_ResourceString_t units;
	SENSOR_ResourceValue_t minValue;
	SENSOR_ResourceValue_t maxValue;
	SENSOR_ResourceRange_t rangeMin;
	SENSOR_ResourceRange_t rangeMax;
	bool resetMinMax;
	SENSOR_ResourceFloat_t currentCalibration;
	SENSOR_ResourceString_t applicationType;
	SENSOR_ResourceTime_t timestamp;
	SENSOR_ResourceFloat_t fracTimestamp;
	SENSOR_ResourceInteger_t qualityIndicator;
	SENSOR_ResourceInteger_t qualityLevel;
}SENSOR_ObjectVoltage_t;

typedef struct
{
	SENSOR_ResourceValue_t value;
	SENSOR_ResourceString_t units;
	SENSOR_ResourceValue_t minValue;
	SENSOR_ResourceValue_t maxValue;
	SENSOR_ResourceRange_t rangeMin;
	SENSOR_ResourceRange_t rangeMax;
	bool resetMinMax;
	SENSOR_ResourceFloat_t currentCalibration;
	SENSOR_ResourceString_t applicationType;
	SENSOR_ResourceTime_t timestamp;
	SENSOR_ResourceFloat_t fracTimestamp;
	SENSOR_ResourceInteger_t qualityIndicator;
	SENSOR_ResourceInteger_t qualityLevel;
}SENSOR_ObjectCurrent_t;

typedef struct
{
	SENSOR_ResourceValue_t value;
	SENSOR_ResourceString_t units;
	SENSOR_ResourceValue_t minValue;
	SENSOR_ResourceValue_t maxValue;
	SENSOR_ResourceRange_t rangeMin;
	SENSOR_ResourceRange_t rangeMax;
	bool resetMinMax;
	SENSOR_ResourceFloat_t currentCalibration;
	SENSOR_ResourceString_t applicationType;
	SENSOR_ResourceTime_t timestamp;
	SENSOR_ResourceFloat_t fracTimestamp;
	SENSOR_ResourceInteger_t qualityIndicator;
	SENSOR_ResourceInteger_t qualityLevel;
}SENSOR_ObjectRate_t;

typedef struct
{
	SENSOR_ResourceValueInteger_t level;
	SENSOR_ResourceValue_t capacity;
	SENSOR_ResourceValue_t voltage;
	SENSOR_ResourceString_t type;
	SENSOR_ResourceValueInteger_t lowThreshold;
	SENSOR_ResourceValueBool_t batteryIsLow;
	SENSOR_ResourceValueBool_t batteryShutdown;
	SENSOR_ResourceValueInteger_t cycles;
	SENSOR_ResourceValueBool_t supplyLoss;
	SENSOR_ResourceValueInteger_t supplyLossCount;
	bool resetSupplyLossCount;
	SENSOR_ResourceString_t supplyLossReason;
}SENSOR_ObjectBattery_t;

typedef struct
{
	SENSOR_ResourceValue_t state;
	SENSOR_ResourceValue_t counter;
	SENSOR_ResourceString_t applicationType;
	SENSOR_ResourceTime_t timestamp;
	SENSOR_ResourceFloat_t fracTimestamp;
}SENSOR_ObjectPushButton_t;

typedef struct
{
	uint16_t objectId;
	float objectVersion;
	SENSOR_ObjectType_t objectType;
	SENSOR_ResourceValue_t resource[16];
}SENSOR_Object_t;

typedef enum
{
	NONE_SensorTLVTag= 0,
	GET_VALUE_SensorTLVTag,
	GET_ALL_VALUES_SensorTLVTag,
	CLEAR_VALUE_SensorTLVTag,
	GET_FLAG_SensorTLVTag,
	SET_FLAG_SensorTLVTag,
	GET_CONFIG_SensorTLVTag,
	SET_CONFIG_SensorTLVTag,
	MAX_SensorTLVTag,
}SENSOR_SensorTLVTag_t;

/*TO EXPORT OUT TO CFG*/
typedef struct __attribute__((aligned(8)))/*compulsory alignment*/
{
	uint32_t checksum;/*compulsory checksum*/
	uint8_t  version;

	uint32_t taskInterval_ms;
	uint32_t enableMask;
	uint32_t ADCSensorsSamplingInterval_ms;/*miminum time between consecutive adc scanning(to mimimize power consumption) in milisecond*/
	uint8_t acceleroRWRegisters[23];
	int8_t temperatureOffset;

	struct
	{
		float RSense;/*in ohm*/
		float designCapacity_Ah;/*in Ah*/
		float lowThreshold;

		uint16_t rtePrevQH;
		uint32_t rteQHMultiplier;
		float rteMinCurrent;
		float rteMaxCurrent;
	}battery;

	FLOWSENSOR_t flow;

	struct
	{
		uint8_t intArmedThreshold;//0x3E/*80 degree*//*region of which 6D interrupt is generated*/
		uint8_t intDisarmedThreshold;/*30 degree*//*clearance from threshold of which 6d interrupt of the axis need to be disabled*/
	}position;

	uint8_t reserve[112];/*to add more param reduce this, thus no need to do backward compatible thing. remember to set the default value after config*/
}SENSOR_t;

float SENSOR_GetValue(SENSOR_Sensor_t _sensorType);
void SENSOR_ClearValue(SENSOR_Sensor_t _sensorType);
bool SENSOR_GetFlag(SENSOR_Sensor_t _sensorType);
void SENSOR_SetFlag(SENSOR_Sensor_t _sensorType, bool _flag);
uint8_t SENSORS_GetStatusCode(void);
void SENSORS_SetStatusCode(uint8_t _value);
void SENSOR_HibernateBattSensor(bool _true);
void SENSOR_TLVRequest(TLV_t *_tlv);
void SENSOR_Init(SENSOR_t *_config);
void SENSOR_Task(void);
uint8_t SENSOR_TaskState(void);

#endif /* SENSOR_SENSOR_H_ */
