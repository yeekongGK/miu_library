/*
 * flowsensor.h
 *
 *  Created on: 8 Feb 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef SENSOR_FLOWSENSOR_H_
#define SENSOR_FLOWSENSOR_H_

#include "main.h"

typedef enum
{
	FLOWRATE_FlowStats,
	FLOWRATEMIN_FlowStats,
	FLOWRATEMAX_FlowStats,
	FLOWRATEHOURLY_FlowStats,
	BACKFLOW_FlowStats,
	BACKFLOWMAX_FlowStats,
	BACKFLOWFLAG_FlowStats,
	BURSTFLAG_FlowStats,
	NOFLOWFLAG_FlowStats,
	LEAKAGEFLAG_FlowStats,
}FLOWSENSOR_FlowStats_t;

typedef struct
{
	bool enabled;
	uint32_t samplingPeriod_s;
	uint32_t samplingTimeMultiplier;
	uint32_t backflowThreshold;
	uint32_t burstSamplingPeriod_s;
	float burstThreshold;
	uint32_t noflowSamplingPeriod_s;
	float leakageThreshold;
	uint32_t leakageSamplingPeriod_s;

	struct
	{
		float flowrate;
		float flowrateMin;
		float flowrateMax;
		float flowrateHourly;
		float backflow;
		float backflowMax;
		bool backflowFlag;
		bool backflowDetected;
		bool burstFlag;
		bool burstDetected;
		bool noflowFlag;
		bool noflowDetected;
		bool leakageFlag;
		bool leakageDetected;
	}status;

	struct
	{
		float prevReading_l;
		struct
		{
			PULSER_CounterDirection_t prevDirection;
			uint32_t currValue;
			int32_t prevValue;
			int32_t prevMarker;
			uint32_t totalPulse;
			uint32_t valueMarker;
		}backflow;

		struct
		{
			uint32_t samplingCount;
		}burst;

		struct
		{
			uint32_t samplingCount;
		}noflow;

		struct
		{
			uint32_t samplingCount;
		}leakage;
	}rte;
}FLOWSENSOR_t;

float FLOWSENSOR_Get(FLOWSENSOR_FlowStats_t _type);
void FLOWSENSOR_SetFlag(FLOWSENSOR_FlowStats_t _type, bool _flag);
bool FLOWSENSOR_GetFlag(FLOWSENSOR_FlowStats_t _type);
void FLOWSENSOR_Init(FLOWSENSOR_t *_config);
void FLOWSENSOR_Task(void);
uint8_t FLOWSENSOR_TaskState(void);

#endif /* SENSOR_FLOWSENSOR_H_ */
