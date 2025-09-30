/*
 * syssleep.h
 *
 *  Created on: 2 Feb 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef SYSTEM_SYSSLEEP_H_
#define SYSTEM_SYSSLEEP_H_

#include "main.h"
#include "sys.h"

#define SYSSLEEP_CFG_MIN_PERIOD				10UL
#define SYSSLEEP_CFG_DEFAULT_NEXT_REQUEST	0x10000

bool SYSSLEEP_SleepAllowed(void);
bool SYSSLEEP_IsAwake(SYS_TaskId_t _taskId);
void SYSSLEEP_RequestSleep(SYS_TaskId_t _taskId, uint32_t _period_ms);
uint64_t SYSSLEEP_GetCurrSleepRequest(void);
uint64_t SYSSLEEP_GetNextSleepRequest(void);
void SYSSLEEP_EventCallback(void);
void SYSSLEEP_Disable(void);
void SYSSLEEP_Init(void);
void SYSSLEEP_EnterLightSleep(void);
void SYSSLEEP_EnterDeepSleep(void);

#endif /* SYSTEM_SYSSLEEP_H_ */
