/******************************************************************************
 * File:        utili.c
 * Author:      Firmware Team
 * Created:     03-10-2025
 * Last Update: -
 *
 * Description:
 *   This file provides a collection of utility functions used throughout the
 *   application. These include functions for random number generation, string
 *   and data type conversions (BCD, hex, float), checksum calculations
 *   (CRC-16, CRC-8), and time/date manipulation. It also includes helpers for
 *   time-based calculations and scheduling.
 *
 * Notes:
 *   - Provides essential helper functions for various modules.
 *
 * To Do:
 *   - Some of the time manipulation logic, especially around
 *     `UTILI_Mask_GetMatchedTime`, is complex and could be simplified.
 *
 ******************************************************************************/

//#include "common.h"
#include <string.h>
#include <math.h>
#include "utili.h"
#include "time.h"

#include "pulser.h"
#include "cfg.h"

//#include "rtc.h"
//#include "printf.h"
//#include "transmission.h"
#define tfp_sprintf(...)

void RTC_ConfigureWakeupTimerInMs(uint32_t _period)
{

}

void UTILI_Init(void)
{
	uint32_t _seed= 0;

	_seed^= LL_GetUID_Word0();
	_seed^= LL_GetUID_Word1();
	_seed^= LL_GetUID_Word2();
	_seed^= (uint32_t)(SYS_GetTimestamp_ms());

	srand(_seed);/*set random seed once*/
}

bool UTILI_IsArrayTheSame(uint8_t *_array1, uint8_t *_array2, uint32_t _arrayLen)
{
	for(int i= 0; i< _arrayLen; i++)
	{
		if(_array1[i]!= _array2[i])
		{
			return false;
		}
	}

	return true;
}

uint32_t UTILI_GetRandom(uint32_t _min, uint32_t _max)
{
	return (_min+ (rand()% ((_max- _min)+ 1)));
}

void UTILI_GetFloatString(float _floatValue, char *_floatString)
{
	_floatString[0]= '\0';/*reset string*/
	int32_t _floatInteger= (int32_t)_floatValue;
	float	_floatRemainder_f= 	    fabs(_floatValue)- (abs(_floatInteger)/1.0);
	uint32_t	_floatRemainder_i= 	(uint32_t)((fabs(_floatValue)- abs(_floatInteger))* 100000);
	while((0== (_floatRemainder_i% 10))&& (0!= _floatRemainder_i))
	{
		_floatRemainder_i/= 10;/*remove trailing zero*/
	}
	uint8_t _noOfLeadingZeroes= 0;

	if(0!= _floatRemainder_f)
	{
		for(float _i= 0.1; _i> 0.00000001; _i/= 10)
		{
			if(_i> _floatRemainder_f)
			{
				_noOfLeadingZeroes++;
			}
			else
			{
				break;
			}
		}
	}

	if(0== _noOfLeadingZeroes)
	{
		tfp_sprintf(_floatString, "%d.%u", _floatInteger, _floatRemainder_i);
	}
	else
	{
		tfp_sprintf(_floatString, "%d.", _floatInteger);
		uint8_t _currentLen= strlen(_floatString);
		for(int _i= 0; _i<_noOfLeadingZeroes; _i++)
		{
			if(32!= _currentLen)
			{
				_floatString[_currentLen++]= '0';
			}
		}
		tfp_sprintf(_floatString+ _currentLen, "%u", _floatRemainder_i);
	}
}

uint8_t UTILI_BCDToBin(uint8_t _value)
{
	return (uint8_t)(((uint8_t)((_value) & (uint8_t)0xF0U) >> (uint8_t)0x4U) * 10U + ((_value) & (uint8_t)0x0FU));
}

uint8_t UTILI_BinToBCD(uint8_t _value)
{
	return (uint8_t)((((_value) / 10U) << 4U) | ((_value) % 10U));
}

uint8_t UTILI_DecimalCharToBCD(char _value)
{
	if(('0'<= _value)&& ('9'>= _value))
	{
		return _value- 0x30;
	}
	else
	{
		return 0xF;
	}
}

uint64_t UTILI_DecimalStringToBCD(char * _value, uint8_t _len)
{
	uint32_t _bcd= 0;

	if(_len> 16)
	{
		_len= 16;
	}

	for(int i= 0; i< _len; i++)
	{
		_bcd|= UTILI_DecimalCharToBCD(_value[(_len- 1)- i])<< (i* 4);
	}

	return _bcd;
}

