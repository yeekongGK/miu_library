/*
 * bc66.h
 *
 *  Created on: 30 Mar 2020
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef NBIOT_BC66_H_
#define NBIOT_BC66_H_

#include "main.h"

#if ENABLE_NBIOT_UPKI == 1
	#define NBIOT_APP_UPKI_MAX_DATA_SIZE			(512- (/*upki header*/1+ /*iv*/16+ 12/*tag*/))
	#define NBIOT_APP_UPKI_MAX_LOGS					(1+ (/*max upki lib buff*/NBIOT_APP_UPKI_MAX_DATA_SIZE- (/*rssi*/1+ /*1st log(full)*/16+ /*hasMore*/1+ /*hasMore2*/1+ /*radio log*/40+ /*diag info*/20+ /*padding?*/1))/ /*subsequent log size*/10)
#if NBIOT_APP_UPKI_MAX_LOGS > 31
	#define NBIOT_APP_MAX_LOGS						31
#else
	#define NBIOT_APP_MAX_LOGS						NBIOT_APP_UPKI_MAX_LOGS
#endif
	#define NBIOT_APP_FOTA_PACKET_COUNT				(NBIOT_APP_UPKI_MAX_DATA_SIZE/ 128)//1/*expecting to do only 128(1* 128)  FOTA at a time, not enough RAM for PKI*/
#else
	#define NBIOT_APP_MAX_LOGS						31
	#define NBIOT_APP_FOTA_PACKET_COUNT				4/*expecting to do 512(4* 128) FOTA at a time*/
#endif

#define NBIOT_APP_FOTA_RESPONSE_SIZE			(128* (NBIOT_APP_FOTA_PACKET_COUNT))
#define NBIOT_APP_REPORT_REGISTER_RESPONSE_SIZE	(/*time*/6+ /*no of command*/1+ /*maximum tlv message payload*/256)

#define NBIOT_APP_UPLINK_PAYLOAD_BUF_SIZE	   	(/*rssi*/1+ /*1st log(full)*/16+ /*hasMore*/1+ /*10B x subsequent logs*/(10* (NBIOT_APP_MAX_LOGS- 1))+ /*hasMore2*/1+ /*radio log*/40+ /*diag info*/20+ /*padding?*/1)
#if NBIOT_APP_FOTA_RESPONSE_SIZE > NBIOT_APP_REPORT_REGISTER_RESPONSE_SIZE
	#define NBIOT_APP_DOWNLINK_PAYLOAD_BUF_SIZE 	NBIOT_APP_FOTA_RESPONSE_SIZE
#else
	#define NBIOT_APP_DOWNLINK_PAYLOAD_BUF_SIZE 	NBIOT_APP_REPORT_REGISTER_RESPONSE_SIZE
#endif

#if NBIOT_APP_UPLINK_PAYLOAD_BUF_SIZE > NBIOT_APP_DOWNLINK_PAYLOAD_BUF_SIZE
	#define NBIOT_APP_PAYLOAD_BUF_SIZE	NBIOT_APP_UPLINK_PAYLOAD_BUF_SIZE
#else
	#define NBIOT_APP_PAYLOAD_BUF_SIZE	NBIOT_APP_DOWNLINK_PAYLOAD_BUF_SIZE
#endif

#define NBIOT_APP_LOG_BUF_SIZE 					(NBIOT_APP_MAX_LOGS* 16)//sizeof(METER_LOG_LogTypeDef))
#define NBIOT_APP_MSG_BUF_SIZE 					((/*flash msg header*/8+ /*fota packet*/128)* NBIOT_APP_FOTA_PACKET_COUNT/*queue all at the same time*/)/*this is specifically for fota, for tlv message downlink, we use nbiot_packet buffer*/

#define NBIOT_APP_MAX_TRX_RETRY					3
#define NBIOT_APP_MAX_EEPROM_TRX_RETRY

#if ENABLE_NBIOT_UPKI == 1
	#define NBIOT_DOWNLINK_PAYLOAD_SIZE		NBIOT_MGT_DOWNLINK_PAYLOAD_BUF_SIZE
#else
	#define NBIOT_DOWNLINK_PAYLOAD_SIZE		NBIOT_APP_DOWNLINK_PAYLOAD_BUF_SIZE
#endif

#define BC66PHY_CFG_MAX_REPLY_MSG_LEN		32/*maximum reply pattern: can make do with about 30, and truncate the access*/
#define BC66PHY_CFG_MAX_REPLY_MSG_ARRAY		6
#define BC66PHY_CFG_MAX_URC_MSG_LEN			(34+ ((1+ NBIOT_DOWNLINK_PAYLOAD_SIZE+ 1+ 2)* 2)+ 3) + 20/*extra jic*/	// +QCOAPURC: "rsp",2,2.03,12205,518,<status-downlink data(in hex string)-counter-crc1crc2>\r\n\0
#define BC66PHY_CFG_MAX_URC_MSG_ARRAY 		2

