/*
 * logger.h
 *
 *  Created on: 13 Jan 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef LOGGER_LOGGER_H_
#define LOGGER_LOGGER_H_

#include "main.h"
#include "rtc.h"
#include "rtc_cfg.h"
#include "devicelog.h"

#define LOGGER_CFG_VERSION 				0x01
#define LOGGER_CFG_MAX_TRX_QUEUE 		3
#define LOGGER_CFG_EEPROM_TIMEOUT_MS	500
#define LOGGER_CFG_MAX_RETRY_COUNT		2

typedef enum
{
	LOG_TYPE_DEVICE= 0,
	LOG_TYPE_RADIO,
	LOG_TYPE_EVENT,
	LOG_TYPE_MAX
}LOGGER_LogType_t;

typedef enum
{
	TRANSACTION_TYPE_DEVICELOG_WRITE= 0,
	TRANSACTION_TYPE_DEVICELOG_READ,
	TRANSACTION_TYPE_CUSTOMLOG_WRITE,
	TRANSACTION_TYPE_CUSTOMLOG_READ,
}LOGGER_TransactionType_t;

typedef struct
{
	LOGGER_TransactionType_t transactionType;
	uint8_t *logBuffer;
	uint32_t logIndex;
	uint32_t logCount;
	__IO bool transactionInQueue;
	__IO bool transactionInProgress;
	__IO bool transactionStatusOK;
	__IO bool transactionCompleted;
	void (*TransactionCompletedCb)(void);
}LOGGER_Transaction_t;

typedef struct
{
	uint8_t partition;
	uint16_t head;
	uint16_t elementCount;
	uint16_t elementMax;
	uint16_t elementSize;
}LOGGER_SyncRte_t;

typedef enum
{
	NONE_LoggerTLVTag= 0,
	EEPROM_CHECK_LoggerTLVTag,
	GET_PERIODIC_LOG_LoggerTLVTag,
	SET_PERIODIC_LOG_LoggerTLVTag,
	GET_TAMPER_LOG_LoggerTLVTag,
	SET_TAMPER_LOG_LoggerTLVTag,
	GET_DEVICE_LOG_LoggerTLVTag,
}LOGGER_LoggerTLVTag_t;

/*TO EXPORT OUT TO CFG*/
typedef struct __attribute__((aligned(8)))/*compulsory alignment*/
{
	uint32_t checksum;/*compulsory checksum*/
	uint8_t  version;

	struct
	{
		bool contextSaved;
		bool logConfigured;
		bool deviceLogEnabled;
		bool tamperLogEnabled;
		DEVICELOG_StartType_t startType;
		DEVICELOG_StartMarker_t startMarker;
		DEVICELOG_TickType_t tickType;
		uint32_t tickSize;
		DEVICELOG_Rte_t rte;

		uint8_t reserve[32];
	}device;

	struct
	{
		bool contextSaved;
		bool logEnabled;

		uint8_t reserve[32];

		struct
		{
			uint32_t currentEEPROMAddress;
			uint32_t logsInEEPROMFloorIndex;
			uint32_t logsInEEPROMCeilingIndex;
			uint32_t logCount;

			uint8_t reserve[32];
		}status;
	}radio;

	//uint8_t reserve[128];/*to add more param reduce this, thus no need to do backward compatible thing. remember to set the default value after config*/
}LOG_t;

uint8_t LOGGER_QueueDepth(void);
ErrorStatus LOGGER_Enqueue(LOGGER_Transaction_t *_trx);
LOGGER_Transaction_t *LOGGER_Dequeue(void);
void LOGGER_Device_StartPeriodicLog(RTC_TickType_t _tickType, uint32_t _tickSize, RTC_AlarmStartMarker_t _startTime);
void LOGGER_Device_StartTamperLog(bool _start);
void LOGGER_App_Radio(void);
ErrorStatus LOGGER_Test(void);
ErrorStatus LOGGER_InitSync(LOGGER_SyncRte_t *_rte, uint8_t _partition, uint16_t _elementSize, uint16_t _elementMax);
ErrorStatus LOGGER_WriteSync(LOGGER_SyncRte_t *_rte, void *_element);
ErrorStatus LOGGER_ReadSync(LOGGER_SyncRte_t *_rte, uint16_t _index, uint16_t _count,  uint8_t *_buf);
void LOGGER_SaveContext(void);
void LOGGER_RestoreContext(void);
void LOGGER_TLVRequest(TLV_t *_tlv);
void LOGGER_Init(LOG_t *_config);
void LOGGER_Task(void);
uint8_t LOGGER_TaskState(void);

#endif /* LOGGER_LOGGER_H_ */
