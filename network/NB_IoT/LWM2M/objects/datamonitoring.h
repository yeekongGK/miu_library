/*
 * datamonitoring.h
 *
 *  Created on: 2 Nov 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef NBIOT_LWM2M_OBJECT_DATAMONITORING_H_
#define NBIOT_LWM2M_OBJECT_DATAMONITORING_H_

#include "main.h"
#include "lwresource.h"
#include "utili.h"
#include "nbiot_define.h"

#define DTMON_CFG_MASK_RESOURCE_LEN				13 /*"YYMMDDHHmmss\0"*/
#define DTMON_CFG_START_MASK_RESOURCE_DEFAULT	"----------00"
#define DTMON_CFG_DATA_RESOURCE_LEN				(1/*array*/+ (9/*float&int*/*4)/*bitmap, nrtbackflow, timestamp, meterReading*/+ 12/*reserve*/)
#define DTMON_CFG_REF_RESOURCE_LEN				(9/*float*/)
#define DTMON_CFG_DATA_REF_RESOURCE_DEFAULT		{0xFA, 0x00, 0x00, 0x00, 0x00}/*float value 0.0*/


typedef enum
{
	NEXT_SAMPLE_TIME_DtmonDiagCode,
	STOP_TIME_DtmonDiagCode,
}DTMON_DtmonDiagCode_t;

typedef enum
{
	ALARMS_DtmonInstance= 0,
	//BACKFLOW_DtmonInstance,
	MAX_DtmonInstance,
}DTMON_DtmonInstance_t;

typedef enum
{
	NAME_DtmonResource= 0,
	DESC_DtmonResource,
	SETTINGS_DtmonResource,
	DATA_DtmonResource,
	SAMPLING_START_MASK_DtmonResource,
	SAMPLING_INTERVAL_DtmonResource,
	SAMPLING_RUN_PERIOD_DtmonResource,
	REFERENCE_A_DtmonResource,
	COMPARISON_A_DtmonResource,
	REFERENCE_B_DtmonResource,
	COMPARISON_B_DtmonResource,
	REFERENCE_C_DtmonResource,
	COMPARISON_C_DtmonResource,
	RESULTS_DtmonResource,
	MAX_DtmonResource,
}DTMON_DtmonResource_t;

typedef enum
{
	STOPPED_DtmonState= 0,
	WAIT_START_DtmonState,
	STARTED_DtmonState,
	MAX_DtmonState,
}DTMON_DtmonState_t;

typedef struct
{
	time_t rteClearTimestamp;
	uint32_t activePeriod_s;/*after which data will be cleared*/
	uint32_t backflowThreshold_l;
	float burstThreshold_lph;
	uint32_t burstSamplingPeriod_s;
	uint32_t noflowSamplingPeriod_s;
	float leakageThreshold_lph;
	uint32_t leakageSamplingPeriod_s;
	uint8_t lowBatteryLevel_percent;
	uint32_t nrtBackflowSamplingPeriod_s;/*pub requirement*/
}DTMON_AlarmsSettings_t;

typedef struct
{
	union
	{
		DTMON_AlarmsSettings_t alarms;
	};
}DTMON_Settings_t;

typedef struct
{
	float value;
}DTMON_DtmonData_t;

typedef enum
{
	NONE_DtmonComparison= 0,
	DATA_GREATER_THAN_REF_DtmonComparison,
	DATA_GREATER_THAN_OR_EQUAL_TO_REF_DtmonComparison,
	DATA_LESS_THAN_REF_DtmonComparison,
	DATA_LESS_THAN_OR_EQUAL_TO_REF_DtmonComparison,
	DATA_EQUALS_TO_REF_DtmonComparison,
	DATA_NOT_EQUAL_TO_REF_DtmonComparison,
	MAX_DtmonComparison,
}DTMON_DtmonComparison_t;

typedef struct
{
	uint32_t index;
	uint32_t timestamp;
	uint32_t value;
}DTMON_Result_t;

typedef struct
{
	bool enabled;
	LOGGER_SyncRte_t log;
	DTMON_DtmonState_t state;
	DTMON_Settings_t settings;
	time_t startSamplingTime;/*this is use as reference when we are adjusting new Periodic Interval*/
	time_t nextSamplingTime;
	time_t stopSamplingTime;
	float prevData;
	uint32_t resultIndex;
	uint32_t prevResultValue;
	uint64_t samplingStartMask;
	time_t backflowStartTime;
	uint32_t resetLogCounter;
	float nrtBackflowValue;
}DTMON_Rte_t;

typedef struct
{
	LWOBJ_Resource_t resource[MAX_DtmonInstance][MAX_DtmonResource];
	DTMON_Rte_t rte[MAX_DtmonInstance];
	uint8_t settings[MAX_DtmonInstance][DTMON_CFG_SETTINGS_RESOURCE_MAX_LEN];/*opaque*/
	char samplingStartMask[MAX_DtmonInstance][DTMON_CFG_MASK_RESOURCE_LEN];
	uint8_t data[MAX_DtmonInstance][DTMON_CFG_DATA_RESOURCE_LEN];/*opaque*/
	uint8_t referenceA[MAX_DtmonInstance][DTMON_CFG_REF_RESOURCE_LEN];/*opaque*/
	uint8_t referenceB[MAX_DtmonInstance][DTMON_CFG_REF_RESOURCE_LEN];/*opaque*/
	uint8_t referenceC[MAX_DtmonInstance][DTMON_CFG_REF_RESOURCE_LEN];/*opaque*/
}DTMON_ResourceHolder_t;

extern const char *const *const DTMON_TEXT[];

#endif /* NBIOT_LWM2M_OBJECT_DATAMONITORING_H_ */
