/*
 * eventdatadelivery.h
 *
 *  Created on: 18 Sep 2022
 *      Author: muhgin
 */

#ifndef NBIOT_LWM2M_OBJECTS_EVENTDATADELIVERY_H_
#define NBIOT_LWM2M_OBJECTS_EVENTDATADELIVERY_H_

#include "main.h"
#include "lwresource.h"

typedef enum
{
	ALARM_EDDInstance= 0,
	MAX_EDDInstance,
}EDD_SwMgtInstance_t;

typedef enum
{
	NAME_EDDResource= 0,
	EVENT_DATA_VERSION_EDDResource= 1,
	LATEST_EVENT_LOG_EDDResource= 2,
	SCHEDULE_EDDResource= 3,
	MAX_EDDResource,/*total count*/
}EDD_EDDResource_t;

typedef enum
{
	CUST_LEAKAGE_EDDAlarmCode= 100,
	REVERSE_FLOW_NRT_EDDAlarmCode= 101,
	REVERSE_FLOW_EDDAlarmCode= 102,
	EMPTY_PIPE_EDDAlarmCode= 103,
	TAMPER_EDDAlarmCode= 104,
	HIGH_TEMP_EDDAlarmCode= 107,
	LOW_TEMP_EDDAlarmCode= 108,
	LOW_BATT_EDDAlarmCode= 109,
	DBOARD_FAILURE_EDDAlarmCode= 112,
	REBOOT_EDDAlarmCode= 113,
	TIME_SYNC_EDDAlarmCode= 114,
	MAX_EDDEventCode,
}EDD_EDDAlarmCode_t;

typedef struct
{
	uint16_t code;
	uint16_t type;
	uint32_t timestamp1;
	bool alarm1;
	uint32_t value1;
	uint32_t timestamp2;
	bool alarm2;
	uint32_t value2;
}EDD_EventPayload_t;

typedef struct
{
	bool enabled;
	uint8_t prevBitmap;
}EDD_Rte_t;

typedef struct
{
	LWOBJ_Resource_t resource[MAX_EDDResource];
	EDD_Rte_t rte;
}EDD_ResourceHolder_t;

#endif /* NBIOT_LWM2M_OBJECTS_EVENTDATADELIVERY_H_ */
