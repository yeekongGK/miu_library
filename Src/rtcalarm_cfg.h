/******************************************************************************
 * File:        rtcalarm_cfg.h
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file defines the public interface for the RTC Alarm A module. It
 *   provides function prototypes for managing the alarm, including setting
 *   callbacks, getting and setting tick counters, and enabling/disabling the
 *   alarm with specific start markers.
 *
 * Notes:
 *   - -
 *
 * To Do:
 *   - -
 *
 ******************************************************************************/

#ifndef RTCALARM_CFG_H_
#define RTCALARM_CFG_H_

#include "main.h"

void RTCALARM_A_Callback(void);
uint32_t RTCALARM_A_GetTick(void);
void RTCALARM_A_SetTick(uint32_t _tick);
void RTCALARM_A_Disable(void);
void RTCALARM_A_Enable(RTC_TickType_t _tickType, RTC_AlarmStartMarker_t _alarmStartMarker);
void RTCALARM_Init(void);


#endif /* RTC_RTCALARM_H_ */
