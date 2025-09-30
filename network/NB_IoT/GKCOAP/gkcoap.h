/*
 * nbiotgkcoap.h
 *
 *  Created on: 14 Feb 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef NBIOT_GKCOAP_H_
#define NBIOT_GKCOAP_H_

#include "main.h"
#include <time.h>
#include "BC66/bc66link.h"

#define GKCOAP_CFG_BUFFER_SIZE			(128*4)
#define GKCOAP_CFG_LOG_COUNT_MAX		31
#define GKCOAP_CFG_LOG_BUFFER_SIZE		(GKCOAP_CFG_LOG_COUNT_MAX* sizeof(DEVICELOG_Log_t))
#define GKCOAP_CFG_MSG_BUFFER_SIZE		(256)//(/*flash msg header*/8+ /*fota packet*/128)

typedef struct
{
	BC66LINK_Coap_t coapType;
	uint32_t messageId;
	uint32_t objectId;
	uint32_t instanceId;
	uint8_t resourceIndex;
	uint8_t resourceCount;
	uint32_t valueType;
	uint32_t len;
	uint32_t value;
	uint32_t index;
	uint8_t flag;
}GKCOAP_COAP_Request_t;

typedef enum
{
	START_TIME_ReportTrigger,
	LOG_COUNT_ReportTrigger,
	MAX_ReportTrigger,
} GKCOAP_ReportTrigger_t;

typedef enum
{
	GKCOAP_REG_STATE_IDLE,
	GKCOAP_REG_STATE_SEND_PACKET,
	GKCOAP_REG_STATE_CHECK_SEND_PACKET,
	GKCOAP_REG_STATE_SET_BACKOFF,
	GKCOAP_REG_STATE_BACKOFF,
	GKCOAP_REG_STATE_FAILED,
	GKCOAP_REG_STATE_COMPLETE,
	GKCOAP_REG_STATE_MAX
} GKCOAP_REG_STATE_t;

typedef enum
{
	GKCOAP_REPORT_STATE_IDLE,
	GKCOAP_REPORT_CHECK_LOG,
	GKCOAP_REPORT_FETCH_LOG,
	GKCOAP_REPORT_STATE_SEND_PACKET,
	GKCOAP_REPORT_STATE_CHECK_SEND_PACKET,
	GKCOAP_REPORT_STATE_SET_BACKOFF,
	GKCOAP_REPORT_STATE_BACKOFF,
	GKCOAP_REPORT_STATE_FAILED,
	GKCOAP_REPORT_STATE_COMPLETE,
	GKCOAP_REPORT_STATE_MAX
} GKCOAP_REPORT_STATE_t;

typedef enum
{
	Idle_FotaState,
	Get_Version_FotaState,
	Check_Version_FotaState,
	Flash_Init_FotaState,
	Get_Packet_FotaState,
	Process_Packet_FotaState,
	Switch_Bank_FotaState,
	Switch_Bank_Check_FotaState,
	Set_Backoff_FotaState,
	Backoff_FotaState,
	Suspend_FotaState,
	Failed_FotaState,
	Complete_FotaState,
	Max_FotaState
} GKCOAP_FotaState_t;

typedef enum
{
	GKCOAP_MSG_STATE_IDLE,
	GKCOAP_MSG_STATE_SEND_PACKET,
	GKCOAP_MSG_STATE_CHECK_SEND_PACKET,
	GKCOAP_MSG_STATE_SET_BACKOFF,
	GKCOAP_MSG_STATE_BACKOFF,
	GKCOAP_MSG_STATE_FAILED,
	GKCOAP_MSG_STATE_COMPLETE,
	GKCOAP_MSG_STATE_MAX
} GKCOAP_MSG_STATE_t;

typedef struct
{
	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;
	uint8_t date;
	uint8_t month;
	uint8_t year;
	uint8_t meterPulse[4];
	uint8_t statusCode;
	uint8_t temperature;
	uint8_t adcVoltage[4];
}GKCOAP_Log_t;

typedef struct
{
	uint8_t configOptionStatus;
	uint8_t sendStatus;/*0= success, 1= failed, 0xFF= pending*/
	uint8_t responseStatus;/*0= success, 1= failed, 0xFF= pending*/

	struct
	{
		GKCOAP_REG_STATE_t state;
		bool isRequested;
		bool isCompleted;
		uint8_t retryCount;
	}reg;

	struct
	{
		GKCOAP_REPORT_STATE_t state;
		bool isRequested;
		bool isCompleted;
		uint8_t retryCount;
		uint32_t noOfLogsToReport;
		uint32_t noOfLogsReported;
	}report;

	struct
	{
		GKCOAP_FotaState_t state;
		GKCOAP_FotaState_t prevState;
		bool isRequested;
		bool isCompleted;
		bool isOnGoing;
		uint8_t retryCount;
		uint16_t currentPacket;
		uint16_t totalPacket;
		uint32_t packetSize;
		uint16_t flashWriteTotal;
		uint16_t flashWriteCount;
		uint32_t flashWrittenBytes;
		uint16_t flashChecksum;
		char version[35];
	}fota;

	struct
	{
		GKCOAP_FotaState_t state;
		GKCOAP_FotaState_t prevState;
		bool isRequested;
		bool isCompleted;
		bool isOnGoing;
		uint8_t retryCount;
		uint16_t currentPacket;
		uint16_t totalPacket;
		uint16_t flashWriteTotal;
		uint16_t flashWriteCount;
		uint32_t flashWrittenBytes;
		uint16_t flashChecksum;
		char version[16];
	}swmgt;

}GKCOAP_Rte_t;

typedef struct{

	struct
	{
		GKCOAP_MSG_STATE_t state;
		uint8_t retryCount;
	}msg;
}GKCOAP_Rte2_t;

typedef struct
{
	bool enabled;
	uint16_t localPort;
	char serverIP[150];/*max 150 bytes*/
	uint16_t serverPort;
	char serverURI[35+ 1];
	char fotaServerIP[150];/*max 150 bytes*/
	uint16_t fotaServerPort;
	char fotaServerURI[35+ 1];
	//BC66LINK_COAP_Encryption_Mode_t mode;
	uint32_t retryBackoffMin_s;
	uint32_t retryBackoffMax_s;
	uint8_t retryMax;
	uint8_t packetType;
	uint8_t logTickType;
	uint32_t logTickSize;
	uint32_t logTickStartMask;
	uint64_t reportStartMask;
	uint32_t reportInterval_s;
	GKCOAP_ReportTrigger_t reportTriggerType;
	uint32_t reportTriggerLogCount;

	time_t rteStartTime;
	time_t rteNextTime;

	GKCOAP_Rte_t rte;

	uint32_t alarmActivePeriod_s;
	time_t rteAlarmClearTimestamp;

	GKCOAP_Rte2_t rte2;

	uint8_t reserve[22];/*to add more param reduce this, thus no need to do backward compatible thing. remember to set the default value after config*/
}GKCOAP_t;

void GKCOAP_Link_HardReset(void);
void GKCOAP_Register_Request(void);
bool GKCOAP_Register_Completed(void);
void GKCOAP_Report_Request(void);
void GKCOAP_Fota_Request(void);
void GKCOAP_Init(GKCOAP_t *_config);
void GKCOAP_Task(void);
uint8_t GKCOAP_TaskState(void);

#endif /* NBIOT_GKCOAP_H_ */
