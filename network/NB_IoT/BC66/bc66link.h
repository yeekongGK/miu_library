/*
 * bc66_link.h
 *
 *  Created on: 5 Jan 2021
 *      Author: muhammad.ahmad@georgekenet.net
 */

#ifndef NBIOT_BC66LINK_H_
#define NBIOT_BC66LINK_H_

#include "main.h"

#define BC66LINK_CFG_PRODUCTION_DELAY_S				10
#define BC66LINK_CFG_TX_BUF_SIZE 					(1024/*exaggerated size*/+ 1024/*lwm2m max response*/)
#define BC66LINK_CFG_MAX_TRX_QUEUE 					1
#define BC66LINK_CFG_DFT_BAUDRATE					9600
#define BC66LINK_CFG_LWM2M_RESPONSE_MAX_LEN			1024

#define BC66LINK_CFG_LWM2M_RECOVERY_WAIT_S			60/*we need to wait few seconds before sending lwupdate, for ecl2 this can be very long*/

typedef enum
{
	INITIAL_Bc66LinkState= 100,
	REALLY_RESET_Bc66LinkState,
	INIT_LINK_Bc66LinkState,
	CHECK_MODE_Bc66LinkState,
	STALL_Bc66LinkState,
	PRODUCTION_BREAKPOINT_Bc66LinkState,
	RF_SIGNALLING_Bc66LinkState,
	CONFIGURE_Bc66LinkState,
	ATTACHMENT_Bc66LinkState,
	WAIT_REQUEST_Bc66LinkState,
	PROCESS_REQUEST_Bc66LinkState,
	EXIT_SLEEP_Bc66LinkState,
	ENTER_SLEEP_Bc66LinkState,
	DISABLE_Bc66LinkState,

	/*below are experimental*/
	SET_BAND_LIST_Bc66LinkState,
	ENABLE_EXT_ERROR_REPORTING_Bc66LinkState,
	RAI_UDP_OPEN_Bc66LinkState,
	SET_TIME_ZONE_Bc66LinkState,
	FACTORY_RESET_Bc66LinkState,
	MAX_Bc66LinkState,
} BC66LINK_State_t;

typedef enum
{
	CHECK_ECHO_InitLinkState= 0,
	FIRST_AT_InitLinkState,
	SET_BC66_BAUD_InitLinkState,
	SET_MCU_BAUD_InitLinkState,
	DISABLE_ECHO_InitLinkState,
	CMEERROR_InitLinkState,
	VERSION_InitLinkState,
	CHECK_VERSION_InitLinkState,
	SNO_InitLinkState,
	END_InitLinkState,
}BC66LINK_InitLinkState_t;

typedef enum
{
	MIN_FUNCT_ConfigureState= 0,
	APN_ConfigureState,
	RESET_ConfigureState,
	WAIT_RESET_ConfigureState,
	DISABLE_ECHO2_ConfigureState,
	CMEERROR_ConfigureState,
	LED_PATTERN_ConfigureState,
	PSM_URC_ConfigureState,
	PDP_URC_ConfigureState,
	CEREG_URC_ConfigureState,
	CSCON_URC_ConfigureState,
	TAU_ACTIVETIME_ConfigureState,
	PAGING_EDRX_ConfigureState,
	SET_SLEEP_MODE_ConfigureState,
	AT_ConfigureState,
	END_ConfigureState,
}BC66LINK_ConfigureState_t;

typedef enum
{
	JIC_WAKEUP_AttachmentState= 0,
	FULL_FUNCT_AttachmentState,
	WAIT_FULL_FUNCT_AttachmentState,
	SET_BAND_AttachmentState,
	CHECK_SIM_AttachmentState,
	CCID_AttachmentState,
	IMSI_AttachmentState,
	CSQ_AttachmentState,
	WAIT_AttachmentState,
	CHECK_AttachmentState,
	BS_CLK_SYNC_AttachmentState,
	RAI_INIT_AttachmentState,
	END_AttachmentState,
}BC66LINK_AttachmentState_t;

