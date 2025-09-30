/*
 * nbiotcoap.c
 *
 *  Created on: 14 Feb 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#include "common.h"
#include "gkcoap.h"
#include "gkcoappacket.h"
#include "bc66link.h"
#include "logger.h"
#include "printf.h"
#include "sensor.h"
#include "msg.h"

#define DBG_Print

static GKCOAP_t *pConfig;
__IO static BC66LINK_Transaction_t eBc66Trx= {
		.type= COAP_Bc66LinkTransactionType,
		.timeout_ms= 60000/*long timeout for ecl2*/,//20000,
		.TransactionCompletedCb= NULL,
};
__IO static LOGGER_Transaction_t eLoggerTrx;
__IO static uint8_t pucBuffer[GKCOAP_CFG_BUFFER_SIZE];
__IO static uint8_t pucLogBuffer[GKCOAP_CFG_LOG_BUFFER_SIZE];
__IO static uint8_t pucMsgBuf[GKCOAP_CFG_MSG_BUFFER_SIZE];
__IO static GKCOAP_COAP_Request_t eRequest;

static void GKCOAP_SetTimeout_s(uint32_t _seconds)
{
	if(0!= _seconds)
	{
		SYS_Sleep(NBIOT_GKCOAP_TaskId, _seconds);
	}
}

static bool GKCOAP_IsTimeout(void)
{
	return (true== SYS_IsAwake(NBIOT_GKCOAP_TaskId));
}

static void GKCOAP_Report_SetTimeout_s(uint32_t _seconds)
{
	DBG_Print("GKCOAP_Report_SetTimeout: %d sec\r\n", _seconds);
	if(0!= _seconds)
	{
		SYS_Sleep(NBIOT_GKCOAP_REPORT_TaskId, _seconds* 1000);
	}
}

static bool GKCOAP_Report_IsTimeout(void)
{
	return (true== SYS_IsAwake(NBIOT_GKCOAP_REPORT_TaskId));
}

void GKCOAP_Link_HardReset(void)
{
	BC66LINK_ResetModem();
}

void GKCOAP_Response(void *_param)
{
	BC66LINK_Coap_Response_t *_response= (BC66LINK_Coap_Response_t *)_param;

	pConfig->rte.responseStatus= 0;
}

void GKCOAP_Request(void *_param)
{
	BC66LINK_Coap_Request_t *_request= (BC66LINK_Coap_Request_t *)_param;
}

void GKCOAP_Recovery(void *_param)
{
	BC66LINK_Coap_Recovery_t *_recovery= (BC66LINK_Coap_Recovery_t *)_param;

	DBG_Print("GKCOAP Recovery.\r\n");

	if(0!= _recovery->state)/*fail, need to re-create using AT+QCOAPCREATE after failure*/
	{
		DBG_Print("GKCOAP Recovery triggered.\r\n");

		if(0xFF== pConfig->rte.sendStatus)
		{
			pConfig->rte.sendStatus= 1;/*failed*/
		}

		if(0xFF== pConfig->rte.responseStatus)
		{
			pConfig->rte.responseStatus= 1;/*failed*/
		}

		if(true== eBc66Trx.transactionInQueue)
		{
			eBc66Trx.transactionInQueue= false;
		}

		if(true== eBc66Trx.transactionInProgress)
		{
			eBc66Trx.transactionInProgress= false;
		}
	}
}

void GKCOAP_Register_Request(void)
{
	pConfig->rte.reg.isRequested= true;
}

bool GKCOAP_Register_Completed(void)
{
	return pConfig->rte.reg.isCompleted;
}

