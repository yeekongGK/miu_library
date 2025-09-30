/*
 * softwaremgt.c
 *
 *  Created on: 9 Dec 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#include "common.h"
#include "lwm2m.h"
#include "cbor.h"
#include "lwobject.h"
#include "sensor.h"
#include "softwaremgt.h"
#include "msg.h"

#define DBG_Print

__IO static BC66LINK_Transaction_t eBc66Trx= {
		.type= COAP_Bc66LinkTransactionType,
		.coapType= LWM2M_GET_Bc66LinkCoap,
		.timeout_ms= 40000,
		.TransactionCompletedCb= NULL,
};
__IO static uint8_t pucBuffer[1024];
char pcIPAddr[255];
uint16_t wPort;
char pcURI[255];
uint8_t ucConfigOptionStatus;/*0= success, 1= failed, 0xFF= pending*/
uint8_t ucSendStatus;/*0= success, 1= failed, 0xFF= pending*/
uint8_t ucResponseStatus;/*0= success, 1= failed, 0xFF= pending*/
bool bCheckURI= false;
time_t tFetchTimestamp= 0;
TLV_t eTlv;
static uint8_t pucV[7];
uint8_t pucRv[2];

void SWMGT_ResetToInitial(LWOBJ_Obj_t *pLwObj, bool _setResult);

void SWGT_Diag_UpdateState(LWOBJ_Obj_t *pLwObj)
{
//	DIAG_Code(SWGT_STATE_NbiotLwm2mDCode,
//			(pLwObj->resource[UPDATE_STATE_SwMgtResource].value.integer<< 24)|
//			(pLwObj->resource[UPDATE_RESULT_SwMgtResource].value.integer<< 16)|
//			(pLwObj->resource[ACTIVATION_STATE_SwMgtResource].value.integer<< 8)|
//			((pLwObj->rte.swMgt->swUpdate.state<< 0)& 0x000000FF)
//	);
}

void SWMGT_Link_HardReset(void)
{
	BC66LINK_ResetModem();
}

void SWMGT_Response(void *_param)
{
	BC66LINK_Coap_Response_t *_response= (BC66LINK_Coap_Response_t *)_param;

	if((404== _response->rspCode)|| (0== _response->len))
	{
		ucResponseStatus= 3;
	}
	else
	{
		ucResponseStatus= 0;
	}
}

void SWMGT_Request(void *_param)
{
	BC66LINK_Coap_Request_t *_request= (BC66LINK_Coap_Request_t *)_param;
}

void SWMGT_Recovery(void *_param)
{
	BC66LINK_Coap_Recovery_t *_recovery= (BC66LINK_Coap_Recovery_t *)_param;

	//DBG_Print("GKCOAP Recovery.\r\n");

	if(1== _recovery->state)/*fail, need to re-create using AT+QCOAPCREATE after failure*/
	{
		eBc66Trx.coap.Lwm2mGet.created= false;
		//DBG_Print("GKCOAP Recovery triggered.\r\n");
	}
}

uint16_t SWMGT_PopulateSwUpdatePacket(uint8_t _type, uint32_t _currPacket, char * _version, uint8_t *_payloadBuf)
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

void SWMGT_RefreshResources(LWOBJ_Obj_t *pLwObj)
{
	/*resource are written, check URI*/
	if(INITIAL_SwMgtUpdateState== pLwObj->resource[UPDATE_STATE_SwMgtResource].value.integer)
	{
		bCheckURI= true;/*open to process uri*/
	}
}

void SWMGT_NotifyResource(LWOBJ_Obj_t *pLwObj, uint8_t _resourceNo, bool _force)
{
	SWGT_Diag_UpdateState(pLwObj);

	if((false== _force) && (false== pLwObj->resource[_resourceNo].observe))
	{
		return;
	}
	else
	{
		pLwObj->resource[_resourceNo].notifyState= DO_NOTIFY_NotifyState;
	}
}