typedef enum
{
	SET_MIN_FUNCT_ProdBreakState= 0,
	PWR_DOWN_ProdBreakState,
	PAUSE_1_ProdBreakState,
	PWR_UP_ProdBreakState,
	PAUSE_2_ProdBreakState,
	CLEAR_FLAGS_ProdBreakState,
	DISABLE_ECHO_ProdBreakState,
	END_ProdBreakState,
}BC66LINK_ProdBreakState_t;

typedef enum
{
	INIT_RFSignallingState= 0,
	ENTER_RFSignallingState,
	START_RFSignallingState,
	CHECK_RFSignallingState,
	EXIT_RFSignallingState,
	END_RFSignallingState,
}BC66LINK_RFSignallingState_t;

typedef enum
{
	PROCESSING_ProcessRequestState= 0,
	END_ProcessRequestState,
}BC66LINK_ProcessRequestState_t;

typedef enum
{
	START_ExitSleepState= 0,
	WAIT_ExitSleepState,
	DISABLE_SLEEP_ExitSleepState,
	WAIT_CSCON_ExitSleepState,
	END_ExitSleepState,
}BC66LINK_ExitSleepState_t;

typedef enum
{
	START_EnterSleepState= 0,
	WAIT_EnterSleepState,
	GET_QENG_EnterSleepState,
	GET_CLK_EnterSleepState,
	ENTER_PSM_EnterSleepState,
	ENTER_DEEP_SLEEP_EnterSleepState,
	END_EnterSleepState,
}BC66LINK_EnterSleepState_t;

typedef enum
{
	SET_MIN_FUNCT_DisableState= 0,
	PWR_DOWN_DisableState,
	SET_PIN_OD_DisableState,
	CHECK_FOR_ENABLE_DisableState,
	END_DisableState,
}BC66LINK_DisableState_t;

typedef enum
{
	UNREGISTERED_BC66LinkNetwork= 0,
	REGISTERED_AS_HOME_BC66LinkNetwork= 1,
	REGISTERING_BC66LinkNetwork= 2,
	REGISTRATION_DENIED_BC66LinkNetwork= 3,
	REGISTRATION_UNKNWON_BC66LinkNetwork= 4,
	REGISTERED_AS_ROAMING_BC66LinkNetwork= 5
}BC66LINK_Network_t;

