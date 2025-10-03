/******************************************************************************
 * File:        syssleep.h
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file defines the public interface for the system sleep management
 *   module. It includes function prototypes for initializing the sleep
 *   manager, requesting sleep periods, checking task wakeup status, and
 *   entering low-power modes. It also defines constants for sleep period
 *   configuration.
 *
 * Notes:
 *   - -
 *
 * To Do:
 *   - -
 *
 ******************************************************************************/

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
