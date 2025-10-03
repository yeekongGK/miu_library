/******************************************************************************
 * File:        alarm.h
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file defines the interface for the alarm system. It includes
 *   definitions for alarm types, states, thresholds, and data structures
 *   for managing alarm objects. It also declares functions for initializing,
 *   configuring, and processing alarms.
 *
 * Notes:
 *   - The ALARM_t structure requires 8-byte alignment.
 *
 * To Do:
 *   - -
 *
 ******************************************************************************/

#ifndef ALARM_ALARM_H_
#define ALARM_ALARM_H_

#include "main.h"
#include "sensor.h"

#define ALARM_CFG_VERSION			0x01

typedef enum
{
	RUN_AlarmOpState,
	BACKOFF_AlarmOpState,
	MAX_AlarmOpState,
}ALARM_AlarmOpState_t;

typedef enum
{
	PBMAGCOUNT_Alarm,
	TAMPERINCOUNT_Alarm,
	RSTAMPERCOUNT_Alarm,
	VOLTAGE_Alarm,
	TEMPERATURE_Alarm,
	CELLCAPACITY_Alarm,
	AVECURRENT_Alarm,
	POSITION_X_Alarm,
	POSITION_Y_Alarm,
	POSITION_Z_Alarm,
	TILTCOUNT_Alarm,
	PIPE_BURST_Alarm,
	PIPE_EMPTY_Alarm,
	PIPE_LEAK_Alarm,
	PIPE_BACKFLOW_Alarm,
	MAX_Alarm,
}ALARM_Alarm_t;

typedef enum
{
	NONE_AlarmThreshold= 0,
	ABOVE_AlarmThreshold,
	ABOVE_AND_EQUAL_AlarmThreshold,
	UNDER_AlarmThreshold,
	UNDER_AND_EQUAL_AlarmThreshold,
	EQUAL_AlarmThreshold,
	AND_AlarmThreshold,
	OR_AlarmThreshold,
}ALARM_AlarmThreshold_t;

typedef enum
{
	IDLE_AlarmObjectState,
	ARMED_AlarmObjectState,
	ACTIVATED_AlarmObjectState,
	CLEARED_AlarmObjectState,
}ALRAM_AlarmObjectState_t;

typedef enum
{
	NONE_AlarmTLVTag= 0,
	GET_BACKOFF_PERIOD_AlarmTLVTag,
	SET_BACKOFF_PERIOD_AlarmTLVTag,
	GET_OBJECT_AlarmTLVTag,
	GET_ALL_OBJECTS_AlarmTLVTag,
	SET_OBJECT_AlarmTLVTag,
}ALARM_AlarmTLVTag_t;

typedef struct
{
	bool enabled;
	SENSOR_Sensor_t sensor;
	ALARM_AlarmThreshold_t threshold1Type;
	float threshold1;
	ALARM_AlarmThreshold_t threshold12Type;
	ALARM_AlarmThreshold_t threshold2Type;
	float threshold2;
	uint32_t debouncePeriod_ms;
	uint32_t stayActivePeriod_ms;
	ALRAM_AlarmObjectState_t state;
	bool isArmed;
	bool isActive;
	uint32_t timestamp;
}ALARM_AlarmObject_t;

/*TO EXPORT OUT TO CFG*/
typedef struct __attribute__((aligned(8)))/*compulsory alignment*/
{
	uint32_t checksum;/*compulsory checksum*/
	uint8_t  version;

	uint32_t backOffPeriod_ms;
	ALARM_AlarmObject_t object[MAX_Alarm];

	uint8_t reserve[128];/*to add more param reduce this, thus no need to do backward compatible thing. remember to set the default value after config*/
}ALARM_t;

uint32_t ALARM_GetActiveBitmap(void);
ALARM_AlarmObject_t ALARM_Get(ALARM_Alarm_t _type);
void ALARM_TLVRequest(TLV_t *_tlv);
void ALARM_Init(ALARM_t *_config);
void ALARM_Task(void);
uint8_t ALARM_TaskState(void);

#endif /* ALARM_ALARM_H_ */