typedef struct
{
	bool enable;
	BC66LINK_State_t state;
	uint8_t subState;
	bool echoEnabled;

	struct
	{
		bool ufotas01Received;//temp
		bool isWaitingForUfotas01;
		uint16_t cgpaddrCnt;//temp
		uint16_t cgpaddrRetried;//temp
		bool cmeErrorReceived;
		uint16_t cmeErrorType;
		//bool inPowerSavingMode;
		bool cpsmsIsEnabled;
		uint8_t cpsmsTauValue;
		uint8_t cpsmsActiveTimeValue;
		uint8_t rssi;
		bool rssiQueryReceived;
		uint16_t csqAttemptCount;
		uint8_t ceregAttemptCount;
		uint8_t regDeniedCount;
		bool isAttached;
		bool resetPending;

		/*quectel flag*/
		bool bResetExpected;/*we use this to rule out unintended bc66 reset*/
		bool isConnected;
		bool inPSM;
		bool PSMExitTriggered;
		bool inDeepSleep;
		bool inhibitSleep;
		bool sendLwUpdate;/*to resume lwm2m, jic*/
		bool qlwregReceived;
		uint16_t reattachWaitCounter;
		bool qiOpened;

		struct
		{
			uint8_t actType;
			uint32_t requested;
			uint32_t provided;
			uint32_t pagingTime;
		}eDrx;

		struct
		{
			BC66LINK_Network_t regStatus;
			uint16_t trackingAreaCode;
			uint32_t cellId;
			uint8_t accessTechnology;
			uint8_t rejectType;
			uint8_t rejectCause;
			uint8_t activeTime_raw;
			uint8_t periodicTau_raw;
			uint32_t activeTime;
			uint32_t periodicTau;
		}cereg;

		struct
		{
			uint8_t year;
			uint8_t month;
			uint8_t day;
			uint8_t hour;
			uint8_t minute;
			uint8_t second;
			int8_t gmt;
		}time;

		char ip[150];/*max 150 bytes*/
	}status;

	struct
	{
		uint8_t QCOAPSEND;
		struct
		{
			struct
			{
				uint8_t type;
				uint16_t rspCode;
				uint32_t messageId;
				uint32_t len;/*max is 1400*/
				char *data;/*hex string*/
			}rsp;
		}QCOAPURC;

		uint8_t QLWREG;
		uint8_t QLWDEREG;
		uint32_t QLWADDOBJ[15];/*max 15 objects*/
		uint8_t QLWADDOBJCount;
		uint8_t CSCON;
		uint8_t IP[45];
		uint8_t QLWWRRSP;
		uint8_t QLWRDRSP;
		uint8_t QLWOBSRSP;
		uint8_t QLWEXERSP;
		uint8_t QLWNOTIFY;
		uint8_t QLWUPDATE;
		struct
		{
			uint8_t ping;
			uint8_t buffer;
			bool write;
			bool read;
			bool execute;
			bool observe;
			uint8_t bs_finished;
			//bool report;
			bool report_ack;
			uint32_t report_messageId;
			uint8_t report_ack_status;
			uint32_t report_ack_messageId;
			uint8_t lwstatus;
			uint8_t lifetime_changed;
			uint8_t binding_changed;
			uint8_t min_period_changed;
			uint8_t max_period_changed;
			uint8_t recovered;
		}QLWURC;

		struct
		{
			uint32_t earfcn;
			uint8_t earfcnOffset;
			uint16_t pci;
			uint32_t cellID;
			int16_t rsrp;
			int16_t rsrq;
			int16_t rssi;
			int16_t sinr;
			uint16_t band;
			uint16_t tac;
			uint8_t ecl;
			int16_t txPwr;
			uint8_t oprMode;

			uint32_t neighborEarfcn;
			uint8_t neighborEarfcnOffset;
			uint16_t neighborPci;
			int16_t neighborRsrp;

			uint8_t rlcUlBler;
			uint8_t rlcDlBler;
			uint8_t macUlBler;
			uint8_t macDlBler;
			uint32_t macUlTotalBytes;
			uint32_t macDlTotalBytes;
			uint32_t macUlTotalHarqTx;
			uint32_t macDlTotalHarqTx;
			uint32_t macUlTotalHarqReTx;
			uint32_t macDlTotalHarqReTx;
			uint32_t rlcUlTput;
			uint32_t rlcDlTput;
			uint32_t macUlTput;
			uint32_t macDlTput;

			uint32_t sleepDuration;
			uint32_t rxTime;
			uint32_t txTime;

			uint8_t emmState;
			uint8_t emmMode;
			uint8_t plmnState;
			uint8_t plmnType;
			uint32_t selectePlmn;
			uint32_t err;
		}QENG;
	}urc;

	uint8_t coapContext;
	struct
	{
		void (*Response)(void *);
		void (*Request)(void *);
		void (*Recovery)(void *);
	}coapCallback;

	struct
	{
		void (*Response)(void *);
		void (*Request)(void *);
		void (*Recovery)(void *);
	}coapCallback2;/*to cater for Lwm2m coap*/

	struct
	{
		void (*WriteRequest)(void *);
		void (*ReadRequest)(void *);
		void (*ExecuteRequest)(void *);
		void (*ObserveRequest)(void *);
		void (*Recovery)(void *);
		void (*Register)(void *);
	}lwm2mCallback;

	struct
	{
		bool enable;
		uint32_t channel;
		uint8_t dBm;
		uint32_t currChannel;
		uint8_t currDBm;
	}rfSignalling;

	uint8_t BC66Version[32];/*12+ 1 null, add some buffer just in case*/
}BC66LINK_t;

typedef enum
{
	COAP_Bc66LinkTransactionType= 0,
	LWM2M_Bc66LinkTransactionType,
} BC66LINK_Transaction_Type_t;