typedef enum
{
  ERROR_Bc66PhyStatus = 0,
  SUCCESS_Bc66PhyStatus = 1,
  BUSY_Bc66PhyStatus = 2
} BC66PHY_Status;

typedef enum
{
  DISABLED_Bc66PhyPower = 0,
  SLEEP_Bc66PhyPower = 1,
  ACTIVE_Bc66PhyPower = 2,
} BC66PHY_Power;

typedef enum
{
  UINTENTIONAL_RESET_Bc66PhyError= 1,
  ERROR_RECEIVED_Bc66PhyError,
  RESP_ERR_Bc66PhyError,
  RESP_TIMEOUT_Bc66PhyError,
  HARD_RESET_Bc66PhyError,
} BC66PHY_Error;

typedef enum
{
	IDLE_Bc66PhyJobState= 0,
	PREPARING_Bc66PhyJobState,
	TRANSMITTING_Bc66PhyJobState,
	RECEIVING_Bc66PhyJobState
} BC66PHY_Job_States;

typedef enum
{
	SUCCESS_Bc66PhyJobstatus = 0,
	BUSY_Bc66PhyJobstatus,
	TX_TIMEOUT_Bc66PhyJobstatus,
	RESPONSE_TIMEOUT_Bc66PhyJobstatus,
	RESPONSE_ERROR_Bc66PhyJobstatus
}BC66PHY_Job_Status;

typedef struct
{
	BC66PHY_Job_Status status;
	BC66PHY_Job_States state;
	char *txString;
	uint8_t expectedRxCnt;
	const char **expectedRx;
	uint16_t timeout;
	uint8_t noOfRetry;
	char *rxBuffer;
}BC66PHY_Job_t;

void BC66PHY_Init(void);
void BC66PHY_InitBaudrate(uint32_t _baudrate);
uint32_t BC66PHY_GetBaudrate(void);
void BC66PHY_SetRxTxPinOD(bool _rxPin, bool _txPin);
void BC66PHY_PowerState(BC66PHY_Power _state);
void BC66PHY_PowerSignalPin(uint8_t _state);
void BC66PHY_ResetPin(uint8_t _state);
void BC66PHY_PSMPin(uint8_t _state);
bool BC66PHY_SIMDetected(void);
uint32_t BC66PHY_DiagCode(uint8_t _code);
void BC66PHY_PowerDown(void);
void BC66PHY_PowerUp(void);
void BC66PHY_Sleep(void);
void BC66PHY_Wakeup(void);
void BC66PHY_Reset(void);
void BC66PHY_ExitPSM(void);
ErrorStatus BC66PHY_Tx(__IO char* _string);
bool BC66PHY_IsTxInProgress(void);
ErrorStatus BC66PHY_PrepareRx(void);
uint8_t BC66PHY_NoOfMsgInQueue(void);
char* BC66PHY_MsgDequeue(void);
uint8_t BC66PHY_ReplyMsg_Depth(void);
char* BC66PHY_ReplyMsg_Dequeue(void);
char* BC66PHY_ReplyMsg_Peek(void);
uint8_t BC66PHY_URCMsg_Depth(void);
char* BC66PHY_URCMsg_Dequeue(void);
char* BC66PHY_URCMsg_Peek(void);
void BC66PHY_Job(void);
void BC66PHY_InhibitDiag(void);
void BC66PHY_InhibitErrorReset(bool _true);
void BC66PHY_TxRxJob(__IO char *_txString, uint8_t _expectedRxCnt,  const char **_expectedRx, uint16_t _timeout, uint8_t _noOfRetry);
void BC66PHY_TxRxJob2(__IO char *_txString, uint8_t _expectedRxCnt,  const char **_expectedRx, uint16_t _timeout, uint8_t _noOfRetry, char* _rxBuffer);
void BC66PHY_PauseJob_s(uint16_t _period);
void BC66PHY_WakeupUsingAT(uint16_t _waitPeriod);
void BC66PHY_JobTransition(void);
void BC66PHY_ResetJob(void);
void BC66PHY_HaltErrorCount(uint8_t _cycle);
bool BC66PHY_IsIdle(void);
bool BC66PHY_IsHardReset(void);
void BC66PHY_ClearHardResetFlag(void);
BC66PHY_Job_Status BC66PHY_GetJobStatus(void);
uint8_t BC66PHY_TaskState(void);

#endif /* NBIOT_BC66PHY_H_ */
