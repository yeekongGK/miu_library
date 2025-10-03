/******************************************************************************
 * File:        utili.h
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file defines a collection of utility functions and macros used
 *   throughout the application. It includes helpers for data conversion
 *   (BCD, hex, string), time and date manipulation, checksum calculations,
 *   and other common tasks.
 *
 * Notes:
 *   - Provides a set of general-purpose tools to simplify common operations.
 *
 * To Do:
 *   - -
 *
 ******************************************************************************/

#ifndef UTILI_H_
#define UTILI_H_

#include "main.h"
#include <time.h>

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

#define NIBBLE_TO_BINARY_PATTERN "%c%c%c%c"
#define NIBBLE_TO_BINARY(byte)  \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

#define MAKEWORD(msb, lsb)                    ((uint16_t)( (((uint16_t)((uint8_t)(msb))) << 8) | ((uint8_t)(lsb))) )
#define MAKELONG(val3, val2, val1, val0)      ((uint32_t)( (((uint32_t)((uint8_t)(val3))) << 24) | (((uint32_t)((uint8_t)(val2))) << 16) | (((uint32_t)((uint8_t)(val1))) << 8) | ((uint8_t)(val0))) )

#define SECONDS_TO_MILISECONDS(x)		(x* 1000)
#define MINUTE_TO_MILISECONDS(x)		(x* 60* 1000)
#define HOUR_TO_MILISECONDS(x)			(x* 60* 60* 1000)
#define DAY_TO_MILISECONDS(x)			(x* 24* 60* 60* 1000)

#define MINUTE_TO_SECONDS(x)			(x* 60)
#define HOUR_TO_SECONDS(x)				(x* 60* 60)
#define DAY_TO_SECONDS(x)				(x* 24* 60* 60)

#define HZ_TO_32768HZ_COUNTER(x)		((uint16_t)(((1/(x/1.0))/(1.0/32768.0))- 1))
#define MICROSEC_TO_32768HZ_COUNTER(x)	((uint16_t)(((x/1000000.0)/(1.0/32768.0))- 1))

typedef enum
{
	MILISEC1_MaskIndex= 0,
	MILISEC2_MaskIndex,
	SECOND_MaskIndex,
	MINUTE_MaskIndex,
	HOUR_MaskIndex,
	DATE_MaskIndex,
	MONTH_MaskIndex,
	YEAR_MaskIndex,
	MAX_MaskIndex,
}UTILI_MaskIndex;

void UTILI_Init(void);
bool UTILI_IsArrayTheSame(uint8_t *_array1, uint8_t *_array2, uint32_t _arrayLen);
uint32_t UTILI_GetRandom(uint32_t _min, uint32_t _max);
void UTILI_GetFloatString(float _floatValue, char *_floatString);
uint8_t UTILI_BCDToBin(uint8_t _value);
uint8_t UTILI_BinToBCD(uint8_t _value);
uint8_t UTILI_DecimalCharToBCD(char _value);
uint64_t UTILI_DecimalStringToBCD(char * _value, uint8_t _len);
uint32_t UTILI_BCDAddition(uint32_t _value1, uint32_t _value2);
uint32_t UTILI_BCDSubstraction(uint32_t _value1, uint32_t _value2);
void UTILI_HexStringToBytes(uint8_t *_bytes, char *_hexStr, uint16_t _len);
uint32_t UTILI_HexStringToUint32(char *_hexStr);
char* UTILI_BytesToHexString(uint8_t *_bytes, uint16_t _byteLen, char *_hexStr);
void UTILI_usDelay(uint32_t _value);
//uint8_t UTILI_GetMeterModel(CFG_Transmission_t _transmissionType, float _literPerPulse, PULSER_Mode_t _pulseCounterMode, char _band);
uint16_t UTILI_GetChecksum(uint16_t _crc, uint8_t *_buf, uint32_t _len);
uint8_t UTILI_GetCRC8(uint8_t _crc, uint8_t *_in, uint16_t _len);
uint16_t UTILI_daysInYear(uint8_t _day, uint8_t _mon, uint16_t _year);
void UTILI_WaitRTCSync(void);
uint32_t UTILI_100usTimeStamp(void);
uint32_t UTILI_100usTimDiffFromNow(uint32_t t0);
bool UTILI_100usIsTimeout(uint32_t _from, uint32_t _period);
uint32_t UTILI_msTimeStamp(void);
uint32_t UTILI_msTimDiffFromNow(uint32_t t0);
bool UTILI_msIsTimeout(uint32_t _from, uint32_t _period);
uint32_t UTILI_sTimeStamp(void);
uint32_t UTILI_sTimDiffFromNow(uint32_t t0);
bool UTILI_sIsTimeout(uint32_t _from, uint32_t _period);
uint32_t UTILI_dayTimeStamp(void);
uint16_t UTILI_minutesSinceMonthBegin(void);
uint32_t UTILI_secondsSinceMidnight(void);
uint16_t UTILI_Array_Copy(uint8_t *_dest, uint8_t *_source, uint16_t _len);
uint16_t UTILI_Array_CopyString(void *_dest, char *_source);
uint16_t UTILI_Array_CopyUntil(uint8_t *_dest, uint8_t *_source, uint8_t *_end);
uint16_t UTILI_Array_Copy16(uint8_t *_dest, uint16_t _source);
uint16_t UTILI_Array_Copy16_Ptr(uint8_t *_dest, uint8_t *_source);
uint16_t UTILI_Array_Copy32(uint8_t *_dest, uint32_t _source);
uint16_t UTILI_Array_Copy32_Ptr(uint8_t *_dest, uint8_t *_source);
uint16_t UTILI_Array_Copy64(uint8_t *_dest, uint64_t _source);
uint16_t UTILI_Array_Copy64_Ptr(uint8_t *_dest, uint8_t *_source);
#define UTILI_WaitUntil(_compare1, _compare2, _function, _timeout_ms) {uint64_t _ts0= SYS_GetTimestamp_ms(); while(_compare1!= _compare2){void (*_fptr)(void)= _function; if(NULL!=_fptr)_fptr(); if((SYS_GetTimestamp_ms()- _ts0)> _timeout_ms){break;}}}
uint64_t UTILI_Mask_Decode(char *_stringMask);
uint64_t UTILI_Mask_Convert(time_t _time);
time_t UTILI_Mask_GetMatchedTime(time_t _currTime, uint64_t _mask);
time_t UTILI_Mask_GetMatchedTimeFromNow(uint64_t _mask);
time_t UTILI_ComputeNextTime(time_t _currTime, time_t _referenceTime, time_t _interval);
time_t UTILI_GetSmallerTime(time_t _comparedTime, time_t _nextTime);
time_t UTILI_GetSmallerPeriod(time_t _comparedPeriod, time_t _currTime, time_t _nextTime);

#endif /* UTILI_H_ */
