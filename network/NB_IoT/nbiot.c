/*
 * nbiot.c
 *
 *  Created on: 5 Feb 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#include "common.h"
#include "nbiot.h"
#include "lwm2m.h"
#include "gkcoap.h"

static NBIOT_t *pConfig;
static bool bGKCOAPTaskEnabled= false;
static bool bLWM2MTaskEnabled= false;


void NBIOT_TLVRequest(TLV_t *_tlv)
{
	_tlv->rv[0]= SUCCESS;
	_tlv->rl= 1;

	switch(_tlv->t)
	{
		case GET_INFO_NbiotTLVTag:
			{
				uint8_t _len= 0;
				char *_string;
				_tlv->rv[_tlv->rl++]= _tlv->v[0];
				switch(_tlv->v[0])
				{
					case 0:
						_string= (char*)sBC66Link.BC66Version;
						break;
					case 1:
						_string= pConfig->imei;
						break;
					case 2:
						_string= pConfig->imsi;
						break;
					case 3:
						_string= pConfig->iccid;
						break;
					case 4:
						_string= pConfig->pdpAddress;
						break;
					default:
						_tlv->rv[0]= ERROR;
						return;
				}
				_len= strlen(_string);
				memcpy(_tlv->rv+ _tlv->rl, _string, _len);
				_tlv->rl+= _len;
				_tlv->rv[_tlv->rl++]= '\0';
			}
			break;

		case GET_CONFIG_NbiotTLVTag:
			{
				_tlv->rv[_tlv->rl++]= _tlv->v[0];
				switch(_tlv->v[0])
				{
					case 0:
						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->activeTime);
						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->periodicTau);
						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->edrx);
						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->pagingTime);
						_tlv->rl+= UTILI_Array_Copy16(_tlv->rv+ _tlv->rl, pConfig->delayBetweenCSQ_s);
						_tlv->rl+= UTILI_Array_Copy16(_tlv->rv+ _tlv->rl, pConfig->maximumCSQRetry);
						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->restartDelay_s);
						_tlv->rl+= UTILI_Array_Copy16(_tlv->rv+ _tlv->rl, pConfig->maxUnknwonFailureAllowed);
						break;
					case 1:
						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->rteActiveTime);
						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->rtePeriodicTau);
						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->rteEdrx);
						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->rtePagingTime);
						break;
					case 2:
						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->stats.noOfTransmission);
						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->stats.noOfFailedTransmission);
						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->stats.noOfAttach);
						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->stats.noOfDisattach);
						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->stats.noOfSimError);
						break;
					default:
						_tlv->rv[0]= ERROR;
						return;
				}
			}
			break;

		case SET_CONFIG_NbiotTLVTag:
			{
				uint16_t _index= 1;
				_tlv->rv[_tlv->rl]= _tlv->v[0];
				switch(_tlv->v[0])
				{
					case 0:
						_index+= UTILI_Array_Copy32_Ptr(&(pConfig->activeTime), _tlv->v+ _index);
						_index+= UTILI_Array_Copy32_Ptr(&(pConfig->periodicTau), _tlv->v+ _index);
						_index+= UTILI_Array_Copy32_Ptr(&(pConfig->edrx), _tlv->v+ _index);
						_index+= UTILI_Array_Copy32_Ptr(&(pConfig->pagingTime), _tlv->v+ _index);
						_index+= UTILI_Array_Copy16_Ptr(&(pConfig->delayBetweenCSQ_s), _tlv->v+ _index);
						_index+= UTILI_Array_Copy16_Ptr(&(pConfig->maximumCSQRetry), _tlv->v+ _index);
						_index+= UTILI_Array_Copy32_Ptr(&(pConfig->restartDelay_s), _tlv->v+ _index);
						_index+= UTILI_Array_Copy16_Ptr(&(pConfig->maxUnknwonFailureAllowed), _tlv->v+ _index);
						break;
					case 1:
						_index+= UTILI_Array_Copy32_Ptr(&(pConfig->rteActiveTime), _tlv->v+ _index);
						_index+= UTILI_Array_Copy32_Ptr(&(pConfig->rtePeriodicTau), _tlv->v+ _index);
						_index+= UTILI_Array_Copy32_Ptr(&(pConfig->rteEdrx), _tlv->v+ _index);
						_index+= UTILI_Array_Copy32_Ptr(&(pConfig->rtePagingTime), _tlv->v+ _index);
						break;
					case 2:
						_index+= UTILI_Array_Copy32_Ptr(&(pConfig->stats.noOfTransmission), _tlv->v+ _index);
						_index+= UTILI_Array_Copy32_Ptr(&(pConfig->stats.noOfFailedTransmission), _tlv->v+ _index);
						_index+= UTILI_Array_Copy32_Ptr(&(pConfig->stats.noOfAttach), _tlv->v+ _index);
						_index+= UTILI_Array_Copy32_Ptr(&(pConfig->stats.noOfDisattach), _tlv->v+ _index);
						_index+= UTILI_Array_Copy32_Ptr(&(pConfig->stats.noOfSimError), _tlv->v+ _index);
						break;
					default:
						_tlv->rv[0]= ERROR;
						return;
				}
			}
			break;

		case RESET_MODEM_NbiotTLVTag:
			{
				BC66LINK_ResetModem();
			}
			break;

		case GET_NETWORK_INFO_NbiotTLVTag:
			{
				_tlv->rv[_tlv->rl++]= pConfig->modemMode;
				_tlv->rv[_tlv->rl++]= sBC66Link.state;
				_tlv->rv[_tlv->rl++]= BC66LINK_SIMIsPresent();
				_tlv->rv[_tlv->rl++]= BC66LINK_NetworkIsAvailable();
				_tlv->rv[_tlv->rl++]= BC66LINK_RegistrationStatus();
				_tlv->rv[_tlv->rl++]= BC66LINK_NetworkDeniedCount();
				_tlv->rv[_tlv->rl++]= BC66LINK_TransmissionIsReady();
				_tlv->rv[_tlv->rl++]= sBC66Link.status.isConnected;
				_tlv->rv[_tlv->rl++]= sBC66Link.status.inPSM;
				_tlv->rv[_tlv->rl++]= (((true== pConfig->gkcoap.enabled)&& GKCOAP_Register_Completed())
						|| ((true== pConfig->lwm2m.enabled)&& LWM2M_IsRegistered()))? true: false;
			}
			break;

		case SET_MODEM_MODE_NbiotTLVTag:
			{
				BC66LINK_RequestRFSignalling(false, 0, 0);/*exit RF signalling mode*/
				pConfig->modemMode= (NBIOT_ModemMode_t)_tlv->v[0];
				if(RF_SIGNALLING_ModemMode== pConfig->modemMode)
				{
					BC66LINK_RequestRFSignalling(_tlv->v[1],  MAKELONG(_tlv->v[5], _tlv->v[4], _tlv->v[3], _tlv->v[2]), _tlv->v[6]);
				}
				BC66LINK_ResetJob();/*compulsory reset*/
			}
			break;

		case GET_OPERATION_INFO_NbiotTLVTag:
			{
				_tlv->rv[_tlv->rl++]= _tlv->v[0];
				switch(_tlv->v[0])
				{
					case 0:
						_tlv->rl+= UTILI_Array_Copy(_tlv->rv+ _tlv->rl, (uint8_t*)&(sBC66Link.urc.QENG), sizeof(sBC66Link.urc.QENG));
						break;
					case 1:
						_tlv->rl+= UTILI_Array_Copy(_tlv->rv+ _tlv->rl, (uint8_t*)&(config.nbiot.stats), sizeof(config.nbiot.stats));
						break;
					default:
						_tlv->rv[0]= ERROR;
						return;
				}
			}
			break;

		case LWM2M_GET_CONFIG_NbiotTLVTag:
			{
				LWOBJ_Obj_t *_obj;
				LWOBJ_Resource_t *_rsc;
				_tlv->rv[_tlv->rl++]= _tlv->v[0];
				switch(_tlv->v[0])
				{
					case 0:
						_tlv->rv[_tlv->rl++]= pConfig->lwm2m.enabled;
						_tlv->rv[_tlv->rl++]= pConfig->lwm2m.enableBootstrap;
						_tlv->rl+= UTILI_Array_CopyString(_tlv->rv+ _tlv->rl, pConfig->lwm2m.defaultConnection.serverIP);
						_tlv->rl+= UTILI_Array_Copy16(_tlv->rv+ _tlv->rl, pConfig->lwm2m.defaultConnection.serverPort);
						_tlv->rl+= UTILI_Array_CopyString(_tlv->rv+ _tlv->rl, pConfig->lwm2m.defaultConnection.endpointName);
						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->lwm2m.defaultConnection.lifetime_s);
						_tlv->rv[_tlv->rl++]= pConfig->lwm2m.defaultConnection.securityMode;
						_tlv->rl+= UTILI_Array_CopyString(_tlv->rv+ _tlv->rl, pConfig->lwm2m.defaultConnection.pskId);
						_tlv->rl+= UTILI_Array_CopyString(_tlv->rv+ _tlv->rl, pConfig->lwm2m.defaultConnection.psk);
						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->lwm2m.registerBackoff_s);
						_tlv->rv[_tlv->rl++]= pConfig->lwm2m.notifyACK;
						_tlv->rv[_tlv->rl++]= pConfig->lwm2m.notifyRAI;
						_tlv->rv[_tlv->rl++]= pConfig->lwm2m.reregisterAfterPSM;
						_tlv->rv[_tlv->rl++]= MAX_ObjName;
						break;
					case 1:
						_obj= &(pConfig->lwm2m.lwObj[_tlv->v[1]]);
						_tlv->rv[_tlv->rl++]= _tlv->v[1];
						_tlv->rv[_tlv->rl++]= _obj->type;
						_tlv->rl+= UTILI_Array_Copy16(_tlv->rv+ _tlv->rl, _obj->id);
						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, _obj->version);
						_tlv->rv[_tlv->rl++]= _obj->instance;
						_tlv->rv[_tlv->rl++]= _obj->resourceCount;
						break;
					case 2:
						_obj= &(pConfig->lwm2m.lwObj[_tlv->v[1]]);
						_rsc= &(_obj->resource[_tlv->v[2]]);
						_tlv->rv[_tlv->rl++]= _tlv->v[1];
						_tlv->rv[_tlv->rl++]= _tlv->v[2];
						_tlv->rl+= UTILI_Array_Copy16(_tlv->rv+ _tlv->rl, _rsc->attr.id);
						_tlv->rv[_tlv->rl++]= _rsc->attr.operation;
						_tlv->rv[_tlv->rl++]= _rsc->attr.type;
						_tlv->rv[_tlv->rl++]= _rsc->observe;
						_tlv->rv[_tlv->rl++]= _rsc->notifyState;
						_tlv->rv[_tlv->rl++]= _rsc->written;
						_tlv->rl+= UTILI_Array_Copy16(_tlv->rv+ _tlv->rl, _rsc->valueLen);
						_tlv->rl+= UTILI_Array_Copy16(_tlv->rv+ _tlv->rl, _rsc->valueMaxLen);
						_tlv->rl+= UTILI_Array_Copy(_tlv->rv+ _tlv->rl, (uint8_t*)&(_rsc->value), sizeof(LWOBJ_ValueType_t));
						break;
					case 3:
						_obj= &(pConfig->lwm2m.lwObj[_tlv->v[1]]);
						_rsc= &(_obj->resource[_tlv->v[2]]);
						uint16_t _offset= MAKEWORD(_tlv->v[4], _tlv->v[3]);
						uint16_t _length= MAKEWORD(_tlv->v[6], _tlv->v[5]);
						_tlv->rv[_tlv->rl++]= _tlv->v[1];
						_tlv->rv[_tlv->rl++]= _tlv->v[2];
						_tlv->rl+= UTILI_Array_Copy16(_tlv->rv+ _tlv->rl, _offset);
						_tlv->rl+= UTILI_Array_Copy16(_tlv->rv+ _tlv->rl, _length);
						if((STRING_ResourceType== _rsc->attr.type)|| (OPAQUE_ResourceType== _rsc->attr.type))
						{
							_tlv->rl+= UTILI_Array_Copy(_tlv->rv+ _tlv->rl, (_rsc->value.opaque)+ _offset, _length);
						}
						break;
					default:
						_tlv->rv[0]= ERROR;
						return;
				}
			}
			break;

		case LWM2M_SET_CONFIG_NbiotTLVTag:
			{
				LWOBJ_Obj_t *_obj;
				LWOBJ_Resource_t *_rsc;
				uint16_t _index= 1;
				_tlv->rv[_tlv->rl]= _tlv->v[0];
				switch(_tlv->v[0])
				{
					case 0:
						{
							bool _prevEnabled= pConfig->gkcoap.enabled;

							pConfig->lwm2m.enabled= _tlv->v[_index++];
							pConfig->lwm2m.enableBootstrap= _tlv->v[_index++];
							_index+= UTILI_Array_CopyString(pConfig->lwm2m.defaultConnection.serverIP, _tlv->v+ _index);
							_index+= UTILI_Array_Copy16_Ptr(&(pConfig->lwm2m.defaultConnection.serverPort), _tlv->v+ _index);
							_index+= UTILI_Array_CopyString(pConfig->lwm2m.defaultConnection.endpointName, _tlv->v+ _index);
							_index+= UTILI_Array_Copy32_Ptr(&(pConfig->lwm2m.defaultConnection.lifetime_s), _tlv->v+ _index);
							pConfig->lwm2m.defaultConnection.securityMode= _tlv->v[_index++];
							_index+= UTILI_Array_CopyString(pConfig->lwm2m.defaultConnection.pskId, _tlv->v+ _index);
							_index+= UTILI_Array_CopyString(pConfig->lwm2m.defaultConnection.psk, _tlv->v+ _index);
							_index+= UTILI_Array_Copy32_Ptr(&(pConfig->lwm2m.registerBackoff_s), _tlv->v+ _index);
							pConfig->lwm2m.notifyACK= _tlv->v[_index++];
							pConfig->lwm2m.notifyRAI= _tlv->v[_index++];
							pConfig->lwm2m.reregisterAfterPSM= _tlv->v[_index++];

							if(_prevEnabled!= pConfig->gkcoap.enabled)
							{
								//LWM2M_Init(&(config.nbiot.lwm2m));/*not sure consequence of this*/
								SYS_Request(SOFT_REBOOT_SysRequest);
							}
						}
						break;
					case 1:
						_obj= &(pConfig->lwm2m.lwObj[_tlv->v[_index++]]);
						_obj->type= _tlv->v[_index++];
						_index+= UTILI_Array_Copy16_Ptr(&(_obj->id), _tlv->v+ _index);
						_index+= UTILI_Array_Copy32_Ptr(&(_obj->version), _tlv->v+ _index);
						_obj->instance= _tlv->v[_index++];
						_obj->resourceCount= _tlv->v[_index++];
						break;
					case 2:
						_obj= &(pConfig->lwm2m.lwObj[_tlv->v[_index++]]);
						_rsc= &(_obj->resource[_tlv->v[_index++]]);
						_index+= UTILI_Array_Copy16_Ptr(&(_rsc->attr.id), _tlv->v+ _index);
						_rsc->attr.operation= _tlv->v[_index++];
						_rsc->attr.type= _tlv->v[_index++];
						_rsc->observe= _tlv->v[_index++];
						_rsc->notifyState= _tlv->v[_index++];
						_rsc->written= _tlv->v[_index++];
						if(EXECUTE_ResourceOperation!= _rsc->attr.operation)
						{
							_index+= UTILI_Array_Copy16_Ptr(&(_rsc->valueLen), _tlv->v+ _index);
							_index+= UTILI_Array_Copy16_Ptr(&(_rsc->valueMaxLen), _tlv->v+ _index);
							if((STRING_ResourceType!= _rsc->attr.type)&& (OPAQUE_ResourceType!= _rsc->attr.type))
							{
								_index+= UTILI_Array_Copy((uint8_t*)&(_rsc->value), _tlv->v+ _index, sizeof(LWOBJ_ValueType_t));
							}
						}
						break;
					case 3:
						_obj= &(pConfig->lwm2m.lwObj[_tlv->v[_index++]]);
						_rsc= &(_obj->resource[_tlv->v[_index++]]);
						uint16_t _offset;
						uint16_t _length;
						_index+= UTILI_Array_Copy16_Ptr(&_offset, _tlv->v+ _index);
						_index+= UTILI_Array_Copy16_Ptr(&_length, _tlv->v+ _index);
						if(EXECUTE_ResourceOperation!= _rsc->attr.operation)
						{
							if((STRING_ResourceType== _rsc->attr.type)|| (OPAQUE_ResourceType== _rsc->attr.type))
							{
								_index+= UTILI_Array_Copy((_rsc->value.opaque)+ _offset, _tlv->v+ _index, _length);
								if((_rsc->valueLen)< (_offset + _length))
								{
									_rsc->valueLen=  (_offset + _length);
								}
							}
						}
						break;
					default:
						_tlv->rv[0]= ERROR;
						return;
				}
			}
			break;

		case LWM2M_REQUEST_NbiotTLVTag:
			{
				_tlv->rv[_tlv->rl++]= _tlv->v[0];
				switch(_tlv->v[0])
				{
					case 0:
						LWM2M_RequestRegister();
						break;
					case 1:
						break;
					default:
						_tlv->rv[0]= ERROR;
						return;
				}
			}
			break;

		case GKCOAP_GET_CONFIG_NbiotTLVTag:
			{
				_tlv->rv[_tlv->rl++]= _tlv->v[0];
				switch(_tlv->v[0])
				{
					case 0:
						_tlv->rv[_tlv->rl++]= pConfig->gkcoap.enabled;
						_tlv->rl+= UTILI_Array_Copy16(_tlv->rv+ _tlv->rl, pConfig->gkcoap.localPort);
						_tlv->rl+= UTILI_Array_CopyString(_tlv->rv+ _tlv->rl, pConfig->gkcoap.serverIP);
						_tlv->rl+= UTILI_Array_Copy16(_tlv->rv+ _tlv->rl, pConfig->gkcoap.serverPort);
						_tlv->rl+= UTILI_Array_CopyString(_tlv->rv+ _tlv->rl, pConfig->gkcoap.serverURI);
						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->gkcoap.retryBackoffMin_s);
						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->gkcoap.retryBackoffMax_s);
						_tlv->rv[_tlv->rl++]= pConfig->gkcoap.retryMax;
						_tlv->rv[_tlv->rl++]= pConfig->gkcoap.packetType;
						_tlv->rv[_tlv->rl++]= pConfig->gkcoap.logTickType;
						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->gkcoap.logTickSize);
						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->gkcoap.logTickStartMask);
						_tlv->rl+= UTILI_Array_Copy64(_tlv->rv+ _tlv->rl, pConfig->gkcoap.reportStartMask);
						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->gkcoap.reportInterval_s);
						_tlv->rv[_tlv->rl++]= pConfig->gkcoap.reportTriggerType;
						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->gkcoap.reportTriggerLogCount);
						break;
					case 1:
						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->gkcoap.rteStartTime);
						_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->gkcoap.rteNextTime);
						break;
					default:
						_tlv->rv[0]= ERROR;
						return;
				}
			}
			break;

		case GKCOAP_SET_CONFIG_NbiotTLVTag:
			{
				uint16_t _index= 1;
				_tlv->rv[_tlv->rl]= _tlv->v[0];
				switch(_tlv->v[0])
				{
					case 0:
						{
							bool _prevEnabled= pConfig->gkcoap.enabled;
							pConfig->gkcoap.enabled= _tlv->v[_index++];
							_index+= UTILI_Array_Copy16_Ptr(&(pConfig->gkcoap.localPort), _tlv->v+ _index);
							_index+= UTILI_Array_CopyString(pConfig->gkcoap.serverIP, _tlv->v+ _index);
							_index+= UTILI_Array_Copy16_Ptr(&(pConfig->gkcoap.serverPort), _tlv->v+ _index);
							_index+= UTILI_Array_CopyString(pConfig->gkcoap.serverURI, _tlv->v+ _index);
							_index+= UTILI_Array_Copy32_Ptr(&(pConfig->gkcoap.retryBackoffMin_s), _tlv->v+ _index);
							_index+= UTILI_Array_Copy32_Ptr(&(pConfig->gkcoap.retryBackoffMax_s), _tlv->v+ _index);
							pConfig->gkcoap.retryMax= _tlv->v[_index++];
							pConfig->gkcoap.packetType= _tlv->v[_index++];
							pConfig->gkcoap.logTickType= _tlv->v[_index++];
							_index+= UTILI_Array_Copy32_Ptr(&(pConfig->gkcoap.logTickSize), _tlv->v+ _index);
							_index+= UTILI_Array_Copy32_Ptr(&(pConfig->gkcoap.logTickStartMask), _tlv->v+ _index);
							_index+= UTILI_Array_Copy64_Ptr(&(pConfig->gkcoap.reportStartMask), _tlv->v+ _index);
							_index+= UTILI_Array_Copy32_Ptr(&(pConfig->gkcoap.reportInterval_s), _tlv->v+ _index);
							pConfig->gkcoap.reportTriggerType= _tlv->v[_index++];
							_index+= UTILI_Array_Copy32_Ptr(&(pConfig->gkcoap.reportTriggerLogCount), _tlv->v+ _index);
							_index+= UTILI_Array_Copy32_Ptr(&(pConfig->gkcoap.rteStartTime), _tlv->v+ _index);
							_index+= UTILI_Array_Copy32_Ptr(&(pConfig->gkcoap.rteNextTime), _tlv->v+ _index);

							if(_prevEnabled!= pConfig->gkcoap.enabled)
							{
								//GKCOAP_Init(&(config.nbiot.gkcoap));/*not sure consequence of this*/
								SYS_Request(SOFT_REBOOT_SysRequest);
							}
						}
						break;
					case 1:
						_index+= UTILI_Array_Copy32_Ptr(&(pConfig->gkcoap.rteStartTime), _tlv->v+ _index);
						_index+= UTILI_Array_Copy32_Ptr(&(pConfig->gkcoap.rteNextTime), _tlv->v+ _index);
						break;
				}
			}
			break;

		case GKCOAP_REQUEST_NbiotTLVTag:
			{
				_tlv->rv[_tlv->rl++]= _tlv->v[0];
				switch(_tlv->v[0])
				{
					case 0:
						GKCOAP_Register_Request();
						break;
					case 1:
						GKCOAP_Report_Request();
						break;
					case 2:
						GKCOAP_Fota_Request();
						break;
					default:
						_tlv->rv[0]= ERROR;
						return;
				}
			}
			break;

		case LWMWM_LOG_NbiotTLVTag:
			{
				_tlv->rv[_tlv->rl++]= _tlv->v[0];
				switch(_tlv->v[0])
				{
					case 0:/*get metric*/
						{
							LWOBJ_Obj_t *_obj= &(pConfig->lwm2m.lwObj[_tlv->v[1]]);
							_tlv->rv[_tlv->rl++]= _tlv->v[1];
							_tlv->rv[_tlv->rl++]= _obj->rte.prAct->log.partition;
							_tlv->rl+= UTILI_Array_Copy16(_tlv->rv+ _tlv->rl, _obj->rte.prAct->log.head);
							_tlv->rl+= UTILI_Array_Copy16(_tlv->rv+ _tlv->rl, _obj->rte.prAct->log.elementCount);
							_tlv->rl+= UTILI_Array_Copy16(_tlv->rv+ _tlv->rl, _obj->rte.prAct->log.elementMax);
							_tlv->rl+= UTILI_Array_Copy16(_tlv->rv+ _tlv->rl, _obj->rte.prAct->log.elementSize);
						}
						break;

					case 1:/*get log*/
						{
							uint16_t _index= 1;
							LWOBJ_Obj_t *_obj= &(pConfig->lwm2m.lwObj[_tlv->v[_index++]]);
							uint16_t _logIndex;
							uint16_t _logCount;
							_tlv->rv[_tlv->rl++]= _tlv->v[1];
							_index+= UTILI_Array_Copy16_Ptr(&_logIndex, _tlv->v+ _index);
							_index+= UTILI_Array_Copy16_Ptr(&_logCount, _tlv->v+ _index);
							if(SUCCESS== LOGGER_ReadSync(&(_obj->rte.prAct->log), _logIndex, _logCount, _tlv->rv+ _tlv->rl))
							{
								_tlv->rl+= _logCount* _obj->rte.prAct->log.elementSize;
							}
							else
							{
								_tlv->rv[0]= ERROR;
							}
						}
						break;
				}
			}
			break;

		default:
			_tlv->rv[0]= ERROR;
			break;
	}
}

