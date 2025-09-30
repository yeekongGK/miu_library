/*
 * rtcalarm.h
 *
 *  Created on: 2 Feb 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef RTC_RTCALARM_H_
#define RTC_RTCALARM_H_

#include "main.h"

void RTCALARM_A_Callback(void);
uint32_t RTCALARM_A_GetTick(void);
void RTCALARM_A_SetTick(uint32_t _tick);
void RTCALARM_A_Disable(void);
void RTCALARM_A_Enable(RTC_TickType_t _tickType, RTC_AlarmStartMarker_t _alarmStartMarker);
void RTCALARM_Init(void);


#endif /* RTC_RTCALARM_H_ */
