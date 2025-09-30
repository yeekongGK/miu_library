/*
 * bc66_util.c
 *
 *  Created on: 7 Aug 2018
 *      Author: muhammad.ahmad@georgekent.net
 */

#include <bc66util.h>
#include "common.h"

uint32_t BC66UTIL_EncodeLwM2MLifetime(uint32_t _lifetime_s)
{
	if(_lifetime_s<= 30)
	{
		//return
	}
}

void BC66UTIL_EncodeCPSMS(uint8_t pucNbiotBuffer[], RTC_TickType_t _tickType, uint32_t _transmissionTick, uint16_t _activeTime/*seconds*/)
{
	uint8_t //_periodicRAU= 0,/*coded as GPRS Timer 3 value*/
			//_GPRSReady= 0,/*coded as GPRS Timer 3 value*/
			_periodicTAU= 0b00100000,/*coded as GPRS Timer 3 value*/
			_requestedActive;/*coded as GPRS Timer 2 value*/

	if(0== (_activeTime/60))/*less than a minute*/
	{
		_requestedActive= 0b00000000+ _activeTime/2;
	}
	else if(0== (_activeTime/(30* 60)))/*less than 30 minutes*/
	{
		_requestedActive= 0b00100000+ (_activeTime/ 60);
	}
	else if(0!= (_activeTime/60))/*more than 30 minutes*/
	{
		_requestedActive= 0b00100000+ 0b11111;/*set max 30mins*/
	}

	switch(_tickType)
	{
		case SECOND_TickType:
			_periodicTAU= 0b00100000+ 0b1;/*minimum is 1 hour*///_periodicTAU= 0b01100000+ (_transmissionTick/2);
			break;
		case MINUTE_TickType:
			_periodicTAU= 0b00100000+ 0b1;/*minimum is 1 hour*///_periodicTAU= 0b10100000+ _transmissionTick;
			break;
		case HOUR_TickType:
			_periodicTAU= 0b00100000+ _transmissionTick;
			break;
		case DAY_TickType:
			_periodicTAU= 0b00100000+ 24;/*TAU Everyday*/
			break;
		case MONTH_TickType:
			_periodicTAU= 0b00100000+ 24;/*TAU Everyday*/
			break;
	}

	tfp_sprintf((char *)pucNbiotBuffer, "AT+CPSMS=1,,,\""BYTE_TO_BINARY_PATTERN"\",\""BYTE_TO_BINARY_PATTERN"\"\r\n", BYTE_TO_BINARY(_periodicTAU), BYTE_TO_BINARY(_requestedActive));
}

uint8_t BC66UTIL_SecondsToGPRSTimer2(uint32_t _value)
{
	uint8_t _timerValue= 0;

	if(0== (_value/ 60))/*less than a minute*/
	{
		if(1== _value)
		{
			_value= 2;/*guarantee minimum*/
		}
		_timerValue= 0b00000000+ (_value/ 2);/*value is incremented in multiples of 2 seconds*/
	}
	else if(0== (_value/ (30* 60)))/*less than 30 minutes*/
	{
		_timerValue= 0b00100000+ (_value/ 60);/*value is incremented in multiples of 1 minute*/
	}
	else
	{
		_timerValue= 0b01000000+ (_value/ (60* 6));/*value is incremented in multiples of decihours(6 minutes)*/
	}

	return _timerValue;
}

uint32_t BC66UTIL_GPRSTimer2ToSeconds(uint8_t _value)
{
	uint8_t _unit=	(0b11100000&_value)>> 5;
	uint8_t _binCodedValue= (0b00011111&_value);

	switch(_unit)
	{
		case 0b000:/*value is incremented in multiples of 2 seconds*/
			return (_binCodedValue* 2);
		case 0b001:/*value is incremented in multiples of 1 minute*/
			return (_binCodedValue* 60);
		case 0b010:/*value is incremented in multiples of decihours(6 minutes)*/
			return (_binCodedValue* (60* 60* 6));
		case 0b111:/*value indicates that the timer is deactivated*/
			return 0;
		default:/*other value is incremented in multiples of 1 minute*/
			return (_binCodedValue* 60);
	}
	return 0;
}