void GKCOAP_Register_App(void)
{
	switch(pConfig->rte.reg.state)
	{
		case GKCOAP_REG_STATE_IDLE:
			if(true== pConfig->rte.reg.isRequested)
			{
				pConfig->rte.reg.isRequested= false;
				pConfig->rte.reg.isCompleted= false;
				pConfig->rte.reg.retryCount= 0;
				pConfig->rte.reg.state= GKCOAP_REG_STATE_SEND_PACKET;
			}
			break;

		case GKCOAP_REG_STATE_SEND_PACKET:
			{
				uint16_t _len= GKCOAPPKT_PopulateRegisterPacket((uint8_t *)pucBuffer);
				eBc66Trx.coapType= GK_POST_Bc66LinkCoap;
				eBc66Trx.timeout_ms= 60000/*long timeout for ecl2*/;//40000;/*longer timeout for reg as per racing condition with lwm2m*/
				eBc66Trx.coap.GKPost.uri= pConfig->serverURI;
				eBc66Trx.coap.GKPost.ipAddr= pConfig->serverIP;
				eBc66Trx.coap.GKPost.port= pConfig->serverPort;
				eBc66Trx.coap.GKPost.localPort= pConfig->localPort;
				eBc66Trx.coap.GKPost.length= _len;
				eBc66Trx.coap.GKPost.data= (char *)pucBuffer;
				eBc66Trx.coap.GKPost.imei= config.nbiot.imei;
				eBc66Trx.coap.GKPost.packetType= pConfig->packetType;
				eBc66Trx.coap.GKPost.rxLength= 0;
				eBc66Trx.coap.GKPost.rxData= (char *)pucBuffer;/*we can use same buffer as tx, as the txdata will be copied to bc66link own buffer*/

				if(SUCCESS== BC66LINK_Enqueue(&eBc66Trx))
				{
					pConfig->rte.sendStatus= 0xFF;/*pending*/
					pConfig->rte.responseStatus= 0xFF;/*pending*/
					pConfig->rte.reg.state= GKCOAP_REG_STATE_CHECK_SEND_PACKET;
				}
			}
			break;

		case GKCOAP_REG_STATE_CHECK_SEND_PACKET:
			if((0== pConfig->rte.sendStatus)&& (0== pConfig->rte.responseStatus))
			{
				if(SUCCESS== GKCOAPPKT_ProcessDownlinkPacket(eBc66Trx.coap.GKPost.rxData, eBc66Trx.coap.GKPost.rxLength))
				{
					pConfig->rte.reg.state= GKCOAP_REG_STATE_COMPLETE;
				}
				else
				{
					pConfig->rte.reg.state= GKCOAP_REG_STATE_SET_BACKOFF;
				}
			}
			else if((1== pConfig->rte.sendStatus)|| (1== pConfig->rte.responseStatus))
			{
				pConfig->rte.reg.state= GKCOAP_REG_STATE_SET_BACKOFF;
			}
			break;

		case GKCOAP_REG_STATE_SET_BACKOFF:
			if(pConfig->rte.reg.retryCount< pConfig->retryMax)
			{
				pConfig->rte.reg.retryCount++;
				DBG_Print("GKCOAP Register Backoff(%d/ %d).\r\n", pConfig->rte.reg.retryCount, pConfig->retryMax);
				GKCOAP_SetTimeout_s(UTILI_GetRandom(pConfig->retryBackoffMin_s, pConfig->retryBackoffMax_s));
				pConfig->rte.reg.state= GKCOAP_REG_STATE_BACKOFF;
			}
			else
			{
				/*failed*/
				pConfig->rte.reg.state= GKCOAP_REG_STATE_FAILED;
			}
			break;

		case GKCOAP_REG_STATE_BACKOFF:
			if(true== GKCOAP_IsTimeout())
			{
				pConfig->rte.reg.state= GKCOAP_REG_STATE_SEND_PACKET;/*retry*/
			}
			break;

		case GKCOAP_REG_STATE_FAILED:
			DBG_Print("GKCOAP Register Failed.\r\n");
			pConfig->rte.reg.state= GKCOAP_REG_STATE_IDLE;
			break;

		case GKCOAP_REG_STATE_COMPLETE:
			DBG_Print("GKCOAP Register Completed.\r\n");
			pConfig->rte.reg.isCompleted= true;
			GKCOAP_Report_Request();
			pConfig->rte.reg.state= GKCOAP_REG_STATE_IDLE;
			break;

		default:
			break;
	}
}

void GKCOAP_Report_Request(void)
{
	pConfig->rte.report.isRequested= true;
}

void GKCOAP_Report_Completed(void)
{
	pConfig->rte.report.isCompleted= true;
}

