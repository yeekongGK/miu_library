/*
 * gkcoappacket.c
 *
 *  Created on: 14 Jun 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#include "common.h"
#include "gkcoappacket.h"
#include "gkcoap.h"
#include "loggerutil.h"
#include "msg.h"
#include "rtc.h"
#include "msg.h"
#include "security.h"
#include "pulser.h"
#include "sensor.h"

static uint8_t pucMsgBuffer[GKCOAPPKT_CFG_MSG_BUFFER_SIZE];

const char pcRegPayload[] = "35353535353535353032393030343932040000007FD891836FFAA7C0DD2AC36117E05A9D01E1AE76071E196BCE1D33"
						"95B390AA55E82A5A8D01B3995EF9F343655FF5E7B300F625AB87465C2FF6ACA42C7DE7EA2E05DFD4430"
						"A44137BA2FBA899310A723E51403D79F131CBC6BED2E67BD9C2FCD04953718CBCA2939F990BAC513779E461C6F556"
						"A23233671EDC3290A5B02C85490A6C52A2D13040ECCABA88FB97D53ADA5BA24BA9BE11F64C25F99792C2C71E5E96A"
						"8B845B6F82A15D4B02DDA43B1A26F0DFE9BBEE9DF2D320750439957D758651AE98601301ECC361BBCA29AE3B298C1CECA73653413F2646616D3E204D5C7";

uint16_t GKCOAPPKT_PopulateRegisterPacketSample(uint8_t *_payloadBuf)
{
	uint32_t _payloadLen= strlen(pcRegPayload)/ 2;
	UTILI_HexStringToBytes(_payloadBuf, pcRegPayload, _payloadLen);

	return _payloadLen;
}

const char pcRepPayload[] = //header"33353838373830383032313839363632"
		"1f040000160621e3b12500001ae10deb0d17050000831519e3b"
		"12500060000831519e3b12500070000831519e3b1250008000083151ae3b"
		"1250009000083151ae4b1250010000083151b4cb4250011000083151ccab4250012000083151c03b"
		"5250013000083151dbfb5250014000083151e10b625001500008315243eb82500160000831521a0b"
		"82500170000831520feb9250018000083151e2ebb250019000083151d4cbc250020000083151c4cb"
		"c250021000083151b4cbc250022000083151a4cbc250023000083151a4cbc25000000008b151a4cb"
		"c25000100008b151a4cbc25000200008b151a4cbc25000300008b151a4cbc2500010307331706210"
		"101ce740000380cfe5afe06ffe4e60500aba24e0079ea3603001601750e110194ff1100eb3500005"
		"3038900260013000000000000000000002001";
		//footer: "0daa5d";

uint16_t GKCOAPPKT_PopulateReportingPacketSample(uint8_t *_payloadBuf)
{
	uint32_t _payloadLen= strlen(pcRepPayload)/ 2;
	UTILI_HexStringToBytes(_payloadBuf, pcRepPayload, _payloadLen);
	_payloadBuf[0]= sBC66Link.status.rssi;
	return _payloadLen;
}

uint16_t GKCOAPPKT_PopulateRegisterPacket(uint8_t *_payloadBuf)
{
	uint32_t _payloadLen= 0;
	uint8_t _packetVersion= (0xF0& config.nbiot.gkcoap.packetType);

	memcpy(_payloadBuf+ _payloadLen, config.system.hwVersion, 5);_payloadLen+= 5;
	memcpy(_payloadBuf+ _payloadLen, (uint8_t *)&(config.system.fwVersion[1]), 5);/*remove v infront*/_payloadLen+= 5;

	uint32_t _uint32;
	_uint32= config.nbiot.gkcoap.reportInterval_s;
	memcpy(_payloadBuf+ _payloadLen, &_uint32, 4);_payloadLen+= 4;

	_uint32= LOGGERUTIL_TickInSeconds(config.nbiot.gkcoap.logTickType, config.nbiot.gkcoap.logTickSize);
	memcpy(_payloadBuf+ _payloadLen, &_uint32, 4);_payloadLen+= 4;

	memcpy(_payloadBuf+ _payloadLen, &config.system.longitude, 4);_payloadLen+= 4;
	memcpy(_payloadBuf+ _payloadLen, &config.system.latitude, 4);_payloadLen+= 4;

	_payloadBuf[_payloadLen++]= config.system.meterModel;//= UTILI_GetMeterModel(config.system.transmissionType, config.pulser.weight_liter, config.pulser.mode, config.system.serialNo[5]);

	_uint32= 0;
	memcpy(_payloadBuf+ _payloadLen, &_uint32, 4);_payloadLen+= 4;
	memcpy(_payloadBuf+ _payloadLen, &_uint32, 4);_payloadLen+= 4;

	memcpy(_payloadBuf+ _payloadLen, &config.nbiot.imsi, 16);_payloadLen+= 16;
	memcpy(_payloadBuf+ _payloadLen, &config.nbiot.iccid, 20);_payloadLen+= 20;

	_payloadBuf[_payloadLen++]= config.sensors.enableMask;
	float _battLowThreshold= 2.8;
	memcpy(_payloadBuf+ _payloadLen, &_battLowThreshold, 4);_payloadLen+= 4;

	float _float;/*adjust from l/s to l/h*/
	_float= config.sensors.flow.burstThreshold* 3600.0;
	memcpy(_payloadBuf+ _payloadLen, (uint8_t *)&(_float), 4);_payloadLen+= 4;/*adjust from l/hr to l/s*/
	memcpy(_payloadBuf+ _payloadLen, (uint8_t *)&(config.sensors.flow.burstSamplingPeriod_s), 4);_payloadLen+= 4;
	memcpy(_payloadBuf+ _payloadLen, (uint8_t *)&(config.sensors.flow.noflowSamplingPeriod_s), 4);_payloadLen+= 4;
	_float= config.sensors.flow.leakageThreshold* 3600.0;
	memcpy(_payloadBuf+ _payloadLen, (uint8_t *)&(_float), 4);_payloadLen+= 4;/*adjust from l/hr to l/s*/
	memcpy(_payloadBuf+ _payloadLen, (uint8_t *)&(config.sensors.flow.noflowSamplingPeriod_s), 4);_payloadLen+= 4;
	memcpy(_payloadBuf+ _payloadLen, (uint8_t *)&(config.sensors.flow.backflowThreshold), 4);_payloadLen+= 4;

	memcpy(_payloadBuf+ _payloadLen, &config.nbiot.gkcoap.serverIP, 16);_payloadLen+= 16;
	memcpy(_payloadBuf+ _payloadLen, &config.nbiot.gkcoap.serverPort, 5);_payloadLen+= 5;
	memcpy(_payloadBuf+ _payloadLen, &config.nbiot.gkcoap.serverIP, 16);_payloadLen+= 16;
	memcpy(_payloadBuf+ _payloadLen, &config.nbiot.gkcoap.serverPort, 5);_payloadLen+= 5;
	_payloadBuf[_payloadLen++]= 1;//config.nbiot.gkcoap.activeServer;

	memcpy(_payloadBuf+ _payloadLen, (uint8_t *)&(config.nbiot.apn), 33);_payloadLen+= 33;
	memcpy(_payloadBuf+ _payloadLen, (uint8_t *)&(config.nbiot.periodicTau), 4);_payloadLen+= 4;
	memcpy(_payloadBuf+ _payloadLen, (uint8_t *)&(config.nbiot.activeTime), 4);_payloadLen+= 4;
	memcpy(_payloadBuf+ _payloadLen, (uint8_t *)&(config.nbiot.pagingTime), 4);_payloadLen+= 4;
	memcpy(_payloadBuf+ _payloadLen, (uint8_t *)&(config.nbiot.edrx), 4);_payloadLen+= 4;

	memcpy(_payloadBuf+ _payloadLen, (uint8_t *)&(config.nbiot.rtePeriodicTau), 4);_payloadLen+= 4;
	memcpy(_payloadBuf+ _payloadLen, (uint8_t *)&(config.nbiot.rteActiveTime), 4);_payloadLen+= 4;
	memcpy(_payloadBuf+ _payloadLen, (uint8_t *)&(config.nbiot.rtePagingTime), 4);_payloadLen+= 4;
	memcpy(_payloadBuf+ _payloadLen, (uint8_t *)&(config.nbiot.rteEdrx), 4);_payloadLen+= 4;

	memcpy(_payloadBuf+ _payloadLen, (uint8_t *)&(config.nbiot.restartDelay_s), 4);_payloadLen+= 4;

	_payloadBuf[_payloadLen++]= config.failsafe.PVDEnable;
	_payloadBuf[_payloadLen++]= config.failsafe.PVDLevel;
	_payloadBuf[_payloadLen++]= 0x01;//config.system.BOREnabled;
	_payloadBuf[_payloadLen++]= config.failsafe.BORLevel;


	if((0x20== _packetVersion)|| (0x30== _packetVersion)|| (0x40== _packetVersion))
	{
		_payloadBuf[_payloadLen++]= config.system.utc;
	}
	_payloadBuf[_payloadLen++]= 0x00;/*reg payload type: 0x00*/

	return _payloadLen;
}

