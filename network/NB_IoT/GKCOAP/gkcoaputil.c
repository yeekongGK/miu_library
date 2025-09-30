/*
 * gkcoaputil.c
 *
 *  Created on: 18 Jun 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#include "common.h"
#include "gkcoaputil.h"
#include "gkcoappacket.h"
#include "devicelog.h"
#include "pulser.h"
#include <time.h>

void GKCOAPUTIL_DeviceToGKCoapLogs(uint8_t *_in, uint8_t *_out, uint16_t _count)
{
	for(uint16_t i= 0; i< _count; i++)
	{
		DEVICELOG_Log_t _deviceLog;
		GKCOAP_Log_t	_coapLog;
	    time_t _time;
	    struct tm _timeinfo;
	    uint32_t _uint32;

	    /*copy to struct, we have to do this method because of compiler optimization will give hardfault if direct cast*/
		memcpy(&_deviceLog, _in+ (i* sizeof(DEVICELOG_Log_t)), sizeof(DEVICELOG_Log_t));

	    _time= _deviceLog.timestamp;
	    _time-= 86400;/*TODO: extremely important, UTILI_sTimeStamp and localtime diff by -1 day!!!. Please fix this*/
	    _timeinfo= *localtime(&_time);
	    _coapLog.hours= UTILI_BinToBCD(_timeinfo.tm_hour);
	    _coapLog.minutes= UTILI_BinToBCD(_timeinfo.tm_min);
	    _coapLog.seconds= UTILI_BinToBCD(_timeinfo.tm_sec);
	    _coapLog.date= UTILI_BinToBCD(_timeinfo.tm_mday);
		_coapLog.month= UTILI_BinToBCD(_timeinfo.tm_mon+ 1);
		_coapLog.year= UTILI_BinToBCD(_timeinfo.tm_year- 100);
		_uint32= PULSER_ConvertToPulseValue(_deviceLog.meterReading);
		memcpy(&(_coapLog.meterPulse), &_uint32, 4);
		_coapLog.statusCode= _deviceLog.statusByte;
		_coapLog.temperature= _deviceLog.temperature;
		_uint32= _deviceLog.voltage;
		memcpy(&(_coapLog.adcVoltage), &_uint32, 4);

		memcpy(_out+ (i* sizeof(GKCOAP_Log_t)), &_coapLog, sizeof(GKCOAP_Log_t));
	}
}