void GKCOAP_Report_App(void)
{
	switch(pConfig->rte.report.state)
	{
		case GKCOAP_REPORT_STATE_IDLE:
			if(true== pConfig->rte.report.isRequested)
			{
				pConfig->rte.report.isRequested= false;
				pConfig->rte.report.isCompleted= false;
				pConfig->rte.report.retryCount= 0;
				pConfig->rte.report.state= GKCOAP_REPORT_CHECK_LOG;
			}
			break;

		case GKCOAP_REPORT_CHECK_LOG:
			DBG_Print("GKCOAP availLogs:%d, reportedLogs:%d.\r\n",LOGGER_DeviceLog_GetLogCount(), pConfig->rte.report.noOfLogsReported);
			if(LOGGER_DeviceLog_GetLogCount()> pConfig->rte.report.noOfLogsReported)
			{
				if(LOGGER_DeviceLog_GetLogFloor()> pConfig->rte.report.noOfLogsReported)
				{
					pConfig->rte.report.noOfLogsReported= LOGGER_DeviceLog_GetLogFloor();/*adjusted so not to transmit rubbish logs. TODO:correct name*/
				}
				pConfig->rte.report.state= GKCOAP_REPORT_FETCH_LOG;
			}
			else
			{
				pConfig->rte.report.state= GKCOAP_REPORT_STATE_IDLE;

				if(Suspend_FotaState== pConfig->rte.fota.state)
				{
					pConfig->rte.fota.state= pConfig->rte.fota.prevState;/*resume fota*/
				}
			}
			break;

		case GKCOAP_REPORT_FETCH_LOG:
			{
				__IO LOGGER_Transaction_t _trx;
				uint32_t _availableLogs= LOGGER_DeviceLog_GetLogCount()- pConfig->rte.report.noOfLogsReported;

				pConfig->rte.report.noOfLogsToReport= (_availableLogs<= GKCOAP_CFG_LOG_COUNT_MAX)? _availableLogs: GKCOAP_CFG_LOG_COUNT_MAX;

				_trx.logIndex= pConfig->rte.report.noOfLogsReported;
				_trx.logCount= pConfig->rte.report.noOfLogsToReport;
				_trx.transactionType= TRANSACTION_TYPE_DEVICELOG_READ;
				_trx.logBuffer= pucLogBuffer;
				_trx.TransactionCompletedCb= NULL;
				LOGGER_Enqueue(&_trx);
				while(false== _trx.transactionCompleted)
				{
					LOGGER_Task();/*sue me for doing this here :p*/
				}
				/*we can use same in and out buffer because out is smaller than in.*/
				DBG_Print("GKCOAP logIndex:%d, logCount:%d.\r\n", _trx.logIndex, _trx.logCount);
				GKCOAPUTIL_DeviceToGKCoapLogs(pucLogBuffer, pucLogBuffer, pConfig->rte.report.noOfLogsToReport);
			}
			pConfig->rte.report.state= GKCOAP_REPORT_STATE_SEND_PACKET;//GKCOAP_REPORT_STATE_CONFIG_URI;
			break;

		case GKCOAP_REPORT_STATE_SEND_PACKET:
			{
				//uint16_t _len= GKCOAPPKT_PopulateReportingPacketSample((uint8_t *)pucBuffer);
				/*
				 * GKCOAPPKT_BuildReportingPacket will change pConfig->rte.report.noOfLogsToReport.
				 * The func will cut the log array when the status field different from previous's.*/
				uint16_t _len= GKCOAPPKT_BuildReportingPacket(pConfig->packetType, sBC66Link.status.rssi, &pConfig->rte.report.noOfLogsToReport, pucLogBuffer, pucBuffer);
				eBc66Trx.coapType= GK_POST_Bc66LinkCoap;
				eBc66Trx.timeout_ms= 60000/*long timeout for ecl2*/;//20000;
				eBc66Trx.coap.GKPost.uri= pConfig->serverURI;
				eBc66Trx.coap.GKPost.ipAddr= pConfig->serverIP;
				eBc66Trx.coap.GKPost.port= pConfig->serverPort;
				eBc66Trx.coap.GKPost.length= _len;
				eBc66Trx.coap.GKPost.data= (char *)pucBuffer;
				eBc66Trx.coap.GKPost.imei= config.nbiot.imei;
				eBc66Trx.coap.GKPost.packetType= pConfig->packetType;
				eBc66Trx.coap.GKPost.rxLength= 0;
				eBc66Trx.coap.GKPost.rxData= (char *)pucBuffer;/*we can use same buffer as tx, as the txdata will be copied to bc66link own buffer*/

				if(SUCCESS== BC66LINK_Enqueue(&eBc66Trx))
				{
					pConfig->rte.sendStatus= 0xFF;/*pending*/
					pConfig->rte.responseStatus= 0xFF;/*pending*/
					pConfig->rte.report.state= GKCOAP_REPORT_STATE_CHECK_SEND_PACKET;
				}
			}
			break;

		case GKCOAP_REPORT_STATE_CHECK_SEND_PACKET:
			if((0== pConfig->rte.sendStatus)&& (0== pConfig->rte.responseStatus))
			{
				if(SUCCESS== GKCOAPPKT_ProcessDownlinkPacket(eBc66Trx.coap.GKPost.rxData, eBc66Trx.coap.GKPost.rxLength))
				{
					pConfig->rte.report.state= GKCOAP_REPORT_STATE_COMPLETE;
				}
				else
				{
					pConfig->rte.report.state= GKCOAP_REPORT_STATE_SET_BACKOFF;
				}
			}
			else if((1== pConfig->rte.sendStatus)|| (1== pConfig->rte.responseStatus))
			{
				pConfig->rte.report.state= GKCOAP_REPORT_STATE_SET_BACKOFF;
			}
			break;

		case GKCOAP_REPORT_STATE_SET_BACKOFF:
			if(pConfig->rte.report.retryCount< pConfig->retryMax)
			{
				pConfig->rte.report.retryCount++;
				DBG_Print("GKCOAP Reporting Backoff(%d/ %d).\r\n", pConfig->rte.report.retryCount, pConfig->retryMax);
				GKCOAP_SetTimeout_s(UTILI_GetRandom(pConfig->retryBackoffMin_s, pConfig->retryBackoffMax_s));
				pConfig->rte.report.state= GKCOAP_REPORT_STATE_BACKOFF;
			}
			else
			{
				/*failed*/
				pConfig->rte.report.state= GKCOAP_REPORT_STATE_FAILED;
			}
			break;

		case GKCOAP_REPORT_STATE_BACKOFF:
			if(true== GKCOAP_IsTimeout())
			{
				pConfig->rte.report.state= GKCOAP_REPORT_STATE_SEND_PACKET;/*retry*/
			}
			break;

		case GKCOAP_REPORT_STATE_FAILED:
			DBG_Print("GKCOAP Reporting Failed.\r\n");
			pConfig->rte.report.state= GKCOAP_REPORT_STATE_IDLE;
			break;

		case GKCOAP_REPORT_STATE_COMPLETE:
			DBG_Print("GKCOAP Reporting Completed.\r\n");
			pConfig->rte.report.isCompleted= true;
			pConfig->rte.report.state= GKCOAP_REPORT_CHECK_LOG;/*check if we have new logs*/
			pConfig->rte.report.noOfLogsReported+= pConfig->rte.report.noOfLogsToReport;
			break;

		default:
			break;
	}
}

