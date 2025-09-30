/*
 * nbiot_utility.h
 *
 *  Created on: 7 Aug 2018
 *      Author: muhammad.ahmad@georgekent.net
 */

#include "main.h"
#include "rtc.h"

#ifndef NBIOT_BC66UTILITY_H_
#define NBIOT_BC66UTILITY_H_

void BC66UTIL_EncodeCPSMS(uint8_t pucNbiotBuffer[], RTC_TickType_t _tickType, uint32_t _transmissionTick, uint16_t _activeTime/*seconds*/);
uint8_t BC66UTIL_SecondsToGPRSTimer2(uint32_t _value);
uint32_t BC66UTIL_GPRSTimer2ToSeconds(uint8_t _value);
uint8_t BC66UTIL_SecondsToGPRSTimer3(uint32_t _value);
uint32_t BC66UTIL_GPRSTimer3ToSeconds(uint8_t _value);
uint8_t BC66UTIL_MilisecondsToPagingTime(uint32_t _value);
uint32_t BC66UTIL_PagingTimeToMiliseconds(uint8_t _value);
uint8_t BC66UTIL_MilisecondsToEDrx(uint32_t _value);
uint32_t BC66UTIL_EDrxToMiliseconds(uint8_t _value);

#endif /* NBIOT_BC66UTILITY_H_ */