static uint32_t NBIOT_PACKET_PopulateRadioParam(uint8_t _packetVersion, uint8_t *_buffer)
{
	//memset(_buffer, 0xFF, 40);return;
	UTILI_WaitRTCSync();/*wait for sync*/
	_buffer[0]= 	LL_RTC_TIME_GetHour(RTC);/* 5 bit */
	_buffer[1]= 	LL_RTC_TIME_GetMinute(RTC);/* 6 bit */
	_buffer[2]= 	LL_RTC_TIME_GetSecond(RTC);/* 6 bit */
	_buffer[3]= 	LL_RTC_DATE_GetDay(RTC);/* 5 bit */
	_buffer[4]= 	LL_RTC_DATE_GetMonth(RTC);/* 4 bit */
	_buffer[5]= 	LL_RTC_DATE_GetYear(RTC);/* 7 bit (0-99years) */

	_buffer[6]= (uint8_t)SENSOR_GetValue(CELLCAPACITY_PERCENTAGE_Sensor);/*replaced with battery level, jic needed in the future*///(sBC66Link.status.inPSM== true)? 1: 0;

	_buffer[7]= sBC66Link.status.cereg.regStatus;
	memcpy(_buffer+ 8, (uint8_t*)&(sBC66Link.status.cereg.trackingAreaCode), 2);
	//_log->regRejectType= sBC66Link.status.cereg.rejectType;
	_buffer[10]= sBC66Link.status.cereg.rejectCause;
	_buffer[11]= sBC66Link.status.cereg.activeTime_raw;
	_buffer[12]= sBC66Link.status.cereg.periodicTau_raw;

	uint16_t _uint16= sBC66Link.urc.QENG.rsrp* 10;
	memcpy(_buffer+ 13, (uint8_t*)&(_uint16/*nbiot.status.radio.signalPower*/), 2);
	memcpy(_buffer+ 15, (uint8_t*)&(_uint16/*nbiot.status.radio.totalPower*/), 2);
	memcpy(_buffer+ 17, (uint8_t*)&(sBC66Link.urc.QENG.txPwr), 2);
	memcpy(_buffer+ 19, (uint8_t*)&(sBC66Link.urc.QENG.txTime), 4);
	memcpy(_buffer+ 23, (uint8_t*)&(sBC66Link.urc.QENG.rxTime), 4);
	memcpy(_buffer+ 27, (uint8_t*)&(sBC66Link.urc.QENG.cellID), 4);
	_buffer[31]= sBC66Link.urc.QENG.ecl;
	memcpy(_buffer+ 32, (uint8_t*)&(sBC66Link.urc.QENG.sinr), 2);
	memcpy(_buffer+ 34, (uint8_t*)&(sBC66Link.urc.QENG.earfcn), 2);
	memcpy(_buffer+ 36, (uint8_t*)&(sBC66Link.urc.QENG.pci), 2);
	memcpy(_buffer+ 38, (uint8_t*)&(sBC66Link.urc.QENG.rsrq), 2);

	return 40;
}