uint32_t UTILI_BCDAddition(uint32_t _value1, uint32_t _value2)
{
	/*refer http://homepage.divms.uiowa.edu/~jones/bcd/bcd.html*/
    /*
     *add(a,b)
       t1 = a + 0x06666666
       t2 = t1 + b
       t3 = t1 ^ b
       t4 = t2 ^ t3
       t5 = ~t4 & 0x11111110
       t6 = (t5 >> 2) | (t5 >> 3)
       return t2 - t6
      */
	uint32_t _t;

	_value1+= 0x06666666;
	_t= _value1+ _value2;
	_value2^= _value1^ _t;
	_value2= (~_value2& 0x11111110)>> 3;
	_value2|= _value2* 2;

	return (_t- _value2);
}

uint32_t UTILI_BCDSubstraction(uint32_t _value1, uint32_t _value2)
{
	/*refer http://homepage.divms.uiowa.edu/~jones/bcd/bcd.html*/
    /*10's complement addition should be used for substraction.
     *tencomp(a)
       t1 = 0xF9999999 - a
       return add( t1, 0x00000001 )
      */
	uint32_t _tenComplement;

	_tenComplement= 0xF9999999- _value2;
	_tenComplement= UTILI_BCDAddition(_tenComplement, 0x00000001);

	return UTILI_BCDAddition(_value1, _tenComplement);
}

void UTILI_HexStringToBytes(uint8_t *_bytes, char *_hexStr, uint16_t _len)
{
	for(int i= 0, j= 0; i< (_len* 2); i+=2, j++)
	{
		_bytes[j]=  ((_hexStr[i]> '9')? (_hexStr[i]>= 'a')? (_hexStr[i]- 'a')+ 10: (_hexStr[i]- 'A')+ 10: _hexStr[i]- '0')<< 4;
		_bytes[j]|= (_hexStr[i+ 1]> '9')? (_hexStr[i+ 1]>= 'a')? (_hexStr[i+ 1]- 'a')+ 10: (_hexStr[i+ 1]- 'A')+ 10: _hexStr[i+ 1]- '0';
	}
    return;
}

uint32_t UTILI_HexStringToUint32(char *_hexStr)
{
	uint8_t _hexString[8]={'0','0','0','0','0','0','0','0'};
	uint8_t _bytes[4];
	for(int i= (strlen(_hexStr)- 1), j= 7; i>= 0; i--, j--)
	{
		_hexString[j]= _hexStr[i];
	}
	UTILI_HexStringToBytes(_bytes, _hexString, 4);

	return MAKELONG(_bytes[0], _bytes[1], _bytes[2], _bytes[3]);
}

char* UTILI_BytesToHexString(uint8_t *_bytes, uint16_t _byteLen, char *_hexStr)
{
	static uint8_t _internalBuffer[256];

	if(NULL== _hexStr)
	{
		_hexStr= _internalBuffer;
		if((_byteLen* 2)> sizeof(_internalBuffer))
		{
			_byteLen= sizeof(_internalBuffer)/ 2;
		}
	}

    const char * _hex = "0123456789ABCDEF";

    int j= 0;
	for(int i= 0; i< _byteLen; i++)
	{
		_hexStr[j++]= _hex[(_bytes[i]>> 4)& 0x0F];
		_hexStr[j++]= _hex[_bytes[i]& 0x0F];
	}
	_hexStr[j]= '\0';/*null termination*/

	return _hexStr;
}

void UTILI_usDelay(uint32_t _value)
{
	__IO uint32_t _multiplier= SystemCoreClock/ (1000000);/*assuming clock in Mhz range*/
	__IO uint32_t _waitLoop= _value* _multiplier;

	while(_waitLoop!= 0)
	{
		_waitLoop--;
	}
}