void GKCOAP_Fota_Request(void)
{
	if(Idle_FotaState== pConfig->rte.fota.state)
	{
		pConfig->rte.fota.isRequested= true;
	}
}

void GKCOAP_Fota_Completed(void)
{
	pConfig->rte.fota.isCompleted= true;
}

void GKCOAP_Fota_App(void)
{
	switch(pConfig->rte.fota.state)
	{
		case Idle_FotaState:
			if(true== pConfig->rte.fota.isRequested)
			{
				pConfig->rte.fota.isRequested= false;
				pConfig->rte.fota.isCompleted= false;
				pConfig->rte.fota.retryCount= 0;
				pConfig->rte.fota.isOnGoing= false;
				pConfig->rte.fota.currentPacket= 0;
				pConfig->rte.fota.flashWrittenBytes= 0x00;
				pConfig->rte.fota.flashChecksum= 0x0000;
				pConfig->rte.fota.state= pConfig->rte.fota.prevState= Get_Version_FotaState;
			}
			break;

		case Get_Version_FotaState:
			{
				uint16_t _len= GKCOAPPKT_PopulateFotaPacket(0, 0, NULL, (uint8_t *)pucBuffer);
				eBc66Trx.coapType= GK_POST_Bc66LinkCoap;
				eBc66Trx.timeout_ms= 60000/*long timeout for ecl2*/;//20000;
				eBc66Trx.coap.GKPost.uri= pConfig->fotaServerURI;
				eBc66Trx.coap.GKPost.ipAddr= pConfig->fotaServerIP;
				eBc66Trx.coap.GKPost.port= pConfig->fotaServerPort;
				eBc66Trx.coap.GKPost.length= _len;
				eBc66Trx.coap.GKPost.data= (char *)pucBuffer;
				eBc66Trx.coap.GKPost.imei= config.nbiot.imei;
				eBc66Trx.coap.GKPost.packetType= pConfig->packetType;
				eBc66Trx.coap.GKPost.rxLength= 0;
				eBc66Trx.coap.GKPost.rxData= (char *)pucBuffer;/*we can use same buffer as tx, as the txdata will be copied to bc66link own buffer*/

				if(SUCCESS== BC66LINK_Enqueue(&eBc66Trx))
				{
					pConfig->rte.sendStatus= 0xFF;/*pending*/
					pConfig->rte.responseStatus= 0xFF;/*pending*/
					pConfig->rte.fota.state= Check_Version_FotaState;
				}
			}
			break;

		case Check_Version_FotaState:
			{
				if((0== pConfig->rte.sendStatus)&& (0== pConfig->rte.responseStatus))
				{
					pConfig->rte.fota.retryCount= 0;
					char _version[35];
					uint16_t _size= strlen((char *)(eBc66Trx.coap.GKPost.rxData));
					memcpy(_version, (char *)(config.system.fwVersion), strlen((char *)(config.system.fwVersion)));
					memcpy(pConfig->rte.fota.version, eBc66Trx.coap.GKPost.rxData, _size);
					memcpy(&pConfig->rte.fota.totalPacket, eBc66Trx.coap.GKPost.rxData+ _size+ 1, 4);
					memcpy(&pConfig->rte.fota.packetSize, eBc66Trx.coap.GKPost.rxData+ _size+ 1+ 4, 4);

					if(0!= strcmp(_version, pConfig->rte.fota.version))
					{
						DBG_Print("GKCOAP Fota version:%s.\r\n", pConfig->rte.fota.version);
						pConfig->rte.fota.state= Flash_Init_FotaState;
					}
					else
					{
						pConfig->rte.fota.state= Idle_FotaState;/*no update required*/
					}
				}
				else if((1== pConfig->rte.sendStatus)|| (1== pConfig->rte.responseStatus))
				{
					pConfig->rte.fota.state= Set_Backoff_FotaState;
				}
			}
			break;

		case Flash_Init_FotaState:
			{
				TLV_t _tlv;
				uint8_t _v[1];
				uint8_t _rv[1];
				_tlv.Tg= SYS_TLVTag;
				_tlv.t= ERASE_PARTITION_SysTLVTag;
				_tlv.v= _v;
				_tlv.rv= _rv;
				_v[0]= (uint8_t)((config.flash.partition== CFG_PARTITION_1)? CFG_PARTITION_2: CFG_PARTITION_1);
				SYS_TLVRequest(&_tlv);
				if(SUCCESS== _tlv.rv[0])
				{
					pConfig->rte.fota.state= pConfig->rte.fota.prevState= Get_Packet_FotaState;
				}
				else
				{
					pConfig->rte.fota.state= Failed_FotaState;/*we are not expecting this to failed*/
				}
				pConfig->rte.fota.currentPacket= 0;
			}
			break;

		case Get_Packet_FotaState:
			{
				DBG_Print("Get_Packet_FotaState.\r\n");
				if(pConfig->rte.fota.currentPacket< pConfig->rte.fota.totalPacket)
				{
					uint16_t _len= GKCOAPPKT_PopulateFotaPacket(1, pConfig->rte.fota.currentPacket, pConfig->rte.fota.version, (uint8_t *)pucBuffer);
					eBc66Trx.coapType= GK_POST_Bc66LinkCoap;
					eBc66Trx.timeout_ms= 60000/*long timeout for ecl2*/;//20000;
					eBc66Trx.coap.GKPost.uri= pConfig->fotaServerURI;
					eBc66Trx.coap.GKPost.ipAddr= pConfig->fotaServerIP;
					eBc66Trx.coap.GKPost.port= pConfig->fotaServerPort;
					eBc66Trx.coap.GKPost.length= _len;
					eBc66Trx.coap.GKPost.data= (char *)pucBuffer;
					eBc66Trx.coap.GKPost.imei= config.nbiot.imei;
					eBc66Trx.coap.GKPost.packetType= pConfig->packetType;
					eBc66Trx.coap.GKPost.rxLength= 0;
					eBc66Trx.coap.GKPost.rxData= (char *)pucBuffer+ 6;/*We can use same buffer as tx, as the txdata will be copied to bc66link own buffer.
					We shift the buffer by 6 since we also want to use the buffer for SysRequest which requires header*/

					if(SUCCESS== BC66LINK_Enqueue(&eBc66Trx))
					{
						pConfig->rte.sendStatus= 0xFF;/*pending*/
						pConfig->rte.responseStatus= 0xFF;/*pending*/
						pConfig->rte.fota.state= Process_Packet_FotaState;
					}
				}
				else
				{
					pConfig->rte.fota.state= Switch_Bank_FotaState;
				}
			}
			break;

		case Process_Packet_FotaState:
			{
				//DBG_Print("Process_Packet_FotaState.\r\n");
				if((0== pConfig->rte.sendStatus)&& (0== pConfig->rte.responseStatus))
				{
					pConfig->rte.fota.retryCount= 0;
					uint32_t _offset= pConfig->rte.fota.currentPacket* pConfig->rte.fota.packetSize;
					uint16_t _length= (uint16_t)pConfig->rte.fota.packetSize;
					/*Note: We already shift 6 places from Get Packet*/
					memcpy(pucBuffer, &_offset, 4);
					memcpy(pucBuffer+ 4, &_length, 2);

					TLV_t _tlv;
					uint8_t _rv[1];
					_tlv.Tg= SYS_TLVTag;
					_tlv.t= WRITE_FLASH_SysTLVTag;
					_tlv.v= pucBuffer;
					_tlv.rv= _rv;
					SYS_TLVRequest(&_tlv);
					if(SUCCESS== _tlv.rv[0])
					{
						pConfig->rte.fota.flashChecksum= UTILI_GetChecksum(pConfig->rte.fota.flashChecksum, pucBuffer+ 6, pConfig->rte.fota.packetSize);
						pConfig->rte.fota.flashWriteCount++;
						pConfig->rte.fota.flashWrittenBytes+= pConfig->rte.fota.packetSize;

						pConfig->rte.fota.state= pConfig->rte.fota.prevState= Get_Packet_FotaState;
						pConfig->rte.fota.currentPacket++;
					}
					else
					{
						pConfig->rte.fota.state= FAILED_SwDownloadState;
					}
				}
				else if((1== pConfig->rte.sendStatus)|| (1== pConfig->rte.responseStatus))
				{
					DBG_Print("STORE_PACKET failed!");
					pConfig->rte.fota.state= Set_Backoff_FotaState;
				}
			}
			break;

		case Switch_Bank_FotaState:
			{
				DBG_Print("Switch_Bank_FotaState.\r\n");
				TLV_t _tlv;
				uint8_t _v[7];
				uint8_t _rv[1];
				_tlv.Tg= SYS_TLVTag;
				_tlv.t= SWITCH_FLASH_SysTLVTag;
				_tlv.v= _v;
				_tlv.rv= _rv;
				_v[0]= (uint8_t)((config.flash.partition== CFG_PARTITION_1)? CFG_PARTITION_2: CFG_PARTITION_1);
				memcpy(_v+ 1, &pConfig->rte.fota.flashWrittenBytes, 4);
				memcpy(_v+ 5, &pConfig->rte.fota.flashChecksum, 2);
				SYS_TLVRequest(&_tlv);
				pConfig->rte.fota.state= (SUCCESS== _tlv.rv[0])? Complete_FotaState: Failed_FotaState;/*we are not expecting this to failed*/;
				pConfig->rte.fota.currentPacket= 0;
			}
			break;

		case Switch_Bank_Check_FotaState:
			DBG_Print("Switch_Bank_Check_FotaState.\r\n");
			pConfig->rte.fota.state= Idle_FotaState;
			break;

		case Set_Backoff_FotaState:
			if(pConfig->rte.fota.retryCount< pConfig->retryMax)
			{
				pConfig->rte.fota.retryCount++;
				GKCOAP_SetTimeout_s(UTILI_GetRandom(pConfig->retryBackoffMin_s, pConfig->retryBackoffMax_s));
				pConfig->rte.fota.state= Backoff_FotaState;
				DBG_Print("Backoff_FotaState.\r\n");
			}
			else
			{
				/*failed*/
				pConfig->rte.fota.state= Suspend_FotaState;/*try again after next report cycle*/
				DBG_Print("Suspend.\r\n");
			}
			break;

		case Backoff_FotaState:
			if(true== GKCOAP_IsTimeout())
			{
				pConfig->rte.fota.state= pConfig->rte.fota.prevState;
			}
			break;

		case Suspend_FotaState:
			/*wait report trigger*/
			break;

		case Failed_FotaState:
			DBG_Print("Failed_FotaState.\r\n");
			pConfig->rte.fota.state= Idle_FotaState;
			break;

		case Complete_FotaState:
			DBG_Print("Complete_FotaState.\r\n");
			pConfig->rte.fota.state= Idle_FotaState;
			break;
	}
}