uint8_t BC66UTIL_SecondsToGPRSTimer3(uint32_t _value)
{
	uint8_t _timerValue= 0;

	if(0== (_value/ 60))/*less than 1 minute*/
	{
		if(0== _value)
		{
			_timerValue= 0b11100000;/*disable*/
		}
		else
		{
			if(1== _value)
			{
				_value= 2;/*guarantee minimum*/
			}
			_timerValue= 0b01100000+ (_value/ 2);/*value is incremented in multiples of 2 seconds*/
		}
	}
	else if(0== (_value/ (60* 15)))/*less than 15 minute*/
	{
		_timerValue= 0b10000000+ (_value/ 30);/*value is incremented in multiples of 30 seconds*/
	}
	else if(0== (_value/ (60* 31)))/*less than 31 minutes*/
	{
		_timerValue= 0b10100000+ (_value/ 60);/*value is incremented in multiples of 1 minute*/
	}
	else if(0== (_value/ (60* 60* 5)))/*less than 5 hour*/
	{
		_timerValue= 0b00000000+ (_value/ (60* 10));/*value is incremented in multiples of 10 minutes*/
	}
	else if(0== (_value/ (60* 60* 31)))/*less than 31 hour*/
	{
		_timerValue= 0b00100000+ (_value/ (60* 60));/*value is incremented in multiples of 1 hour*/
	}
	else
	{
		_timerValue= 0b01000000+ (_value/ (60* 60* 10));/*value is incremented in multiples of 10 hour*/
	}

	return _timerValue;
}

uint32_t BC66UTIL_GPRSTimer3ToSeconds(uint8_t _value)
{
	uint8_t _unit=	(0b11100000&_value)>> 5;
	uint8_t _binCodedValue= (0b00011111&_value);
	switch(_unit)
	{
		case 0b000:/*value is incremented in multiples of 10 minutes*/
			return (_binCodedValue* (60* 10));
		case 0b001:/*value is incremented in multiples of 1 hour*/
			return (_binCodedValue* (60* 60));
		case 0b010:/*value is incremented in multiples of 10 hours*/
			return (_binCodedValue* (60* 60* 10));
		case 0b011:/*value is incremented in multiples of 2 seconds*/
			return (_binCodedValue* 2);
		case 0b100:/*value is incremented in multiples of 30 seconds*/
			return (_binCodedValue* 30);
		case 0b101:/*value is incremented in multiples of 1 minute*/
			return (_binCodedValue* 60);
		case 0b111:/*value indicates that the timer is deactivated*/
			return 0;
		default:/*other value is incremented in multiples of 1 hour*/
			return (_binCodedValue* (60* 60));
	}

	return 0;
}

uint8_t BC66UTIL_MilisecondsToPagingTime(uint32_t _value)
{
	if(_value<= ((0b00001111+ 1)* 2560))
	{
		return (_value/ 2560)- 1;
	}

	return 0;
}

uint32_t BC66UTIL_PagingTimeToMiliseconds(uint8_t _value)
{
	if(_value<= 0b00001111)
	{
		return (_value+ 1)* 2560;
	}

	return 0;
}

uint8_t BC66UTIL_MilisecondsToEDrx(uint32_t _value)
{
	if(_value<= 5120)
	{
		return 0b0000;
	}
	else if(_value<= 10240)
	{
		return 0b0001;
	}
	else if(_value<= 20480)
	{
		return 0b0010;
	}
	else if(_value<= 40960)
	{
		return 0b0011;
	}
	else if(_value<= 61440)
	{
		return 0b0100;
	}
	else if(_value<= 81920)
	{
		return 0b0101;
	}
	else if(_value<= 102400)
	{
		return 0b0110;
	}
	else if(_value<= 122880)
	{
		return 0b0111;
	}
	else if(_value<= 143360)
	{
		return 0b1000;
	}
	else if(_value<= 163840)
	{
		return 0b1001;
	}
	else if(_value<= 327680)
	{
		return 0b1010;
	}
	else if(_value<= 655360)
	{
		return 0b1011;
	}
	else if(_value<= 1310720)
	{
		return 0b1100;
	}
	else if(_value<= 2621440)
	{
		return 0b1101;
	}
	else if(_value<= 5242880)
	{
		return 0b1110;
	}
	else if(_value<= 10485760)
	{
		return 0b1111;
	}

	return 0;
}

