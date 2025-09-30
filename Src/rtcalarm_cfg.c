/*
 * rtcalarm.c
 *
 *  Created on: 2 Feb 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#include "common.h"
#include "rtcalarm.h"

__IO static uint32_t ulAlarmATick= 0;

void RTCALARM_A_Callback(void)
{
	++ulAlarmATick;
}

uint32_t RTCALARM_A_GetTick(void)
{
	return ulAlarmATick;
}

void RTCALARM_A_SetTick(uint32_t _tick)
{
	ulAlarmATick= _tick;
}

void RTCALARM_A_Disable(void)
{
	LL_RTC_DisableWriteProtection(RTC);
	LL_RTC_DisableIT_ALRB(RTC);
	LL_RTC_ALMA_Disable(RTC);
	LL_RTC_EnableWriteProtection(RTC);
}

void RTCALARM_A_Enable(RTC_TickType_t _tickType, RTC_AlarmStartMarker_t _alarmStartMarker)
{
	LL_RTC_AlarmTypeDef RTC_AlarmStruct;

	RTCALARM_A_Disable();

	LL_RTC_ALMA_StructInit(&RTC_AlarmStruct);/*default value*/
	RTC_AlarmStruct.AlarmTime.Hours= _alarmStartMarker.hour;
	RTC_AlarmStruct.AlarmTime.Minutes= _alarmStartMarker.minute;
	RTC_AlarmStruct.AlarmTime.Seconds= _alarmStartMarker.second;
	RTC_AlarmStruct.AlarmDateWeekDay= _alarmStartMarker.date;

	switch(_tickType)
	{
		case SECOND_TickType:
			RTC_AlarmStruct.AlarmMask = LL_RTC_ALMA_MASK_ALL;
			break;

		case MINUTE_TickType:
			RTC_AlarmStruct.AlarmMask = LL_RTC_ALMA_MASK_DATEWEEKDAY| LL_RTC_ALMA_MASK_HOURS| LL_RTC_ALMA_MASK_MINUTES;
			break;

		case HOUR_TickType:
			RTC_AlarmStruct.AlarmMask = LL_RTC_ALMA_MASK_DATEWEEKDAY| LL_RTC_ALMA_MASK_HOURS;
			break;

		case DAY_TickType:
			RTC_AlarmStruct.AlarmMask = LL_RTC_ALMA_MASK_DATEWEEKDAY;
			break;

		case MONTH_TickType:
			RTC_AlarmStruct.AlarmMask = LL_RTC_ALMA_MASK_NONE;
			break;
	}

	LL_RTC_ALMA_Init(RTC, LL_RTC_FORMAT_BCD, &RTC_AlarmStruct);

	LL_RTC_DisableWriteProtection(RTC);/* needed as RTC registers are write protected */

	LL_RTC_ALMA_Enable(RTC);
	LL_RTC_ClearFlag_ALRA(RTC);
	LL_RTC_EnableIT_ALRA(RTC);

	/* RTC Alarm Interrupt Configuration: EXTI configuration */
	LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_18);
	LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_18);
	NVIC_SetPriority(RTC_Alarm_IRQn, SYS_CFG_RTCALARM_PRIORITY);
	NVIC_EnableIRQ(RTC_Alarm_IRQn);

	LL_RTC_EnableWriteProtection(RTC);
}

void RTCALARM_Init(void)
{
	RTC_Init();
}