typedef enum
{
//	CREATE_Bc66LinkCoap= 0,
//	DELETE_Bc66LinkCoap,
//	ADD_RESOURCE_Bc66LinkCoap,
//	CONFIG_HEAD_Bc66LinkCoap,
//	CONFIG_OPTION_Bc66LinkCoap,
//	SEND_Bc66LinkCoap,
//	SEND_STATUS_Bc66LinkCoap,
//	CONFIG_COMMAND_Bc66LinkCoap,
//	ALI_SIGN_Bc66LinkCoap,
	GK_POST_Bc66LinkCoap,
	LWM2M_GET_Bc66LinkCoap,
} BC66LINK_Coap_t;

typedef enum
{
	CONFIGURE_Bc66LinkLwm2m= 0,
	REGISTER_Bc66LinkLwm2m,
	POSTREGISTER_Bc66LinkLwm2m,
	UPDATE_Bc66LinkLwm2m,
	DEREGISTER_Bc66LinkLwm2m,
	ADD_OBJECT_Bc66LinkLwm2m,
	DELETE_OBJECT_Bc66LinkLwm2m,
	WRITE_RESPONSE_Bc66LinkLwm2m,
	READ_RESPONSE_Bc66LinkLwm2m,
	EXECUTE_RESPONSE_Bc66LinkLwm2m,
	OBSERVE_RESPONSE_Bc66LinkLwm2m,
	NOTIFY_Bc66LinkLwm2m,
	READ_DATA_Bc66LinkLwm2m,
	STATUS_Bc66LinkLwm2m,
} BC66LINK_Lwm2m_t;

typedef enum
{
	REGISTERED_Lwm2mRegStatus= 0,
	REJECTED_Lwm2mRegStatus= 1,
	ONGOING_Lwm2mRegStatus= 0xFF,
	UNKNWON_Lwm2mRegStatus= 0xFE,
} BC66LINK_Lwm2mRegStatus;

typedef enum
{
	SUCCESS_TransactionStatus= 0,
	ERROR_TransactionStatus,
	TIMEOUT_TransactionStatus,
} BC66LINK_Transaction_Status;

typedef enum
{
	NONE_Bc66LinkCoapEncryption= 0,
	PSK_Bc66LinkCoapEncryption= 1,
}BC66LINK_Coap_Encryption_Mode_t;

typedef enum
{
	CON_Bc66LinkCoapMsgType= 0,
	NON_Bc66LinkCoapMsgType= 1,
	ACK_Bc66LinkCoapMsgType= 2,
	RST_Bc66LinkCoapMsgType= 3,
}BC66LINK_Coap_MsgType_t;

typedef enum
{
	GET_Bc66LinkCoapMethod= 1,
	POST_Bc66LinkCoapMethod= 2,
	PUT_Bc66LinkCoapMethod= 3,
	DELETE_Bc66LinkCoapMethod= 4,
}BC66LINK_Coap_Method_t;

//typedef struct
//{
//	uint16_t port;
//	BC66LINK_Coap_Encryption_Mode_t mode;
//	uint8_t *pskId;/*max 150 bytes*/
//	uint8_t *psk;/*max 256 bytes, must be even and hex string*/
//}BC66LINK_Coap_Create_t;
//
//typedef struct
//{
//}BC66LINK_Coap_Delete_t;
//
//typedef struct
//{
//	uint16_t length;
//	char * resourceName;
//}BC66LINK_Coap_AddResource_t;
//
//typedef struct
//{
//	uint8_t mode;
//	uint16_t msgid;
//	uint8_t tokenLength;
//	char *token;
//}BC66LINK_Coap_ConfigHead_t;
//
//typedef struct
//{
//	uint8_t optCount;/*1-12*/
//	uint8_t optName[12];
//	char* optValue[12];
//}BC66LINK_Coap_ConfigOption_t;
//
//typedef struct
//{
//	uint8_t type;
//	uint16_t methodRspcode;
//	char *ipAddr;
//	uint16_t port;
//	uint16_t length;
//	char *data;
//}BC66LINK_Coap_Send_t;
//
//typedef struct
//{
//}BC66LINK_Coap_SendStatus_t;
//
//typedef struct
//{
//	uint8_t showra;
//	uint8_t showrspopt;
//}BC66LINK_Coap_ConfigCommand_t;
//
//typedef struct
//{
//	char *deviceId;
//	char *deviceName;
//	char *deviceSecretKey;
//	char *productKey;
//}BC66LINK_Coap_AliSign_t;

