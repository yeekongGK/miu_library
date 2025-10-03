/******************************************************************************
 * File:        internalsensor.c
 * Author:      Firmware Team
 * Created:     03-10-2025
 * Last Update: -
 *
 * Description:
 *   This file implements the driver for the microcontroller's internal sensors,
 *   primarily the temperature sensor and the internal voltage reference (VRef).
 *   It serves as a wrapper for the MCU's ADC driver (`mcuadc`), providing
 *   functions to retrieve sensor readings and manage status flags.
 *
 * Notes:
 *   - This module relies on the `mcuadc` driver for the actual hardware
 *     interaction and data acquisition.
 *
 * To Do:
 *   - The voltage detection logic (`INTSENSOR_Voltage_Detect`) is currently
 *     commented out and should be reviewed or removed.
 *
 ******************************************************************************/

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
