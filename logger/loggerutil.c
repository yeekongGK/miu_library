/******************************************************************************
 * File:        loggerutil.c
 * Author:      Firmware Team
 * Created:     03-10-2025
 * Last Update: -
 *
 * Description:
 *   This file provides utility functions for the logger module. It includes
 *   helpers for time-based calculations related to log scheduling,
 *   configuration of log start times, and detection of device status changes
 *   to trigger event logs.
 *
 * Notes:
 *   - Much of the functionality, particularly in `LOGGERUTIL_ConfigureLogTickStart`,
 *     is commented out and appears to be legacy code.
 *
 * To Do:
 *   - Review and clean up the commented-out code.
 *
 ******************************************************************************/

#include "common.h"
#include "loggerutil.h"
#include "rtc.h"
#include "rtcalarm.h"

static uint8_t ucDevicePrevStatus= 0;
static bool bIsTamperLog= false;
static uint32_t ulStatusChangedLogCount= 0;
static bool bWaitForStartTick;

static bool bDoRadioLog= false;

bool bLoggerIsFunctional= false;


void LOGGERUTIL_RefactorSecondsTick(uint32_t _secondsTick)
{
//	//TODO: month is not constant, how to support month?
//	if(0== (_secondsTick% (60* 60* 24)))/*day*/
//	{
//		config.log.tickType= Day;
//		config.log.logTickValue= (_secondsTick/ (60* 60* 24));
//	}
//	else if(0== (_secondsTick% (60* 60)))/*hour*/
//	{
//		config.log.tickType= Hour;
//		config.log.logTickValue= (_secondsTick/ (60* 60));
//	}
//	else if(0== (_secondsTick% (60)))/*minute*/
//	{
//		config.log.tickType= Minute;
//		config.log.logTickValue= (_secondsTick/ 60);
//	}
//	else
//	{
//		config.log.tickType= Second;
//		config.log.logTickValue= _secondsTick;/*not quantifiable but other units, use seconds*/
//	}
}

uint32_t LOGGERUTIL_TickInSeconds(RTC_TickType_t _tickType, uint32_t _tickValue)
{
	switch(_tickType)
	{
		case SECOND_TickType:
			return _tickValue;
		case MINUTE_TickType:
			return 60* _tickValue;
		case HOUR_TickType:
			return 60* 60* _tickValue;
		case DAY_TickType:
			return 60* 60* 24* _tickValue;
		case MONTH_TickType:
			return 60* 60* 24* 30* _tickValue;
	}

	return 0;
}

void LOGGERUTIL_ConfigureLogTickStart(bool _startNow, bool _contextSaved)
{
//	if(true== _contextSaved)
//	{
//		/*typically case after reboot/bank switched(context is saved*/
//		RTCALARM_A_Enable(config.log.tickType, config.log.tickMarker);
//		config.log.nextLogTick= RTCALARM_A_GetTick()+ 0x01;/*start immediately*/
//		bWaitForStartTick= false;
//	}
//	else
//	{
//		if(
//			(true== _startNow)
//			||(
//				(0xFF== config.log.tickMarker.date)&& (0xFF== config.log.tickMarker.hour)
//				&&(0xFF== config.log.tickMarker.minute)&& (0xFF== config.log.tickMarker.second)
//			  )
//		  )
//		{
//			RTC_DateTime_GetBCD(
//					&(config.log.tickMarker.date),
//					NULL,
//					NULL,
//					&(config.log.tickMarker.hour),
//					&(config.log.tickMarker.minute),
//					&(config.log.tickMarker.second)
//					);
//			RTCALARM_A_Enable(config.log.tickType, config.log.tickMarker);
//
//			config.log.nextLogTick= RTCALARM_A_GetTick()+ 0x01;/*start immediately*/
//			bWaitForStartTick= false;
//		}
//		else
//		{
//			if((0xFF== config.log.tickMarker.minute)&& (0xFF== config.log.tickMarker.hour)&& (0xFF== config.log.tickMarker.date))
//			{
//				RTCALARM_A_Enable(Minute, config.log.tickMarker);/*use Minute to wait for /ss start datetime.*/
//			}
//			else if((0xFF== config.log.tickMarker.hour)&& (0xFF== config.log.tickMarker.date))
//			{
//				RTCALARM_A_Enable(Hour, config.log.tickMarker);/*use Hour to wait for mm/ss start datetime.*/
//			}
//			else if(0xFF== config.log.tickMarker.date)
//			{
//				RTCALARM_A_Enable(Day, config.log.tickMarker);/*use Day to wait for hh/mm/ss start datetime.*/
//			}
//			else
//			{
//				RTCALARM_A_Enable(Month, config.log.tickMarker);/*use Month to wait for dd hh/mm/ss start datetime.*/
//			}
//			config.log.nextLogTick= RTCALARM_A_GetTick()+ 0x01;
//			bWaitForStartTick= true;
//		}
//	}
//
//	config.log.logIsConfigured= true;
}

bool LOGGERUTIL_GetWaitForStartTickFlag(void)
{
	return bWaitForStartTick;
}

void LOGGERUTIL_SetWaitForStartTickFlag(bool _wait)
{
	bWaitForStartTick= true;
}

bool LOGGERUTIL_DeviceStatusChanged(void)
{
	if(SENSORS_GetStatusCode()> ucDevicePrevStatus)/*only care for new flag set, not cleared*/
	{
		return true;
	}
	return false;
}

void LOGGERUTIL_ClearDeviceStatusChanged(void)
{
	ucDevicePrevStatus= SENSORS_GetStatusCode();
}

uint32_t LOGGERUTIL_GetDeviceStatusChangedLogCount(void)
{
	return ulStatusChangedLogCount;
}

void LOGGERUTIL_IncrementDeviceStatusChangedLogCount(void)
{
	ulStatusChangedLogCount++;
}

bool LOGGERUTIL_StartTickPending(void)
{
	return bWaitForStartTick;
}

void LOGGERUTIL_ClearStartTickPending(void)
{
	bWaitForStartTick= false;
}

void LOGGERUTIL_DoRadio(void)
{
	bDoRadioLog= true;
}

uint8_t LOGGERUTIL_Status(void)
{
	return (true== bLoggerIsFunctional)? 1: 0;
}

void LOGGERUTIL_Init(void)
{
	RTCALARM_Init();
	ucDevicePrevStatus= SENSORS_GetStatusCode();
}