typedef struct
{
	char *uri;
	char *ipAddr;
	uint16_t port;
	uint16_t localPort;
	uint16_t length;
	char *data;
	char *imei;
	uint8_t packetType;
	uint16_t rxLength;
	char *rxData;
}BC66LINK_Coap_GKPost_t;

typedef struct
{
	bool created;
	uint16_t localPort;
	char *serverIP;
	uint16_t serverPort;
	char *serverURI;
	BC66LINK_Coap_Encryption_Mode_t encMode;
	uint8_t *pskId;/*max 150 bytes*/
	uint8_t *psk;/*max 256bytes, must be even and hex string*/
	uint16_t block2Code;
	uint16_t rxLength;
	char *rxData;
}BC66LINK_Coap_Lwm2mGet_t;

typedef struct
{
	union
	{
//		BC66LINK_Coap_Create_t Create;
//		BC66LINK_Coap_Delete_t Delete;
//		BC66LINK_Coap_AddResource_t AddResource;
//		BC66LINK_Coap_ConfigHead_t ConfigHead;
//		BC66LINK_Coap_ConfigOption_t ConfigOption;
//		BC66LINK_Coap_Send_t Send;
//		BC66LINK_Coap_SendStatus_t SendStatus;
//		BC66LINK_Coap_ConfigCommand_t ConfigCommand;
//		BC66LINK_Coap_AliSign_t AliSign;
		BC66LINK_Coap_GKPost_t GKPost;
		BC66LINK_Coap_Lwm2mGet_t Lwm2mGet;
	};
}BC66LINK_Coap_Transaction_t;

typedef struct
{
	uint8_t type;
	uint16_t rspCode;
	uint32_t messageId;
	uint32_t len;/*max is 1400*/
	char *data;/*hex string*/
}BC66LINK_Coap_Response_t;

typedef struct
{
	uint8_t type;
	char* method;
	uint32_t messageId;
	char *mode;/*hex string*/
	uint32_t tokenLen;
	char *token;/*hex string*/
	uint32_t optionName;
	uint32_t optionValue;
	uint32_t len;/*max is 1400*/
	char *data;/*hex string*/
}BC66LINK_Coap_Request_t;

typedef struct
{
	uint8_t state;
}BC66LINK_Coap_Recovery_t;

typedef enum
{
	PSK_Bc66LinkLwm2mSecurityMode= 0,
	NONE_Bc66LinkLwm2mSecurityMode= 3,
}BC66LINK_Lwm2m_Security_Mode_t;

typedef enum
{
	CONTENT_Bc66LinkLwm2mRequestResult= 1,/*correct result*/
	CHANGED_Bc66LinkLwm2mRequestResult= 2,/*indicate correct result for execute request*/
	BADREQUEST_Bc66LinkLwm2mRequestResult= 11,
	UNAUTHORIZED_Bc66LinkLwm2mRequestResult= 12,
	NOTFOUND_Bc66LinkLwm2mRequestResult= 13,
	METHODNOTALLOWED_Bc66LinkLwm2mRequestResult= 14,
	NOTACCEPTABLE_Bc66LinkLwm2mRequestResult= 15,
}BC66LINK_Lwm2m_Request_Result_t;

typedef enum
{
	STRING_Bc66LinkLwm2mValueType= 1,
	OPAQUE_Bc66LinkLwm2mValueType= 2,
	INTEGER_Bc66LinkLwm2mValueType= 3,
	FLOAT_Bc66LinkLwm2mValueType= 4,
	BOOLEAN_Bc66LinkLwm2mValueType= 5,
}BC66LINK_Lwm2m_Value_Type_t;

typedef enum
{
	OBSERVE_Bc66LinkLwm2mObserveType= 0,
	CANCEL_Bc66LinkLwm2mObserveType= 1,
}BC66LINK_Lwm2m_Observe_Type_t;

typedef struct
{
	bool enableBootstrap;
	uint8_t *serverIP;/*max 150 bytes*/
	uint16_t serverPort;
	uint8_t *endpointName;/*max 150 bytes*/
	uint32_t lifetime;
	BC66LINK_Lwm2m_Security_Mode_t securityMode;
	uint8_t *pskId;/*max 150 bytes*/
	uint8_t *psk;/*max 256bytes, must be even and hex string*/
}BC66LINK_Lwm2m_Configure_t;