static uint32_t NBIOT_PACKET_PopulateDiagnosticParam(uint8_t _packetVersion, uint8_t *_buffer)
{
	uint16_t _uint16;

	_buffer[0]= (uint8_t) (config.diagnostic.rteNbModemSelfResetCount> 0xFF)? 0xFF: config.diagnostic.rteNbModemSelfResetCount;
	_buffer[1]= (uint8_t) (config.diagnostic.rteHardfaultRebootCount> 0xFF)? 0xFF: config.diagnostic.rteHardfaultRebootCount;
	memcpy(_buffer+ 2, (uint8_t*)&(config.log.device.rte.logCount), 4);
	_uint16= (uint16_t)config.nbiot.stats.noOfTransmission;memcpy(_buffer+ 6,  (uint8_t*)&_uint16, 2);
	_uint16= (uint16_t)config.nbiot.stats.noOfFailedTransmission;	memcpy(_buffer+ 8, (uint8_t*)&_uint16, 2);
	_uint16= (uint16_t)config.nbiot.stats.noOfAttach;		memcpy(_buffer+ 10, (uint8_t*)&_uint16, 2);
	_uint16= (uint16_t)config.nbiot.stats.noOfDisattach;	memcpy(_buffer+ 12, (uint8_t*)&_uint16, 2);
	_buffer[14]= (uint8_t) (config.diagnostic.rteVRefDippedCount> 0xFF)? 0xFF: config.diagnostic.rteVRefDippedCount;
	_buffer[15]= (uint8_t) (config.diagnostic.rteFailsafeRebootCount> 0xFF)? 0xFF: config.diagnostic.rteFailsafeRebootCount;
	_buffer[16]= (uint8_t) (config.diagnostic.rtePVDRebootCount> 0xFF)? 0xFF: config.diagnostic.rtePVDRebootCount;
	_buffer[17]= (uint8_t) (config.diagnostic.rteBORCount> 0xFF)? 0xFF: config.diagnostic.rteBORCount;
	memcpy(_buffer+ 18, (uint8_t*)&(config.pulser.tracsens.rteErrorPatternCount), 4);
	_buffer[22]= (uint8_t) (config.nbiot.stats.noOfSimError> 0xFF)? 0xFF: config.nbiot.stats.noOfSimError;

	return 23;
}

