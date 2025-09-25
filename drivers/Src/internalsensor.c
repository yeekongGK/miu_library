/*
 * tempsensor.c
 *
 *  Created on: 18 Jan 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#include "common.h"
#include "internalsensor.h"
#include "mcuadc.h"

static void INTSENSOR_Voltage_Detect(void);

static uint16_t uwVRefLowDebounceCounter= 0;

static bool bTemperatureFlag= false;/*this flag has to be manuallay cleared*/
static bool bVoltageFlag= false;/*this flag has to be manuallay cleared*/

int32_t INTSENSOR_Temperature_Get(void)
{
	return MCUADC_Temperature_GetValue();
}

int32_t INTSENSOR_Temperature_GetMin(void)
{
	return MCUADC_Temperature_GetValueMin();
}

int32_t INTSENSOR_Temperature_GetMax(void)
{
	return MCUADC_Temperature_GetValueMax();
}

uint32_t INTSENSOR_Voltage_Get(void)
{
	return MCUADC_VRef_GetValue();
}

uint32_t INTSENSOR_Voltage_GetMin(void)
{
	return MCUADC_VRef_GetValueMin();
}

uint32_t INTSENSOR_Voltage_GetMax(void)
{
	return MCUADC_VRef_GetValueMax();
}

bool INTSENSOR_Temperature_GetFlag(void)
{
	return bTemperatureFlag;
}

void INTSENSOR_Temperature_SetFlag(bool _flag)
{
	bTemperatureFlag= _flag;
}

#ifdef COMMENT
static void INTSENSOR_Voltage_Detect(void)
{
	uint32_t _uwVRef= INTSENSOR_Voltage_Get();

	/*deliberate low voltage*/
	if((_uwVRef/ 1000.0)< config.sensors.lowVBattThreshold)
	{
		config.diagnostic.vRefDippedCount++;
		uwVRefLowDebounceCounter++;
		if(config.sensors.lowVBattDebounceValue<= uwVRefLowDebounceCounter)
		{
			uwVRefLowDebounceCounter= 0;
			bVoltageFlag= true;
		}
	}
}
#endif

bool INTSENSOR_Voltage_GetFlag(void)
{
	return bVoltageFlag;
}

void INTSENSOR_Voltage_SetFlag(bool _flag)
{
	bVoltageFlag= _flag;
}

void INTSENSOR_Init(void)
{
	MCUADC_Init();
}

void INTSENSOR_Task(void)
{
	MCUADC_Task();
}

uint8_t INTSENSOR_TaskState(void)
{
	return MCUADC_TaskState();
}
