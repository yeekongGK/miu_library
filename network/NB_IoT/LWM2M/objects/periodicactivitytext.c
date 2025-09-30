/*
 * periodicactivitytext.c
 *
 *  Created on: 29 Oct 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#include "common.h"
#include "periodicactivity.h"

const char *const *const PRACT_TEXT[]=
{
    (const char*[]) {
    	"Get Reading\0",
//    	"This activity returns the following:"
//		"1) Meter reading [liter]"
//		"2) Consumption [liter^3]"
//		"3) Temperature [Celsius]"
//		"4) Battery Level [%]."/*no \r\n at the end please*/
    	"\0",
    },
    (const char*[]) {
    	"Get Status\0",
//    	"This activity returns device's status."
//		"1) No of transmission\r\n"
//		"2) No of failed transmission\r\n"
//		"3) No of attachment\r\n"
//		"4) No of disattachment\r\n"
//		"5) No of SIM error\r\n"
//		"6) Latency [ms]\r\n"
//		"7) Avg latency [ms]\r\n"
//		"8) Min latency [ms]\r\n"
//		"9) Max latency [ms]\r\n"
//		"10) Rsrp\r\n"
//		"11) Avg rsrp\r\n"
//		"12) Min rsrp\r\n"
//		"13) Max rsrp\r\n"
//		"14) Rssi\r\n"
//		"15) Avg rssi\r\n"
//		"16) Min rssi\r\n"
//		"17) Max rssi\r\n"
//		"18) Sinr\r\n"
//		"19) Avg sinr\r\n"
//		"20) Min sinr\r\n"
//		"21) Max sinr\r\n"
//		"22) Rsrq\r\n"
//		"23) Avg rsrq\r\n"
//		"24) Min rsrq\r\n"
//		"25) Max rsrq\r\n"
//		"26) Max rsrq\r\n"
//		"27) Ping latency [ms]\r\n"
//		"28) Avg ping latency [ms]\r\n"
//		"29) Min ping latency [ms]\r\n"
//		"30) Max ping latency [ms]\r\n"
//		"31) Transmission power\r\n"
//		"32) CE Mode\r\n"/*no \r\n at the end please*/
		"\0",
    },
    (const char*[]) {
    	"Query\0",
    	"This activity send query to device(via settings resource). "
    	"\0",
    },
};