uint32_t GKCOAPPKT_BuildReportingPacket(uint8_t _packetType, uint8_t _rssi, uint32_t *_numberOfLogs, GKCOAP_Log_t *_logs, uint8_t *_payloadBuf)
{
	uint32_t _logsToTransmit= 0;
	uint32_t _payloadLen= 0;
	uint32_t _pulseCount;
	float _meterReading;
	uint8_t _packetVersion= (0xF0& _packetType);

	_payloadBuf[_payloadLen++]= _rssi;

	GKCOAP_Log_t *_logPtr= _logs;
	if(0x20== _packetVersion)
	{
		/*change pulse count to liter*/
		memcpy(&_pulseCount, &(_logPtr->meterPulse), 4);
		_meterReading = PULSER_ConvertToMeterReading(_pulseCount);
		memcpy(&(_logPtr->meterPulse), &_meterReading, 4);
	}

	if((0x20== _packetVersion)|| (0x30== _packetVersion)|| (0x40== _packetVersion))
	{
		/*convert adcVoltage to Min and Average Voltage*/
		uint16_t _vBattAve= SENSOR_GetValue(VCELLAVG_Sensor);
		uint16_t _vBattMin= SENSOR_GetValue(VCELLAVG_Sensor);/*TODO: Create and assign VCELL min*/
		_logPtr->adcVoltage[3]= _vBattAve>> 8;
		_logPtr->adcVoltage[2]= _vBattAve;
		_logPtr->adcVoltage[1]= _vBattMin>> 8;
		_logPtr->adcVoltage[0]= _vBattMin;
	}

	memcpy(_payloadBuf+ _payloadLen, (uint8_t*)_logPtr, sizeof(GKCOAP_Log_t)/*16*/);/*copy straight 1st log data*/
	_payloadLen+= sizeof(GKCOAP_Log_t);
	_logsToTransmit++;

	/*Now check if the status code is different, cos we only populate additional logs with same status code*/
	uint8_t _prevStatusCode=  _logPtr->statusCode;
	for(int i= 1; i<*_numberOfLogs; i++)
	{
		_logPtr++;
		if(_prevStatusCode== _logPtr->statusCode)
		{
			_logsToTransmit++;
			_prevStatusCode=  _logPtr->statusCode;
		}
		else
		{
			break;
		}
	}
	*_numberOfLogs= _logsToTransmit;/*adjust*/
	_payloadBuf[_payloadLen++]= _logsToTransmit- 1;/*'has more' byte*/
	/*Now copy the rest log data if status code is the same*/
	_logPtr= _logs;/*revert back*/
	for(int i= 1; i<_logsToTransmit; i++)
	{
		_logPtr++;
		if(0x20== _packetVersion)
		{
			/*change pulse count to liter*/
			memcpy(&_pulseCount, &(_logPtr->meterPulse), 4);
			_meterReading = PULSER_ConvertToMeterReading(_pulseCount);
			memcpy(&(_logPtr->meterPulse), &_meterReading, 4);
		}
		if((0x10== _packetVersion)|| (0x20== _packetVersion)|| (0x30== _packetVersion)|| (0x40== _packetVersion))/*this version we compress to include temperature data*/
		{

			/*we need to do compression to include temperature data as well*/
			//uint8_t hours;/*5bit*/
			//uint8_t minutes;/*6bit*/
			//uint8_t seconds;/*6bit*/
			//uint8_t date;/*5bit*/
			//uint8_t month;/*4bit*/
			//uint8_t year;/*7bit*/
            /*date(5bit) month(4bit) year(7bit) temperature(8bit)*/
			uint32_t _dateTempCompression= 0;
			_dateTempCompression|= (uint32_t)(UTILI_BCDToBin(_logPtr->date))<< 19;
			_dateTempCompression|= (uint32_t)(UTILI_BCDToBin(_logPtr->month))<< 15;
			_dateTempCompression|= (uint32_t)(UTILI_BCDToBin(_logPtr->year))<< 8;
			_dateTempCompression|= (uint32_t)(_logPtr->temperature)<< 0;
			/*10 bytes for each additional log(we only take time and reading field of each additional logs)*/
			memcpy(_payloadBuf+ _payloadLen, (uint8_t*)_logPtr, 3);
			_payloadLen+= 3;
			_payloadBuf[_payloadLen++]= (uint8_t)(_dateTempCompression>> 16);
			_payloadBuf[_payloadLen++]= (uint8_t)(_dateTempCompression>> 8);
			_payloadBuf[_payloadLen++]= (uint8_t)(_dateTempCompression>> 0);
			memcpy(_payloadBuf+ _payloadLen, ((uint8_t*)_logPtr)+ 6, 4);
			_payloadLen+= 4;
		}
		else
		{
			/*10 bytes for each additional log(we only take time and reading field of each additional logs)*/
			memcpy(_payloadBuf+ _payloadLen, (uint8_t*)_logPtr, 10);
			_payloadLen+= 10;
		}
	}

	if((0x10== _packetVersion)|| (0x20== _packetVersion)|| (0x30== _packetVersion))/*this version we compress to include temperature data*/
	{
		/*this to include radio/diag parameters*/
		_payloadBuf[_payloadLen++]= 1;/*'has more 2' byte*/
		_payloadLen+= NBIOT_PACKET_PopulateRadioParam(_packetVersion, _payloadBuf+ _payloadLen);
		_payloadLen+= NBIOT_PACKET_PopulateDiagnosticParam(_packetVersion, _payloadBuf+ _payloadLen);
	}
	else
	{
		/*this to NOT include radio/diag parameters*/
		_payloadBuf[_payloadLen++]= 0;/*'has more 2' byte*/
	}
	/*end: this to include radio/diag parameters*/

	if(0!= (_payloadLen% 2))
	{
		_payloadBuf[_payloadLen++]= 0;/*pad*/
	}


	if((0x20== _packetVersion)|| (0x30== _packetVersion)|| (0x40== _packetVersion))
	{
		_payloadBuf[_payloadLen++]= config.system.utc;
	}
	_payloadBuf[_payloadLen++]= 0x01;/*datachange payload type: 0x01*/

	return _payloadLen;
}