uint8_t UTILI_GetMeterModel(CFG_Transmission_t _transmissionType, float _literPerPulse, PULSER_Mode_t _pulseCounterMode, char _band)
{
	uint8_t _meterModelIndex= 0;

	switch(_pulseCounterMode)
	{
		case TRACSENS_Mode:
		case TRACSENSi_Mode:
			if(0.25== _literPerPulse)
			{
				_meterModelIndex= 0x02;
			}
			else if(2.5== _literPerPulse)
			{
				_meterModelIndex= 0x03;
			}
			break;
		case LCSENS_Mode:
			if(1== _literPerPulse)
			{
				_meterModelIndex= 0x07;
			}
			break;
		case ELSTER_Mode:
			if(1== _literPerPulse)
			{
				_meterModelIndex= 0x05;
			}
			else if(10== _literPerPulse)
			{
				_meterModelIndex= 0x06;
			}
			break;
		default:
			break;
	}

	if(NBIOT_Transmission== _transmissionType)
	{
		if('5'== _band)
		{
			_meterModelIndex|= 0x20;
		}
		else //if('8'== _band)
		{
			_meterModelIndex|= 0x10;
		}
	}

	return _meterModelIndex;
}

/**
 * This checksum can be verified by the following site:
 * website: https://crccalc.com/
 * data: 0050002089980008
 * input type: Hex
 * type: CRC-16/ARC
 * poly: 0x8005
 * init: 0x0000
 * result: 0xB77B
 */
#define POLY 0xA001
uint16_t UTILI_GetChecksum(uint16_t _crc, uint8_t *_buf, uint32_t _len)
{
    while (_len--)
    {
    	_crc ^= *_buf++;
    	_crc = _crc & 1 ? (_crc >> 1) ^ POLY : _crc >> 1;
    	_crc = _crc & 1 ? (_crc >> 1) ^ POLY : _crc >> 1;
    	_crc = _crc & 1 ? (_crc >> 1) ^ POLY : _crc >> 1;
    	_crc = _crc & 1 ? (_crc >> 1) ^ POLY : _crc >> 1;
    	_crc = _crc & 1 ? (_crc >> 1) ^ POLY : _crc >> 1;
    	_crc = _crc & 1 ? (_crc >> 1) ^ POLY : _crc >> 1;
    	_crc = _crc & 1 ? (_crc >> 1) ^ POLY : _crc >> 1;
    	_crc = _crc & 1 ? (_crc >> 1) ^ POLY : _crc >> 1;
    }
    return _crc;
}

/*
This code is from Colin O'Flynn - Copyright (c) 2002.
Modified to suit UTILI style.
*/
#define CRC8POLY    0x18              //0X18 = X^8+X^5+X^4+X^0
uint8_t UTILI_GetCRC8(uint8_t _crc, uint8_t *_in, uint16_t _len)
{
	uint8_t  _data;
	uint8_t  _bitCtr;
	uint8_t  _feedbackBit;

	for(int i= 0; i< _len; i++)
	{
		_data= _in[i];
		_bitCtr= 8;

		do
		{
			_feedbackBit= (_crc^ _data)& 0x01;

			if(_feedbackBit== 0x01)
			{
				_crc= _crc^ CRC8POLY;
			}
			_crc= (_crc>> 1)& 0x7F;
			if(_feedbackBit== 0x01)
			{
				_crc= _crc| 0x80;
			}

			_data= _data>> 1;
			_bitCtr--;
		}while(_bitCtr> 0);
	}
	return _crc;
}

static bool UTILI_isLeapYear(uint16_t _year)
{
    return (((_year% 4)== 0)&& ((_year% 100)!= 0)) || ((_year% 400)== 0);
}

