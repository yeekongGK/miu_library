/******************************************************************************
 * File:        loggerutil.h
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file defines the public interface for the logger utility functions.
 *   It provides function prototypes for time-based calculations, log start
 *   time configuration, and device status change detection.
 *
 * Notes:
 *   - -
 *
 * To Do:
 *   - -
 *
 ******************************************************************************/

#ifndef LOGGER_LOGGERUTIL_H_
#define LOGGER_LOGGERUTIL_H_

#include "main.h"

void LOGGERUTIL_RefactorSecondsTick(uint32_t _secondsTick);
uint32_t LOGGERUTIL_TickInSeconds(RTC_TickType_t _tickType, uint32_t _tickValue);
void LOGGERUTIL_ConfigureLogTickStart(bool _startNow, bool _contextSaved);
bool LOGGERUTIL_GetWaitForStartTickFlag(void);
void LOGGERUTIL_SetWaitForStartTickFlag(bool _wait);
uint32_t LOGGERUTIL_GetStatusChangedLogCount(void);
bool LOGGERUTIL_DeviceStatusChanged(void);

#endif /* LOGGER_LOGGERUTIL_H_ */
