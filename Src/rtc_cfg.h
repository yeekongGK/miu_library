/*
 * rtc.h
 *
 *  Created on: 20 Nov 2020
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef INC_RTC_H_
#define INC_RTC_H_

#include "main.h"

typedef enum
{
  SECOND_TickType= 0,
  MINUTE_TickType= 1,
  HOUR_TickType= 2,
  DAY_TickType= 3,
  MONTH_TickType= 4,
}RTC_TickType_t;

typedef struct
{
  uint8_t second;
  uint8_t minute;
  uint8_t hour;
  uint8_t date;
}RTC_AlarmStartMarker_t;

#define RTC_CFG_SYNC_WORD				0xB4B1
#define RTC_CFG_HOUR_FORMAT		LL_RTC_HOURFORMAT_24HOUR
/*for sub second granulity of 61us, use Async(0), Sync(32767). We need this for WMBUS. Less Async will incur more current btw.*/
/*for sub second granulity of 1ms, use Async(31), Sync(1023). For non-wmbus, 1ms is more than enough.*/
/*by default Async(127), Sync(255)*/
#define RTC_CFG_ASYNCH_PREDIV  	31
#define RTC_CFG_SYNCH_PREDIV	1023

#define RTC_CFG_DFT_HOURS		config.system.hours
#define RTC_CFG_DFT_MINUTES		config.system.minutes
#define RTC_CFG_DFT_SECONDS		config.system.seconds
#define RTC_CFG_DFT_WEEKDAY		config.system.weekday
#define RTC_CFG_DFT_DAY			config.system.day
#define RTC_CFG_DFT_MONTH		config.system.month
#define RTC_CFG_DFT_YEAR		config.system.year

void RTC_Init(void);
uint64_t RTC_GetTick_100us(void);
uint64_t RTC_GetTick_ms(void);
void RTC_WaitSync(void);
void RTC_DateTime_GetBCD(uint8_t *_date, uint8_t *_month, uint8_t  *_year, uint8_t *_hour, uint8_t *_minute, uint8_t *_second);
uint64_t RTC_DateTime_GetBCDMask(void);
void RTC_DateTime_Update(uint8_t _weekday, uint8_t _date, uint8_t _month, uint8_t  _year, uint8_t _hour, uint8_t _minute, uint8_t _second);/*BCD*/
void RTC_DateTime_UpdateConfig(void);

#endif /* INC_RTC_H_ */