typedef struct
{
	uint8_t *pingServerIP;/*max 150 bytes*/
}BC66LINK_Lwm2m_PostRegister_t;

typedef struct
{


}BC66LINK_Lwm2m_Update_t;

typedef struct
{
	uint32_t objectId;/*max 15 objects*/
	uint32_t instanceId;/*of which max 4 instances*/
	uint32_t noOfResource;/*of which max 14 resources*/
	uint32_t resourceId[14];
}BC66LINK_Lwm2m_Add_Object_t;

typedef struct
{
	uint32_t messageId;
	uint32_t objectId;/*for diag*/
	uint32_t instanceId;/*for diag*/
	uint32_t resourceId;/*for diag*/
	BC66LINK_Lwm2m_Request_Result_t	result;
}BC66LINK_Lwm2m_Write_Response_t;

typedef struct
{
	uint32_t messageId;
	BC66LINK_Lwm2m_Request_Result_t	result;
	uint32_t objectId;
	uint32_t instanceId;
	uint32_t resourceId;
	BC66LINK_Lwm2m_Value_Type_t valueType;
	uint16_t len;
	char *value;/*string*/
	uint16_t index;
}BC66LINK_Lwm2m_Read_Response_t;

typedef struct
{
	uint32_t messageId;
	BC66LINK_Lwm2m_Request_Result_t	result;
	uint32_t objectId;
	uint32_t instanceId;
	uint32_t resourceId;
	BC66LINK_Lwm2m_Value_Type_t valueType;
	uint16_t len;
	char *value;/*string*/
	uint16_t index;
	bool observe;
}BC66LINK_Lwm2m_Observe_Response_t;

typedef struct
{
	uint32_t messageId;
	uint32_t objectId;/*for diag*/
	uint32_t instanceId;/*for diag*/
	uint32_t resourceId;/*for diag*/
	BC66LINK_Lwm2m_Request_Result_t	result;
}BC66LINK_Lwm2m_Execute_Response_t;

typedef struct
{
	uint32_t lifetime;/*for notify*/
	uint32_t objectId;
	uint32_t instanceId;
	uint32_t resourceId;
	BC66LINK_Lwm2m_Value_Type_t valueType;
	uint16_t len;
	char *value;/*string*/
	uint16_t index;
	uint8_t ack;
	uint8_t raiFlag;
	uint8_t objectIndex;/*for own reference, not used by bc66*/
	uint8_t resourceIndex;/*for own reference, not used by bc66*/
	uint8_t ackStatus;
}BC66LINK_Lwm2m_Notify_t;

typedef struct
{
	union
	{
		BC66LINK_Lwm2m_Configure_t Configure;
		BC66LINK_Lwm2m_PostRegister_t PostRegister;
		BC66LINK_Lwm2m_Update_t Update;
		BC66LINK_Lwm2m_Add_Object_t AddObject;
		BC66LINK_Lwm2m_Write_Response_t WriteResponse;
		BC66LINK_Lwm2m_Read_Response_t ReadResponse;
		BC66LINK_Lwm2m_Observe_Response_t ObserveResponse;
		BC66LINK_Lwm2m_Execute_Response_t ExecuteResponse;
		BC66LINK_Lwm2m_Notify_t Notify;
	};
}BC66LINK_Lwm2m_Transaction_t;

typedef struct
{
	BC66LINK_Transaction_Type_t type;
	union
	{
		BC66LINK_Coap_t coapType;
		BC66LINK_Lwm2m_t lwm2mType;
	};

	union
	{
		BC66LINK_Coap_Transaction_t coap;
		BC66LINK_Lwm2m_Transaction_t lwm2m;
	};

	uint8_t state;
	char *coapURI;
	uint8_t *txBuffer;
	uint16_t txLen;
	uint8_t *rxBuffer;
	uint16_t rxLen;
	bool transactionInQueue;
	bool transactionInProgress;
	bool transactionCompleted;
	bool txSent;
	bool rxReceived;
	uint32_t 	timeout_ms;
	BC66LINK_Transaction_Status status;
	void (*TransactionProcess)(void *);
	void (*TransactionCompletedCb)(void);
	BC66LINK_t *link;
}BC66LINK_Transaction_t;