void GKCOAP_Msg_App(void)
{
	switch(pConfig->rte2.msg.state)
	{
		case GKCOAP_MSG_STATE_IDLE:
			if((0!= MSG_Msg_Depth())&& (NBIOT_GKCOAP_PKT_TaskId== MSG_Msg_TaskId_Peek()))
			{
				MSG_t _msg= MSG_Msg_Dequeue();
				if(GKCOAP_CFG_MSG_BUFFER_SIZE>= _msg.bufferLen)
				{
					uint16_t _len= GKCOAPPKT_PopulateMsgPacket(pucBuffer, _msg.buffer, _msg.bufferLen);
					eBc66Trx.coapType= GK_POST_Bc66LinkCoap;
					eBc66Trx.timeout_ms= 60000/*long timeout for ecl2*/;//20000;
					eBc66Trx.coap.GKPost.uri= pConfig->serverURI;
					eBc66Trx.coap.GKPost.ipAddr= pConfig->serverIP;
					eBc66Trx.coap.GKPost.port= pConfig->serverPort;
					eBc66Trx.coap.GKPost.localPort= pConfig->localPort;
					eBc66Trx.coap.GKPost.length= _len;
					eBc66Trx.coap.GKPost.data= (char *)pucBuffer;
					eBc66Trx.coap.GKPost.imei= config.nbiot.imei;
					eBc66Trx.coap.GKPost.packetType= pConfig->packetType;
					eBc66Trx.coap.GKPost.rxLength= 0;
					eBc66Trx.coap.GKPost.rxData= (char *)pucBuffer;/*we can use same buffer as tx, as the txdata will be copied to bc66link own buffer*/

					if(SUCCESS== BC66LINK_Enqueue(&eBc66Trx))
					{
						pConfig->rte.sendStatus= 0xFF;/*pending*/
						pConfig->rte.responseStatus= 0xFF;/*pending*/
						pConfig->rte2.msg.state= GKCOAP_MSG_STATE_CHECK_SEND_PACKET;
					}
				}
			}
			break;

		case GKCOAP_MSG_STATE_CHECK_SEND_PACKET:
			if((0== pConfig->rte.sendStatus)&& (0== pConfig->rte.responseStatus))
			{
				if(SUCCESS== GKCOAPPKT_ProcessDownlinkPacket(eBc66Trx.coap.GKPost.rxData, eBc66Trx.coap.GKPost.rxLength))
				{
					pConfig->rte2.msg.state= GKCOAP_MSG_STATE_COMPLETE;
				}
				else
				{
					pConfig->rte2.msg.state= GKCOAP_MSG_STATE_SET_BACKOFF;
				}
			}
			else if((1== pConfig->rte.sendStatus)|| (1== pConfig->rte.responseStatus))
			{
				pConfig->rte2.msg.state= GKCOAP_MSG_STATE_SET_BACKOFF;
			}
			break;

		case GKCOAP_MSG_STATE_SET_BACKOFF:
			if(pConfig->rte2.msg.retryCount< pConfig->retryMax)
			{
				pConfig->rte2.msg.retryCount++;
				DBG_Print("GKCOAP Msg Backoff(%d/ %d).\r\n", pConfig->rte2.msg.retryCount, pConfig->retryMax);
				GKCOAP_SetTimeout_s(UTILI_GetRandom(pConfig->retryBackoffMin_s, pConfig->retryBackoffMax_s));
				pConfig->rte2.msg.state= GKCOAP_MSG_STATE_BACKOFF;
			}
			else
			{
				/*failed*/
				pConfig->rte2.msg.state= GKCOAP_MSG_STATE_FAILED;
			}
			break;

		case GKCOAP_REG_STATE_BACKOFF:
			if(true== GKCOAP_IsTimeout())
			{
				pConfig->rte2.msg.state= GKCOAP_MSG_STATE_SEND_PACKET;/*retry*/
			}
			break;

		case GKCOAP_MSG_STATE_FAILED:
			DBG_Print("GKCOAP Msg Failed.\r\n");
			pConfig->rte2.msg.state= GKCOAP_MSG_STATE_IDLE;
			break;

		case GKCOAP_MSG_STATE_COMPLETE:
			DBG_Print("GKCOAP Msg Completed.\r\n");
			pConfig->rte2.msg.state= GKCOAP_MSG_STATE_IDLE;
			break;

		default:
			break;
	}
}

