/*
 * meterlog.h
 *
 *  Created on: 13 Jan 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef LOGGER_DEVICELOG_H_
#define LOGGER_DEVICELOG_H_

#include "main.h"
#include "m95m01.h"
#include "rtc.h"

/*NOTE: Bare in mind our strategy of implementing eeprom is to write page by page basis, cos page write(256 bytes) is equal to byte write which is 5ms.
 * Thus, before writing to eeprom, the logger(both Meter/Radio logs) has to accumulate 256 bytes in RAM first.
 * As a result, log size has to be divisible by 256(1, 2, 4, 8, 16, 32, 64, 128, 256)*/

typedef struct
{
	uint32_t nextTick;
	uint32_t currentEEPROMAddress;
	uint32_t logsInEEPROMFloorIndex;
	uint32_t logsInEEPROMCeilingIndex;
	uint32_t logCount;
	uint32_t periodicLogCount;
	uint32_t tamperLogCount;
	uint32_t currentLogTimestamp;
}DEVICELOG_Rte_t;

typedef struct //__attribute__((aligned(32))) /*must be divisible by 128 for now. TODO: remove this limitation*/
{
	uint32_t timestamp;
	float meterReading;/*liter*/
	uint8_t pbmagCount;
	uint8_t tamperInCount;
	uint8_t rsTamperCount;
	uint8_t temperature;/*celcius*/
	uint16_t voltage;/*milivolt*/
	uint32_t cellCapacity;/*Ah unit*/
	uint32_t aveCurrent;/*uA*/
	uint8_t positionX;
	uint8_t positionY;
	uint8_t positionZ;
	float flowRate;
	float backflowReading;/*liter*/
	uint8_t statusByte;
} DEVICELOG_Log_t;

typedef enum
{
	DeviceLog_TrxType,
	TamperLog_TrxType
}DEVICELOG_TrxType_t;

typedef struct
{
	M95M01_Transaction_t M95M01Trx;
	DEVICELOG_TrxType_t type;
}DEVICELOG_Trx_t;

typedef enum
{
	NOTSTARTED_StartType= 0,
	IMMEDIATE_StartType= 1,
	WAITSTARTTIME_StartType= 2,
	MAX_StartType
}DEVICELOG_StartType_t;

#define DEVICELOG_TickType_t	RTC_TickType_t
#define DEVICELOG_StartMarker_t RTC_AlarmStartMarker_t

#define DEVICELOG_CFG_EEPROM_TIMEOUT 		1000

#define DEVICELOG_CFG_MAX_RETRY_COUNT		0x03 /*try 3 times*/
#define DEVICELOG_CFG_WRITE_BUFF_SIZE		(1* sizeof(DEVICELOG_Log_t))
#define DEVICELOG_CFG_MAX_LOGS_IN_EEPROM	(M95M01_CFG_PARTITION_8_SIZE/ sizeof(DEVICELOG_Log_t))

void DEVICELOG_AppWrite_Callback(void);
ErrorStatus DEVICELOG_WriteLog(M95M01_Transaction_t *_trx, DEVICELOG_Log_t _log);
ErrorStatus DEVICELOG_WriteLogs(M95M01_Transaction_t *_trx, DEVICELOG_Log_t _log[], uint16_t _logCount);
ErrorStatus DEVICELOG_ReadLogs(M95M01_Transaction_t *_trx, uint32_t _logIndex, uint32_t _logCount, DEVICELOG_Log_t *_logBuf);
uint32_t DEVICELOG_GetLogCount(void);
uint32_t DEVICELOG_GetLogFloor(void);
uint32_t DEVICELOG_GetPeriodicLogCount(void);
uint32_t DEVICELOG_GetTamperLogCount(void);
void DEVICELOG_EnablePeriodicLog(bool _enable);
void DEVICELOG_EnableTamperLog(bool _enable);
DEVICELOG_StartType_t DEVICELOG_ReStartLog(DEVICELOG_StartType_t _startType, RTC_TickType_t _tickType, uint32_t _tickSize, RTC_AlarmStartMarker_t _startTime, uint32_t _savedTick);
DEVICELOG_StartType_t DEVICELOG_StartLog(DEVICELOG_StartType_t _startType, RTC_TickType_t _tickType, uint32_t _tickSize, RTC_AlarmStartMarker_t _startTime);
void DEVICELOG_Init(void);
void DEVICELOG_Task(void);
uint8_t DEVICELOG_TaskState(void);

#endif /* LOGGER_DEVICELOG_H_ */
