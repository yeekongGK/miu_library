/*
 * nbiot_define.h
 *
 *  Created on: 9 May 2022
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef NBIOT_NBIOT_DEFINE_H_
#define NBIOT_NBIOT_DEFINE_H_

typedef struct
{
	uint32_t noOfTransmission;
	uint32_t noOfFailedTransmission;
	uint32_t noOfAttach;
	uint32_t noOfDisattach;
	uint32_t noOfSimError;
	uint16_t latency_ms;
	uint16_t aveLatency_ms;
	uint16_t minLatency_ms;
	uint16_t maxLatency_ms;
	uint16_t pingLatency_ms;
	uint16_t failsafeRebootCount;////avePingLatency_ms;
	uint16_t PVDRebootCount;//minPingLatency_ms;
	uint16_t BORRebootCount;//maxPingLatency_ms;
	int16_t rsrp;
	int16_t aveRsrp;
	int16_t minRsrp;
	int16_t maxRsrp;
	int16_t rssi;
	int16_t aveRssi;
	int16_t minRssi;
	int16_t maxRssi;
	int8_t sinr;
	int8_t aveSinr;
	int8_t minSinr;
	int8_t maxSinr;
	int8_t rsrq;
	int8_t aveRsrq;
	int8_t minRsrq;
	int8_t maxRsrq;
	uint8_t txPower;
	uint8_t aveTxPower;
	uint8_t minTxPower;
	uint8_t maxTxPower;
	uint8_t ceMode;
	uint8_t ecl;
	uint16_t battVoltage_mV;
}NBIOT_Stats_t;


#define PRACT_CFG_0_RECORD_RESOURCE_MAX_COUNT		1464/*61 days*/
#define PRACT_CFG_0_RECORD_VALUE_LEN				sizeof(PRACT_GetReadingRecord_t)//(4+ 1+ 2)/*meter reading, temperature, status word*/
#define PRACT_CFG_0_RECORD_CBOR_BYTE_LEN			(1/*array header*/+ 1+sizeof(uint32_t) + 1+sizeof(uint32_t) + 1+sizeof(float) + 1+sizeof(uint32_t) + 1+sizeof(uint32_t))
#define PRACT_CFG_0_SETTINGS_RESOURCE_MAX_LEN		32
#define PRACT_CFG_0_SETTINGS_RESOURCE_DEFAULT		0

#define PRACT_CFG_1_RECORD_RESOURCE_MAX_COUNT		1
#define PRACT_CFG_1_RECORD_VALUE_LEN				256
#define PRACT_CFG_1_RECORD_CBOR_BYTE_LEN			(1/*array header*/+ 1+sizeof(uint32_t) + 1+sizeof(uint32_t) + 1+PRACT_CFG_1_RECORD_VALUE_LEN)
#define PRACT_CFG_1_SETTINGS_RESOURCE_MAX_LEN		32
#define PRACT_CFG_1_SETTINGS_RESOURCE_DEFAULT		0

#define PRACT_CFG_2_RECORD_RESOURCE_MAX_COUNT		1
#define PRACT_CFG_2_RECORD_VALUE_LEN				256
#define PRACT_CFG_2_RECORD_CBOR_BYTE_LEN			(1/*array header*/+ 1+sizeof(uint32_t) + 1+sizeof(uint32_t) + 1+PRACT_CFG_2_RECORD_VALUE_LEN)
#define PRACT_CFG_2_SETTINGS_RESOURCE_MAX_LEN		256
#define PRACT_CFG_2_SETTINGS_RESOURCE_DEFAULT		0

#define DTMON_CFG_RESULTS_RESOURCE_MAX_COUNT		50
#define DTMON_CFG_RESULTS_VALUE_LEN					4
#define DTMON_CFG_RESULTS_CBOR_BYTE_LEN				(1/*array header*/+ 1+sizeof(uint32_t)+ 1+sizeof(uint32_t) + 1+sizeof(uint32_t))
#define DTMON_CFG_SETTINGS_RESOURCE_MAX_LEN			96

#endif /* NBIOT_NBIOT_DEFINE_H_ */
