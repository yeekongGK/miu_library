/*
 * loggerutil.h
 *
 *  Created on: 8 Feb 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

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