uint16_t GKCOAPPKT_PopulateFotaPacket(uint8_t _type, uint32_t _currPacket, char * _version, uint8_t *_payloadBuf)
{
	uint16_t _payloadLen= 0;

	switch(_type)
	{
		case 0:/*get info*/
			_payloadBuf[_payloadLen++]= _type;/*request type: get info*/
			_payloadBuf[_payloadLen++]= (uint8_t)((config.flash.partition== CFG_PARTITION_1)? CFG_PARTITION_2: CFG_PARTITION_1);
			break;

		case 1:/*get packet*/
			_payloadBuf[_payloadLen++]= _type;/*request type: get data*/
			_payloadBuf[_payloadLen++]= (uint8_t)((config.flash.partition== CFG_PARTITION_1)? CFG_PARTITION_2: CFG_PARTITION_1);
			memcpy(_payloadBuf+ _payloadLen, _version, strlen(_version));
			_payloadLen+= strlen(_version);
			_payloadBuf[_payloadLen++]= '\0';/*string termination*/
			memcpy(_payloadBuf+ _payloadLen, &_currPacket, 4);
			_payloadLen+= 4;
			break;
	}

	return _payloadLen;
}

uint16_t GKCOAPPKT_PopulateMsgPacket(uint8_t *_payloadBuf, uint8_t *_msg, uint16_t _msgLen)
{
	uint16_t _payloadLen= 0;
	uint8_t _packetVersion= (0xF0& config.nbiot.gkcoap.packetType);

	memcpy(_payloadBuf+ _payloadLen, _msg, _msgLen);_payloadLen+= _msgLen;

	if((0x20== _packetVersion)|| (0x30== _packetVersion)|| (0x40== _packetVersion))
	{
		_payloadBuf[_payloadLen++]= config.system.utc;
	}
	_payloadBuf[_payloadLen++]= 0x02;/*msg payload type: 0x02*/

	return _payloadLen;
}