void NBIOT_Init(NBIOT_t *_config)
{
	pConfig= _config;

	BC66LINK_Init();

	if(true== pConfig->lwm2m.enabled)
	{
		bLWM2MTaskEnabled= true;
		LWM2M_Init(&(pConfig->lwm2m));
	}

	if(true== pConfig->gkcoap.enabled)
	{
		bGKCOAPTaskEnabled= true;
		GKCOAP_Init(&(pConfig->gkcoap));
	}
}

void NBIOT_AutoAttachTask(void)/*this task will check sensors parameter to reboot modem(force connection)*/
{
	static float _prevPBMagCount= 0;
	static bool _pulserThresholdChecked= false;
	static float _prevTiltCount= 0;
	static time_t _tiltTamperConnectBackofEndTime= 0;

	time_t _currTime= SYS_GetTimestamp_s();
	bool _rebootModem= false;

	if(true== pConfig->enableAttachOnMagnetTamper)
	{
		float _currPBMagCount= SENSOR_GetValue(PBMAGCOUNT_Sensor);
		if(_currPBMagCount> _prevPBMagCount)
		{
			_rebootModem= true;
		}
		_prevPBMagCount= _currPBMagCount;
	}

	if(true== pConfig->enableAttachOnPulserThreshold)
	{
		if((false== _pulserThresholdChecked)&& (PULSER_ValueInLiter_Get()>= pConfig->pulserThresholdValueForAttach_liter))
		{
			_pulserThresholdChecked= true;
			_rebootModem= true;
		}
		else if((true== _pulserThresholdChecked)&& (PULSER_ValueInLiter_Get()< pConfig->pulserThresholdValueForAttach_liter))
		{
			_pulserThresholdChecked= false;/*reset back when reverse flow, aka gk production flow test jic*/
		}
	}

	if(true== pConfig->enableAttachOnTiltTamper)
	{
		float _currTiltCount= SENSOR_GetValue(TILTCOUNT_Sensor);
		if((_currTiltCount> _prevTiltCount)&& (_currTime>= _tiltTamperConnectBackofEndTime))
		{
			_tiltTamperConnectBackofEndTime= _currTime+ pConfig->tiltTamperAttachBackoff_s;
			_rebootModem= true;
		}
		_prevPBMagCount= _currTiltCount;
	}

	if(true== _rebootModem)
	{
		_rebootModem= false;
		if(true== BC66LINK_ModemDisabled())/*only reboot when modem is already disabled*/
		{
			BC66LINK_ResetJob();/*reset job is preferred to reset modem to confirm modem restart*/
		}
	}
}

void NBIOT_Task(void)
{
	BC66LINK_Task();

	if((true== bLWM2MTaskEnabled)|| (true== bGKCOAPTaskEnabled))
	{
		if(true== bLWM2MTaskEnabled)
		{
			LWM2M_Task();
		}

		if(true== bGKCOAPTaskEnabled)
		{
			GKCOAP_Task();
		}
	}
	else
	{
		NBIOT_AutoAttachTask();
	}
}

uint8_t NBIOT_TaskState(void)
{
	return (LWM2M_TaskState()| GKCOAP_TaskState());
}