typedef struct
{
	uint32_t messageId;
	uint32_t objectId;
	uint32_t instanceId;
	uint32_t resourceId;
	BC66LINK_Lwm2m_Value_Type_t valueType;
	uint32_t len;
	uint8_t *value;
	uint32_t index;
	uint8_t flag;
}BC66LINK_Lwm2m_Request_t;

typedef struct
{
	uint8_t state;
}BC66LINK_Lwm2m_Recovery_t;

typedef struct
{
	uint8_t state;
}BC66LINK_Lwm2m_Register_t;

extern BC66LINK_t sBC66Link;

extern const char *rx_reset[];
extern const char *rx_resetFull[];
extern const char *rx_reboot_part0[];
extern const char *rx_reboot_part1[];
extern const char *rx_reboot_part2[];
extern const char *rx_EchoATE0[];
extern const char *rx_ATI[];
extern const char *rx_EchoOK[];
extern const char *rx_OK[];
extern const char *socket_OK[];
extern const char *socketRx_OK[];
extern const char *rx_CIMI[];
extern const char *rx_CGMR[];

uint8_t BC66LINK_QueueDepth(void);
ErrorStatus BC66LINK_Enqueue(BC66LINK_Transaction_t *_trx);
BC66LINK_Transaction_t *BC66LINK_Dequeue(void);
void BC66LINK_COAP_SetResponseCallback(void (*Response)(void *));
void BC66LINK_COAP_SetRequestCallback(void (*Request)(void *));
void BC66LINK_COAP_SetRecoveryCallback(void (*Recovery)(void *));
void BC66LINK_COAP_SetResponseCallback2(void (*Response)(void *));
void BC66LINK_COAP_SetRequestCallback2(void (*Request)(void *));
void BC66LINK_COAP_SetRecoveryCallback2(void (*Recovery)(void *));
void BC66LINK_LWM2M_SetWriteRequestCallback(void (*WriteRequest)(void *));
void BC66LINK_LWM2M_SetReadRequestCallback(void (*ReadRequest)(void *));
void BC66LINK_LWM2M_SetExecuteRequestCallback(void (*ExecuteRequest)(void *));
void BC66LINK_LWM2M_SetObserveRequestCallback(void (*ObserveRequest)(void *));
void BC66LINK_LWM2M_SetRecoveryCallback(void (*Recovery)(void *));
void BC66LINK_LWM2M_SetRegisterCallback(void (*Register)(void *));
void BC66LINK_StatusInit(void);
void BC66LINK_Init(void);
void BC66LINK_Enable(bool _enable);
bool BC66LINK_IsEnable(void);
bool BC66LINK_SIMIsPresent(void);
bool BC66LINK_NetworkIsAvailable(void);
bool BC66LINK_NetworkIsRegistered(void);
uint8_t BC66LINK_RegistrationStatus();
uint8_t BC66LINK_NetworkDeniedCount(void);
bool BC66LINK_TransmissionIsReady(void);
bool BC66LINK_ModemDisabled(void);
BC66LINK_Lwm2mRegStatus BC66LINK_LWM2MRegistrationState(void);
bool BC66LINK_IsInBackOff(void);
bool BC66LINK_IsInPSM(void);
uint8_t BC66LINK_GetState(void);
uint8_t BC66LINK_GetSubState(void);
void BC66LINK_SaveContext(void);
void BC66LINK_ResetModem(void);
void BC66LINK_ResetJob(void);
void BC66LINK_CancelTransaction(void);
void BC66LINK_RequestRFSignalling(bool _enable, uint32_t _channel, uint8_t _dbm);
void BC66LINK_Task(void);
uint8_t BC66LINK_SyncRssi(void);
uint8_t BC66LINK_GetRssi(void);
void BC66LINK_TurnOff(void);
void BC66LINK_TurnOn(void);
uint8_t BC66LINK_TaskState(void);

#endif /* NBIOT_BC66LINK_H_ */