ErrorStatus GKCOAPPKT_ProcessDownlinkPacket(uint8_t *_rxBuffer, uint32_t _rxLen)
{
	/*00hhmmssddMMyyTTC1..C2..CRC1CRC2
	 *  00: status
	 *  hhmmssddMMyy: time
	 *  TT: total dl command
	 *  C1..C2: downlink command
	 *  CRC1CRC2: checksum*/

	/*sometimes we got bug from server,
	 * we check if the time is valid to validate*/
	uint8_t _day=  UTILI_BCDToBin(_rxBuffer[3]), _month=  UTILI_BCDToBin(_rxBuffer[4]), _year=  UTILI_BCDToBin(_rxBuffer[5]),
			_hours=  UTILI_BCDToBin(_rxBuffer[0]), _minutes=  UTILI_BCDToBin(_rxBuffer[1]), _seconds=  UTILI_BCDToBin(_rxBuffer[2]);
	if((_day< 1)|| (_day> 31)|| (_month< 1)|| (_month> 12)|| (_year> 99)||
		(_hours> 23)|| (_minutes> 59)|| (_seconds> 59))
	{
		return ERROR;
	}

	/*adjust time if necessary*/
	LL_RTC_TimeTypeDef RTC_TimeStruct;
	LL_RTC_DateTypeDef RTC_DateStruct;
	RTC_DateStruct.WeekDay= LL_RTC_DATE_GetWeekDay(RTC);
	RTC_DateStruct.Day= 	_rxBuffer[3];
	RTC_DateStruct.Month= 	_rxBuffer[4];
	RTC_DateStruct.Year= 	_rxBuffer[5];
	RTC_TimeStruct.Hours= 	_rxBuffer[0];
	RTC_TimeStruct.Minutes= _rxBuffer[1];
	RTC_TimeStruct.Seconds= _rxBuffer[2];

	uint32_t _localSeconds= (UTILI_BCDToBin(LL_RTC_TIME_GetHour(RTC))*3600)+ (UTILI_BCDToBin(LL_RTC_TIME_GetMinute(RTC))*60)+ UTILI_BCDToBin(LL_RTC_TIME_GetSecond(RTC));
	uint32_t _serverSeconds= (UTILI_BCDToBin(RTC_TimeStruct.Hours)*3600)+ (UTILI_BCDToBin(RTC_TimeStruct.Minutes)*60)+ UTILI_BCDToBin(RTC_TimeStruct.Seconds);
	/*due diligence check, cos there is transmission delay kot*/
	if(
			(RTC_DateStruct.Day!= LL_RTC_DATE_GetDay(RTC))
			||(RTC_DateStruct.Month!= LL_RTC_DATE_GetMonth(RTC))
			||(RTC_DateStruct.Year!= LL_RTC_DATE_GetYear(RTC))
			||((_localSeconds< _serverSeconds)&& ((_serverSeconds- _localSeconds)> 20))/*only changed when there is >20 sec difference*/
			||((_serverSeconds< _localSeconds)&& ((_localSeconds- _serverSeconds)> 20))/*only changed when there is >20 sec difference*/
	)
	{
		SYS_DateTime_Update(RTC_DateStruct.WeekDay, RTC_DateStruct.Day, RTC_DateStruct.Month, RTC_DateStruct.Year,
				RTC_TimeStruct.Hours, RTC_TimeStruct.Minutes, RTC_TimeStruct.Seconds,
				config.system.utc);
	}

	/*check if there is downlink command(s)*/
	if(8<= _rxLen)
	{
		uint8_t _totalCmd= _rxBuffer[6];
		uint16_t 	 _tlvsIndex= 0;
		TLV_Typedef  _tlv;
		for(int i= 0; i< _totalCmd; i++)
		{
			_tlv.tag= 		((_rxBuffer+ 7)+ _tlvsIndex)[0];
			_tlv.len= 		((_rxBuffer+ 7)+ _tlvsIndex)[1];
			_tlv.value=		((_rxBuffer+ 7)+ _tlvsIndex)+ 2;

			switch(_tlv.tag)
			{
				case 0xFF:/*direct to MSG task*/
					if((0!= MSG_Msg_Depth())/*&& (NBIOT_GKCOAP_PKT_TaskId== MSG_Msg_TaskId_Peek())*/)
						/*MSG_Msg_TaskId_Peek() will returned the tail, doesnt mean the task id is not in the queue.
						 * Thus to play safe, we don't queue when there is unconsumed message regardless from which task.*/
					{
						/*limit one for now*/
					}
					else
					{
						memcpy(pucMsgBuffer, _tlv.value, _tlv.len);
						MSG_t _msg= {0};
						_msg.taskId= MSG_TaskId;
						_msg.buffer= pucMsgBuffer;
						_msg.bufferLen= _tlv.len;
						_msg.sendResponse= true;
						_msg.responseTaskId= NBIOT_GKCOAP_PKT_TaskId;
						MSG_Msg_Enqueue(_msg);
					}
					break;
				case 0x01:/*Reset Meter*/
					if(0== _tlv.len)
					{
						SYS_Request(SOFT_REBOOT_SysRequest);
					}
					break;
				case 0x02:/*Update Firmware*/
					if(0== _tlv.len)
					{
						GKCOAP_Fota_Request();
					}
					break;
				case 0x03:/*Configure Transmit Interval*/
					if(4== _tlv.len)
					{
						if(START_TIME_ReportTrigger== config.nbiot.gkcoap.reportTriggerType)
						{
							uint32_t _secondsTick;
							memcpy(&_secondsTick, _tlv.value, 4);
							memcpy(&config.nbiot.gkcoap.reportInterval_s, &_secondsTick, 4);
						}

						GKCOAP_Register_Request();
					}
					else if(8== _tlv.len)
					{
						config.nbiot.gkcoap.reportStartMask= 0xFFFFFF0000000000;/*0xYYMMDDhhmmss0000*/
						config.nbiot.gkcoap.reportStartMask|= UTILI_BinToBCD(_tlv.value[1])<< 32;
						config.nbiot.gkcoap.reportStartMask|= UTILI_BinToBCD(_tlv.value[2])<< 24;
						config.nbiot.gkcoap.reportStartMask|= UTILI_BinToBCD(_tlv.value[3])<< 16;
						memcpy(&config.nbiot.gkcoap.reportInterval_s, _tlv.value+ 4, 4);
						config.nbiot.gkcoap.reportTriggerType= START_TIME_ReportTrigger;

						config.nbiot.gkcoap.rteStartTime= config.nbiot.gkcoap.rteNextTime= UTILI_Mask_GetMatchedTimeFromNow(config.nbiot.gkcoap.reportStartMask);
						GKCOAP_Register_Request();
					}
					break;
				case 0x04:/*Configure Log Interval*/
					{
						if(8== _tlv.len)
						{
							uint32_t _secondsTick;
							memcpy(&_secondsTick, _tlv.value, 4);

							if(0== (_secondsTick% (60* 60* 24)))/*day*/
							{
								config.nbiot.gkcoap.logTickType= 3;
								config.nbiot.gkcoap.logTickSize= (_secondsTick/ (60* 60* 24));
							}
							else if(0== (_secondsTick% (60* 60)))/*hour*/
							{
								config.nbiot.gkcoap.logTickType= 2;
								config.nbiot.gkcoap.logTickSize= (_secondsTick/ (60* 60));
							}
							else if(0== (_secondsTick% (60)))/*minute*/
							{
								config.nbiot.gkcoap.logTickType= 1;
								config.nbiot.gkcoap.logTickSize= (_secondsTick/ 60);
							}
							else
							{
								config.nbiot.gkcoap.logTickType= 0;
								config.nbiot.gkcoap.logTickSize= _secondsTick;/*not quantifiable but other units, use seconds*/
							}

							RTC_AlarmStartMarker_t _marker;
							_marker.date= (_tlv.value[4]== 0xFF)?0xFF: UTILI_BinToBCD(_tlv.value[4]);
							_marker.hour= (_tlv.value[5]== 0xFF)?0xFF: UTILI_BinToBCD(_tlv.value[5]);
							_marker.minute= (_tlv.value[6]== 0xFF)?0xFF: UTILI_BinToBCD(_tlv.value[6]);
							_marker.second= (_tlv.value[7]== 0xFF)?0xFF: UTILI_BinToBCD(_tlv.value[7]);

							config.nbiot.gkcoap.logTickStartMask= _marker.date<< 24;
							config.nbiot.gkcoap.logTickStartMask|= _marker.hour<< 16;
							config.nbiot.gkcoap.logTickStartMask|= _marker.minute<< 8;
							config.nbiot.gkcoap.logTickStartMask|= _marker.second<< 0;
							DEVICELOG_EnablePeriodicLog(true);
							DEVICELOG_StartLog(NOTSTARTED_StartType, config.nbiot.gkcoap.logTickType, config.nbiot.gkcoap.logTickSize, _marker);
						}
						GKCOAP_Register_Request();
					}
					break;
				case 0x05:/*Clear Alarm, 1 is clear*/
					if(1== _tlv.len)
					{
						SENSORS_SetStatusCode((_tlv.value[0]^ 0xFF)& SENSORS_GetStatusCode());

						GKCOAP_Register_Request();
					}
					break;
//				case 0x06:/*Set Alarm Enabled Mask*/
//					if((1== _tlv.len)|| (17== _tlv.len))
//					{
//						config.sensors.enableMask= _tlv.value[0];
//
//						if(17== _tlv.len)
//						{
//							memcpy(&config.sensors.tiltAutoClearBackOffPeriod, &_tlv.value[1], 2);
//							memcpy(&config.sensors.magnetAutoClearBackOffPeriod, &_tlv.value[3], 2);
//							memcpy(&config.sensors.backFlowAutoClearBackOffPeriod, &_tlv.value[5], 2);
//							memcpy(&config.sensors.lowBattAutoClearBackOffPeriod, &_tlv.value[7], 2);
//							memcpy(&config.sensors.tamperAutoClearBackOffPeriod, &_tlv.value[9], 2);
//							memcpy(&config.sensors.burstAutoClearBackOffPeriod, &_tlv.value[11], 2);
//							memcpy(&config.sensors.noConsumptionAutoClearBackOffPeriod, &_tlv.value[13], 2);
//							memcpy(&config.sensors.leakageAutoClearBackOffPeriod, &_tlv.value[15], 2);
//						}
//						GKCOAP_Register_Request();
//					}
//					break;
//				case 0x07:/*Set Low-battery threshold*/
//					if(4== _tlv.len)
//					{
//						memcpy(&config.sensors.battery.lowThreshold, _tlv.value, 4);
//
//						GKCOAP_Register_Request();
//					}
//					break;
//				case 0x08:/*Set Pipe parameter*/
//					if(24== _tlv.len)
//					{
//						float adjustedThreshold;/*adjust from l/hr to l/s*/
//						memcpy(&adjustedThreshold, &_tlv.value[0], 4);
//						adjustedThreshold/= 3600.0;
//						memcpy(&config.sensors.flow.burstThreshold, &adjustedThreshold, 4);
//						memcpy(&config.sensors.flow.burstSamplingPeriod_s, &_tlv.value[4], 4);
//						memcpy(&config.sensors.flow.noflowSamplingPeriod_s, &_tlv.value[8], 4);
//						memcpy(&adjustedThreshold, &_tlv.value[12], 4);
//						adjustedThreshold/= 3600.0;
//						memcpy(&config.sensors.flow.leakageThreshold, &adjustedThreshold, 4);
//						memcpy(&config.sensors.flow.leakageSamplingPeriod_s, &_tlv.value[16], 4);
//						memcpy(&config.sensors.flow.backflowThreshold, &_tlv.value[20], 4);
//
//						GKCOAP_Register_Request();
//					}
//					FLOWSENSOR_Init(&(config.sensors.flow));
//					break;
//				case 0x09:/*Set IP/Port*/
//					if(42== _tlv.len)
//					{
//						memcpy(&config.nbiot.gkcoap.serverIP, &_tlv.value[0], 16);
//						memcpy(&config.nbiot.gkcoap.serverPort, &_tlv.value[16], 5);
//						memcpy(&config.nbiot.gkcoap.serverIP, &_tlv.value[21], 16);
//						memcpy(&config.nbiot.gkcoap.serverPort, &_tlv.value[37], 5);
//
//						GKCOAP_Register_Request();
//					}
//					break;
//				case 0x0A:/*Set Active IP/Port*/
//					if(1== _tlv.len)
//					{
//						if(1== _tlv.value[0])
//						{
//							//config.nbiot.gkcoap.activeServer= 1;
//						}
//						else if(2== _tlv.value[0])
//						{
//							//config.nbiot.gkcoap.activeServer= 2;
//						}
//
//						GKCOAP_Register_Request();
//					}
//					break;
//				case 0x0B:/*set APN*/
//					if(33== _tlv.len)
//					{
//						memcpy(&config.nbiot.apn, &_tlv.value[0], 33);
//						GKCOAP_Register_Request();
//					}
//					break;
//				case 0x0C:/*set nbiot network param*/
//					if(16== _tlv.len)
//					{
//						memcpy(&config.nbiot.periodicTau, &_tlv.value[0], 4);
//						memcpy(&config.nbiot.activeTime, &_tlv.value[4], 4);
//						memcpy(&config.nbiot.pagingTime, &_tlv.value[8], 4);
//						memcpy(&config.nbiot.edrx, &_tlv.value[12], 4);
//
//						GKCOAP_Register_Request();
//					}
//					break;
//				case 0x0D:/*set sara attach backoff*/
//					if(4== _tlv.len)
//					{
//						memcpy(&config.nbiot.restartDelay_s, &_tlv.value[0], 4);
//
//						GKCOAP_Register_Request();
//					}
//					break;
				case 0x0E:/*reset sara*/
					if(0== _tlv.len)
					{
						GKCOAP_Link_HardReset();

						GKCOAP_Register_Request();
					}
					break;
//				case 0x0F:/*set flow rate interval*/
//					if(4== _tlv.len)
//					{
//						memcpy(&config.sensors.flow.samplingPeriod_s, &_tlv.value[0], 4);
//
//						GKCOAP_Register_Request();
//					}
//					break;
				case 0x10:/*key change*/
					if(35== _tlv.len)
					{
						uint8_t _keyNo= _tlv.value[0];
						uint8_t *_keyPtr= 0;
						uint16_t _keyLen;
						memcpy(&_keyLen, &_tlv.value[1], 2);

						switch(_keyNo)
						{
							case 0:
								_keyPtr= &config.system.key.master[0];
								break;
							case 1:
								_keyPtr= &config.system.key.cfg.user[0];
								break;
							case 2:
								_keyPtr= &config.system.key.opr.a[0];
								break;
							case 3:
								_keyPtr= &config.system.key.opr.b[0];
								break;
							default:
								_keyPtr= 0;
								break;
						}

						if((16== _keyLen)&& (0!= _keyPtr)&& (false!= UTILI_IsArrayTheSame(_keyPtr, &_tlv.value[3], _keyLen) ))
						{
							memcpy(_keyPtr, &_tlv.value[3+ _keyLen], _keyLen);
							SECURE_KeyChange(_keyNo, _keyPtr, _keyLen);

							GKCOAP_Register_Request();
						}
					}
					break;
				default:
					break;
			}
			_tlvsIndex+= (1+ 1+ _tlv.len);/*increment TLV buffer index*/
		}
	}

	return SUCCESS;
}