void GKCOAP_AlarmTask(void)
{
	time_t _currTime= SYS_GetTimestamp_s();
	static uint8_t _prevStatusCode= 0;
	uint8_t _currStatusCode= SENSORS_GetStatusCode();

	if(_currStatusCode!= _prevStatusCode)
	{
		DIAG_Code(STATUS_CODE_CHANGED_GKCoapDCode, _currStatusCode);
		if(_currStatusCode> _prevStatusCode)/*only care for new flag set, not cleared*/
		{
			 if(LOGGER_DeviceLog_GetLogCount()> pConfig->rte.report.noOfLogsReported)
			 {
				GKCOAP_Report_Request();
				DBG_Print("GKCOAP_Report_Request. reason: alarm\r\n");
				pConfig->rteAlarmClearTimestamp= _currTime+ pConfig->alarmActivePeriod_s;
			 }
		}
		_prevStatusCode= _currStatusCode;
	}

	if(
		(0!= pConfig->alarmActivePeriod_s)
		&&(0!= pConfig->rteAlarmClearTimestamp)
		&&(_currTime>= pConfig->rteAlarmClearTimestamp)
	)
	{
		SENSORS_SetStatusCode(0xFF);
		/*check persistent magnet, required by PTP*/
		if(true== DIGISENSOR_PBMag_IsActive())
		{
			/*extend clear time*/
			pConfig->rteAlarmClearTimestamp= _currTime+ pConfig->alarmActivePeriod_s;
		}
		else
		{
			pConfig->rteAlarmClearTimestamp= 0;
		}
		DIAG_Code(REPORT_NEXT_TIME_GKCoapDCode, SENSORS_GetStatusCode());
	}
}