uint16_t UTILI_daysInYear(uint8_t _day, uint8_t _mon, uint16_t _year)
{
    static const uint16_t _days[2][13]=
    {
        {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
        {0, 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}
    };
    uint8_t _leap= (true== UTILI_isLeapYear(_year))? 1: 0;

    return _days[_leap][_mon]+ _day;
}

uint16_t UTILI_GetDays(void)
{
	uint8_t _day= 	UTILI_BCDToBin(LL_RTC_DATE_GetDay(RTC));
	uint8_t _month= UTILI_BCDToBin(LL_RTC_DATE_GetMonth(RTC));
	uint8_t _year= 	UTILI_BCDToBin(LL_RTC_DATE_GetYear(RTC));

	return UTILI_daysInYear(_day, _month, 2000+ _year);
}

void UTILI_WaitRTCSync(void)
{
	__IO uint32_t _timeout= 10;//500;
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

uint32_t UTILI_100usTimeStamp(void)
{
	UTILI_WaitRTCSync();/*wait for sync*/

	/*TODO: optimize this if needed*/
	__IO float _subseconds= (LL_RTC_GetSynchPrescaler(RTC)- LL_RTC_TIME_GetSubSecond(RTC))/ ((LL_RTC_GetSynchPrescaler(RTC)+ 1)/ 1.0);
	uint32_t _sec= UTILI_BCDToBin(LL_RTC_TIME_GetSecond(RTC));
	uint32_t _min= UTILI_BCDToBin(LL_RTC_TIME_GetMinute(RTC));
	uint32_t _hour= UTILI_BCDToBin(LL_RTC_TIME_GetHour(RTC));
	uint32_t _day= UTILI_BCDToBin(LL_RTC_DATE_GetDay(RTC));
	uint32_t _month= UTILI_BCDToBin(LL_RTC_DATE_GetMonth(RTC));
	uint32_t _year= UTILI_BCDToBin(LL_RTC_DATE_GetYear(RTC));
	uint32_t _daysInYear= UTILI_daysInYear(_day, _month, 2000+ _year);
	_year+= 100;

	uint32_t  _timestamp;
	_timestamp= _sec;
	_timestamp+= (_min* 60);
	_timestamp+= (_hour* 3600);
	_timestamp+= (_daysInYear* 86400);
	_timestamp+= ((_year-70)* 31536000);
	_timestamp+= (((_year-69)/4)* 86400);
	_timestamp-= (((_year-1)/100)* 86400);
	_timestamp+= (((_year+299)/400)* 86400);

	_timestamp*= 10000;/*seconds to 100us*/
	_timestamp+= (_subseconds* 10000.0);/*seconds to 100us*/

	return _timestamp;
}

uint32_t UTILI_100usTimDiffFromNow(uint32_t t0)
{
	uint32_t _now= UTILI_100usTimeStamp();
	uint32_t _timeDiff;

	if(_now< t0)
	{
	/*
		case in which the current now is after the 32 bits max
				_t0       2^32|0        now
		---------|------------|----------|----
		*/
		_timeDiff= (0xffffffff- t0+ _now);
	}
	else
	{
		_timeDiff= (_now- t0);/* else return the simple difference */
	}

	return _timeDiff;
}

bool UTILI_100usIsTimeout(uint32_t _from, uint32_t _period)
{
	uint32_t _timDiffFromNow= UTILI_100usTimDiffFromNow(_from);

	if(_timDiffFromNow>= _period)
	{
		RTC_ConfigureWakeupTimerInMs(0);
		return true;
	}
	else
	{
		RTC_ConfigureWakeupTimerInMs((_period- _timDiffFromNow)/ 10);
	}

	return false;
}

uint32_t UTILI_msTimeStamp(void)
{
	return (UTILI_100usTimeStamp()/ 10);
}

uint32_t UTILI_msTimDiffFromNow(uint32_t t0)
{
	uint32_t _now= UTILI_msTimeStamp();
	uint32_t _timeDiff;

	if(_now< t0)
	{
	/*
		case in which the current now is after the 32 bits max
				_t0       2^32|0        now
		---------|------------|----------|----
		*/
		_timeDiff= (0xffffffff- t0+ _now);
	}
	else
	{
		_timeDiff= (_now- t0);/* else return the simple difference */
	}

	return _timeDiff;
}

bool UTILI_msIsTimeout(uint32_t _from, uint32_t _period)
{
	uint32_t _timDiffFromNow= UTILI_msTimDiffFromNow(_from);

	if(_timDiffFromNow>= _period)
	{
		RTC_ConfigureWakeupTimerInMs(0);
		return true;
	}
	else
	{
		RTC_ConfigureWakeupTimerInMs(_period- _timDiffFromNow);
	}

	return false;
}

uint32_t UTILI_sTimeStamp(void)
{
	UTILI_WaitRTCSync();/*wait for sync*/

	/*TODO: optimize this if needed*/
	uint32_t _sec= UTILI_BCDToBin(LL_RTC_TIME_GetSecond(RTC));
	uint32_t _min= UTILI_BCDToBin(LL_RTC_TIME_GetMinute(RTC));
	uint32_t _hour= UTILI_BCDToBin(LL_RTC_TIME_GetHour(RTC));
	uint32_t _day= UTILI_BCDToBin(LL_RTC_DATE_GetDay(RTC));
	uint32_t _month= UTILI_BCDToBin(LL_RTC_DATE_GetMonth(RTC));
	uint32_t _year= UTILI_BCDToBin(LL_RTC_DATE_GetYear(RTC));
	uint32_t _daysInYear= UTILI_daysInYear(_day, _month, 2000+ _year);
	_year+= 100;

	uint32_t  _timestamp;
	_timestamp= _sec;
	_timestamp+= (_min* 60);
	_timestamp+= (_hour* 3600);
	_timestamp+= (_daysInYear* 86400);
	_timestamp+= ((_year-70)* 31536000);
	_timestamp+= (((_year-69)/4)* 86400);
	_timestamp-= (((_year-1)/100)* 86400);
	_timestamp+= (((_year+299)/400)* 86400);

	return _timestamp;
}

uint32_t UTILI_sTimDiffFromNow(uint32_t t0)
{
	uint32_t _now= UTILI_sTimeStamp();
	uint32_t _timeDiff;

	if(_now< t0)
	{
	/*
		case in which the current now is after the 32 bits max
				_t0       2^32|0        now
		---------|------------|----------|----
		*/
		_timeDiff= (0xffffffff- t0+ _now);
	}
	else
	{
		_timeDiff= (_now- t0);/* else return the simple difference */
	}

	return _timeDiff;
}

bool UTILI_sIsTimeout(uint32_t _from, uint32_t _period)
{
	uint32_t _timDiffFromNow= UTILI_sTimDiffFromNow(_from);

	if(_timDiffFromNow>= _period)
	{
		RTC_ConfigureWakeupTimerInMs(0);
		return true;
	}
	else
	{
		RTC_ConfigureWakeupTimerInMs((_period- _timDiffFromNow)* 1000);
	}

	return false;
}

uint32_t UTILI_dayTimeStamp(void)/*this is wrong, should get numbers of days isntead of zeroing hr/min/sec, need to resync with wmbus mobile in the future*/
{
	UTILI_WaitRTCSync();/*wait for sync*/

	/*TODO: optimize this if needed*/
	float _subseconds= 0;
	uint8_t _sec= 0;
	uint8_t _min= 0;
	uint8_t _hour= 0;
	uint8_t _day= UTILI_BCDToBin(LL_RTC_DATE_GetDay(RTC));
#if WMBUS_USE_BUG_ENCRYPT == 1
	uint8_t _month= UTILI_BCDToBin(LL_RTC_DATE_GetMonth(RTC))- 1;
#else
	uint8_t _month= UTILI_BCDToBin(LL_RTC_DATE_GetMonth(RTC));
#endif
	uint8_t _year= UTILI_BCDToBin(LL_RTC_DATE_GetYear(RTC));
	uint16_t _daysInYear= UTILI_daysInYear(_day, _month, 2000+ _year);
	_year+= 100;

	uint32_t  _timestamp= _sec + _min*60 + _hour*3600 + _daysInYear*86400 +
		    (_year-70)*31536000 + ((_year-69)/4)*86400 -
		    ((_year-1)/100)*86400 + ((_year+299)/400)*86400;

	_timestamp*= 10000;/*seconds to 100us*/
	_timestamp+= (_subseconds* 10000.0);/*seconds to 100us*/

	return _timestamp;
}

uint16_t UTILI_minutesSinceMonthBegin(void)
{
	UTILI_WaitRTCSync();/*wait for sync*/

	/*TODO: optimize this if needed*/
	uint8_t _min= UTILI_BCDToBin(LL_RTC_TIME_GetMinute(RTC));
	uint8_t _hour= UTILI_BCDToBin(LL_RTC_TIME_GetHour(RTC));
	uint8_t _day= UTILI_BCDToBin(LL_RTC_DATE_GetDay(RTC));

	return _min + _hour*60 + _day*60*24;
}

uint32_t UTILI_secondsSinceMidnight(void)
{
	time_t _now= time(NULL);
	struct tm _midnight= *(localtime(&_now));

	_midnight.tm_hour= _midnight.tm_min= _midnight.tm_sec= 0;

	return (uint32_t) difftime(mktime(&_midnight), _now);
}

uint16_t UTILI_Array_Copy(uint8_t *_dest, uint8_t *_source, uint16_t _len)
{
	memcpy(_dest, _source, _len);
	return _len;
}

uint16_t UTILI_Array_CopyString(void *_dest, char *_source)
{
	uint8_t *_dest_= (uint8_t *)_dest;
	uint16_t _len=  UTILI_Array_Copy(_dest_, (uint8_t *)_source, strlen(_source));
	_dest_[_len++]= '\0';

	return _len;
}

uint16_t UTILI_Array_CopyUntil(uint8_t *_dest, uint8_t *_source, uint8_t *_end)
{
	uint8_t *_src= _source;
	while(_src!= _end)
	{
		*(_dest++)= *(_src++);
	}

	return (_end- _source);
}

uint16_t UTILI_Array_Copy16(uint8_t *_dest, uint16_t _source)
{
	return UTILI_Array_Copy(_dest, &_source, 2);
}

uint16_t UTILI_Array_Copy16_Ptr(uint8_t *_dest, uint8_t *_source)
{
	return UTILI_Array_Copy(_dest, _source, 2);
}

uint16_t UTILI_Array_Copy32(uint8_t *_dest, uint32_t _source)
{
	return UTILI_Array_Copy(_dest, &_source, 4);
}

uint16_t UTILI_Array_Copy32_Ptr(uint8_t *_dest, uint8_t *_source)
{
	return UTILI_Array_Copy(_dest, _source, 4);
}

uint16_t UTILI_Array_Copy64(uint8_t *_dest, uint64_t _source)
{
	return UTILI_Array_Copy(_dest, &_source, 8);
}

uint16_t UTILI_Array_Copy64_Ptr(uint8_t *_dest, uint8_t *_source)
{
	return UTILI_Array_Copy(_dest, _source, 8);
}

uint64_t UTILI_Mask_Decode(char *_stringMask)
{
	uint64_t _datetime;
	uint8_t *_pDateTime= (uint8_t *)(&_datetime);

	_pDateTime[7]= (uint8_t)UTILI_DecimalStringToBCD(_stringMask+ 0, 2);
	_pDateTime[6]= (uint8_t)UTILI_DecimalStringToBCD(_stringMask+ 2, 2);
	_pDateTime[5]= (uint8_t)UTILI_DecimalStringToBCD(_stringMask+ 4, 2);
	_pDateTime[4]= (uint8_t)UTILI_DecimalStringToBCD(_stringMask+ 6, 2);
	_pDateTime[3]= (uint8_t)UTILI_DecimalStringToBCD(_stringMask+ 8, 2);
	_pDateTime[2]= (uint8_t)UTILI_DecimalStringToBCD(_stringMask+ 10, 2);
	_pDateTime[1]= 0x00;
	_pDateTime[0]= 0x00;

	return _datetime;
}

uint64_t UTILI_Mask_Convert(time_t _time)
{
	uint64_t _mask;
	uint8_t *_pMask= (uint8_t *)(&_mask);
	struct tm _timeinfo= *(localtime(&_time));

	_pMask[YEAR_MaskIndex]= UTILI_BinToBCD(_timeinfo.tm_year- 100);
	_pMask[MONTH_MaskIndex]= UTILI_BinToBCD(_timeinfo.tm_mon+ 1);
	_pMask[DATE_MaskIndex]= UTILI_BinToBCD(_timeinfo.tm_mday);
	_pMask[HOUR_MaskIndex]= UTILI_BinToBCD(_timeinfo.tm_hour);
	_pMask[MINUTE_MaskIndex]= UTILI_BinToBCD(_timeinfo.tm_min);
	_pMask[SECOND_MaskIndex]= UTILI_BinToBCD(_timeinfo.tm_sec);
	_pMask[MILISEC2_MaskIndex]= 0x00;
	_pMask[MILISEC1_MaskIndex]= 0x00;

	return _mask;
}

struct tm UTILI_Mask_ToTimeinfo(uint64_t _mask)
{
	uint8_t *_pMask= (uint8_t *)(&_mask);
	struct tm _timeinfo;

	_timeinfo.tm_year= UTILI_BCDToBin(_pMask[YEAR_MaskIndex])+ 100;
	_timeinfo.tm_mon= UTILI_BCDToBin(_pMask[MONTH_MaskIndex])- 1;
	_timeinfo.tm_mday= UTILI_BCDToBin(_pMask[DATE_MaskIndex]);
	_timeinfo.tm_hour= UTILI_BCDToBin(_pMask[HOUR_MaskIndex]);
	_timeinfo.tm_min= UTILI_BCDToBin(_pMask[MINUTE_MaskIndex]);
	_timeinfo.tm_sec= UTILI_BCDToBin(_pMask[SECOND_MaskIndex]);

	return _timeinfo;
}

time_t UTILI_Mask_ToTime(uint64_t _mask)
{
	struct tm _timeinfo= UTILI_Mask_ToTimeinfo(_mask);

	return mktime(&_timeinfo);
}

time_t UTILI_Mask_GetMatchedTime(time_t _currTime, uint64_t _mask)
{
	/*strategy:
	 * 1. translate the mask into time
	 * 2. using time lib, get the epoch seconds
	 * 3. plus currtime with matched time
	 * */
	uint64_t _currMask= UTILI_Mask_Convert(_currTime);
	uint64_t _projectedMask= 0;
	uint64_t _minMask= 0x0001010000000000;
	uint8_t *_pCurr= (uint8_t *)&_currMask;
	uint8_t *_pMask= (uint8_t *)&_mask;
	uint8_t *_pMinMask= (uint8_t *)&_minMask;
	uint8_t *_pProjectedMask= (uint8_t *)&_projectedMask;
	__IO uint8_t _curr_;
	__IO uint8_t _mask_;
	__IO uint8_t _min_;

	__IO bool _setMin= false;
	__IO uint8_t _minIndex= YEAR_MaskIndex* 2;

	for(int i= YEAR_MaskIndex; i>= SECOND_MaskIndex; i--)
	{
		_curr_= (0xF0& _pCurr[i]);
		_mask_= (0xF0& _pMask[i]);
		_min_= (0xF0& _pMinMask[i]);
		if(0xF0== _mask_)
		{
			_pProjectedMask[i]|= ((true== _setMin)? _min_: _curr_);
			if(false== _setMin)
			{
				_minIndex= (i* 2)+ 1;
			}
		}
		else //(0x0F!= _mask_)
		{
			_pProjectedMask[i]|= _mask_;
			_setMin= true;
		}

		_curr_= (0x0F& _pCurr[i]);
		_mask_= (0x0F& _pMask[i]);
		_min_= (0x0F& _pMinMask[i]);
		if(0x0F== _mask_)
		{
			_pProjectedMask[i]|= ((true== _setMin)? _min_: _curr_);
			if(false== _setMin)
			{
				_minIndex= i* 2;
			}
		}
		else //(0x0F!= _mask_)
		{
			_pProjectedMask[i]|= _mask_;
			_setMin= true;
		}
	}

	time_t _projectedTime= UTILI_Mask_ToTime(_projectedMask);
	struct tm _projectedTimeinfo= UTILI_Mask_ToTimeinfo(_projectedMask);
	while(_projectedTime< _currTime)
	{
		switch(_minIndex)
		{
			case 4:/*sec*/
				_projectedTime+= 1;
				break;
			case 5:/*decisec*/
				_projectedTime+= 10;
				break;
			case 6:/*min*/
				_projectedTime+= 60;
				break;
			case 7:/*decimin*/
				_projectedTime+= 600;
				break;
			case 8:/*hr*/
				_projectedTime+= 3600;
				break;
			case 9:/*decihr*/
				_projectedTime+= 36000;
				break;
			case 10:/*day*/
				_projectedTime+= 86400;
				break;
			case 11:/*deciday*/
				_projectedTime+= 864000;
				break;
			case 12:/*month*/
				if((_projectedTimeinfo.tm_mon+ 1)< 12)
				{
					_projectedTimeinfo.tm_mon+= 1;
				}
				else
				{
					_projectedTimeinfo.tm_year+= 1;
				}
				_projectedTime= mktime(&_projectedTimeinfo);
				break;
			case 13:/*decimonth*/
				if((_projectedTimeinfo.tm_mon+ 1)< 2)/*only Jan and Feb can increase decimally*/
				{
					_projectedTimeinfo.tm_mon+= 10;
				}
				else
				{
					_projectedTimeinfo.tm_year+= 1;
				}
				_projectedTime= mktime(&_projectedTimeinfo);
				break;
			case 14:/*year*/
				_projectedTimeinfo.tm_year+= 1;
				_projectedTime= mktime(&_projectedTimeinfo);
				break;
			case 15:/*deciyear*/
				_projectedTimeinfo.tm_year+= 10;
				_projectedTime= mktime(&_projectedTimeinfo);
				break;
		}
	}

	return _projectedTime;
}

time_t UTILI_Mask_GetMatchedTimeOld(time_t _currTime, uint64_t _currMask, uint64_t _mask)
{
	uint8_t *_pCurr= (uint8_t *)&_currMask;
	uint8_t *_pMask= (uint8_t *)&_mask;
	__IO bool _setMin= false;
	__IO time_t _nextTime= 0;
	__IO int16_t _add[MAX_MaskIndex+ 1]= {0};
	__IO uint8_t _curr_;
	__IO uint8_t _mask_;

	for(int i= YEAR_MaskIndex; i>= SECOND_MaskIndex; i--)
	{
		_curr_= ((0xF0& _pCurr[i])>> 4);
		_mask_= ((0xF0& _pMask[i])>> 4);
		if(0x0F== _mask_)
		{
			_add[i]= (true== _setMin)?0: _curr_* 10;
		}
		else //(0x0F!= _mask_)
		{
			if(_mask_> _curr_)
			{
				_add[i]+= (true== _setMin)?0: _curr_* 10;
				_add[i]+= (true== _setMin)?0: ((_mask_- _curr_)* 10);
				_setMin= true;
			}
			else if(_mask_< _curr_)
			{
				_add[i]+= (true== _setMin)?0: _mask_* 10;
				_add[i+ 1]+= (true== _setMin)?0: 1;
				_setMin= true;
			}
			else
			{
				_add[i]+= (true== _setMin)?0: _mask_* 10;
			}
		}

		_curr_= (0x0F& _pCurr[i]);
		_mask_= (0x0F& _pMask[i]);
		if(0x0F== _mask_)
		{
			_add[i]+= (true== _setMin)?0: _curr_;
		}
		else //(0x0F!= _mask_)
		{
			if(_mask_> _curr_)
			{
				_add[i]+= (true== _setMin)?0: _curr_;
				_add[i]+= (true== _setMin)?0: (_mask_- _curr_);
				_setMin= true;
			}
			else if(_mask_< _curr_)
			{
				_add[i]+= (true== _setMin)?0: _mask_;
				_add[i]+= (true== _setMin)?0: 10;
				_setMin= true;
			}
			else
			{
				_add[i]+= (true== _setMin)?0: _mask_* 10;
			}
		}
	}

	if(false== _setMin)
	{
		return _currTime;
	}

	_add[YEAR_MaskIndex]+= (_add[YEAR_MaskIndex+ 1]* 100);
	_add[YEAR_MaskIndex]+= (_add[MONTH_MaskIndex]/ 13);
	_add[MONTH_MaskIndex]%= 13;

	_nextTime= 0;
	_nextTime+= _add[SECOND_MaskIndex];
	_nextTime+= (_add[MINUTE_MaskIndex]* 60);
	_nextTime+= (_add[HOUR_MaskIndex]* 3600);
	_nextTime+= (_add[DATE_MaskIndex]* 86400);
	_nextTime+= (UTILI_daysInYear(0, _add[MONTH_MaskIndex], 2000+ _add[YEAR_MaskIndex])* 86400);

	struct tm _timeinfo;
	_timeinfo.tm_year= _add[YEAR_MaskIndex]+ 100;  // check assumption here years start from 1900 in tm
	_timeinfo.tm_mon= 0;   // check assumption here Jan = 0 in tm
	_timeinfo.tm_mday= 1;
	_timeinfo.tm_hour= 0 ;
	_timeinfo.tm_min= 0;
	_timeinfo.tm_sec= 0;

	_nextTime+= (mktime(&_timeinfo)- 86400);

	return _nextTime;
}

time_t UTILI_Mask_GetMatchedTimeFromNow(uint64_t _mask)
{
	time_t _currTime= SYS_GetTimestamp_s();
	return UTILI_Mask_GetMatchedTime(_currTime, _mask);
}

time_t UTILI_ComputeNextTime(time_t _currTime, time_t _referenceTime, time_t _interval)
{
	time_t _nextTime= _referenceTime;
	while(_nextTime<= _currTime)
	{
		_nextTime+= _interval;
	}

	return _nextTime;
}

time_t UTILI_GetSmallerTime(time_t _comparedTime, time_t _nextTime)
{
	return (_nextTime< _comparedTime)? _nextTime: _comparedTime;
}

time_t UTILI_GetSmallerPeriod(time_t _comparedPeriod, time_t _currTime, time_t _nextTime)
{
	time_t _diff= _nextTime- _currTime;
	if(_diff< 0)
	{
		_diff= 0;
	}
	return (_diff< _comparedPeriod)? _diff: _comparedPeriod;
}

