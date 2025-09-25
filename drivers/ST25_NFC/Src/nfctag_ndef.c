/*
 * nfctag_ndef.c
 *
 *  Created on: 26 March 2019
 *      Author: muhammad.ahmad@georgekent.net
 */
#include <string.h>
#include <math.h>
#include "main.h"
#include "nfctag.h"

static uint32_t uwNDEFLockTimeoutValue= 0;

static uint16_t usNDEFOffset;

NFCTAG_Status_t NFCTAG_NDEF_Init(void)
{
	ST25DV_MEM_SIZE _memorySize;
	uint32_t _memorySizeInByte;
	uint8_t _data[10];
	NFCTAG_Status_t _status;

	/*refer to nfc forum tag type 5*/

	St25Dv_i2c_ExtDrv.ReadMemSize(&_memorySize );
	_memorySizeInByte = (_memorySize.BlockSize+1) * (_memorySize.Mem_Size+1);

	if(_memorySizeInByte<= (0xFF* 8))/* Size must be specified by block of 8 bytes */
	{
		/* 1) write capability container */
		_data[0] = NFCT5_MAGICNUMBER_E1_CCFILE;
		_data[1] = NFCT5_VERSION_V1_0;
		_data[2] = (uint8_t)(_memorySizeInByte/ 8);
		_data[3] = NDEF_DEMO_CC3_COMPLIANT_VALUE;
		usNDEFOffset= 4;
		/* 2) write TLV with length 0, after CC bytes */
		_data[4] = NFCT5_NDEF_MSG_TLV; /*T*/
		_data[5] = 0x00;/*L*/
		_status= St25Dv_i2c_Drv.WriteData(_data, 0x00, 6);
	}
	else/*size larger than 16kbits*/
	{
		/* 1) write capability container */
		_data[0] = NFCT5_MAGICNUMBER_E2_CCFILE;
		_data[1] = NFCT5_VERSION_V1_0;
		_data[2] = NFCT5_EXTENDED_CCFILE;
		_data[3] = NDEF_DEMO_CC3_COMPLIANT_VALUE;
		_data[5] = 0;
		_data[6] = (uint8_t)((_memorySizeInByte/ 8)>> 8);
		_data[7] = (uint8_t)((_memorySizeInByte/ 8)>> 0);
		usNDEFOffset= 8;
		/* 2) write TLV with length 0, after CC bytes */
		_data[8] = NFCT5_NDEF_MSG_TLV; /*T*/
		_data[9] = 0x00;/*L*/
		_status= St25Dv_i2c_Drv.WriteData(_data, 0x00, 10);
	}
	return _status;
}

NFCTAG_Status_t NFCTAG_NDEF_Clear(void)
{
	uint8_t _data[10];

	_data[0] = 0;
	return St25Dv_i2c_Drv.WriteData(_data, 0x00, 6);
}

//
//NFCTAG_Status_t NFCTAG_NDEF_writeURI(void)
//{
//	uint8_t _tlvBytes[512/*256*/+ 2];
//	uint8_t *_NDEFMsg= _tlvBytes+ 2;
//	uint8_t _NDEFPayloadLen;
//	int32_t _pulseCounter;
//	char 	_meterReadingStr[32];
//	char 	_flowRateStr[32];
//	char 	_literPerPulseStr[32];
//
//	_pulseCounter= 	(int32_t)PULSE_CNTR_Value();
//
//	UTILI_GetFloatString(_pulseCounter* config.pulseCntr.literPerPulse, _meterReadingStr);
//	if(0!= config.pulseCntr.rate)
//	{
//		UTILI_GetFloatString(config.pulseCntr.rate* config.pulseCntr.literPerPulse, _flowRateStr);
//	}
//	else
//	{
//		UTILI_GetFloatString(config.pulseCntr.slowRate* config.pulseCntr.literPerPulse, _flowRateStr);
//	}
//	UTILI_GetFloatString(config.pulseCntr.literPerPulse, _literPerPulseStr);
//
//	_NDEFMsg[4]= URI_ID_0x03;/*http*/
//
//	tfp_sprintf((char*)(_NDEFMsg+ 5),
//			"%s?"
//			"&a=%s"/*config.info.name*/
//			"&b=%s"/*config.info.serialNo,*/
//			"&c=%s"/*config.info.meterSerialNo*/
//			"&d=%u"/*SENSORS_GetStatusCode()*/
//			"&e=%d"/*METER_LOG_GetCurrentIndex()*/
//
//#ifdef NBIOT
//			"&f=%u"/*nbiot.isDisabled*/
//			"&g=%s"/*config.info.imei*/
//			"&h=%s"/*config.info.iccid*/
//			"&i=%d"/*config.nbiot.transmittedLogs*/
//#else
//#ifdef WMBUS
//			"&m=%s"/*config.info.uid*/
//			"&n=%d"/*config.nbiot.transmittedLogs*//*change to wmbus log*/
//#endif
//#endif
//
//			"&j=%d"/*_pulseCounter*/
//			"&o=%s"/*UTILI_GetFloatString(_literPerPulse)*/
//			"&k=%s"/*UTILI_GetFloatString(_meterReading)*/
//			"&l=%s"/*UTILI_GetFloatString(_flowRate)*/
//			"\0",
//
//			config.info.ndefUrl,
//			config.info.name,
//			config.info.serialNo,
//			config.info.meterSerialNo,
//			SENSORS_GetStatusCode(),
//			METER_LOG_GetCurrentIndex(),
//
//#ifdef NBIOT
//			nbiot.enable,
//			config.info.imei,
//			config.info.iccid,
//			config.nbiot.transmittedLogs,
//#else
//#ifdef WMBUS
//			config.info.uid,
//			config.nbiot.transmittedLogs,
//#endif
//#endif
//
//			_pulseCounter,
//			_literPerPulseStr,
//			_meterReadingStr,
//			_flowRateStr
//			);
//
//	_NDEFPayloadLen= 1/*uri id*/+ strlen((char*)(_NDEFMsg+ 5));
//	_NDEFMsg[2+ 256]= '\0';/*truncate long messages. TODO enable more than 256 ndef*/
//
//	_NDEFMsg[0]= 0b11010001;/*MB:1(start of ndef msg) ME:1(last record) CF:0 SR:1(record len max 255) IL:0 TNF:1(well known type) */
//	_NDEFMsg[1]= 1;/*payload type length*/
//	_NDEFMsg[2]= _NDEFPayloadLen;/*payload length*/
//	_NDEFMsg[3]= 'U';/*payload type: URI*/
//
//	_tlvBytes[0]= NFCT5_NDEF_MSG_TLV;
//	_tlvBytes[1]= 4+ _NDEFPayloadLen;/*ndef message length*/
//	_tlvBytes[2+ _tlvBytes[1]]= NFCT5_TERMINATOR_TLV;
//
//	return St25Dv_i2c_Drv.WriteData( (uint8_t*)&_tlvBytes, usNDEFOffset, 2+ _tlvBytes[1]+ 1);
//}
//
//void NFCTAG_NDEF_SetLock(uint16_t _seconds)
//{
//	uwNDEFLockTimeoutValue= RTC_GetSecondsTick()+ _seconds;
//	RTC_RequestSecondsTick(_seconds);
//}
//
//bool NFCTAG_NDEF_IsLocked(void)
//{
//	if(uwNDEFLockTimeoutValue<= RTC_GetSecondsTick())
//	{
//		return false;
//	}
//
//	return true;
//}