void GKCOAP_PeriodicReportInit(void)
{
	//if(0== pConfig->rteStartTime)
	{
		pConfig->rteStartTime= pConfig->rteNextTime= UTILI_Mask_GetMatchedTimeFromNow(pConfig->reportStartMask);
	}
	GKCOAP_Report_SetTimeout_s(pConfig->rteNextTime-  SYS_GetTimestamp_s());
	DIAG_Code(REPORT_START_TIME_GKCoapDCode, pConfig->rteStartTime);
	//DBG_Print("\r\n\r\n");
	//DBG_Print("Start time: %s\r\n", asctime(localtime(&(pConfig->rteStartTime))));
	//DBG_Print("\r\n\r\n");

	DEVICELOG_EnablePeriodicLog(true);
	DEVICELOG_EnableTamperLog(true);/*PTP wants this*/

	RTC_AlarmStartMarker_t _marker;
	_marker.date=  pConfig->logTickStartMask>> 24;
	_marker.hour=  pConfig->logTickStartMask>> 16;
	_marker.minute=  pConfig->logTickStartMask>> 8;
	_marker.second=  pConfig->logTickStartMask>> 0;
	DEVICELOG_StartLog(NOTSTARTED_StartType, pConfig->logTickType, pConfig->logTickSize, _marker);
}

