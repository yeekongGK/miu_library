/*
 * rtc.c
 *
 *  Created on: 16 Dec 2020
 *      Author: muhammad.ahmad@georgekent.net
 */

//#include "common.h"
#include "rtc.h"
#include "utili.h"
#include "exit.h"

static void RTC_Disable(void)
{
	LL_RTC_DisableWriteProtection(RTC);/* needed as RTC registers are write protected */
	LL_RTC_ALMA_Disable(RTC);
	LL_RTC_ALMB_Disable(RTC);
	LL_RTC_WAKEUP_Disable(RTC);
	LL_RTC_ClearFlag_ALRA(RTC);
	LL_RTC_ClearFlag_ALRB(RTC);
	LL_RTC_ClearFlag_WUT(RTC);
	LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_17| LL_EXTI_LINE_20);/* clear exti line 17(alarm) and 20(wakeup timer) -- this connect to nvic*/
	LL_RTC_EnableWriteProtection(RTC);
}

void RTC_Init(void)
{
	static bool _isInitialized= false;

	if(true== _isInitialized)
	{
		return;
	}
	_isInitialized= true;

	LL_RTC_InitTypeDef RTC_InitStruct;


	/*Due diligent since RTC is a bit tricky. They might not even stop when reset.
	* The problem faced was WUT not restarted after reset with power on. Possibly stucked at isr flag.
	* Refer datasheet:
	* "System reset, as well as low-power modes (Sleep, Stop and Standby) have no influence on the wakeup timer."
	*/
	RTC_Disable();

	/* Peripheral clock enable */

	LL_RCC_EnableRTC();
//	if(SUCCESS!= LL_RTC_DeInit(RTC)) /*De-Initializes the RTC registers to their default reset values, except RTC Clock source and RTC Backup Data*/
//	{
//		/* Initialization Error */
//	}

	/**Initialize RTC and set the Time and Date*/
	RTC_InitStruct.HourFormat= 		RTC_CFG_HOUR_FORMAT;
	RTC_InitStruct.AsynchPrescaler= RTC_CFG_ASYNCH_PREDIV;
	RTC_InitStruct.SynchPrescaler= 	RTC_CFG_SYNCH_PREDIV;
	LL_RTC_Init(RTC, &RTC_InitStruct);

	UTILI_WaitRTCSync();/*wait for sync*/
	/**Initialize RTC and set the Time and Date, if not initialized yet*/
	if((RTC_CFG_SYNC_WORD!= LL_RTC_BAK_GetRegister(RTC, RTC_SYNC_BKPReg))
			||(
					(0x00== LL_RTC_TIME_GetHour(RTC))&& (0x00== LL_RTC_TIME_GetMinute(RTC))/*&& (0x00== LL_RTC_TIME_GetSecond(RTC))*/
					&& (0x00== LL_RTC_DATE_GetYear(RTC))&& (0x01== LL_RTC_DATE_GetMonth(RTC))&& (0x01== LL_RTC_DATE_GetDay(RTC))
			)
	)
	{
		RTC_DateTime_Update(RTC_CFG_DFT_WEEKDAY, RTC_CFG_DFT_DAY, RTC_CFG_DFT_MONTH, RTC_CFG_DFT_YEAR, RTC_CFG_DFT_HOURS, RTC_CFG_DFT_MINUTES, RTC_CFG_DFT_SECONDS);
		DBG_Print("ERROR: RTC Register not valid\r\n");

		for(int _reg= LL_RTC_BKP_DR0; _reg<= LL_RTC_BKP_DR31; _reg++)
		{
			LL_RTC_BAK_SetRegister(RTC, _reg, 0); /*register need to be initialized when RTC failed, otherwise we will get weird values(like in the case of faulty sensor count in PUB 2022 project)*/
		}
	}
}

static bool RTC_isLeapYear(uint16_t _year)
{
    return (((_year% 4)== 0)&& ((_year% 100)!= 0)) || ((_year% 400)== 0);
}