uint32_t BC66UTIL_EDrxToMiliseconds(uint8_t _value)
{
	switch(_value)
	{
		case 0b0000:
			return 5120;
		case 0b0001:
			return 10240;
		case 0b0010:
			return 20480;
		case 0b0011:
			return 40960;
		case 0b0100:
			return 61440;
		case 0b0101:
			return 81920;
		case 0b0110:
			return 102400;
		case 0b0111:
			return 122880;
		case 0b1000:
			return 143360;
		case 0b1001:
			return 163840;
		case 0b1010:
			return 327680;
		case 0b1011:
			return 655360;
		case 0b1100:
			return 1310720;
		case 0b1101:
			return 2621440;
		case 0b1110:
			return 5242880;
		case 0b1111:
			return 10485760;
	}

	return 0;
}
/**
 * AT+CPSMS=1,,,00100001(TAU:T3412 - octet 3 gprs timer 3),00100010(activeTime:T3324 - octet 3 gprs timer 2)
 * sara default TAU= 01100000 - 54 minutes
 * sara default activeTime= 00000000 - 60 seconds
 *
 * AT+NPTWEDRXS=1,5,0111(pagingTime),0101(eDrx)
 * sara default pagingTime= 0111 - 2048 miliseconds
 * sara default eDRX= 0101 - 8192 miliseconds
 */
/*
 *
GPRS Timer & Timer 2 value (octet 2)
Bits 5 to 1 represent the binary coded timer value.
Bits 6 to 8 defines the timer value unit for the GPRS timer as follows:
Bits
8 7 6
0 0 0 value is incremented in multiples of 2 seconds
0 0 1 value is incremented in multiples of 1 minute
0 1 0 value is incremented in multiples of decihours
1 1 1 value indicates that the timer is deactivated.
Other values shall be interpreted as multiples of 1 minute in this version of the
protocol.
 *
GPRS Timer 3 value (octet 3)
Bits 5 to 1 represent the binary coded timer value.
Bits 6 to 8 defines the timer value unit for the GPRS timer as follows:
Bits
8 7 6
0 0 0 value is incremented in multiples of 10 minutes
0 0 1 value is incremented in multiples of 1 hour
0 1 0 value is incremented in multiples of 10 hours
0 1 1 value is incremented in multiples of 2 seconds
1 0 0 value is incremented in multiples of 30 seconds
1 0 1 value is incremented in multiples of 1 minute
1 1 1 value indicates that the timer is deactivated.
Other values shall be interpreted as multiples of 1 hour in this version of the
protocol.
 */
/*NB-S1 mode
The field contains the PTW value in seconds for NB-S1 mode.The PTW value is used
as specified in 3GPP TS 23.682 [133a].The PTW value is derived as follows:
bit
8 7 6 5 Paging Time Window length
0 0 0 0 2,56 seconds
0 0 0 1 5,12 seconds
0 0 1 0 7,68 seconds
0 0 1 1 10,24 seconds
0 1 0 0 12,8 seconds
0 1 0 1 15,36 seconds
0 1 1 0 17,92 seconds
0 1 1 1 20,48 seconds
1 0 0 0 23,04 seconds
1 0 0 1 25,6 seconds
1 0 1 0 28,16 seconds
1 0 1 1 30,72 seconds
1 1 0 0 33,28 seconds
1 1 0 1 35,84 seconds
1 1 1 0 38,4 seconds
1 1 1 1 40,96 seconds
*/
/*
 * S1 mode
The field contains the eDRX value for S1 mode. The E-UTRAN eDRX cycle length
duration value and the eDRX cycle parameter 'TeDRX' as defined in
3GPP TS 36.304 [121] are derived from the eDRX value as follows:
bit
4 3 2 1 E-UTRAN eDRX cycle length
duration
eDRX cycle parameter 'TeDRX'
0 0 0 0 5,12 seconds (NOTE 5) NOTE 3
0 0 0 1 10,24 seconds (NOTE 5) 20
0 0 1 0 20,48 seconds 21
0 0 1 1 40,96 seconds 22
0 1 0 0 61,44 seconds (NOTE 5) 6
0 1 0 1 81,92 seconds 23
0 1 1 0 102,4 seconds (NOTE 5) 10
0 1 1 1 122,88 seconds (NOTE 5) 12
1 0 0 0 143,36 seconds (NOTE 5) 14
1 0 0 1 163,84 seconds 24
1 0 1 0 327,68 seconds 25
1 0 1 1 655,36 seconds 26
1 1 0 0 1310,72 seconds 27
1 1 0 1 2621,44 seconds 28
1 1 1 0 5242,88 seconds (NOTE 4) 29
1 1 1 1 10485,76 seconds (NOTE 4) 210
All other values shall be interpreted as 0000 by this version of the protocol.
NOTE 3: For E-UTRAN eDRX cycle length duration of 5,12 seconds the eDRX cycle
parameter 'TeDRX' is not used as a different algorithm compared to the other
values is applied. See 3GPP TS 36.304 [121] for details.
NOTE 4: The value is applicable only in NB-S1 mode. If received in WB-S1 mode it is
interpreted as 1101 by this version of the protocol.
NOTE 5: The value is applicable only in WB-S1 mode. If receive
*/
