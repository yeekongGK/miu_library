/*
 * periodicactivity.h
 *
 *  Created on: 28 Oct 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef NBIOT_LWM2M_OBJECT_PERIODICACTIVITY_H_
#define NBIOT_LWM2M_OBJECT_PERIODICACTIVITY_H_

#include "main.h"
#include "lwresource.h"
#include "logger.h"
#include "failsafe.h"
#include "utili.h"
#include "nbiot_define.h"

#define PRACT_CFG_MASK_RESOURCE_LEN					13 /*"YYMMDDHHmmss\0"*/

typedef enum
{
	NEXT_RUN_TIME_PractDiagCode,
	NEXT_DISPATCH_TIME_PractDiagCode,
}PRACT_PractDiagCode_t;

typedef enum
{
	GET_READING_PractInstance= 0,
	GET_STATUS_PractInstance,
	QUERY_PractInstance,
	MAX_PractInstance,
}PRACT_PractInstance_t;

typedef enum
{
	NAME_PractResource= 0,
	DESC_PractResource,
	SETTINGS_PractResource,
	START_MASK_PractResource,
	PERIODIC_INTERVAL_PractResource,
	RUN_PERIOD_PractResource,
	RECORD_PractResource,
	RECORD_HEAD_PractResource,
	RECORD_TAIL_PractResource,
	RECORD_READ_PractResource,
	RECORD_DISPATCH_START_MASK_PractResource,
	RECORD_DISPATCH_PERIODIC_INTERVAL_PractResource,
	MAX_PractResource,
}PRACT_PractResource_t;

typedef enum
{
	STOPPED_PractRunState= 0,
	WAIT_START_PractRunState,
	STARTED_PractRunState,
	MAX_PractRunState,
}PRACT_PractRunState_t;

typedef enum
{
	STOPPED_PractDispatchState= 0,
	WAIT_START_PractDispatchState,
	STARTED_PractDispatchState,
	MAX_PractDispatchState,
}PRACT_PractDispatchState_t;

typedef struct
{
	uint32_t index;
	uint32_t timestamp;
	float meterReading;
	float meterConsumption;
	int8_t temperature;
	uint8_t batteryLevel_percent;
}PRACT_GetReadingRecord_t;

typedef struct
{
	uint32_t index;
	uint32_t timestamp;
	NBIOT_Stats_t stats;
}PRACT_GetStatusRecord_t;

typedef struct
{
	uint32_t index;
	uint32_t timestamp;
	uint16_t datalen;
	uint8_t data[256];
}PRACT_QueryRecord_t;

typedef struct
{
	bool enabled;
	LOGGER_SyncRte_t log;
	PRACT_PractRunState_t runState;
	PRACT_PractDispatchState_t dispatchState;
	time_t startTime;/*this is use as reference when we are adjusting new Periodic Interval*/
	time_t nextRunTime;
	time_t stopTime;
	time_t clearTime;
	uint64_t startMask;
	uint64_t recordDispatchStartMask;
	time_t recordDistpatchStartTime;/*this is use as reference when we are adjusting new Periodic Interval*/
	time_t nextRecordDistpatchTime;
	uint32_t recordDispatched;
}PRACT_Rte_t;

typedef struct
{
	LWOBJ_Resource_t resource[MAX_PractInstance][MAX_PractResource];
	PRACT_Rte_t rte[MAX_PractInstance];
	uint8_t practSetting0[PRACT_CFG_0_SETTINGS_RESOURCE_MAX_LEN];
	uint8_t practSetting1[PRACT_CFG_1_SETTINGS_RESOURCE_MAX_LEN];
	uint8_t practSetting2[PRACT_CFG_2_SETTINGS_RESOURCE_MAX_LEN];
	char startMask[MAX_PractInstance][PRACT_CFG_MASK_RESOURCE_LEN];
	char recordDispatchStartMask[MAX_PractInstance][PRACT_CFG_MASK_RESOURCE_LEN];
}PRACT_ResourceHolder_t;

extern const char *const *const PRACT_TEXT[];

#endif /* NBIOT_LWM2M_OBJECT_PERIODICACTIVITY_H_ */