void SWMGT_Execute(LWOBJ_Obj_t *pLwObj, uint8_t _resourceNo)
{
	switch(_resourceNo)
	{
		case UNINSTALL_SwMgtResource:
			SWMGT_ResetToInitial(pLwObj, true);
			break;

		case INSTALL_SwMgtResource:
			pLwObj->resource[UPDATE_STATE_SwMgtResource].value.integer=	INSTALLED_SwMgtUpdateState;
			SWMGT_NotifyResource(pLwObj, UPDATE_STATE_SwMgtResource, false);
			pLwObj->resource[UPDATE_RESULT_SwMgtResource].value.integer= SUCCESSSFULLY_INSTALLED_SwMgtUpdateResult;
			SWMGT_NotifyResource(pLwObj, UPDATE_RESULT_SwMgtResource, false);
			break;

		case ACTIVATE_SwMgtResource:
			{
				SWMGT_ResetToInitial(pLwObj, true);

				/*do a bank switch*/
				//DBG_Print("SWMGT_Activate.\r\n");
				eTlv.Tg= SYS_TLVTag;
				eTlv.t= SWITCH_FLASH_SysTLVTag;
				eTlv.v= pucV;
				eTlv.rv= pucRv;
				pucV[0]= (uint8_t)((config.flash.partition== CFG_PARTITION_1)? CFG_PARTITION_2: CFG_PARTITION_1);
				memcpy(pucV+ 1, &pLwObj->rte.swMgt->swUpdate.flashWrittenBytes, 4);
				memcpy(pucV+ 5, &pLwObj->rte.swMgt->swUpdate.flashChecksum, 2);

				pLwObj->resource[ACTIVATION_STATE_SwMgtResource].value.integer=	ENABLED_SwMgtActivationState;/*1: enabled*/
				SWMGT_NotifyResource(pLwObj, ACTIVATION_STATE_SwMgtResource, false);
			}
			break;

		case DEACTIVATE_SwMgtResource:
			SWMGT_ResetToInitial(pLwObj, true);
			break;
	}
}

void SWMGT_Activate(LWOBJ_Obj_t *pLwObj)
{
	/*pLwObj ni tak leh pakai,kene check balik value dia*/
	SYS_TLVRequest(&eTlv);
}

void SWMGT_ResetToInitial(LWOBJ_Obj_t *pLwObj, bool _setResult)
{
	pLwObj->resource[UPDATE_STATE_SwMgtResource].value.integer=	INITIAL_SwMgtUpdateState;
	SWMGT_NotifyResource(pLwObj, UPDATE_STATE_SwMgtResource, false);

	if(true== _setResult)
	{
		pLwObj->resource[UPDATE_RESULT_SwMgtResource].value.integer= INITIAL_SwMgtUpdateResult;
		SWMGT_NotifyResource(pLwObj, UPDATE_RESULT_SwMgtResource, false);
	}

	pLwObj->resource[ACTIVATION_STATE_SwMgtResource].value.integer=	DISABLED_SwMgtActivationState;/*1: enabled*/
	SWMGT_NotifyResource(pLwObj, ACTIVATION_STATE_SwMgtResource, false);

	pLwObj->resource[PACKAGE_URI_SwMgtResource].valueLen= 0;
	pLwObj->resource[PACKAGE_URI_SwMgtResource].value.string[0]= '\0';
	SWMGT_NotifyResource(pLwObj, PACKAGE_URI_SwMgtResource, false);

	pLwObj->rte.swMgt->swUpdate.state= IDLE_SwDownloadState;
}