static uint16_t RTC_daysInYear(uint8_t _day, uint8_t _mon, uint16_t _year)
{
    static const uint16_t _days[2][13]=
    {
        {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
        {0, 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}
    };
    uint8_t _leap= (true== RTC_isLeapYear(_year))? 1: 0;

    return _days[_leap][_mon]+ _day;
}

uint64_t RTC_GetTick_ms(void)
{
	UTILI_WaitRTCSync();/*wait for sync*/

	__IO float _subseconds= (LL_RTC_GetSynchPrescaler(RTC)- LL_RTC_TIME_GetSubSecond(RTC))/ ((LL_RTC_GetSynchPrescaler(RTC)+ 1)/ 1.0);
	uint32_t _sec= UTILI_BCDToBin(LL_RTC_TIME_GetSecond(RTC));
	uint32_t _min= UTILI_BCDToBin(LL_RTC_TIME_GetMinute(RTC));
	uint32_t _hour= UTILI_BCDToBin(LL_RTC_TIME_GetHour(RTC));
	uint32_t _day= UTILI_BCDToBin(LL_RTC_DATE_GetDay(RTC));
	uint32_t _month= UTILI_BCDToBin(LL_RTC_DATE_GetMonth(RTC));
	uint32_t _year= UTILI_BCDToBin(LL_RTC_DATE_GetYear(RTC));
	uint32_t _daysInYear= RTC_daysInYear(_day- 1, _month, 2000+ _year);
	_year+= 100;

	uint64_t  _timestamp;
	_timestamp= _sec;
	_timestamp+= (_min* 60);
	_timestamp+= (_hour* 3600);
	_timestamp+= (_daysInYear* 86400);
	_timestamp+= ((_year-70)* 31536000);
	_timestamp+= (((_year-69)/4)* 86400);
	_timestamp-= (((_year-1)/100)* 86400);
	_timestamp+= (((_year+299)/400)* 86400);

	_timestamp*= 1000;/*seconds to ms*/
	_timestamp+= (_subseconds* 1000.0);/*seconds to ms*/

	return _timestamp;
}

void RTC_WaitSync(void)
{
	__IO uint32_t _timeout= 500;
	while((0U!= _timeout)&& (0== LL_RTC_IsActiveFlag_RS(RTC)))/*wait for sync*/
	{
		_timeout-= (1U== LL_SYSTICK_IsActiveCounterFlag())? 1: 0;/*every systick*/
	}

	if(0== _timeout)
	{
		/*this works only second round. TODO:investigate why*/
		LL_RTC_DisableWriteProtection(RTC); /* Disable the write protection for RTC registers */
		if (LL_RTC_EnterInitMode(RTC) != ERROR)
		{
			LL_RTC_DisableInitMode(RTC);
		}
		LL_RTC_EnableWriteProtection(RTC);/* Enable the write protection for RTC registers */
	}
}

void RTC_DateTime_GetBCD(uint8_t *_date, uint8_t *_month, uint8_t  *_year, uint8_t *_hour, uint8_t *_minute, uint8_t *_second)
{
	RTC_WaitSync();

	if(NULL!= _date)
	{
		*_date= LL_RTC_DATE_GetDay(RTC);
	}

	if(NULL!= _month)
	{
		*_month= LL_RTC_DATE_GetMonth(RTC);
	}

	if(NULL!= _year)
	{
		*_year= LL_RTC_DATE_GetYear(RTC);
	}

	if(NULL!= _hour)
	{
		*_hour= LL_RTC_TIME_GetHour(RTC);
	}

	if(NULL!= _minute)
	{
		*_minute= LL_RTC_TIME_GetMinute(RTC);
	}

	if(NULL!= _second)
	{
		*_second= LL_RTC_TIME_GetSecond(RTC);
	}
}

uint64_t RTC_DateTime_GetBCDMask(void)
{
	uint64_t _datetime;
	uint8_t *_pDateTime= (uint8_t *)(&_datetime);

	RTC_WaitSync();
	_pDateTime[7]= LL_RTC_DATE_GetYear(RTC);
	_pDateTime[6]= LL_RTC_DATE_GetMonth(RTC);
	_pDateTime[5]= LL_RTC_DATE_GetDay(RTC);
	_pDateTime[4]= LL_RTC_TIME_GetHour(RTC);
	_pDateTime[3]= LL_RTC_TIME_GetMinute(RTC);
	_pDateTime[2]= LL_RTC_TIME_GetSecond(RTC);
	_pDateTime[1]= 0x00;
	_pDateTime[0]= 0x00;

	return _datetime;
}

void RTC_DateTime_Update(uint8_t _weekday, uint8_t _date, uint8_t _month, uint8_t  _year, uint8_t _hour, uint8_t _minute, uint8_t _second)
{
	LL_RTC_TimeTypeDef RTC_TimeStruct;
	LL_RTC_DateTypeDef RTC_DateStruct;

	config.system.seconds= RTC_TimeStruct.Seconds= _second;
	config.system.minutes= RTC_TimeStruct.Minutes= _minute;
	config.system.hours= RTC_TimeStruct.Hours= _hour;
	config.system.weekday= RTC_DateStruct.WeekDay= _weekday;
	config.system.day= RTC_DateStruct.Day= _date;
	config.system.month= RTC_DateStruct.Month= _month;
	config.system.year= RTC_DateStruct.Year= _year;
	for(int _i= 0; _i< 4; _i++)
	{
		if(SUCCESS== LL_RTC_TIME_Init(RTC, LL_RTC_FORMAT_BCD, &RTC_TimeStruct))
		{
			if(SUCCESS== LL_RTC_DATE_Init(RTC, LL_RTC_FORMAT_BCD, &RTC_DateStruct))
			{
				LL_RTC_BAK_SetRegister(RTC, RTC_SYNC_BKPReg, RTC_CFG_SYNC_WORD);/*indication time had been set*/
				break;
			}
		}
		LL_mDelay(1+ _i);
	}
}

void RTC_DateTime_UpdateConfig(void)
{
	UTILI_WaitRTCSync();/*wait for sync*/

	config.system.seconds= LL_RTC_TIME_GetSecond(RTC);
	config.system.minutes= LL_RTC_TIME_GetMinute(RTC);
	config.system.hours= LL_RTC_TIME_GetHour(RTC);
	config.system.weekday= LL_RTC_DATE_GetWeekDay(RTC);
	config.system.day= LL_RTC_DATE_GetDay(RTC);
	config.system.month= LL_RTC_DATE_GetMonth(RTC);
	config.system.year= LL_RTC_DATE_GetYear(RTC);
}

/*override time()*/
/*refrenece: https://stackoverflow.com/questions/55297839/problem-with-time-function-in-embedded-application-with-c*/
time_t time(time_t *_timep)
{
	UTILI_WaitRTCSync();/*wait for sync*/

	uint32_t _sec= UTILI_BCDToBin(LL_RTC_TIME_GetSecond(RTC));
	uint32_t _min= UTILI_BCDToBin(LL_RTC_TIME_GetMinute(RTC));
	uint32_t _hour= UTILI_BCDToBin(LL_RTC_TIME_GetHour(RTC));
	uint32_t _day= UTILI_BCDToBin(LL_RTC_DATE_GetDay(RTC));
	uint32_t _month= UTILI_BCDToBin(LL_RTC_DATE_GetMonth(RTC));
	uint32_t _year= 2000+ UTILI_BCDToBin(LL_RTC_DATE_GetYear(RTC));

	// Normalise to time.h library epoch time_t (normally Unix epoch)
	struct tm _timeinfo;
	_timeinfo.tm_mon= _month- 1 ;   // check assumption here Jan = 0 in tm
	_timeinfo.tm_mday= _day ;
	_timeinfo.tm_year= _year- 1900;  // check assumption here years start from 1900 in tm
	_timeinfo.tm_hour= _hour ;
	_timeinfo.tm_min= _min;
	_timeinfo.tm_sec= _sec;

	// Convert to timestamp
	time_t _t= mktime(&_timeinfo);
	if(_timep!= NULL)
	{
		*_timep= _t ;
	}

    return _t;
}
