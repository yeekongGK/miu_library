/*
 * datamonitoringtext.c
 *
 *  Created on: 2 Nov 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#include "common.h"
#include "datamonitoring.h"

const char *const *const DTMON_TEXT[]=
{
	(const char*[]) {
		"Alarm\0",
//		"Monitor device's alarms."
//		"Data is device's alarm bitmap."
//		" as following:"
//		"bit0: Magnet tamper. "
//		"bit1: Tilt. "
//		"bit2: Wire cut. "
//		"bit3: Faulty sensor. "
//		"bit4: Battery low. "
//		"bit5: Back flow. "
//		"bit6: Burst. "
//		"bit7: No flow. "
//		"bit8: Leakage."/*no \r\n at the end please*/
		"\0",
	},
//	(const char*[]) {
//		"Leakage\0",
//		"Monitor leakage using water flow threshold. "
//		"Data is water flow (m3/hr)."/*no \r\n at the end please*/
//		"\0",
//	},
//    (const char*[]) {
//    	"Reverse Flow\0",
//    	"Monitor reverse water flow. "
//    	"Data is reverse water flow (m3/hr)."/*no \r\n at the end please*/
//    	"\0",
//    },
};