void GKCOAP_PeriodicReportTask(void)
{
	if(GKCOAP_Report_IsTimeout())
	{
		pConfig->rteNextTime= UTILI_ComputeNextTime(SYS_GetTimestamp_s(), pConfig->rteStartTime, pConfig->reportInterval_s);
		DIAG_Code(REPORT_NEXT_TIME_GKCoapDCode, pConfig->rteNextTime);
		GKCOAP_Report_SetTimeout_s(pConfig->rteNextTime-  SYS_GetTimestamp_s());

		GKCOAP_Report_Request();

		DBG_Print("GKCOAP_Report_Request.\r\n");
	}
}

void GKCOAP_Init(GKCOAP_t *_config)
{
	pConfig= _config;

	BC66LINK_COAP_SetResponseCallback(GKCOAP_Response);
	BC66LINK_COAP_SetRequestCallback(GKCOAP_Request);
	BC66LINK_COAP_SetRecoveryCallback(GKCOAP_Recovery);/*this is important, we may need to re-create COAP after deep sleep*/

	GKCOAP_PeriodicReportInit();

	pConfig->rte.reg.isRequested= true;/*send register after every reset*/
	pConfig->rte.reg.isCompleted= false;
	pConfig->rte.reg.state= GKCOAP_REG_STATE_IDLE;
	pConfig->rte.report.state= GKCOAP_REPORT_STATE_IDLE;
	pConfig->rte.fota.state= Idle_FotaState;/*todo: fota continued after reset*/
	pConfig->rte2.msg.state= GKCOAP_MSG_STATE_IDLE;
	pConfig->rte2.msg.retryCount= 0;
}

void GKCOAP_Task(void)
{
	if((true== eBc66Trx.transactionInQueue)|| (true== eBc66Trx.transactionInProgress))
	{
		return;
	}

	if(true== eBc66Trx.transactionCompleted)
	{
		eBc66Trx.transactionCompleted= false;
		switch(eBc66Trx.coapType)
		{
			case GK_POST_Bc66LinkCoap:
				pConfig->rte.sendStatus= (SUCCESS_TransactionStatus== eBc66Trx.status)? 0: 1;
				break;

			default:
				break;

		}
	}

	/*Check if we can already transmit or not.*/
	if(false== BC66LINK_TransmissionIsReady())
	{
		/*TODO: intermittent network disruption handling required, for example device is de-attached halfway.*/
		return;
	}
	if((GKCOAP_REPORT_STATE_IDLE== pConfig->rte.report.state)
			&& (GKCOAP_MSG_STATE_IDLE== pConfig->rte2.msg.state)
			&& ((Idle_FotaState== pConfig->rte.fota.state)|| (Suspend_FotaState== pConfig->rte.fota.state))
			)
	{
		GKCOAP_Register_App();
	}

	if((GKCOAP_REG_STATE_IDLE== pConfig->rte.reg.state)
			&& (GKCOAP_MSG_STATE_IDLE== pConfig->rte2.msg.state)
			&& ((Idle_FotaState== pConfig->rte.fota.state)|| (Suspend_FotaState== pConfig->rte.fota.state))
			)
	{
		GKCOAP_Report_App();
	}

	if((GKCOAP_REG_STATE_IDLE== pConfig->rte.reg.state)
			&& (GKCOAP_REPORT_STATE_IDLE== pConfig->rte.report.state)
			&& ((Idle_FotaState== pConfig->rte.fota.state)|| (Suspend_FotaState== pConfig->rte.fota.state))
			)
	{
		GKCOAP_Msg_App();
	}

	if((GKCOAP_REG_STATE_IDLE== pConfig->rte.reg.state)
			&& (GKCOAP_REPORT_STATE_IDLE== pConfig->rte.report.state)
			&& (GKCOAP_MSG_STATE_IDLE== pConfig->rte2.msg.state)
			)
	{
		GKCOAP_Fota_App();
	}

	GKCOAP_AlarmTask();
	GKCOAP_PeriodicReportTask();
}

uint8_t GKCOAP_TaskState(void)
{
	return BC66LINK_TaskState();
}