ErrorStatus SWMGT_ProcessPackageURI(LWOBJ_Obj_t *pLwObj)
{
	if(0!= strlen(pLwObj->resource[PACKAGE_URI_SwMgtResource].value.string))
	{
		char serverURI[SWMGT_CFG_URI_MAX_LEN]= {0};
		memcpy(serverURI, pLwObj->resource[PACKAGE_URI_SwMgtResource].value.string,  pLwObj->resource[PACKAGE_URI_SwMgtResource].valueLen);
		if(NULL!= strstr(serverURI, "coap"))
		{
			char *_token;
			if(NULL!= strstr(serverURI, "coap://"))
			{
				_token= strtok(serverURI+ strlen("coap://"), ":");
			}
			else if(NULL!= strstr(serverURI, "coaps://"))
			{
				_token= strtok(serverURI+ strlen("coaps://"), ":");
			}
			else
			{
				goto SWMGT_ProcessPackageURI_error;
			}

			UTILI_Array_CopyString(pcIPAddr, _token);
			wPort= atoi(strtok(NULL, "/"));
			_token= strtok(NULL, "\0");
			UTILI_Array_CopyString(pcURI, _token);

			eBc66Trx.coap.Lwm2mGet.serverIP= pcIPAddr;
			eBc66Trx.coap.Lwm2mGet.serverPort= wPort;
			eBc66Trx.coap.Lwm2mGet.serverURI= pcURI;
			eBc66Trx.coap.Lwm2mGet.localPort= 56831;

			if((0== strcmp(eBc66Trx.coap.Lwm2mGet.serverIP, config.nbiot.lwm2m.currentConnection.serverIP))&& (eBc66Trx.coap.Lwm2mGet.serverPort== config.nbiot.lwm2m.currentConnection.serverPort))
			{
				eBc66Trx.coap.Lwm2mGet.encMode= config.nbiot.lwm2m.currentConnection.securityMode;
				eBc66Trx.coap.Lwm2mGet.pskId= config.nbiot.lwm2m.currentConnection.pskId;
				eBc66Trx.coap.Lwm2mGet.psk= config.nbiot.lwm2m.currentConnection.psk;
			}
			else if((0== strcmp(eBc66Trx.coap.Lwm2mGet.serverIP, config.nbiot.lwm2m.defaultConnection.serverIP))&& (eBc66Trx.coap.Lwm2mGet.serverPort== config.nbiot.lwm2m.defaultConnection.serverPort))
			{
				eBc66Trx.coap.Lwm2mGet.encMode= config.nbiot.lwm2m.defaultConnection.securityMode;
				eBc66Trx.coap.Lwm2mGet.pskId= config.nbiot.lwm2m.defaultConnection.pskId;
				eBc66Trx.coap.Lwm2mGet.psk= config.nbiot.lwm2m.defaultConnection.psk;
			}
			else
			{
				eBc66Trx.coap.Lwm2mGet.encMode= NONE_Bc66LinkLwm2mSecurityMode;
			}

			/*convert lwm2m sec mode to coap sec mode*/
			eBc66Trx.coap.Lwm2mGet.encMode= (PSK_Bc66LinkLwm2mSecurityMode== eBc66Trx.coap.Lwm2mGet.encMode)? PSK_Bc66LinkCoapEncryption: NONE_Bc66LinkCoapEncryption;

			return SUCCESS;
		}
	}

SWMGT_ProcessPackageURI_error:
	pLwObj->resource[PACKAGE_URI_SwMgtResource].valueLen= 0;
	pLwObj->resource[PACKAGE_URI_SwMgtResource].value.string[0]= '\0';
	return ERROR;
}

