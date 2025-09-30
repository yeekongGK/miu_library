/*
 * nbiot.h
 *
 *  Created on: 5 Feb 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef NBIOT_NBIOT_H_
#define NBIOT_NBIOT_H_

#include "main.h"
#include "GKCOAP/gkcoap.h"
#include "LWM2M/lwm2m.h"
#include "nbiot_define.h"

#define NBIOT_CFG_VERSION 				0x01
#define NBIOT_CFG_IMEI_LEN				15/*15 characters*/
#define NBIOT_CFG_IMSI_LEN				15/*int mobile subscriber identity 15 digits long.*/
#define NBIOT_CFG_ICCID_LEN				20/*ICCID is either 19 or 20 digits long.*/
#define NBIOT_CFG_PDP_ADDR_LEN			15/*255.255.255.255*/
#define NBIOT_CFG_IP_LEN				150
#define NBIOT_CFG_PORT_LEN				4
#define NBIOT_CFG_URI_LEN				35
#define NBIOT_CFG_APN_LEN				32

typedef enum
{
	APPLICATION_ModemMode= 0,/*normal application mode*/
	FW_UPDATE_ModemMode,/*when modem need to do firmware mode*/
	RF_SIGNALLING_ModemMode, /*RF signalling compliancy test*/
	EMULATION_ModemMode, /*LPUART connect to PC emulator*/
	DISABLE_ModemMode,
	BACKOFF_ModemMode, /*LPUART connect to PC emulator*/
}NBIOT_ModemMode_t;

typedef enum
{
	NONE_NbiotTLVTag= 0,
	GET_INFO_NbiotTLVTag,
	GET_CONFIG_NbiotTLVTag,
	SET_CONFIG_NbiotTLVTag,
	GET_NETWORK_INFO_NbiotTLVTag,
	RESET_MODEM_NbiotTLVTag,
	SET_MODEM_MODE_NbiotTLVTag,
	GET_OPERATION_INFO_NbiotTLVTag,
	LWM2M_GET_CONFIG_NbiotTLVTag,
	LWM2M_SET_CONFIG_NbiotTLVTag,
	LWM2M_REQUEST_NbiotTLVTag,
	GKCOAP_GET_CONFIG_NbiotTLVTag,
	GKCOAP_SET_CONFIG_NbiotTLVTag,
	GKCOAP_REQUEST_NbiotTLVTag,
	LWMWM_LOG_NbiotTLVTag,
	MAX_NbiotTLVTag,
}NBIOT_NbiotTLVTag_t;

typedef enum
{
	MODEM_VERSION_NbiotInfo= 0,
	IMEI_NbiotInfo,
	IMSI_NbiotInfo,
	ICCID_NbiotInfo,
	PDP_ADDRESS_NbiotInfo,
	MAX_NbiotInfo,
}NBIOT_NbiotInfo_t;

/*TO EXPORT OUT TO CFG*/
typedef struct __attribute__((aligned(8)))/*compulsory alignment*/
{
	uint32_t checksum;/*compulsory checksum*/
	uint8_t  version;

	char apn[NBIOT_CFG_APN_LEN+ 1];/*to be set externally*/
	char pdpAddress[NBIOT_CFG_PDP_ADDR_LEN+ 1];/*to be set externally*/
	char imei[NBIOT_CFG_IMEI_LEN+ 1];
	char imsi[NBIOT_CFG_IMSI_LEN+ 1];
	char iccid[NBIOT_CFG_ICCID_LEN+ 1];

	NBIOT_ModemMode_t modemMode;
	uint32_t activeTime;
	uint32_t periodicTau;
	uint32_t edrx;
	uint32_t pagingTime;
	uint16_t delayBetweenCSQ_s;
	uint16_t maximumCSQRetry;
	uint32_t restartDelay_s;
	uint16_t maxUnknwonFailureAllowed;

	uint32_t rteActiveTime;
	uint32_t rtePeriodicTau;
	uint32_t rteEdrx;
	uint32_t rtePagingTime;

	uint32_t rteTotalLatency_ms;
	uint32_t rteTotalPingLatency_ms;
	int32_t rteSignalSampleCount;
	int32_t rteTotalRsrp;
	int32_t rteTotalRssi;
	int32_t rteTotalSinr;
	int32_t rteTotalRsrq;
	uint32_t rteTotalTxPower;

	NBIOT_Stats_t stats;//+16-8-6
	bool 	enableRRCDrop;
	uint16_t RRCDropPeriod_s;
	uint16_t downlinkWaitPeriod_s;
	bool enableAttachOnMagnetTamper;
	bool enableAttachOnPulserThreshold;
	float pulserThresholdValueForAttach_liter;
	bool enableAttachOnTiltTamper;
	uint16_t tiltTamperAttachBackoff_s;
	bool enableGkcoapOnAttach;
	bool enableLwm2mOnAttach;
	bool backoffOnCereg0;
	bool bypassBuckBoost;
	bool fastModemReboot;/*to reduce bc66 reset & 800mA spike*/
	uint16_t reattachWait_s;
	uint8_t reserve1[11];/*to add more param reduce this, thus no need to do backward compatible thing. remember to set the default value after config*/

	GKCOAP_t gkcoap;
	uint8_t reserve2[96];

	LWM2M_t lwm2m;
	uint8_t reserve3[96];/*to add more param reduce this, thus no need to do backward compatible thing. remember to set the default value after config*/
}NBIOT_t;

void NBIOT_TLVRequest(TLV_t *_tlv);
void NBIOT_Init(NBIOT_t *_config);
void NBIOT_Task(void);
uint8_t NBIOT_TaskState(void);

#endif /* NBIOT_NBIOT_H_ */