void SWMGT_Init(LWOBJ_Obj_t *pLwObj)
{
	BC66LINK_Init();
	BC66LINK_COAP_SetResponseCallback2(SWMGT_Response);
	BC66LINK_COAP_SetRequestCallback2(SWMGT_Request);
	BC66LINK_COAP_SetRecoveryCallback2(SWMGT_Recovery);/*this is important, we may need to re-create COAP after deep sleep*/

	pLwObj->resource[PKG_NAME_SwMgtResource].value.string= config.system.name;
	pLwObj->resource[PKG_NAME_SwMgtResource].valueLen= strlen(config.system.name);
	pLwObj->resource[PKG_VERSION_SwMgtResource].value.string= config.system.fwVersion;
	pLwObj->resource[PKG_VERSION_SwMgtResource].valueLen= strlen(config.system.fwVersion);

	pLwObj->resource[INSTALL_SwMgtResource].value.execute=
	pLwObj->resource[UNINSTALL_SwMgtResource].value.execute=
	pLwObj->resource[ACTIVATE_SwMgtResource].value.execute=
	pLwObj->resource[DEACTIVATE_SwMgtResource].value.execute= SWMGT_Execute;

	SWMGT_RefreshResources(pLwObj);
	SWMGT_ProcessPackageURI(pLwObj);/*to support resuming download after reboot(untested)*/

	eBc66Trx.coap.Lwm2mGet.created= false;

	if(DOWNLOAD_STARTED_SwMgtUpdateState== pLwObj->resource[UPDATE_STATE_SwMgtResource].value.integer)
	{
		if(REST_SwDownloadState< pLwObj->rte.swMgt->swUpdate.state)
		{
			pLwObj->rte.swMgt->swUpdate.state= REST_SwDownloadState;
		}
	}
	else
	{
		pLwObj->rte.swMgt->swUpdate.state= IDLE_SwDownloadState;
	}
}

time_t SWMGT_Task(LWOBJ_Obj_t *pLwObj, time_t _currTime, uint64_t _currMask)
{
	time_t _awakeTime= _currTime+ LWOBJ_CFG_MAX_SLEEP_TIME_S;

	if(true== pLwObj->written)
	{
		pLwObj->written= false;
		SWMGT_RefreshResources(pLwObj);
	}

	if(IDLE_SwDownloadState!= pLwObj->rte.swMgt->swUpdate.state)
	{
		if((true== eBc66Trx.transactionInQueue)|| (true== eBc66Trx.transactionInProgress)
		   || (false== BC66LINK_TransmissionIsReady()))/*Check if we can already transmit or not.*/
		{
			_awakeTime= SYS_GetTimestamp_s()+ LWOBJ_CFG_MIN_SLEEP_TIME_S;
			return _awakeTime;
		}

		if(false==LWM2M_IsRegistered())
		{
			_awakeTime= SYS_GetTimestamp_s()+ 60;
			return _awakeTime;/*we don't want to interfere too much during installation process*/
		}

		if(true== eBc66Trx.transactionCompleted)
		{
			eBc66Trx.transactionCompleted= false;
			switch(eBc66Trx.coapType)
			{
				case LWM2M_GET_Bc66LinkCoap:
					ucSendStatus= (SUCCESS_TransactionStatus== eBc66Trx.status)? 0: 1;
					break;

				default:
					break;

			}
		}
	}

	switch(pLwObj->resource[UPDATE_STATE_SwMgtResource].value.integer)
	{
		case INITIAL_SwMgtUpdateState:
			if(true== bCheckURI)
			{
				bCheckURI= false;
				if(SUCCESS== SWMGT_ProcessPackageURI(pLwObj))
				{
					pLwObj->resource[UPDATE_STATE_SwMgtResource].value.integer= DOWNLOAD_STARTED_SwMgtUpdateState;
					SWMGT_NotifyResource(pLwObj, UPDATE_STATE_SwMgtResource, false);
					pLwObj->resource[UPDATE_RESULT_SwMgtResource].value.integer= DOWNLOADING_SwMgtUpdateResult;
					SWMGT_NotifyResource(pLwObj, UPDATE_RESULT_SwMgtResource, false);
					pLwObj->rte.swMgt->swUpdate.state= IDLE_SwDownloadState;
					_awakeTime= _currTime;
				}
				else if(0!= strlen(pLwObj->resource[PACKAGE_URI_SwMgtResource].value.string))
				{
					pLwObj->resource[UPDATE_RESULT_SwMgtResource].value.integer= INVALID_URI_SwMgtUpdateResult;
					SWMGT_NotifyResource(pLwObj, UPDATE_RESULT_SwMgtResource, false);
				}
			}
			break;
		case DOWNLOAD_STARTED_SwMgtUpdateState:
			_awakeTime= _currTime;

			if(IDLE_SwDownloadState== pLwObj->rte.swMgt->swUpdate.state)
			{
				pLwObj->rte.swMgt->swUpdate.state= pLwObj->rte.swMgt->swUpdate.prevState= ERASE_PARTITION_SwDownloadState;
			}

			switch(pLwObj->rte.swMgt->swUpdate.state)
			{
				case IDLE_SwDownloadState:
					pLwObj->rte.swMgt->swUpdate.isRequested= false;
					pLwObj->rte.swMgt->swUpdate.isCompleted= false;
					pLwObj->rte.swMgt->swUpdate.retryCount= 0;
					pLwObj->rte.swMgt->swUpdate.isOnGoing= false;
					pLwObj->rte.swMgt->swUpdate.status= 0;
					break;
				case ERASE_PARTITION_SwDownloadState:
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
							pLwObj->rte.swMgt->swUpdate.state= pLwObj->rte.swMgt->swUpdate.prevState= REST_SwDownloadState;
							pLwObj->rte.swMgt->swUpdate.currentBlock= 0;
							pLwObj->rte.swMgt->swUpdate.isFirstBlock= true;
							pLwObj->rte.swMgt->swUpdate.blockSize= 512;
							pLwObj->rte.swMgt->swUpdate.flashWrittenBytes= 0x00;
							pLwObj->rte.swMgt->swUpdate.flashSize= 0x01;
							pLwObj->rte.swMgt->swUpdate.flashChecksum= 0x0000;
						}
						else
						{
							pLwObj->rte.swMgt->swUpdate.state= FAILED_SwDownloadState;
							pLwObj->rte.swMgt->swUpdate.status= (uint8_t)NOT_ENOUGH_STORAGE_SwMgtUpdateResult;
						}
					}
					break;
				case REST_SwDownloadState:/*to mitigate ecl 2 ota error*/
					tFetchTimestamp= SYS_GetTimestamp_s()+ 1;
					_awakeTime= UTILI_GetSmallerTime(_awakeTime, tFetchTimestamp);
					pLwObj->rte.swMgt->swUpdate.state= GET_PACKET_SwDownloadState;
					break;
				case GET_PACKET_SwDownloadState:
					{
						//DBG_Print("Get_Packet_FotaState.\r\n");
						if(pLwObj->rte.swMgt->swUpdate.flashWrittenBytes< pLwObj->rte.swMgt->swUpdate.flashSize)
						{
//							eBc66Trx.coap.Lwm2mGet.localPort= 56831;
//							eBc66Trx.coap.Lwm2mGet.serverIP= pcIPAddr;
//							eBc66Trx.coap.Lwm2mGet.serverPort= wPort;
//							eBc66Trx.coap.Lwm2mGet.serverURI= pcURI;
//							eBc66Trx.coap.Lwm2mGet.encMode= NONE_Bc66LinkCoapEncryption;
//							eBc66Trx.coap.Lwm2mGet.pskId= 0;
//							eBc66Trx.coap.Lwm2mGet.psk= 0;
							eBc66Trx.coap.Lwm2mGet.rxLength= 0;
							eBc66Trx.coap.Lwm2mGet.rxData= (char *)pucBuffer+ 6;/*We shift the buffer by 6 since we also want to use the buffer for SysRequest which requires header*/
							eBc66Trx.coap.Lwm2mGet.block2Code= pLwObj->rte.swMgt->swUpdate.currentBlock;

							if(SUCCESS== BC66LINK_Enqueue(&eBc66Trx))
							{
								ucSendStatus= 0xFF;/*pending*/
								ucResponseStatus= 0xFF;/*pending*/
								pLwObj->rte.swMgt->swUpdate.state= STORE_PACKET_SwDownloadState;

								DIAG_Code(SWGT_CURR_BLOCK_NbiotLwm2mDCode, eBc66Trx.coap.Lwm2mGet.block2Code);
							}
						}
						else
						{
							uint16_t _checksum= CFG_GetFWUChecksum(0x0000, (uint8_t *)( pLwObj->rte.swMgt->swUpdate.flashWriteAddress- pLwObj->rte.swMgt->swUpdate.flashWrittenBytes), pLwObj->rte.swMgt->swUpdate.flashWrittenBytes);
							if(_checksum== pLwObj->rte.swMgt->swUpdate.flashChecksum)
							{
								pLwObj->rte.swMgt->swUpdate.state= DOWNLOADED_SwDownloadState;
							}
							else
							{
								DBG_Print("CHECKSUM FAILED! received %d, calculated %d",pLwObj->rte.swMgt->swUpdate.flashChecksum, _checksum);
								DIAG_Code(SWGT_CRC_ERROR_NbiotLwm2mDCode, (pLwObj->rte.swMgt->swUpdate.flashChecksum<< 16)| (_checksum<< 0));
								/*no need to update*/
								pLwObj->rte.swMgt->swUpdate.state= FAILED_SwDownloadState;
								pLwObj->rte.swMgt->swUpdate.status= (uint8_t)INTEGRITY_FAILURE_SwMgtUpdateResult;
							}
						}
					}
					break;
				case STORE_PACKET_SwDownloadState:
					if((0== ucSendStatus)&& (0== ucResponseStatus))
					{
						uint8_t _currVersion;
						uint32_t _fwOffset= 0;/*block may not start with firmware in first block*/
						uint8_t _version;
						uint8_t _id;
						uint8_t _IV[16]= {0};

						if(true== pLwObj->rte.swMgt->swUpdate.isFirstBlock)
						{
							pLwObj->rte.swMgt->swUpdate.isFirstBlock= false;
							pLwObj->rte.swMgt->swUpdate.blockSize= eBc66Trx.coap.Lwm2mGet.rxLength;

							if(0== pLwObj->rte.swMgt->swUpdate.currentBlock)
							{
								/*4 bytes of each firmware section is the length:
								 * |4 bytes partition1 Size| 1 byte version| 1 bytes id| 2bytes checksum| partition1 Fw ...
								 * |4 Bytes partition2 Size| 1 byte version| 1 bytes id| 2bytes checksum| partition2 Fw ... |
								 * at block 0, we can straight away get the length of partition1 Fw*/
								SECURE_CTR_Transcrypt(3, _IV, eBc66Trx.coap.Lwm2mGet.rxData, 8);
								memcpy(&(pLwObj->rte.swMgt->swUpdate.flashSize), eBc66Trx.coap.Lwm2mGet.rxData, 4);

								/*if we are at partition1, we need to get block that contains partition2 Fw.*/
								if(CFG_PARTITION_1== config.flash.partition)
								{
									pLwObj->rte.swMgt->swUpdate.state= pLwObj->rte.swMgt->swUpdate.prevState= REST_SwDownloadState;
									pLwObj->rte.swMgt->swUpdate.currentBlock= (4+ 4+ pLwObj->rte.swMgt->swUpdate.flashSize)/ pLwObj->rte.swMgt->swUpdate.blockSize;
									pLwObj->rte.swMgt->swUpdate.isFirstBlock= true;
									break;
								}
								else
								{
									/*we are at correct partition and block, can proceed with update*/
									_fwOffset= 4;/*fw starts after length and reserved bytes*/
									pLwObj->rte.swMgt->swUpdate.flashWriteAddress= CFG_PARTITION_1_ADDR;
								}
							}
							else
							{
								/*at this point we already have partition1 length, we need to figure where in the currentBlock the partition2 starts*/
								uint32_t _part2Offset= (pLwObj->rte.swMgt->swUpdate.currentBlock+ 1)* pLwObj->rte.swMgt->swUpdate.blockSize;/*get total downloaded*/
								_part2Offset= _part2Offset- (4+ 4+ pLwObj->rte.swMgt->swUpdate.flashSize);/*get remain*/
								_part2Offset= pLwObj->rte.swMgt->swUpdate.blockSize- _part2Offset;/*get offset*/

								_fwOffset= _part2Offset;
								SECURE_CTR_Transcrypt(3, _IV, eBc66Trx.coap.Lwm2mGet.rxData+ _fwOffset, 8);
								memcpy(&(pLwObj->rte.swMgt->swUpdate.flashSize), eBc66Trx.coap.Lwm2mGet.rxData+ _fwOffset, 4);
								_fwOffset+= 4;
								pLwObj->rte.swMgt->swUpdate.flashWriteAddress= CFG_PARTITION_2_ADDR;
							}

							_version= eBc66Trx.coap.Lwm2mGet.rxData[_fwOffset++];
							_id= eBc66Trx.coap.Lwm2mGet.rxData[_fwOffset++];
							memcpy(&pLwObj->rte.swMgt->swUpdate.flashChecksum, eBc66Trx.coap.Lwm2mGet.rxData+ _fwOffset, 2);
							_fwOffset+= 2;/*fw starts after length and reserved bytes*/

							char _currVersionStr[2]= {config.system.fwVersion[1], config.system.fwVersion[3]};
							UTILI_HexStringToBytes(&_currVersion, _currVersionStr, 1);

							if((_version<= _currVersion)|| (_id!= 0xB1))
							{
								/*no need to update*/
								pLwObj->rte.swMgt->swUpdate.state= FAILED_SwDownloadState;
								pLwObj->rte.swMgt->swUpdate.status= (uint8_t)UNSUPPORTED_PACKAGE_SwMgtUpdateResult;
								break;
							}
						}

						//uint16_t _length= (uint16_t)(pLwObj->rte.swMgt->swUpdate.blockSize- _fwOffset);
						uint16_t _length= (uint16_t)(eBc66Trx.coap.Lwm2mGet.rxLength- _fwOffset);
						uint32_t _remaining= pLwObj->rte.swMgt->swUpdate.flashSize- pLwObj->rte.swMgt->swUpdate.flashWrittenBytes;
						if(_length> _remaining)/*last packet may contain more bytes than needed*/
						{
							_length= _remaining;
						}
						memcpy(pucBuffer, &pLwObj->rte.swMgt->swUpdate.flashWrittenBytes, 4);
						memcpy(pucBuffer+ 4, &_length, 2);
						memcpy(pucBuffer+ 6, eBc66Trx.coap.Lwm2mGet.rxData+ _fwOffset, _length);

						TLV_t _tlv;
						uint8_t _rv[1];
						_tlv.Tg= SYS_TLVTag;
						_tlv.t= WRITE_FLASH_SysTLVTag;
						_tlv.v= pucBuffer;
						_tlv.rv= _rv;
						SYS_TLVRequest(&_tlv);
						if(SUCCESS== _tlv.rv[0])
						{
							pLwObj->rte.swMgt->swUpdate.flashWriteCount++;
							pLwObj->rte.swMgt->swUpdate.flashWrittenBytes+= _length;
							pLwObj->rte.swMgt->swUpdate.flashWriteAddress+= _length;

							pLwObj->rte.swMgt->swUpdate.state= pLwObj->rte.swMgt->swUpdate.prevState= REST_SwDownloadState;
							pLwObj->rte.swMgt->swUpdate.currentBlock++;
							pLwObj->rte.swMgt->swUpdate.retryCount= 0;
						}
						else
						{
							if(pLwObj->rte.swMgt->swUpdate.retryCount< 3)
							{
								pLwObj->rte.swMgt->swUpdate.retryCount++;
								pLwObj->rte.swMgt->swUpdate.state= pLwObj->rte.swMgt->swUpdate.prevState= REST_SwDownloadState;
							}
							else
							{
								pLwObj->rte.swMgt->swUpdate.state= FAILED_SwDownloadState;
								pLwObj->rte.swMgt->swUpdate.status= (uint8_t)INTEGRITY_FAILURE_SwMgtUpdateResult;
							}
						}
					}
					else if(3== ucResponseStatus)
					{
						pLwObj->rte.swMgt->swUpdate.state= FAILED_SwDownloadState;
						pLwObj->rte.swMgt->swUpdate.status= (uint8_t)INVALID_URI_SwMgtUpdateResult;
					}
					else if((1== ucSendStatus)|| (1== ucResponseStatus))
					{
						DBG_Print("STORE_PACKET failed!");
						DIAG_Code(SWGT_STORE_ERROR_NbiotLwm2mDCode, pLwObj->rte.swMgt->swUpdate.flashWrittenBytes);
						pLwObj->rte.swMgt->swUpdate.state= SET_BACKOFF_SwDownloadState;
					}
					break;
				case SET_BACKOFF_SwDownloadState:
					if(pLwObj->rte.swMgt->swUpdate.retryCount< 3)
					{
						pLwObj->rte.swMgt->swUpdate.retryCount++;
						tFetchTimestamp= SYS_GetTimestamp_s()+ UTILI_GetRandom(config.nbiot.gkcoap.retryBackoffMin_s, config.nbiot.gkcoap.retryBackoffMax_s);
						pLwObj->rte.swMgt->swUpdate.state= BACKOFF_SwDownloadState;
						//DBG_Print("Backoff.\r\n");
					}
					else
					{
						pLwObj->rte.swMgt->swUpdate.state= SUSPEND_SwDownloadState;
						tFetchTimestamp= SYS_GetTimestamp_s()+ (2*3600);
						//DBG_Print("Suspend.\r\n");
					}
					DIAG_Code(SWGT_BACKOFF_NbiotLwm2mDCode, tFetchTimestamp);
					break;
				case BACKOFF_SwDownloadState:
					if(SYS_GetTimestamp_s()>= tFetchTimestamp)
					{
						pLwObj->rte.swMgt->swUpdate.state= pLwObj->rte.swMgt->swUpdate.prevState;
						_awakeTime= _currTime;
					}
					else
					{
						_awakeTime= UTILI_GetSmallerTime(_awakeTime, tFetchTimestamp);
					}
					break;
				case SUSPEND_SwDownloadState:
					if(SYS_GetTimestamp_s()>= tFetchTimestamp)
					{
						pLwObj->rte.swMgt->swUpdate.state= pLwObj->rte.swMgt->swUpdate.prevState;
						_awakeTime= _currTime;
					}
					else
					{
						_awakeTime= UTILI_GetSmallerTime(_awakeTime, tFetchTimestamp);
					}
					break;
				case DOWNLOADED_SwDownloadState:
					SWMGT_Link_HardReset();/*lwupdate becomes unresponsive after we finished ota,
					//its safer to reset modem, to put modem in known state*/
					pLwObj->resource[UPDATE_STATE_SwMgtResource].value.integer= DOWNLOADED_SwMgtUpdateState;
					SWMGT_NotifyResource(pLwObj, UPDATE_STATE_SwMgtResource, false);
					pLwObj->resource[UPDATE_RESULT_SwMgtResource].value.integer= SUCCESSSFULLY_DOWNLOADED_SwMgtUpdateResult;
					SWMGT_NotifyResource(pLwObj, UPDATE_RESULT_SwMgtResource, false);
					break;
				case FAILED_SwDownloadState:
					pLwObj->resource[UPDATE_RESULT_SwMgtResource].value.integer= pLwObj->rte.swMgt->swUpdate.status;
					SWMGT_NotifyResource(pLwObj, UPDATE_RESULT_SwMgtResource, false);
					SWMGT_ResetToInitial(pLwObj, false);
					break;
				default:
					break;
			}
			break;
		case DOWNLOADED_SwMgtUpdateState:
			pLwObj->resource[UPDATE_STATE_SwMgtResource].value.integer=	DELIVERED_SwMgtUpdateState;
			SWMGT_NotifyResource(pLwObj, UPDATE_STATE_SwMgtResource, false);
			break;
		case DELIVERED_SwMgtUpdateState:
			/*wait for install execution*/
			break;
		case INSTALLED_SwMgtUpdateState:
			/*wait for activate / de-activation execution*/
			break;
	}

	return _awakeTime;
}
