/*
 * bc66_link.c
 *
 *  Created on: 5 Jan 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#include <bc66handler.h>
#include "common.h"
#include "bc66link.h"
#include "bc66phy.h"
#include "bc66util.h"
#include "utili.h"
#include "batterysensor.h"
#include "security.h"

#define DBG_Print

static void BC66LINK_SetTimeout(uint32_t _miliseconds);
static bool BC66LINK_IsTimeout(void);

BC66LINK_InitLinkState_t BC66LINK_InitBaudrate(void);
BC66LINK_ProdBreakState_t BC66LINK_ProductionBreakpoint(void);
BC66LINK_RFSignallingState_t BC66LINK_RFSignalling(void);
BC66LINK_ConfigureState_t BC66LINK_Configure(void);
BC66LINK_AttachmentState_t BC66LINK_Attachment(void);
BC66LINK_ProcessRequestState_t BC66LINK_ProcessRequest(void);
BC66LINK_ExitSleepState_t BC66LINK_ExitSleep(void);
BC66LINK_EnterSleepState_t BC66LINK_EnterSleep(void);
BC66LINK_DisableState_t BC66LINK_Disable(void);
//void BC66LINK_COAP_Create(void *_param);
//void BC66LINK_COAP_Delete(void *_param);
//void BC66LINK_COAP_AddResource(void *_param);
//void BC66LINK_COAP_ConfigHead(void *_param);
//void BC66LINK_COAP_ConfigOption(void *_param);
//void BC66LINK_COAP_Send(void *_param);
//void BC66LINK_COAP_SendStatus(void *_param);
//void BC66LINK_COAP_ConfigCommand(void *_param);
//void BC66LINK_COAP_AliSign(void *_param);
void BC66LINK_COAP_GKPost(void *_param);
void BC66LINK_COAP_LWM2MGet(void *_param);
void BC66LINK_LWM2M_Configure(void *_param);
void BC66LINK_LWM2M_Register(void *_param);
void BC66LINK_LWM2M_PostRegister(void *_param);/*we need some information post lw registration*/
void BC66LINK_LWM2M_Update(void *_param);
void BC66LINK_LWM2M_Deregister(void *_param);
void BC66LINK_LWM2M_AddObject(void *_param);
void BC66LINK_LWM2M_DeleteObject(void *_param);
void BC66LINK_LWM2M_WriteResponse(void *_param);
void BC66LINK_LWM2M_ReadResponse(void *_param);
void BC66LINK_LWM2M_ExecuteResponse(void *_param);
void BC66LINK_LWM2M_ObserveResponse(void *_param);
void BC66LINK_LWM2M_Notify(void *_param);
void BC66LINK_LWM2M_ReadData(void *_param);
void BC66LINK_LWM2M_Status(void *_param);

static BC66LINK_Transaction_t	eNoTrx= {
											.txBuffer= NULL,
											.rxBuffer= NULL,
											.transactionInProgress= false,
											.transactionCompleted= false,
											.TransactionCompletedCb= NULL,
										};
static BC66LINK_Transaction_t	*eTrxQueue[BC66LINK_CFG_MAX_TRX_QUEUE];
static BC66LINK_Transaction_t	*eCurrentTrx= &eNoTrx;
static uint8_t 					ucTrxQueueDepth= 0;
static uint8_t 					ucTrxDequeueIndex= 0;
__IO char pucTxBuffer[BC66LINK_CFG_TX_BUF_SIZE];
BC66LINK_t sBC66Link={
							.enable= true,
					};

const char *rx_reset[]= 		{"RbbRDY"};
//const char *rx_resetFull[]= 	{NULL, "F1:", "V0:", "00:", "01:", "U0:", "T0:", "Leaving the BROM" "OK"};
//const char *rx_reboot_part0[]= 	{"REBOOTING"};
//const char *rx_reboot_part1[]= 	{"OK", "OK", "REBOOTING"};
//const char *rx_reboot_part2[]= 	{NULL, "REBOOT_CAUSE_APPLICATION_AT", "u-blox ", "OK"};
const char *rx_EchoATE0[]=		{"ATE0","OK"};
//const char *rx_ATI[]=			{"Quectel_Ltd","Quectel_BC66", NULL, "OK"};
////const char *rx_EchoOK[]=		{"ATOK"};/*echo like "AT+IPR=115200"+"OK" will get reduced to "ATOK" cos "+IPR=115200" is treated as URCMsg*/
///*new bc66 fw change echoreply from AT\r\n to AT\r*/
const char *rx_EchoOK[]=		{"AT\r", "OK"};/*echo like "AT+IPR=115200"+"OK" will get reduced to "AT\r", "OK" cos "+IPR=115200" is treated as URCMsg*/
const char *rx_OK[]= 			{"OK", "OK", "OK", "OK", "OK", "OK"};
const char *rx_ERROR[]= 		{"ERROR"};
//const char *socket_OK[]= 		{"0", "OK"};
//const char *socketRx_OK[]= 		{"0,0", "OK"};
////const char *rx_NUESTATS[]= 		{"NUESTATS: \"CELL\"", "OK"};
const char *rx_CIMI[]= 			{NULL, "OK"};
//const char *rx_CGMR[]= 			{NULL, "OK"};
const char *rx_QGMR[]= 			{NULL, "OK"};

static void BC66LINK_SetTimeout(uint32_t _miliseconds)
{
	if(0!= _miliseconds)
	{
		SYS_Sleep(BC66_LINK_TaskId, _miliseconds);
	}
}

static bool BC66LINK_IsTimeout(void)
{
	return (true== SYS_IsAwake(BC66_LINK_TaskId));
}

uint8_t BC66LINK_QueueDepth(void)
{
	return ucTrxQueueDepth;
}

ErrorStatus BC66LINK_Enqueue(BC66LINK_Transaction_t *_trx)
{
	if(BC66LINK_CFG_MAX_TRX_QUEUE!= ucTrxQueueDepth)
	{
		_trx->transactionInQueue= true;
		_trx->transactionInProgress= false;
		_trx->transactionCompleted= false;
		_trx->txSent= false;
		_trx->rxReceived= false;
		_trx->state= 0;
		eTrxQueue[(ucTrxDequeueIndex+ ucTrxQueueDepth)% BC66LINK_CFG_MAX_TRX_QUEUE]= _trx;
		ucTrxQueueDepth++;
		return SUCCESS;
	}

	return ERROR;
}

BC66LINK_Transaction_t *BC66LINK_Dequeue(void)
{
	BC66LINK_Transaction_t *_trx= NULL;
	if(0!= ucTrxQueueDepth)
	{
		_trx= eTrxQueue[ucTrxDequeueIndex];
		_trx->transactionInQueue= false;
		_trx->transactionInProgress= true;
		ucTrxQueueDepth--;
		ucTrxDequeueIndex= (ucTrxDequeueIndex+ 1)% BC66LINK_CFG_MAX_TRX_QUEUE;
	}
	return _trx;
}

void BC66LINK_COAP_SetResponseCallback(void (*Response)(void *))
{
	sBC66Link.coapCallback.Response= Response;
}

void BC66LINK_COAP_SetRequestCallback(void (*Request)(void *))
{
	sBC66Link.coapCallback.Request= Request;
}

void BC66LINK_COAP_SetRecoveryCallback(void (*Recovery)(void *))
{
	sBC66Link.coapCallback.Recovery= Recovery;
}

void BC66LINK_COAP_SetResponseCallback2(void (*Response)(void *))
{
	sBC66Link.coapCallback2.Response= Response;
}

void BC66LINK_COAP_SetRequestCallback2(void (*Request)(void *))
{
	sBC66Link.coapCallback2.Request= Request;
}

void BC66LINK_COAP_SetRecoveryCallback2(void (*Recovery)(void *))
{
	sBC66Link.coapCallback2.Recovery= Recovery;
}

void BC66LINK_LWM2M_SetWriteRequestCallback(void (*WriteRequest)(void *))
{
	sBC66Link.lwm2mCallback.WriteRequest= WriteRequest;
}

void BC66LINK_LWM2M_SetReadRequestCallback(void (*ReadRequest)(void *))
{
	sBC66Link.lwm2mCallback.ReadRequest= ReadRequest;
}

void BC66LINK_LWM2M_SetExecuteRequestCallback(void (ExecuteRequest)(void *))
{
	sBC66Link.lwm2mCallback.ExecuteRequest= ExecuteRequest;
}

void BC66LINK_LWM2M_SetObserveRequestCallback(void (*ObserveRequest)(void *))
{
	sBC66Link.lwm2mCallback.ObserveRequest= ObserveRequest;
}

void BC66LINK_LWM2M_SetRecoveryCallback(void (*Recovery)(void *))
{
	sBC66Link.lwm2mCallback.Recovery= Recovery;
}

void BC66LINK_LWM2M_SetRegisterCallback(void (*Register)(void *))
{
	sBC66Link.lwm2mCallback.Register= Register;
}

void BC66LINK_ClearIP(void)
{
	sBC66Link.status.ip[0]= '\0';
}

bool BC66LINK_IPIsAssigned(void)
{
	return (sBC66Link.status.ip[0]!= '\0')? true: false;
}

void BC66LINK_StatusInit(void)
{
	/*enable is init one time only when declared.*///sBC66Link.enable= false;
	sBC66Link.state= INITIAL_Bc66LinkState;
	sBC66Link.subState= 0;

	sBC66Link.status.bResetExpected= false;/*we use this to rule out unintended bc66 reset*/
	sBC66Link.status.isConnected= false;
	sBC66Link.status.inPSM= false;
	sBC66Link.status.inhibitSleep= false;
	sBC66Link.status.sendLwUpdate= false;/*by default we send*/
	sBC66Link.status.qiOpened= false;/*by default we send qiopen*/

//	sBC66Link.status.ufotas01Received= false;
//	sBC66Link.status.isWaitingForUfotas01= false;
//	sBC66Link.status.cgpaddrCnt= 0;
//	sBC66Link.status.cgpaddrRetried= 0;
	sBC66Link.status.cmeErrorReceived= false;
	sBC66Link.status.cmeErrorType= 0;
//	sBC66Link.status.inPowerSavingMode= false;
//	sBC66Link.status.cpsmsIsEnabled= false;
//	sBC66Link.status.cpsmsTauValue= 0;
//	sBC66Link.status.cpsmsActiveTimeValue= 0;
//	sBC66Link.status.rssi= 99;
//	sBC66Link.status.rssiQueryReceived= false;
	sBC66Link.status.csqAttemptCount= 0;
//	sBC66Link.status.ceregAttemptCount= 0;
	sBC66Link.status.regDeniedCount= 0;
	//	sBC66Link.status.isAttached= false;
	//  sBC66Link.status.timeIsSynced= false;

//
//	sBC66Link.status.eDrx.actType= 0;
//	sBC66Link.status.eDrx.requested= 0;
//	sBC66Link.status.eDrx.provided= 0;
//	sBC66Link.status.eDrx.pagingTime= 0;
//
//	sBC66Link.status.cereg.regStatus= UNREGISTERED;
//	sBC66Link.status.cereg.trackingAreaCode= 0;
//	sBC66Link.status.cereg.cellId= 0;
//	sBC66Link.status.cereg.accessTechnology= 0;
//	sBC66Link.status.cereg.rejectType= 0;
//	sBC66Link.status.cereg.rejectCause= 0;
//	sBC66Link.status.cereg.activeTime= 0;
//	sBC66Link.status.cereg.periodicTau= 0;
//
//	sBC66Link.data.isSent= false;
//	sBC66Link.data.isReceived= false;
//
//	if(1== config.nbiot.activeIPPortSet)
//	{
//		sBC66Link.coapIP= config.info.coapIP;
//		sBC66Link.coapPort= config.info.coapPort;
//	}
//	else
//	{
//		sBC66Link.coapIP= config.info.coapIP2;
//		sBC66Link.coapPort= config.info.coapPort2;
//	}

	sBC66Link.urc.QLWREG= UNKNWON_Lwm2mRegStatus;
	sBC66Link.urc.QLWURC.recovered= 0;/*initially lwm2m context is "recovered"*/
	sBC66Link.coapContext= 0;

	if(true== eCurrentTrx->transactionInProgress)/*sometime sara reset while transaction is still happening*/
	{
		eCurrentTrx->transactionInProgress= false;
		eCurrentTrx->transactionCompleted= true;
	}

	config.nbiot.rtePeriodicTau= 0;
	config.nbiot.rteActiveTime=  0;
	config.nbiot.rteEdrx= 0;
	config.nbiot.rtePagingTime=  0;
}

void BC66LINK_Init(void)
{
	static bool _isInitialized= false;

	if(true== _isInitialized)
	{
		return;
	}
	_isInitialized= true;

	BC66PHY_Init();
	BC66PHY_SetRxTxPinOD(true, true);
	BC66PHY_PowerState(DISABLED_Bc66PhyPower);
	BC66PHY_PowerSignalPin(0);
	BC66PHY_ResetPin(0);

	BC66LINK_StatusInit();
	BC66LINK_ClearIP();
}

void BC66LINK_Enable(bool _enable)
{
	sBC66Link.enable= _enable;
}

bool BC66LINK_IsEnable(void)
{
	return sBC66Link.enable;
}

bool BC66LINK_SIMIsPresent(void)
{
	return BC66PHY_SIMDetected();
}

bool BC66LINK_NetworkIsAvailable(void)
{
	return ((99!= sBC66Link.status.rssi)/*&& (0!= sBC66Link.status.rssi)*/)? true: false;
}

bool BC66LINK_NetworkIsRegistered(void)
{
	return (
			(
				(REGISTERED_AS_HOME_BC66LinkNetwork== sBC66Link.status.cereg.regStatus)
				|| (REGISTERED_AS_ROAMING_BC66LinkNetwork== sBC66Link.status.cereg.regStatus)
			)
			&&(true== BC66LINK_NetworkIsAvailable())
			//&&(true== sBC66Link.status.isConnected)
			)? true: false;
}

uint8_t BC66LINK_RegistrationStatus(void)
{
	return sBC66Link.status.cereg.regStatus;
}

uint8_t BC66LINK_NetworkDeniedCount(void)
{
	return sBC66Link.status.regDeniedCount;
}

bool BC66LINK_TransmissionIsReady(void)
{
	return (
			((WAIT_REQUEST_Bc66LinkState== sBC66Link.state)|| (EXIT_SLEEP_Bc66LinkState== sBC66Link.state)|| (ENTER_SLEEP_Bc66LinkState== sBC66Link.state))
			&&(true== BC66LINK_NetworkIsRegistered())
			)? true: false;
}

bool BC66LINK_ModemDisabled(void)
{
	return (
			(DISABLE_Bc66LinkState== sBC66Link.state)
			||(DISABLE_ModemMode== config.nbiot.modemMode)
			)? true: false;
}

BC66LINK_Lwm2mRegStatus BC66LINK_LWM2MRegistrationState(void)
{
	return (BC66LINK_Lwm2mRegStatus) sBC66Link.urc.QLWREG;
}

bool BC66LINK_IsInBackOff(void)
{
	return false;
}

bool BC66LINK_IsInPSM(void)
{
	return sBC66Link.status.inPSM;
}

uint8_t BC66LINK_GetState(void)
{
	return sBC66Link.state;
}

uint8_t BC66LINK_GetSubState(void)
{
	return sBC66Link.subState;
}

void BC66LINK_SaveContext(void)
{

}

void BC66LINK_ResetModem(void)
{
	BC66PHY_Reset();
}

void BC66LINK_InvokeRecovery(void)
{
	if(NULL!= sBC66Link.coapCallback.Recovery)
	{
		BC66LINK_Coap_Recovery_t _recovery= {1/*fail state*/};
		sBC66Link.coapCallback.Recovery((void *) (&_recovery));
	}

	if(NULL!= sBC66Link.lwm2mCallback.Recovery)
	{
		BC66LINK_Lwm2m_Recovery_t _recovery= {1/*fail state*/};
		sBC66Link.lwm2mCallback.Recovery((void *) (&_recovery));
	}
}

void BC66LINK_ResetJob(void)
{
	/*Unintended reset occur*/
	while(true== BC66PHY_IsTxInProgress())
	{

	}
	BC66LINK_CancelTransaction();
	BC66LINK_InvokeRecovery();
	BC66LINK_SaveContext();/*save previous tx/rx time*/
	sBC66Link.state= INITIAL_Bc66LinkState;
	BC66PHY_ResetJob();
	BC66PHY_Job();/*just place this here to empty queue from POR rubbish*/
}

void BC66LINK_ReAttach(void)
{
	/*Unintended reset occur*/
	while(true== BC66PHY_IsTxInProgress())
	{

	}
	BC66LINK_CancelTransaction();
	BC66LINK_InvokeRecovery();
	BC66LINK_SaveContext();/*save previous tx/rx time*/
	sBC66Link.state= ATTACHMENT_Bc66LinkState;
	BC66PHY_ResetJob();
	BC66PHY_Job();/*just place this here to empty queue from POR rubbish*/

	/*needed flags that are set during init*/
	sBC66Link.urc.QLWREG= UNKNWON_Lwm2mRegStatus;
	sBC66Link.urc.QLWURC.recovered= 0;/*initially lwm2m context is "recovered"*/
}

void BC66LINK_CancelTransaction(void)
{
	while(eCurrentTrx!= NULL)
	{
	    if(true== eCurrentTrx->transactionInProgress)
	    {
	    	eCurrentTrx->status= ERROR_TransactionStatus;
	    	eCurrentTrx->transactionInProgress= false;
	    	eCurrentTrx->transactionCompleted= true;
			if(NULL!= eCurrentTrx->TransactionCompletedCb)
			{
				eCurrentTrx->TransactionCompletedCb();
			}
	    }
    	eCurrentTrx= BC66LINK_Dequeue();
	}
	eCurrentTrx= &eNoTrx;
	sBC66Link.state= WAIT_REQUEST_Bc66LinkState;
}

void BC66LINK_RequestRFSignalling(bool _enable, uint32_t _channel, uint8_t _dbm)
{
	sBC66Link.rfSignalling.enable= _enable;
	sBC66Link.rfSignalling.channel= _channel;
	sBC66Link.rfSignalling.dBm= _dbm;
}

void BC66LINK_StateTransition(BC66LINK_State_t _state, BC66LINK_State_t _nextState, uint8_t _subState, uint8_t _endSubState)
{
	if(_endSubState== _subState)
	{
		sBC66Link.subState= 0;
		if(_state== sBC66Link.state)/*next state may be set inside*/
		{
			sBC66Link.state= _nextState;
		}
	}
}

void BC66LINK_Task(void)
{
	uint16_t _prevState;

	BC66PHY_Job();
	BC66HANDLER_URC(&sBC66Link);//or put b4 and after to increase redundancy
	BC66HANDLER_Reset(&sBC66Link);

	if(BUSY_Bc66PhyJobstatus== BC66PHY_GetJobStatus())
	{
		return;
	}

	_prevState= (sBC66Link.state<< 8)| sBC66Link.subState;

	switch(sBC66Link.state)
	{
		case INITIAL_Bc66LinkState:
//			if(FW_UPDATE_ModemMode!= config.nbiot.modemMode)
//			{
//				BC66PHY_PauseJob_s(BC66LINK_CFG_PRODUCTION_DELAY_S);/*for production current measurement*/
//			}
//			sBC66Link.state= REALLY_RESET_Bc66LinkState;
//			break;

		case REALLY_RESET_Bc66LinkState:
			if(BACKOFF_ModemMode== config.nbiot.modemMode)
			{
				sBC66Link.state= DISABLE_Bc66LinkState;
				sBC66Link.subState= SET_PIN_OD_DisableState;
				break;
			}
			BC66LINK_StatusInit();
			BC66PHY_PrepareRx();//receive interrupt
			BC66PHY_PowerUp();
			//BC66PHY_Sleep();/*temporary for test board*/
			BC66PHY_Reset();
			sBC66Link.status.bResetExpected= true;/*we use this to rule out unintended bc66 reset*/
			if(FW_UPDATE_ModemMode== config.nbiot.modemMode)
			{
				/*supposedly we need to immediately release rxtx pins after reset for quectel fwu.*/
				BC66PHY_SetRxTxPinOD(true, true);
				sBC66Link.state= CHECK_MODE_Bc66LinkState; /*don't init comm for fwupdate mode*/
			}
			else
			{
				BC66PHY_PauseJob_s(1);/*emulate delay after reset*/
				sBC66Link.state= INIT_LINK_Bc66LinkState;
			}
			break;

		case INIT_LINK_Bc66LinkState:
			BC66LINK_StateTransition(INIT_LINK_Bc66LinkState, CHECK_MODE_Bc66LinkState, BC66LINK_InitBaudrate(), END_InitLinkState);
			break;

		case CHECK_MODE_Bc66LinkState:
			{
				switch(config.nbiot.modemMode)
				{
					case DISABLE_ModemMode:/*will disable after getting all sim info*/
					case APPLICATION_ModemMode:
						{
							sBC66Link.state= PRODUCTION_BREAKPOINT_Bc66LinkState;
							//sBC66Link.state= CONFIGURE_Bc66LinkState;
						}
						break;
					case FW_UPDATE_ModemMode:
						{
							config.nbiot.modemMode= APPLICATION_ModemMode;/*we dont want to stay in FWU mode on next reset*/
							sBC66Link.state= STALL_Bc66LinkState; /*don't init comm for fwupdate mode*/
						}
						break;
					case RF_SIGNALLING_ModemMode:
						{
							config.nbiot.modemMode= APPLICATION_ModemMode;/*we dont want to stay in RF signalling*/
							sBC66Link.state= RF_SIGNALLING_Bc66LinkState;
						}
						break;
					case EMULATION_ModemMode:
						{
							BC66PHY_TxRxJob("AT+QPOWD=1\r\n", 1, rx_OK, 1, 0);/*force power down*/
							BC66PHY_Sleep();
							sBC66Link.state= STALL_Bc66LinkState;
						}
						break;
					default:
						break;
				}
			}
			break;

		case STALL_Bc66LinkState:
			break;

		case PRODUCTION_BREAKPOINT_Bc66LinkState:/*used in board electrical test*/
			BC66LINK_StateTransition(PRODUCTION_BREAKPOINT_Bc66LinkState, CONFIGURE_Bc66LinkState, BC66LINK_ProductionBreakpoint(), END_ProdBreakState);
			break;

		case RF_SIGNALLING_Bc66LinkState:/*used in functional test*/
			BC66LINK_StateTransition(RF_SIGNALLING_Bc66LinkState, INITIAL_Bc66LinkState, BC66LINK_RFSignalling(), END_RFSignallingState);
			break;

		case CONFIGURE_Bc66LinkState:
			BC66LINK_StateTransition(CONFIGURE_Bc66LinkState, ATTACHMENT_Bc66LinkState, BC66LINK_Configure(), END_ConfigureState);
			break;

		case ATTACHMENT_Bc66LinkState:
			BC66LINK_StateTransition(ATTACHMENT_Bc66LinkState, WAIT_REQUEST_Bc66LinkState, BC66LINK_Attachment(), END_AttachmentState);
			break;

		case WAIT_REQUEST_Bc66LinkState:
			if(true== sBC66Link.status.cmeErrorReceived)
			{
				if(32== sBC66Link.status.cmeErrorType)
				{
					/*lwm2m Keep connecting error*/
					sBC66Link.state= DISABLE_Bc66LinkState;
					break;
				}
			}
			switch(sBC66Link.status.cereg.regStatus)/*we check network*/
			{
				case REGISTERED_AS_HOME_BC66LinkNetwork:
				case REGISTERED_AS_ROAMING_BC66LinkNetwork:
					/*network is still ok, no need to do anything, just wait for request*/
				    if((sBC66Link.status.ip[0]!= '0')&& (0!= BC66LINK_QueueDepth()))/*transaction queued by other task*/
				    {
						if(((true== sBC66Link.status.inPSM)|| (false== sBC66Link.status.isConnected))
								&& (false== sBC66Link.status.PSMExitTriggered))
						{
							sBC66Link.status.PSMExitTriggered= true; /*sBC66Link.status.isConnected may stay false even after waking up*/
					    	sBC66Link.state= EXIT_SLEEP_Bc66LinkState;
						}
						else
						{
					    	sBC66Link.state= PROCESS_REQUEST_Bc66LinkState;
						}
				    }
					break;
				case REGISTERING_BC66LinkNetwork:/*network searching for network*//*for bc66, when waking up from PSM, cereg will be 2*/
				case REGISTRATION_UNKNWON_BC66LinkNetwork:/*when out of coverage, this will happen*/
				case UNREGISTERED_BC66LinkNetwork:
					/*this will happen when:
					 * 1) sim card is lifted after connect
					 * 2) telco release context while in PSM (like vietnam telco, SAWACO case
					 * */
				case REGISTRATION_DENIED_BC66LinkNetwork:/*MAXIS: when waking up from PSM, CEREG:3 then CEREG: 2 then stucked.*/
// blocked cos its saver to restart than call recovery in middle of queue
//					else if(sBC66Link.status.cereg1WaitCounter== 20)
//					{
//						/*try to manual attach*/
//						BC66PHY_TxRxJob("AT+CFUN=0;+CFUN=1\r\n", 2, rx_OK, 2, 0);
//						BC66LINK_InvokeRecovery();
//						sBC66Link.status.reattachWaitCounter++;
//						break;
//
//					}
					if(sBC66Link.status.reattachWaitCounter<= config.nbiot.reattachWait_s)
					{
						if((true== sBC66Link.status.inPSM)|| (sBC66Link.status.reattachWaitCounter== config.nbiot.reattachWait_s))
						{
							//already slept/waited, we reset modem
							BC66LINK_ReAttach();/*try reattach first, if there's error then modem will reset*///BC66LINK_ResetJob();
							break;
						}
						else
						{
							BC66PHY_PauseJob_s(1);/*to avoid sleep too long*/
							sBC66Link.status.reattachWaitCounter++;
							break;
						}
					}
				default:
					/*when network remove our profile this case could happen, so we simply reset sara*/
					BC66LINK_CancelTransaction();/*need to clear queue otherwise we will trigger swdg*/
					sBC66Link.state= DISABLE_Bc66LinkState;
					//bDoBackoff= false;/*but dont back off, restart immediately*/
					break;
			}
			break;

		case PROCESS_REQUEST_Bc66LinkState:
			BC66LINK_StateTransition(PROCESS_REQUEST_Bc66LinkState, ENTER_SLEEP_Bc66LinkState, BC66LINK_ProcessRequest(), END_ProcessRequestState);
			break;

		case EXIT_SLEEP_Bc66LinkState:
			BC66LINK_StateTransition(EXIT_SLEEP_Bc66LinkState, WAIT_REQUEST_Bc66LinkState, BC66LINK_ExitSleep(), END_ExitSleepState);
			break;

		case ENTER_SLEEP_Bc66LinkState:
			BC66LINK_StateTransition(ENTER_SLEEP_Bc66LinkState, WAIT_REQUEST_Bc66LinkState, BC66LINK_EnterSleep(), END_EnterSleepState);
			break;

		case DISABLE_Bc66LinkState:
			BC66LINK_StateTransition(DISABLE_Bc66LinkState, INITIAL_Bc66LinkState, BC66LINK_Disable(), END_DisableState);
			break;

		/*BELOW ARE EXPERIMENTAL*/
//
//			/*replaced by AT_QBAND after cfun=1. untested. instructed by LW*/
//		case SET_BAND_LIST_Bc66LinkState:
//			BC66PHY_TxRxJob("AT+QBANDSL=1,3,8,5,20\r\n", 1, rx_OK, 5, 2);/*Set B8,B5 and B20 as the preferred bands to be searched.*/
//			break;
//
//		case ENABLE_EXT_ERROR_REPORTING_Bc66LinkState:/*failed on new bc66 firmware*/
//			BC66PHY_TxRxJob("AT+CEER\r\n", 1, rx_OK, 5, 2);/*this will give us extended error report*//*can only be called when cfun=1*/
//			break;
//
//		case RAI_UDP_OPEN_Bc66LinkState:
//			BC66PHY_TxRxJob("AT+QIOPEN=1,0,\"UDP\",\"223.25.247.73\",2100\r\n", 1, rx_OK, 1, 1);
//			break;
//
//		case SET_TIME_ZONE_Bc66LinkState:
//			BC66PHY_TxRxJob("AT+CTZU=3\r\n", 1, rx_OK, 1, 1);/*update Local time to RTC*/
//			break;
//
//		case FACTORY_RESET_Bc66LinkState:
//			//BC66PHY_TxRxJob("AT+QCFG=\"nvrestore\",0\r\n", 1, rx_OK, 5, 2);
//			BC66PHY_TxRxJob("AT+QPRTPARA=3\r\n", 1, rx_OK, 5, 2);
//			break;

		default:
			SYS_FailureHandler();
			break;
	}

	if(((sBC66Link.state<< 8)| sBC66Link.subState)!= _prevState)
	{/*to prevent task from sleep when we are moving to next state/substate without AT command.*/
		BC66PHY_JobTransition();
	}
}

BC66LINK_InitLinkState_t BC66LINK_InitBaudrate(void)
{
	static uint32_t _baudrateToSet= BC66LINK_CFG_DFT_BAUDRATE;

	switch(sBC66Link.subState)
	{
		case END_InitLinkState:
		case CHECK_ECHO_InitLinkState:
			BC66PHY_InhibitErrorReset(true);/*error reset will interrupt baudrate adjustment*/
			BC66PHY_Job();/*just place this here to empty queue from POR rubbish*/
			sBC66Link.status.bResetExpected= false;
			BC66PHY_TxRxJob("AT\r\n", 2, rx_EchoOK, 1, 0);
			sBC66Link.subState= FIRST_AT_InitLinkState;
			break;
		case FIRST_AT_InitLinkState:
			if(SUCCESS_Bc66PhyJobstatus== BC66PHY_GetJobStatus())
			{
				sBC66Link.echoEnabled= true;
			}
			else
			{
				sBC66Link.echoEnabled= false;
				BC66PHY_TxRxJob("AT\r\n", 1, rx_OK, 1, 0);
			}
			sBC66Link.subState= SET_BC66_BAUD_InitLinkState;
			break;
		case SET_BC66_BAUD_InitLinkState:
			if((_baudrateToSet== BC66PHY_GetBaudrate())&& (SUCCESS_Bc66PhyJobstatus== BC66PHY_GetJobStatus()))
			{
				DBG_Print("#stat:nb__: intended and bc66 baud match >\r\n");
				/*we are at intended baudrate and currently at the as baudrate with bc66*/
				/*we can move on*/
				sBC66Link.subState= DISABLE_ECHO_InitLinkState;
			}
			else if(SUCCESS_Bc66PhyJobstatus== BC66PHY_GetJobStatus())
			{
				DBG_Print("#stat:nb__: mcu and bc66 baud match >\r\n");
				/*we are not at intended baudrate, but currently at the as baudrate with bc66*/
				/*we have to set bc66 and mcu baudrate*/
				if(9600== _baudrateToSet)
				{
					if(true== sBC66Link.echoEnabled)
					{
						BC66PHY_TxRxJob("AT+IPR=9600\r\n", 2, rx_EchoOK, 2, 1);
					}
					else
					{
						BC66PHY_TxRxJob("AT+IPR=9600\r\n", 1, rx_OK, 2, 1);
					}
				}
				else if(115200== _baudrateToSet)
				{
					if(true== sBC66Link.echoEnabled)
					{
						BC66PHY_TxRxJob("AT+IPR=115200\r\n", 2, rx_EchoOK, 2, 1);
					}
					else
					{
						BC66PHY_TxRxJob("AT+IPR=115200\r\n", 1, rx_OK, 2, 1);
					}
				}
				sBC66Link.subState= SET_MCU_BAUD_InitLinkState;
			}
			else
			{
				DBG_Print("#stat:nb__: mcu and bc66 baud NOT match >\r\n");
				/*we are not at intended baudrate and not at the same baudrate as bc66*/
				if(9600== BC66PHY_GetBaudrate())
				{
					BC66PHY_InitBaudrate(115200);
				}
				else if(115200== BC66PHY_GetBaudrate())
				{
					BC66PHY_InitBaudrate(9600);
				}
				SYS_Delay(300);/*need some delay for baud change i guess*/
				BC66PHY_InhibitErrorReset(false);/*re-hibit*/
				sBC66Link.subState= END_InitLinkState;
				sBC66Link.state= INITIAL_Bc66LinkState;
			}
			break;
		case SET_MCU_BAUD_InitLinkState:
			BC66PHY_InitBaudrate(_baudrateToSet);
			SYS_Delay(300);/*need some delay for baud change i guess*/
			sBC66Link.subState= DISABLE_ECHO_InitLinkState;
			break;
		case DISABLE_ECHO_InitLinkState:
			BC66PHY_TxRxJob("ATE0\r\n", 2, rx_EchoATE0, 5, 2);/*disable echo*/
			sBC66Link.subState= CMEERROR_InitLinkState;
			break;
		case CMEERROR_InitLinkState:
			BC66PHY_TxRxJob("AT+CMEE=1\r\n", 1, rx_OK, 5, 2);/*enable +CMEE URC and use numeric values(1= numeric, 2= verbose)*/
			sBC66Link.subState= VERSION_InitLinkState;
			break;
		case VERSION_InitLinkState:
			BC66PHY_TxRxJob2("AT+QGMR\r\n", 2, rx_QGMR, 5, 2, (char *)sBC66Link.BC66Version);/*Display Product Identification Information*/
			sBC66Link.subState= CHECK_VERSION_InitLinkState;
			break;
		case CHECK_VERSION_InitLinkState:
			{
				uint8_t _offset= sBC66Link.BC66Version- (uint8_t *)strstr((char *)sBC66Link.BC66Version, "BC66");
				memmove(sBC66Link.BC66Version, sBC66Link.BC66Version+ _offset, strlen(sBC66Link.BC66Version)- _offset);
				sBC66Link.subState= SNO_InitLinkState;
			}
			break;
		case SNO_InitLinkState:
			BC66PHY_InhibitErrorReset(false);/*re-hibit*/
			BC66PHY_TxRxJob("AT+CGSN=1\r\n", 1, rx_OK, 5, 2);/*get product serial number*/
			sBC66Link.subState= END_InitLinkState;
			break;
	}

	return sBC66Link.subState;
}

BC66LINK_ProdBreakState_t BC66LINK_ProductionBreakpoint(void)
{
	switch(sBC66Link.subState)
	{
		case END_ProdBreakState:
		case SET_MIN_FUNCT_ProdBreakState:
			if(true== config.nbiot.fastModemReboot)
			{
				BC66PHY_JobTransition();/*compulsory, otherwise mcu sleep longer*/
				sBC66Link.subState= END_ProdBreakState;
				break;
			}
//			BC66PHY_TxRxJob("AT+CFUN=0\r\n", 1, rx_OK, 1, 1);
//			sBC66Link.subState= PWR_DOWN_ProdBreakState;
//			break;
		case PWR_DOWN_ProdBreakState:
			BC66PHY_TxRxJob("AT+QPOWD=1\r\n", 1, rx_OK, 1, 0);/*force power down*/
			sBC66Link.subState= PAUSE_1_ProdBreakState;
			break;
		case PAUSE_1_ProdBreakState:
			BC66PHY_PowerDown();
			BC66PHY_PauseJob_s(BC66LINK_CFG_PRODUCTION_DELAY_S);/*emulate delay*/
			sBC66Link.subState= PWR_UP_ProdBreakState;
			break;
		case PWR_UP_ProdBreakState:
			DIAG_Code_f(AVECURRENT_UA_SensorDCode, SENSOR_GetValue(AVECURRENT_Sensor));/*we want to get sleep current*/
			sBC66Link.status.bResetExpected= true;/*we use this to rule out unintended bc66 reset*/
			BC66PHY_PowerUp();
			sBC66Link.subState= PAUSE_2_ProdBreakState;
			break;
		case PAUSE_2_ProdBreakState:
			BC66PHY_PauseJob_s(3);/*emulate delay after reset*/
			sBC66Link.subState= CLEAR_FLAGS_ProdBreakState;
			break;
		case CLEAR_FLAGS_ProdBreakState:
			sBC66Link.status.bResetExpected= false;/*we use this to rule out unintended bc66 reset*/
			sBC66Link.subState= DISABLE_ECHO_ProdBreakState;
			break;
		case DISABLE_ECHO_ProdBreakState:
			BC66PHY_TxRxJob("ATE0\r\n", 2, rx_EchoATE0, 5, 2);/*disable echo*/
			sBC66Link.subState= END_ProdBreakState;
			break;
	}

	return sBC66Link.subState;
}

BC66LINK_RFSignallingState_t BC66LINK_RFSignalling(void)
{
	switch(sBC66Link.subState)
	{
		case END_RFSignallingState:
		case INIT_RFSignallingState:
			BC66PHY_TxRxJob("AT+CFUN=0\r\n", 1, rx_OK, 5, 2);/*set minimum functionality*/
			sBC66Link.subState= ENTER_RFSignallingState;
			break;
		case ENTER_RFSignallingState:
			BC66PHY_TxRxJob("AT*MCALDEV=1\r\n", 1, rx_OK, 5, 2);/*enter caldev mode*/
			sBC66Link.subState= START_RFSignallingState;
			break;
		case START_RFSignallingState:
			{
				uint8_t _tempBuf1[9];
				uint8_t _tempBuf2[3];
				UTILI_BytesToHexString((uint8_t*)&(sBC66Link.rfSignalling.channel), 4, _tempBuf1);
				UTILI_BytesToHexString((uint8_t*)&(sBC66Link.rfSignalling.dBm), 1, _tempBuf2);
				sprintf((char *)pucTxBuffer, "AT*MCAL=\"NRF\",0,21,8,\"L%s000000%s\"\r\n", _tempBuf1, _tempBuf2);/*open single tone power*/
				BC66PHY_TxRxJob(pucTxBuffer, 1, rx_OK, 5, 2);
				sBC66Link.rfSignalling.currChannel= sBC66Link.rfSignalling.channel;
				sBC66Link.rfSignalling.currDBm= sBC66Link.rfSignalling.dBm;
				BC66LINK_SetTimeout(60* 1000);/*we don't want to RF Signalling to exceed 1 minutes*/
				sBC66Link.subState= CHECK_RFSignallingState;
			}
			break;
		case CHECK_RFSignallingState:
			if((false== sBC66Link.rfSignalling.enable)|| (true== BC66LINK_IsTimeout()))
			{
				BC66PHY_TxRxJob("AT*MCAL=\"NRF\",0,1,1,\"L00\"\r\n", 1, rx_OK, 5, 2);/*stop single tone power*/
				sBC66Link.subState= EXIT_RFSignallingState;
			}
			else if((sBC66Link.rfSignalling.currChannel!= sBC66Link.rfSignalling.channel)|| (sBC66Link.rfSignalling.currDBm!= sBC66Link.rfSignalling.dBm))
			{
				BC66PHY_TxRxJob("AT*MCAL=\"NRF\",0,1,1,\"L00\"\r\n", 1, rx_OK, 5, 2);/*stop single tone power*/
				sBC66Link.subState= START_RFSignallingState;
			}
			break;
		case EXIT_RFSignallingState:
			BC66PHY_TxRxJob("AT*MCALDEV=0\r\n", 1, rx_OK, 5, 2);/*exit caldev mode*/
			sBC66Link.subState= END_RFSignallingState;
			break;
	}

	return sBC66Link.subState;
}

BC66LINK_ConfigureState_t BC66LINK_Configure(void)
{
	switch(sBC66Link.subState)
	{
		case END_ConfigureState:
		case MIN_FUNCT_ConfigureState:
			if(true== config.nbiot.fastModemReboot)
			{
				BC66PHY_JobTransition();/*compulsory, otherwise mcu sleep longer*/
				sBC66Link.subState= LED_PATTERN_ConfigureState;
				break;
			}
			BC66PHY_TxRxJob("AT+CFUN=0,1\r\n", 1, rx_OK, 5, 2);/*set minimum functionality after subsequent reset*/
			sBC66Link.subState= APN_ConfigureState;
			break;
		case APN_ConfigureState:
			sprintf((char *)pucTxBuffer, "AT+QCGDEFCONT=\"IP\",\"%s\"\r\n", config.nbiot.apn);/*set APN*/
			BC66PHY_TxRxJob(pucTxBuffer, 1, rx_OK, 5, 2);
			sBC66Link.subState= 100;
			break;
		case 100:
			BC66PHY_TxRxJob("AT+QCFG=\"initlocktime\",10\r\n", 1, rx_OK, 1, 1);/*default is 10, tested with 5*//*update: set back to 10 since we have rrc drop*//*update set to 1*/
			sBC66Link.subState= 101;
			break;
		case 101:
			BC66PHY_TxRxJob("AT+QCFG=\"atlocktime\",10\r\n", 1, rx_OK, 1, 1);/*default is 10, tested with 5*//*update: set back to 10 since we have rrc drop*//*update set to 1*/
			sBC66Link.subState= 102;
			break;
		case 102:
			BC66PHY_TxRxJob("AT+QCFG=\"dsevent\",1\r\n", 1, rx_OK, 1, 1);
			sBC66Link.subState= 103;
			break;
		case 103:
			/*rai enable when we bc66 send ping. should we off this? since ping may not receive downlink*//*off it to be safe*/
			//BC66PHY_TxRxJob("AT+QLWCFG=\"rai_enable\",0\r\n", 1, rx_OK, 1, 1);
			sprintf((char *)pucTxBuffer, "AT"
					"+QLWCFG=\"rai_enable\",0;"
					"+QLWCFG=\"CR\",3,0,0,\"George Kent(M) Berhad\";"
					"+QLWCFG=\"CR\",3,0,1,\"%s\";"
					//"+QLWCFG=\"CR\",3,0,2,\"%s\";"
					"+QLWCFG=\"CR\",3,0,17,\"%s:%s\";"
					"+QLWCFG=\"CR\",3,0,18,\"%s\";"
					"+QLWCFG=\"CR\",3,0,19,\"%s\""
					"\r\n",
					"NRP-B0LV",//config.system.meterSerialNo,
					//config.system.serialNo,
					config.system.meterSerialNo, config.system.serialNo,
					config.system.hwVersion,
					config.system.fwVersion
					);
			BC66PHY_TxRxJob(pucTxBuffer, 1, rx_OK, 5, 2);
			sBC66Link.subState= RESET_ConfigureState;
			break;
		case RESET_ConfigureState:
			sBC66Link.status.bResetExpected= true;/*we use this to rule out unintended bc66 reset*/
			BC66PHY_TxRxJob("AT+QRST=1\r\n", 1, rx_reset, 5, 2);/*reset*/
			BC66PHY_InhibitDiag();/*returned few unrecognized rubbishes*/
			sBC66Link.subState= WAIT_RESET_ConfigureState;
			break;
		case WAIT_RESET_ConfigureState:
			BC66PHY_PauseJob_s(3);/*emulate delay after reset*/
			sBC66Link.subState= DISABLE_ECHO2_ConfigureState;
			break;
		case DISABLE_ECHO2_ConfigureState:
			BC66PHY_Job();/*just place this here to empty queue from POR rubbish*/
			sBC66Link.status.bResetExpected= false;
			BC66PHY_TxRxJob("ATE0\r\n", 2, rx_EchoATE0, 5, 2);/*disable echo*/
			sBC66Link.subState= CMEERROR_ConfigureState;
			break;
		case CMEERROR_ConfigureState:
			BC66PHY_TxRxJob("AT+CMEE=1\r\n", 1, rx_OK, 5, 2);/*enable +CMEE URC and use numeric values(1= numeric, 2= verbose)*/
			sBC66Link.subState= LED_PATTERN_ConfigureState;
			break;
		case LED_PATTERN_ConfigureState:
			BC66PHY_TxRxJob("AT+QLEDMODE=1\r\n", 1, rx_OK, 5, 2);/*Configure the Network LED Patterns*/
			sBC66Link.subState= PSM_URC_ConfigureState;
			break;
		case PSM_URC_ConfigureState:
			BC66PHY_TxRxJob("AT+QNBIOTEVENT=1,1\r\n", 1, rx_OK, 5, 2);/*Enable the indication of PSM event*/
			sBC66Link.subState= PDP_URC_ConfigureState;
			break;
		case PDP_URC_ConfigureState:
			BC66PHY_TxRxJob("AT+CGPADDR=1\r\n", 1, rx_OK, 5, 2);/*show pdp address*/
			sBC66Link.subState= CEREG_URC_ConfigureState;
			break;
		case CEREG_URC_ConfigureState:
			BC66PHY_TxRxJob("AT+CEREG=5\r\n", 1, rx_OK, 5, 2);/*enable cereg urc*/
			sBC66Link.subState= CSCON_URC_ConfigureState;
			break;
		case CSCON_URC_ConfigureState:
			BC66PHY_TxRxJob("AT+CSCON=1\r\n", 1, rx_OK, 5, 2);/*enable connection status signalling*/
			sBC66Link.subState= TAU_ACTIVETIME_ConfigureState;
			break;
		case TAU_ACTIVETIME_ConfigureState:
			if((0== config.nbiot.periodicTau) && (0== config.nbiot.activeTime))
			{
				BC66PHY_TxRxJob("AT+CPSMS=2\r\n", 1, rx_OK, 5, 2);
			}
			else
			{
				sprintf((char *)pucTxBuffer, "AT+CPSMS=1,,,\""BYTE_TO_BINARY_PATTERN"\",\""BYTE_TO_BINARY_PATTERN"\"\r\n",
						BYTE_TO_BINARY(BC66UTIL_SecondsToGPRSTimer3(config.nbiot.periodicTau)),
						BYTE_TO_BINARY(BC66UTIL_SecondsToGPRSTimer2(config.nbiot.activeTime)));

				BC66PHY_TxRxJob(pucTxBuffer, 1, rx_OK, 5, 2);
			}
			sBC66Link.subState= PAGING_EDRX_ConfigureState;
			break;
		case PAGING_EDRX_ConfigureState:
			if((0== config.nbiot.edrx) && (0== config.nbiot.pagingTime))
			{
				BC66PHY_TxRxJob("AT+QEDRXCFG=3\r\n", 1, rx_OK, 5, 2);
			}
			else
			{
				sprintf((char *)pucTxBuffer, "AT+QEDRXCFG=2,5,\""NIBBLE_TO_BINARY_PATTERN"\",\""NIBBLE_TO_BINARY_PATTERN"\"\r\n",
						NIBBLE_TO_BINARY(BC66UTIL_MilisecondsToEDrx(config.nbiot.edrx)),
						NIBBLE_TO_BINARY(BC66UTIL_MilisecondsToPagingTime(config.nbiot.pagingTime)));

				BC66PHY_TxRxJob(pucTxBuffer, 1, rx_OK, 5, 2);
			}
			sBC66Link.subState= SET_SLEEP_MODE_ConfigureState;
			break;
		case SET_SLEEP_MODE_ConfigureState:
			/*
			 * 	0 Disable sleep mode
				1 Enable light sleep and deep sleep, wakeup by PSM_EINT (falling edge)
				2 Enable light sleep only, wakeup by the Main UART
			 */
			BC66PHY_TxRxJob("AT+QSCLK=1\r\n", 1, rx_OK, 5, 2);
			sBC66Link.subState= AT_ConfigureState;
			break;
		case AT_ConfigureState:/*to make uart responsive again*/
			BC66PHY_TxRxJob("AT\r\n", 1, rx_OK, 1, 0);
			BC66PHY_InhibitDiag();/*might not respond*/
			sBC66Link.subState= END_ConfigureState;
			break;
	}

	return sBC66Link.subState;
}

BC66LINK_AttachmentState_t BC66LINK_Attachment(void)
{
	switch(sBC66Link.subState)
	{
		case END_AttachmentState:
		case JIC_WAKEUP_AttachmentState:/*to wakeup modem jic we call this during deepsleep*/
			BC66PHY_ExitPSM();/*wakeup using PSM_EINT - sometimes need even for light sleep, jic*/
			BC66PHY_WakeupUsingAT(1);/*to make uart responsive again*/
			sBC66Link.subState= FULL_FUNCT_AttachmentState;
			break;
		case FULL_FUNCT_AttachmentState:
			BC66PHY_TxRxJob("AT+CFUN=0;+CFUN=1\r\n", 1, rx_OK, 5, 2);/*this only reply 1 "OK"*/
			sBC66Link.status.ip[0]= '0';
			sBC66Link.subState= WAIT_FULL_FUNCT_AttachmentState;
			break;
		case WAIT_FULL_FUNCT_AttachmentState:
			if((true== sBC66Link.status.cmeErrorReceived)&& (4== sBC66Link.status.cmeErrorType))/*cfun=1 cannot execute could be cos of sim card not available*/
			{
				/*disable nbiot*/
				sBC66Link.state= DISABLE_Bc66LinkState;
				sBC66Link.subState= END_AttachmentState;
			}
			else
			{
				BC66PHY_PauseJob_s(3);/*emulate delay*///for CFUN to power up
				sBC66Link.subState= SET_BAND_AttachmentState;
			}
			break;
		case SET_BAND_AttachmentState:
			//BC66PHY_TxRxJob("AT+QBAND=1,3\r\n", 1, rx_OK, 5, 2);
			//BC66PHY_TxRxJob("AT+QBAND=5,5,8,18,19,20\r\n", 1, rx_OK, 5, 2);
			//BC66PHY_TxRxJob("AT+QBAND=6,3,5,8,18,19,20\r\n", 1, rx_OK, 5, 2);
			//BC66PHY_TxRxJob("AT+QBAND=6,8,3,5,18,19,20\r\n", 1, rx_OK, 5, 2);
			//BC66PHY_TxRxJob("AT+QBAND=1,8\r\n", 1, rx_OK, 5, 2);
			//BC66PHY_TxRxJob("AT+QBAND=3,8,5,20\r\n", 1, rx_OK, 5, 2);
			BC66PHY_TxRxJob("AT+QBAND=4,8,3,5,20\r\n", 1, rx_OK, 5, 2);
			sBC66Link.subState= CHECK_SIM_AttachmentState;
			break;
		case CHECK_SIM_AttachmentState:
			BC66PHY_TxRxJob("AT+CPIN?\r\n", 1, rx_OK, 5, 2);
			sBC66Link.subState= CCID_AttachmentState;
			break;
		case CCID_AttachmentState:
			if((true== sBC66Link.status.cmeErrorReceived)&&
					((10== sBC66Link.status.cmeErrorType)/*USIM not inserted*/
					||(13== sBC66Link.status.cmeErrorType)/*USIM failure*/
					||(14== sBC66Link.status.cmeErrorType)/*USIM busy*/
					||(14== sBC66Link.status.cmeErrorType))/*USIM memory full*/
			)
			{
				if(true== BC66LINK_SIMIsPresent())
				{
					config.nbiot.iccid[0]= '\0';/*required by Duy(PTP) to clear to indicate SIM damaged*/
				}
				/*disable nbiot*/
				sBC66Link.state= DISABLE_Bc66LinkState;
				sBC66Link.subState= END_AttachmentState;
			}
			else
			{
				BC66PHY_TxRxJob("AT+QCCID\r\n", 1, rx_OK, 10, 2);
				sBC66Link.subState= IMSI_AttachmentState;
			}
			break;
		case IMSI_AttachmentState:
			BC66PHY_TxRxJob2("AT+CIMI\r\n", 2, rx_CIMI, 5, 2, config.nbiot.imsi);
			if(DISABLE_ModemMode== config.nbiot.modemMode)
			{
				sBC66Link.state= DISABLE_Bc66LinkState;
				sBC66Link.subState= END_AttachmentState;
			}
			else
			{
				sBC66Link.subState= CSQ_AttachmentState;
			}
			break;
		case CSQ_AttachmentState:/*check signal strength*/
			sBC66Link.status.rssi = 99;
			BC66PHY_TxRxJob("AT+CSQ\r\n", 1, rx_OK, 1, 0);
			sBC66Link.status.csqAttemptCount++;
			sBC66Link.subState= WAIT_AttachmentState;
			break;
		case WAIT_AttachmentState:
			BC66PHY_PauseJob_s(config.nbiot.delayBetweenCSQ_s);/*emulate delay*///TODO: determine delay
			sBC66Link.subState= CHECK_AttachmentState;
			break;
		case CHECK_AttachmentState:
			if(false== BC66LINK_NetworkIsRegistered())
			{
				if((0!= config.nbiot.maximumCSQRetry)/*if 0, we never turn off sara module*/
					&&(
							(config.nbiot.maximumCSQRetry< sBC66Link.status.csqAttemptCount)/*we going to stop trying*/
							||(REGISTRATION_UNKNWON_BC66LinkNetwork== sBC66Link.status.cereg.regStatus)/*sara it self has stopped trying (it will auto retry later though)*/
						)
					)
				{
					/*disable nbiot*/
					sBC66Link.state= DISABLE_Bc66LinkState;
					sBC66Link.subState= END_AttachmentState;
				}
//				else if(true== BC66LINK_NetworkIsDenied())
//				{
//					/*disable nbiot*/
//					sBC66Link.state= DISABLE_Bc66LinkState;
//				}
				else/*wait and check signal/network*/
				{
					sBC66Link.subState= CSQ_AttachmentState;
				}
			}
			else/*successfully registered*/
			{
				if(0!= sBC66Link.status.csqAttemptCount)
				{
					sBC66Link.status.csqAttemptCount= 0;
					sBC66Link.status.regDeniedCount= 0;
				}
				sBC66Link.subState= BS_CLK_SYNC_AttachmentState;

				if((true== config.nbiot.enableGkcoapOnAttach)&& (false== config.nbiot.gkcoap.enabled))
				{
					config.nbiot.gkcoap.enabled= true;
					SYS_Request(SOFT_REBOOT_SysRequest);
				}

				if((true== config.nbiot.enableLwm2mOnAttach)&& (false== config.nbiot.lwm2m.enabled))
				{
					config.nbiot.lwm2m.enabled= true;
					SYS_Request(SOFT_REBOOT_SysRequest);
				}

				if(157680000== config.nbiot.restartDelay_s)
				{
					config.nbiot.restartDelay_s= DAY_TO_SECONDS(1);
				}

				/*can skip production breakpoint*/
				config.nbiot.fastModemReboot= true;
			}
			break;
		case BS_CLK_SYNC_AttachmentState:/*sync clock with telco base station*/
			BC66PHY_TxRxJob("AT+QCCLK?\r\n", 1, rx_OK, 1, 1);
			sBC66Link.subState= RAI_INIT_AttachmentState;
			break;
		case RAI_INIT_AttachmentState:
			if((true== config.nbiot.enableRRCDrop) && (false== sBC66Link.status.qiOpened))
			{
				char *_ip= "223.25.247.73\0";
				uint16_t _port= 2100;

				if(true== config.nbiot.lwm2m.enabled)
				{
					_ip= config.nbiot.lwm2m.defaultConnection.serverIP;
					_port= config.nbiot.lwm2m.defaultConnection.serverPort;
				}
				else if(true== config.nbiot.gkcoap.enabled)
				{
					_ip= config.nbiot.gkcoap.serverIP;
					_port= config.nbiot.gkcoap.serverPort;
				}
				sprintf((char *)pucTxBuffer, "AT+QIOPEN=1,0,\"UDP\",\"%s\",%d\r\n", _ip, _port);/*set APN*/
				BC66PHY_TxRxJob(pucTxBuffer, 1, rx_OK, 1, 1);

				sBC66Link.status.qiOpened= true;/*cant send this again without modem reboot*/
			}
			sBC66Link.subState= END_AttachmentState;
			break;
	}

	return sBC66Link.subState;
}

BC66LINK_ProcessRequestState_t BC66LINK_ProcessRequest(void)
{
	static bool _isTransmission= false;
	static uint64_t _timestamp;

	if((0!= BC66LINK_QueueDepth())&& (false== eCurrentTrx->transactionInProgress)&& (eCurrentTrx->TransactionProcess== NULL))
    {
    	eCurrentTrx= BC66LINK_Dequeue();
		BC66LINK_SetTimeout(eCurrentTrx->timeout_ms);
		FAILSAFE_CFGStore();/*routine cfg store before risky transmission, so that our config is not too outdated if BOR occurred (na'uzubillahminzalik!)*/

    	switch(eCurrentTrx->type)
    	{
    		case COAP_Bc66LinkTransactionType:
    			switch(eCurrentTrx->coapType)
    			{
//					case CREATE_Bc66LinkCoap:
//						eCurrentTrx->TransactionProcess= BC66LINK_COAP_Create;
//						break;
//
//					case DELETE_Bc66LinkCoap:
//						eCurrentTrx->TransactionProcess= BC66LINK_COAP_Delete;
//						break;
//
//					case ADD_RESOURCE_Bc66LinkCoap:
//						eCurrentTrx->TransactionProcess= BC66LINK_COAP_AddResource;
//						break;
//
//					case CONFIG_HEAD_Bc66LinkCoap:
//						eCurrentTrx->TransactionProcess= BC66LINK_COAP_ConfigHead;
//						break;
//
//					case CONFIG_OPTION_Bc66LinkCoap:
//						eCurrentTrx->TransactionProcess= BC66LINK_COAP_ConfigOption;
//						break;
//
//					case SEND_Bc66LinkCoap:
//						eCurrentTrx->TransactionProcess= BC66LINK_COAP_Send;
//						_isTransmission= true;
//						break;
//
//					case SEND_STATUS_Bc66LinkCoap:
//						eCurrentTrx->TransactionProcess= BC66LINK_COAP_SendStatus;
//						_isTransmission= true;
//						break;
//
//					case CONFIG_COMMAND_Bc66LinkCoap:
//						eCurrentTrx->TransactionProcess= BC66LINK_COAP_ConfigCommand;
//						break;
//
//					case ALI_SIGN_Bc66LinkCoap:
//						eCurrentTrx->TransactionProcess= BC66LINK_COAP_AliSign;
//						_isTransmission= true;
//						break;

					case GK_POST_Bc66LinkCoap:
						eCurrentTrx->TransactionProcess= BC66LINK_COAP_GKPost;
						_isTransmission= true;
						break;

					case LWM2M_GET_Bc66LinkCoap:
						eCurrentTrx->TransactionProcess= BC66LINK_COAP_LWM2MGet;
						_isTransmission= true;
						break;

					default:
						SYS_FailureHandler();
						break;
    			}
    			break;

    		case LWM2M_Bc66LinkTransactionType:
    			switch(eCurrentTrx->lwm2mType)
    			{
					case CONFIGURE_Bc66LinkLwm2m:
						eCurrentTrx->TransactionProcess= BC66LINK_LWM2M_Configure;
						break;

					case REGISTER_Bc66LinkLwm2m:
						eCurrentTrx->TransactionProcess= BC66LINK_LWM2M_Register;
						_isTransmission= true;
						sBC66Link.status.inhibitSleep= true;
						break;

					case POSTREGISTER_Bc66LinkLwm2m:
						eCurrentTrx->TransactionProcess= BC66LINK_LWM2M_PostRegister;
						_isTransmission= true;
						sBC66Link.status.inhibitSleep= false;
						break;

    				case UPDATE_Bc66LinkLwm2m:
    					eCurrentTrx->TransactionProcess= BC66LINK_LWM2M_Update;
    					break;

    				case DEREGISTER_Bc66LinkLwm2m:
    					eCurrentTrx->TransactionProcess= BC66LINK_LWM2M_Deregister;
    					break;

    				case ADD_OBJECT_Bc66LinkLwm2m:
    					eCurrentTrx->TransactionProcess= BC66LINK_LWM2M_AddObject;
    					break;

    				case DELETE_OBJECT_Bc66LinkLwm2m:
    					eCurrentTrx->TransactionProcess= BC66LINK_LWM2M_DeleteObject;
    					break;

    				case WRITE_RESPONSE_Bc66LinkLwm2m:
    					eCurrentTrx->TransactionProcess= BC66LINK_LWM2M_WriteResponse;
						_isTransmission= true;
    					break;

    				case READ_RESPONSE_Bc66LinkLwm2m:
    					eCurrentTrx->TransactionProcess= BC66LINK_LWM2M_ReadResponse;
						_isTransmission= true;
    					break;

    				case EXECUTE_RESPONSE_Bc66LinkLwm2m:
    					eCurrentTrx->TransactionProcess= BC66LINK_LWM2M_ExecuteResponse;
						_isTransmission= true;
    					break;

    				case OBSERVE_RESPONSE_Bc66LinkLwm2m:
    					eCurrentTrx->TransactionProcess= BC66LINK_LWM2M_ObserveResponse;
						_isTransmission= true;
    					break;

    				case NOTIFY_Bc66LinkLwm2m:
    					eCurrentTrx->TransactionProcess= BC66LINK_LWM2M_Notify;
						_isTransmission= true;
    					break;

    				case READ_DATA_Bc66LinkLwm2m:
    					eCurrentTrx->TransactionProcess= BC66LINK_LWM2M_ReadData;
    					break;

    				case STATUS_Bc66LinkLwm2m:
    					eCurrentTrx->TransactionProcess= BC66LINK_LWM2M_Status;
    					break;

    	    		default:
    	    			SYS_FailureHandler();
    	    			break;
    			}
    			break;

    		default:
    			SYS_FailureHandler();
    			break;
    	}

    	if(true== _isTransmission)
    	{
    		_timestamp= SYS_GetTimestamp_ms();
    		if(0== ((++config.nbiot.stats.noOfTransmission)% 10))
    		{
    			DIAG_Code(TXRX_ATTEMPT_NbiotDCode, config.nbiot.stats.noOfTransmission);
    		}
    	}
    }

    if((true== eCurrentTrx->transactionInProgress)&& (eCurrentTrx->TransactionProcess!= NULL))
    {
    	eCurrentTrx->TransactionProcess((void *)eCurrentTrx);/*run the process*/
    }

    /*transaction timeout*/
	if((true== eCurrentTrx->transactionInProgress)&& (true== BC66LINK_IsTimeout()))
	{
		eCurrentTrx->status= TIMEOUT_TransactionStatus;
		eCurrentTrx->txSent= true;
		eCurrentTrx->rxReceived= true;
		eCurrentTrx->transactionInProgress= false;
		eCurrentTrx->transactionCompleted= true;

    	if(true== _isTransmission)
    	{
    		_isTransmission= false;
			DIAG_Code(TXRX_FAILED_NbiotDCode, ++config.nbiot.stats.noOfFailedTransmission);
    	}
	}

    if(true== eCurrentTrx->transactionCompleted)/*transaction completed*/
	{
		if(NULL!= eCurrentTrx->TransactionCompletedCb)
		{
			eCurrentTrx->TransactionCompletedCb();
		}
		eCurrentTrx= &eNoTrx;

    	if(true== _isTransmission)
    	{
    		_isTransmission= false;

    		int32_t _transmissionCount= (config.nbiot.stats.noOfTransmission- config.nbiot.stats.noOfFailedTransmission);

    		uint32_t _latency_ms= SYS_GetTimestamp_ms()- _timestamp;
    		config.nbiot.rteTotalLatency_ms+= _latency_ms;
    		config.nbiot.stats.latency_ms= _latency_ms;
			config.nbiot.stats.aveLatency_ms= config.nbiot.rteTotalLatency_ms/ _transmissionCount;
			config.nbiot.stats.minLatency_ms= (_latency_ms< config.nbiot.stats.minLatency_ms)? _latency_ms: config.nbiot.stats.minLatency_ms;
			config.nbiot.stats.maxLatency_ms= (_latency_ms> config.nbiot.stats.maxLatency_ms)? _latency_ms: config.nbiot.stats.maxLatency_ms;
    	}

		return END_ProcessRequestState;
	}

    return PROCESSING_ProcessRequestState;
}

BC66LINK_ExitSleepState_t BC66LINK_ExitSleep(void)
{
	switch(sBC66Link.subState)
	{
		case END_ExitSleepState:
		case START_ExitSleepState:
			BC66PHY_ExitPSM();/*wakeup using PSM_EINT - sometimes need even for light sleep, jic*/
			BC66PHY_WakeupUsingAT(1);/*to make uart responsive again*/
			sBC66Link.subState= WAIT_ExitSleepState;
			break;
		case WAIT_ExitSleepState:
			if(true== sBC66Link.status.inPSM)
			{
				DBG_Print("Exiting PSM\r\n");
				/*waking up from deep sleep*/
				sBC66Link.status.inPSM= false;
				BC66LINK_ClearIP();
				BC66PHY_PauseJob_s(2);/*emulate delay*///TODO: determine delay/*this delay is extremely important. when waking up from deepsleep, bc66 uart is unreponsive for at least 1 sec.*/
				SYS_Delay(90);/*this delay is just to display debug message properly, can be removed*/
			}
			else
			{
				DBG_Print("Exiting Light Sleep\r\n");
				/*waking up from light sleep*/
				/*no need to do much*/
			}
			sBC66Link.subState= DISABLE_SLEEP_ExitSleepState;
			break;
		case DISABLE_SLEEP_ExitSleepState:/*so that we can manually control deepsleep*/
			BC66PHY_TxRxJob("AT+QSCLK=0\r\n", 1, rx_OK, 5, 1);
			sBC66Link.subState= WAIT_CSCON_ExitSleepState;
			break;
		case WAIT_CSCON_ExitSleepState:/*wait for CSCON= 1*/
			if(false== sBC66Link.status.isConnected)
			{
				BC66PHY_PauseJob_s(1);
			}
			sBC66Link.subState= END_ExitSleepState;
			break;
	}

	return sBC66Link.subState;
}

BC66LINK_EnterSleepState_t BC66LINK_EnterSleep(void)
{
	static time_t _sleepTime_s= 0;
	switch(sBC66Link.subState)
	{
		case END_EnterSleepState:
		case START_EnterSleepState:
			_sleepTime_s= SYS_GetTimestamp_s()+ config.nbiot.RRCDropPeriod_s;
			BC66PHY_JobTransition();/*compulsory, otherwise mcu sleep longer*/
			sBC66Link.subState= WAIT_EnterSleepState;
			break;
		case WAIT_EnterSleepState:
			if((false== config.nbiot.enableRRCDrop)
					|| (0!= BC66LINK_QueueDepth())/*cancel sleep if got new transmission/ already slept*/
					|| (true== sBC66Link.status.inhibitSleep))
			{
				sBC66Link.subState= END_EnterSleepState;
			}
			else if((true== sBC66Link.status.inPSM)|| (false== sBC66Link.status.isConnected))
			{
				sBC66Link.subState= ENTER_DEEP_SLEEP_EnterSleepState;
			}
			else if( SYS_GetTimestamp_s()>= _sleepTime_s)
			{
				sBC66Link.subState= GET_QENG_EnterSleepState;
			}
			else
			{
				BC66PHY_PauseJob_s(1);/*compulsory, otherwise mcu sleep longer*/
			}
			break;
		case GET_QENG_EnterSleepState:
			BC66PHY_TxRxJob("AT+QENG=0;+QENG=1;+QENG=2;+QENG=3\r\n", 1, rx_OK, 5, 1);
			sBC66Link.subState= GET_CLK_EnterSleepState;
			break;
		case GET_CLK_EnterSleepState:
			BC66PHY_TxRxJob("AT+QCCLK?\r\n", 1, rx_OK, 1, 1);
			sBC66Link.subState= ENTER_PSM_EnterSleepState;
			break;
		case ENTER_PSM_EnterSleepState: /*we enter PSM by sending RAI flag*/
			BC66PHY_TxRxJob("AT+QNBIOTRAI=1;+QISEND=0,1,\"M\" \r\n", 1, rx_OK, 5, 0);/*no retry, dont want to trigger modem reset*/
			sBC66Link.subState= ENTER_DEEP_SLEEP_EnterSleepState;
			break;
		case ENTER_DEEP_SLEEP_EnterSleepState:
			if(false== sBC66Link.status.inDeepSleep)
			{
				BC66PHY_TxRxJob("AT+QSCLK=1;+QRELLOCK\r\n", 1, rx_OK, 5, 1);/*enable back deep sleep, disable AT*/
			}
			else
			{
				BC66PHY_JobTransition();
			}
			sBC66Link.subState= END_EnterSleepState;
			sBC66Link.status.sendLwUpdate= true;/*temp solution: for lwm2m plain connection, ip/port will always change by NAT. so we need to keep updating the connection(which rely on oprt number)
			For plain and DTLS, this is used to get downlink command.*/
			sBC66Link.status.reattachWaitCounter= 0;/*renew counter*/
			break;
	}

	return sBC66Link.subState;
}

BC66LINK_DisableState_t BC66LINK_Disable(void)
{
	switch(sBC66Link.subState)
	{
		case END_DisableState:
		case SET_MIN_FUNCT_DisableState:
			BC66PHY_TxRxJob("AT+CFUN=0\r\n", 1, rx_OK, 1, 2);
			sBC66Link.subState= PWR_DOWN_DisableState;

			DIAG_Code(MODEM_DISABLED_NbiotDCode, SYS_GetTimestamp_s()+ config.nbiot.restartDelay_s);
			break;
		case PWR_DOWN_DisableState:
			BC66PHY_TxRxJob("AT+QPOWD=0\r\n", 1, rx_OK, 1, 2);
			sBC66Link.subState= SET_PIN_OD_DisableState;
			break;
		case SET_PIN_OD_DisableState:
			BC66PHY_PowerDown();
			if(DISABLE_ModemMode!= config.nbiot.modemMode)
			{
				sBC66Link.subState= CHECK_FOR_ENABLE_DisableState;
				BC66LINK_SetTimeout(config.nbiot.restartDelay_s* 1000);
			}
			else
			{
				sBC66Link.state= STALL_Bc66LinkState;
				sBC66Link.subState= END_DisableState;
			}
			break;
		case CHECK_FOR_ENABLE_DisableState:
			if(true== BC66LINK_IsTimeout())
			{
				sBC66Link.subState= END_DisableState;
				if(BACKOFF_ModemMode== config.nbiot.modemMode)
				{
					config.nbiot.modemMode= APPLICATION_ModemMode;/*only back to application after backoff done, regardless there's modem/mcu reset or not
					(unless modem mode set via msg task)*/
				}
			}
			break;
	}

	return sBC66Link.subState;
}

//void BC66LINK_COAP_Create(void *_param)
//{
//	BC66LINK_Transaction_t *_trx= (BC66LINK_Transaction_t *)_param;
//
//	switch(_trx->state)
//	{
//		case 0:
//			/*moved to gkpost. need to constantly check coap create before transmit*/
//			_trx->status= SUCCESS_TransactionStatus;
//			_trx->txSent= true;
//			_trx->rxReceived= true;
//			_trx->transactionInProgress= false;
//			_trx->transactionCompleted= true;
//			_trx->state= 4;
//			break;
//		case 1:
//			if(PSK_Bc66LinkCoapEncryption== _trx->coap.Create.mode)
//			{
//				sprintf((char *)pucTxBuffer, "AT"
//						"+QCOAPCREATE="
//						"%d,"/*localport*/
//						"%d,"/*mode*/
//						"%s,"/*pskId*/
//						"%s"/*psk*/
//						"\r\n",
//						_trx->coap.Create.port,
//						_trx->coap.Create.mode,
//						_trx->coap.Create.pskId,
//						_trx->coap.Create.psk
//						);
//			}
//			else
//			{
//				sprintf((char *)pucTxBuffer, "AT"
//						"+QCOAPCREATE="
//						"%d,"/*localport*/
//						"%d"/*mode*/
//						"\r\n",
//						_trx->coap.Create.port,
//						_trx->coap.Create.mode
//						);
//			}
//
//			BC66PHY_TxRxJob(pucTxBuffer, 1, rx_OK, 5, 2);
//
//			//DBG_Print("#stat:nb__: >>>>>>>>>>>>>>>>>>>>>QCOAPCREATE>%d\r\n", BC66LINK_QueueDepth());
//			_trx->state= 2;
//		break;
//
//		case 2:
//
//		case 3:
//			_trx->status= (SUCCESS_Bc66PhyJobstatus== BC66PHY_GetJobStatus())? SUCCESS_TransactionStatus: ERROR_TransactionStatus;
//			_trx->txSent= true;
//			_trx->rxReceived= true;
//			_trx->transactionInProgress= false;
//			_trx->transactionCompleted= true;
//			_trx->state= 4;
//			break;
//
//		case 4:
//			break;
//
//	}
//}
//
//void BC66LINK_COAP_Delete(void *_param)
//{
//	BC66LINK_Transaction_t *_trx= (BC66LINK_Transaction_t *)_param;
//
//	switch(_trx->state)
//	{
//		case 0:
//		case 1:
//			sprintf((char *)pucTxBuffer, "AT"
//					"+QCOAPDEL"
//					"\r\n"
//					);
//
//			BC66PHY_TxRxJob(pucTxBuffer, 1, rx_OK, 5, 2);
//			_trx->state= 2;
//		break;
//
//		case 2:
//
//		case 3:
//			_trx->status= (SUCCESS_Bc66PhyJobstatus== BC66PHY_GetJobStatus())? SUCCESS_TransactionStatus: ERROR_TransactionStatus;
//			_trx->txSent= true;
//			_trx->rxReceived= true;
//			_trx->transactionInProgress= false;
//			_trx->transactionCompleted= true;
//			_trx->state= 4;
//			break;
//
//		case 4:
//			break;
//
//	}
//}
//
//void BC66LINK_COAP_AddResource(void *_param)
//{
//	BC66LINK_Transaction_t *_trx= (BC66LINK_Transaction_t *)_param;
//
//	switch(_trx->state)
//	{
//		case 0:
//		case 1:
//			sprintf((char *)pucTxBuffer, "AT"
//					"+QCOAPADDRES="
//					"%d,"/*length*/
//					"%s,"/*resource*/
//					"\r\n",
//					_trx->coap.AddResource.length,
//					_trx->coap.AddResource.resourceName
//					);
//
//			BC66PHY_TxRxJob(pucTxBuffer, 1, rx_OK, 5, 2);
//			_trx->state= 2;
//		break;
//
//		case 2:
//
//		case 3:
//			_trx->status= (SUCCESS_Bc66PhyJobstatus== BC66PHY_GetJobStatus())? SUCCESS_TransactionStatus: ERROR_TransactionStatus;
//			_trx->txSent= true;
//			_trx->rxReceived= true;
//			_trx->transactionInProgress= false;
//			_trx->transactionCompleted= true;
//			_trx->state= 4;
//			break;
//
//		case 4:
//			break;
//
//	}
//}
//
//void BC66LINK_COAP_ConfigHead(void *_param)
//{
//	BC66LINK_Transaction_t *_trx= (BC66LINK_Transaction_t *)_param;
//
//	switch(_trx->state)
//	{
//		case 0:
//		case 1:
//			if((2== _trx->coap.ConfigHead.mode)|| (5== _trx->coap.ConfigHead.mode))
//			{
//				sprintf((char *)pucTxBuffer, "AT"
//						"+QCOAPHEAD=="
//						"%d,"/*mode*/
//						"%d,"/*msgid*/
//						"%d,"/*tkl*/
//						"%s"/*token*/
//						"\r\n",
//						_trx->coap.ConfigHead.mode,
//						_trx->coap.ConfigHead.msgid,
//						_trx->coap.ConfigHead.tokenLength,
//						_trx->coap.ConfigHead.token
//						);
//			}
//			else
//			{
//				sprintf((char *)pucTxBuffer, "AT"
//						"+QCOAPHEAD=="
//						"%d,"/*mode*/
//						"%d,"/*msgid*/
//						"%s"/*token*/
//						"\r\n",
//						_trx->coap.ConfigHead.mode,
//						_trx->coap.ConfigHead.msgid,
//						_trx->coap.ConfigHead.token
//						);
//			}
//
//			BC66PHY_TxRxJob(pucTxBuffer, 1, rx_OK, 5, 2);
//			_trx->state= 2;
//		break;
//
//		case 2:
//
//		case 3:
//			_trx->status= (SUCCESS_Bc66PhyJobstatus== BC66PHY_GetJobStatus())? SUCCESS_TransactionStatus: ERROR_TransactionStatus;
//			_trx->txSent= true;
//			_trx->rxReceived= true;
//			_trx->transactionInProgress= false;
//			_trx->transactionCompleted= true;
//			_trx->state= 4;
//			break;
//
//		case 4:
//			break;
//
//	}
//}
//
//void BC66LINK_COAP_ConfigOption(void *_param)
//{
//	BC66LINK_Transaction_t *_trx= (BC66LINK_Transaction_t *)_param;
//
//	switch(_trx->state)
//	{
//		case 0:
//		case 1:
//			sprintf((char *)pucTxBuffer, "AT"
//					"+QCOAPOPTION="
//					"%d",
//					_trx->coap.ConfigOption.optCount
//					);
//			uint32_t _strLen= strlen((char *)pucTxBuffer);
//			for(int i= 0; i< _trx->coap.ConfigOption.optCount; i++)
//			{
//				sprintf((char *)(pucTxBuffer+ _strLen), ",%d,\"%s\"", _trx->coap.ConfigOption.optName[i], _trx->coap.ConfigOption.optValue[i]);
//				_strLen= strlen((char *)pucTxBuffer);
//			}
//			sprintf((char *)(pucTxBuffer+ _strLen), "\r\n");
//
//			BC66PHY_TxRxJob(pucTxBuffer, 1, rx_OK, 5, 2);
//			_trx->state= 2;
//		break;
//
//		case 2:
//
//		case 3:
//			_trx->status= (SUCCESS_Bc66PhyJobstatus== BC66PHY_GetJobStatus())? SUCCESS_TransactionStatus: ERROR_TransactionStatus;
//			_trx->txSent= true;
//			_trx->rxReceived= true;
//			_trx->transactionInProgress= false;
//			_trx->transactionCompleted= true;
//			_trx->state= 4;
//			break;
//
//		case 4:
//			break;
//
//	}
//}
//
//void BC66LINK_COAP_Send(void *_param)
//{
//	BC66LINK_Transaction_t *_trx= (BC66LINK_Transaction_t *)_param;
//
//	switch(_trx->state)
//	{
//		case 0:
//		case 1:
//			sprintf((char *)pucTxBuffer, "AT"
//					"+QCOAPSEND="
//					"%d,"/*type*/
//					"%d,"/*method/rspCode*/
//					"%s,"/*ipAddr*/
//					"%d,"/*port*/
//					"%d,",/*length*/
//					_trx->coap.Send.type,
//					_trx->coap.Send.methodRspcode,
//					_trx->coap.Send.ipAddr,
//					_trx->coap.Send.port,
//					_trx->coap.Send.length
//					);
//			uint32_t _strLen= strlen((char *)pucTxBuffer);
//			if(0== _trx->coap.Send.length)
//			{
//				_strLen--;/*remove the last ','*/
//			}
//			else
//			{
//				for(int i= 0; i< _trx->coap.Send.length; i++)
//				{
//					sprintf((char *)(pucTxBuffer+ _strLen), "%02X", _trx->coap.Send.data[i]);
//					_strLen+= 2;
//				}
//			}
//			sprintf((char *)(pucTxBuffer+ _strLen), "\r\n");
//
//			sBC66Link.urc.QCOAPSEND= 0xFF;
//			sBC66Link.urc.QCOAPURC.rsp.rspCode= 0xFFFF;
//			BC66PHY_TxRxJob(pucTxBuffer, 1, rx_OK, 5, 2);
//			_trx->state= 2;
//		break;
//
//		case 2:
//
//		case 3:
//			if((0== sBC66Link.urc.QCOAPSEND)&& (203== sBC66Link.urc.QCOAPURC.rsp.rspCode))/*lwm2m successfully registered*/
//			{
//				_trx->status= SUCCESS_TransactionStatus;
//				_trx->txSent= true;
//				_trx->rxReceived= true;
//				_trx->transactionInProgress= false;
//				_trx->transactionCompleted= true;
//				_trx->state= 4;
//			}
//			else if((1== sBC66Link.urc.QCOAPSEND)|| ((0xFFFF!= sBC66Link.urc.QCOAPURC.rsp.rspCode)&& (203!= sBC66Link.urc.QCOAPURC.rsp.rspCode)))/*lwm2m successfully registered*/
//			{
//				_trx->status= ERROR_TransactionStatus;
//				_trx->txSent= true;
//				_trx->rxReceived= true;
//				_trx->transactionInProgress= false;
//				_trx->transactionCompleted= true;
//				_trx->state= 4;
//			}
//			break;
//
//		case 4:
//			break;
//
//	}
//}
//
//void BC66LINK_COAP_SendStatus(void *_param)
//{
//	BC66LINK_Transaction_t *_trx= (BC66LINK_Transaction_t *)_param;
//	_trx->status= SUCCESS_TransactionStatus;
//	_trx->txSent= true;
//	_trx->rxReceived= true;
//	_trx->transactionInProgress= false;
//	_trx->transactionCompleted= true;
//}
//
//void BC66LINK_COAP_ConfigCommand(void *_param)
//{
//	BC66LINK_Transaction_t *_trx= (BC66LINK_Transaction_t *)_param;
//	_trx->status= SUCCESS_TransactionStatus;
//	_trx->txSent= true;
//	_trx->rxReceived= true;
//	_trx->transactionInProgress= false;
//	_trx->transactionCompleted= true;
//}
//
//void BC66LINK_COAP_AliSign(void *_param)
//{
//	BC66LINK_Transaction_t *_trx= (BC66LINK_Transaction_t *)_param;
//	_trx->status= SUCCESS_TransactionStatus;
//	_trx->txSent= true;
//	_trx->rxReceived= true;
//	_trx->transactionInProgress= false;
//	_trx->transactionCompleted= true;
//}

void BC66LINK_COAP_GKPost(void *_param)/*modification of Send to include GK packet header, footer and encryption*/
{
	BC66LINK_Transaction_t *_trx= (BC66LINK_Transaction_t *)_param;
	static uint8_t packetCounter= 0;
	static uint32_t encIVCounter= 0;
	static uint8_t IV[16];
	static uint16_t totalLen= 0;

	switch(_trx->state)
	{
		case 0:
			if(0== sBC66Link.coapContext)
			{
				_trx->state= 101;
				break;
			}
			else if(1!= sBC66Link.coapContext)
			{
				BC66PHY_TxRxJob("AT+QCOAPDEL\r\n", 1, rx_OK, 5, 2);/*when gkcoap already use the coap context. only one can be*/
				_trx->state= 101;
				break;
			}
			else
			{
				_trx->state= 2;
				break;
			}

		case 101:
			sprintf((char *)pucTxBuffer, "AT"
					"+QCOAPCREATE="
					"%d,"/*localport*/
					"%d"/*mode*/
					"\r\n",
					_trx->coap.GKPost.localPort,
					NONE_Bc66LinkCoapEncryption
					);

			BC66PHY_TxRxJob(pucTxBuffer, 1, rx_OK, 5, 2);
			_trx->state= 102;
			break;

		case 102:
			if(SUCCESS_Bc66PhyJobstatus== BC66PHY_GetJobStatus())
			{
				sBC66Link.coapContext= 1;
				_trx->state= 1;
			}
			else
			{
				_trx->status= ERROR_TransactionStatus;
				_trx->txSent= false;
				_trx->rxReceived= false;
				_trx->transactionInProgress= false;
				_trx->transactionCompleted= true;
				_trx->state= 7;
			}
			break;

		case 1:
			//sBC66Link.status.rssi = 99;/*we don't have to set this to 99 cos it tend to fail and deadlock everything at BC66LINK_TransmissionIsReady(). we can afford to miss csq value*/
			BC66PHY_TxRxJob("AT+CSQ\r\n", 1, rx_OK, 5, 2);
			_trx->state= 2;
			break;

		case 2:
			sprintf((char *)pucTxBuffer, "AT"
					"+QCOAPOPTION="
					"%d,"
					"%d,"
					"\"%s\""
					"\r\n",
					1,/*ConfigOption.optCount*/
					11,/*ConfigOption.optName= 11 as uri*/
					_trx->coap.GKPost.uri);

			BC66PHY_TxRxJob(pucTxBuffer, 1, rx_OK, 5, 2);
			_trx->state= 3;
			break;

		case 3:
			if(SUCCESS_Bc66PhyJobstatus== BC66PHY_GetJobStatus())
			{
				_trx->state= 40;
			}
			else
			{
				_trx->status= ERROR_TransactionStatus;
				_trx->txSent= false;
				_trx->rxReceived= false;
				_trx->transactionInProgress= false;
				_trx->transactionCompleted= true;
				_trx->state= 7;
			}
			break;

		case 40:
//			BC66PHY_TxRxJob("AT+QNBIOTRAI=2\r\n", 1, rx_OK, 5, 2);
//			_trx->state= 4;
//			break;

		case 4:
		{
			totalLen= 30+ 2+ (4* 2)+ ((_trx->coap.GKPost.length+ 2)* 2)+ 2+ 2+ 2;
			totalLen/= 2;
			sprintf((char *)pucTxBuffer, "AT"
					"+QCOAPSEND="
					"%d,"/*type*/
					"%d,"/*method/rspCode*/
					"%s,"/*ipAddr*/
					"%d,"/*port*/
					"%d,",/*length*/
					0,/*CON*/
					2,/*POST*/
					_trx->coap.GKPost.ipAddr,
					_trx->coap.GKPost.port,
					totalLen//_trx->coap.GKPost.length
					);
			uint32_t _strLen= strlen((char *)pucTxBuffer);
    		int j = _strLen;

    		/*append imei*/
			for(int i= 0; i< 15; i++, j+=2)
			{
				sprintf((char *)(pucTxBuffer+ j), "%02X", _trx->coap.GKPost.imei[i]);
			}
			_strLen+= 30;

			/*append packet type*/
			sprintf((char *)(pucTxBuffer+ j), "%02X", _trx->coap.GKPost.packetType);
			j+=2;
			_strLen+= 2;

			/*pre-calculate header checksum*/
			uint16_t _checksum= UTILI_GetChecksum(0x00, (uint8_t *)_trx->coap.GKPost.imei, 15);/*imei checksum*/
			_checksum= UTILI_GetChecksum(_checksum, &(_trx->coap.GKPost.packetType), 1);/*messageCode cheksum*/

			/*simple ctr encryption*/
			encIVCounter++;
			memcpy(IV, _trx->coap.GKPost.imei+ 2, 12); memcpy(IV+ 12, (uint8_t*)(&encIVCounter), 4);/*update IV*/

			_checksum= UTILI_GetChecksum(_checksum, (uint8_t*)(&encIVCounter), 4);
			for(int i= 0; i< 4; i++, j+=2)
			{
				sprintf((char *)(pucTxBuffer+ j), "%02X", ((uint8_t*)(&encIVCounter))[i]);
			}
			_strLen+= (4* 2);

			uint8_t *_tempEncBuf= (uint8_t *)(pucTxBuffer+ j+ 2/*encCrc size*/+ _trx->coap.GKPost.length);/*temporary using last half of buffer(will be overwritten shortly with hex string)*/
			memcpy(_tempEncBuf, _trx->coap.GKPost.data, _trx->coap.GKPost.length);
			uint16_t _encCRC= UTILI_GetChecksum(0x00, _trx->coap.GKPost.data, _trx->coap.GKPost.length);/*append crc before encrypt to check after decrypt*/
			_tempEncBuf[_trx->coap.GKPost.length]= (uint8_t)(_encCRC>> 8);
			_tempEncBuf[_trx->coap.GKPost.length+ 1]= (uint8_t)(_encCRC);

			SECURE_CTR_Transcrypt(2, IV, _tempEncBuf, _trx->coap.GKPost.length+ 2);
			_checksum= UTILI_GetChecksum(_checksum, _tempEncBuf, _trx->coap.GKPost.length+ 2);/*packet checksum*/
			for(int i= 0; i< _trx->coap.GKPost.length+ 2; i++, j+=2)
			{
				sprintf((char *)(pucTxBuffer+ j), "%02X", _tempEncBuf[i]);
			}
			_strLen+= ((_trx->coap.GKPost.length+ 2)* 2);

			/*append packet counter*/
			packetCounter++;
			_checksum= UTILI_GetChecksum(_checksum, &packetCounter, 1);
			sprintf((char *)(pucTxBuffer+ j), "%02X", packetCounter);j+= 2;
			_strLen+= 2;
			/*end append packet counter*/

			/*append checksum*/
			sprintf((char *)(pucTxBuffer+ j), "%02X", (uint8_t)(_checksum>> 8));j+= 2;
			_strLen+= 2;
			sprintf((char *)(pucTxBuffer+ j), "%02X", (uint8_t)(_checksum));j+= 2;
			_strLen+= 2;

			/*append new line*/
			sprintf((char *)(pucTxBuffer+ _strLen), "\r\n");

			sBC66Link.urc.QCOAPSEND= 0xFF;
			sBC66Link.urc.QCOAPURC.rsp.rspCode= 0xFFFF;
			BC66PHY_TxRxJob(pucTxBuffer, 1, rx_OK, 5, 2);
			_trx->state= 5;
		}
		break;

		case 5:

		case 6:
			if(/*(0== sBC66Link.urc.QCOAPSEND)&& */
					(203== sBC66Link.urc.QCOAPURC.rsp.rspCode)/*valid*/
					|| (205== sBC66Link.urc.QCOAPURC.rsp.rspCode)/*content*/
					)
			{
				if(0!= sBC66Link.urc.QCOAPURC.rsp.len)
				{
					uint8_t *_tempBuf= (uint8_t *)sBC66Link.urc.QCOAPURC.rsp.data;
					uint16_t _tempBufLen= sBC66Link.urc.QCOAPURC.rsp.len;
					uint16_t _checksum;
					uint8_t _packetCounter;

					UTILI_HexStringToBytes(_tempBuf, sBC66Link.urc.QCOAPURC.rsp.data, _tempBufLen);/*useback data buf, cos we share tx and rx buffer. don't want to change buffer if no valid rx received*/

					_checksum= MAKEWORD(_tempBuf[_tempBufLen- 2], _tempBuf[_tempBufLen- 1])
							^ UTILI_GetChecksum(0x0000, _tempBuf, _tempBufLen- 2);

					_packetCounter= _tempBuf[_tempBufLen- 3];

					if(_packetCounter!= packetCounter)
					{
						/*sometimes we received previous packet, we ignore and wait again*/
						sBC66Link.urc.QCOAPSEND= 0xFF;
						sBC66Link.urc.QCOAPURC.rsp.rspCode= 0xFFFF;
						/*note: (0== sBC66Link.urc.QCOAPSEND) only comes once though*/
					}
					else if((0x00== _tempBuf[0])&& (0x00== _checksum))
					{
						_tempBuf+= 1;/*offset status*/
						_tempBufLen-= 4;/*remove status byte, 2Bcrc and packet counter*/

						uint8_t _packetEncType= (0x0F& config.nbiot.gkcoap.packetType);/*unused, only support 0x32 packetType*/

						/*simple ctr encryption*/
						SECURE_CTR_Transcrypt(3, IV, _tempBuf, _tempBufLen);/*decrypt*/
						uint16_t _encCRC= MAKEWORD(_tempBuf[_tempBufLen- 2], _tempBuf[_tempBufLen- 1])
												^ UTILI_GetChecksum(0x00, _tempBuf, _tempBufLen- 2);
						if(0x00== _encCRC)
						{
							_tempBufLen-= 2;/*remove _encCRC*/
							_trx->coap.GKPost.rxLength= _tempBufLen;
							memcpy(_trx->coap.GKPost.rxData, _tempBuf, _trx->coap.GKPost.rxLength);
							_trx->status= SUCCESS_TransactionStatus;
							_trx->txSent= true;
							_trx->rxReceived= true;
							_trx->transactionInProgress= false;
							_trx->transactionCompleted= true;
							_trx->state= 7;
							DBG_Print("#stat:nb__: reply-success >\r\n");
						}
						else
						{
							/*wrong enc checksum*/
							_trx->status= ERROR_TransactionStatus;
							_trx->txSent= true;
							_trx->rxReceived= false;
							_trx->transactionInProgress= false;
							_trx->transactionCompleted= true;
							_trx->state= 7;
							DBG_Print("#stat:nb__: reply-wrongcrc >\r\n");
						}
					}
					else
					{
						/*wrong enc checksum*/
						_trx->status= ERROR_TransactionStatus;
						_trx->txSent= true;
						_trx->rxReceived= false;
						_trx->transactionInProgress= false;
						_trx->transactionCompleted= true;
						_trx->state= 7;
						DBG_Print("#stat:nb__: reply-wrongcrc >\r\n");
					}
				}
				else
				{
					_trx->status= ERROR_TransactionStatus;
					_trx->txSent= true;
					_trx->rxReceived= false;
					_trx->transactionInProgress= false;
					_trx->transactionCompleted= true;
					_trx->state= 7;
					DBG_Print("#stat:nb__: reply-wronglencrc >\r\n");
				}
			}
			else if(/*(1== sBC66Link.urc.QCOAPSEND)|| */((0xFFFF!= sBC66Link.urc.QCOAPURC.rsp.rspCode)&& (203!= sBC66Link.urc.QCOAPURC.rsp.rspCode)))
			{
				_trx->status= ERROR_TransactionStatus;
				_trx->txSent= true;
				_trx->rxReceived= false;
				_trx->transactionInProgress= false;
				_trx->transactionCompleted= true;
				_trx->state= 7;
			}
			break;

		case 7:
			break;
	}
}

void BC66LINK_COAP_LWM2MGet(void *_param)/*mainly to get firmawre binary file from coap server*/
{
	BC66LINK_Transaction_t *_trx= (BC66LINK_Transaction_t *)_param;
	uint32_t _strLen;

	switch(_trx->state)
	{
		case 0:
			/*jic, fota inhibit this flag by not going to deep sleep*/
			sBC66Link.status.sendLwUpdate= true;/*we need update for decad mqtt reconnection*/

			if(0== sBC66Link.coapContext)
			{
				_trx->state= 1;
				break;
			}
			else if(2!= sBC66Link.coapContext)//(false== _trx->coap.Lwm2mGet.created)
			{
				BC66PHY_TxRxJob("AT+QCOAPDEL\r\n", 1, rx_OK, 5, 2);/*when gkcoap already use the coap context. only one can be*/
				_trx->state= 1;
				break;
			}
			else
			{
				_trx->state= 3;
				break;
			}

		case 1:
			sprintf((char *)pucTxBuffer, "AT"
					"+QCOAPCREATE="
					"%d,"/*localport*/
					"%d"/*mode*/,
					_trx->coap.Lwm2mGet.localPort,
					_trx->coap.Lwm2mGet.encMode
					);
			_strLen= strlen((char *)pucTxBuffer);
			if(PSK_Bc66LinkCoapEncryption== _trx->coap.Lwm2mGet.encMode)
			{
				sprintf((char *)pucTxBuffer+ _strLen,
						",%s"/*pskId*/
						",%s"/*psk*/,
						_trx->coap.Lwm2mGet.pskId,
						_trx->coap.Lwm2mGet.psk
						);
				_strLen= strlen((char *)pucTxBuffer);
			}
			/*append new line*/
			sprintf((char *)(pucTxBuffer+ _strLen), "\r\n");/*generate msg id and token*/

			BC66PHY_TxRxJob(pucTxBuffer, 1, rx_OK, 5, 2);
			_trx->state= 2;
			break;

		case 2:
			if(SUCCESS_Bc66PhyJobstatus== BC66PHY_GetJobStatus())
			{
				_trx->coap.Lwm2mGet.created= true;
				sBC66Link.coapContext= 2;
				_trx->state= 3;
			}
			else
			{
				_trx->status= ERROR_TransactionStatus;
				_trx->txSent= false;
				_trx->rxReceived= false;
				_trx->transactionInProgress= false;
				_trx->transactionCompleted= true;
				_trx->state= 7;
			}
			break;

		case 3:
			{
				uint8_t _optionsCount= 0;
				uint8_t _options[180]= {0};
				uint8_t _uri[255]= {0};
				memcpy(_uri, _trx->coap.Lwm2mGet.serverURI, strlen(_trx->coap.Lwm2mGet.serverURI));
				char *_uriPath= strtok(_uri, "/");

				while(NULL!= _uriPath)
				{
					if((strlen(_options)+ 1+ 2+ 1+ 1+ strlen(_uriPath)+ 1)< 180)
					{
						sprintf((char *)(_options+ strlen(_options)), ",11,\"%s\"", _uriPath);
						_optionsCount++;
					}
					_uriPath= strtok(NULL, "/");
				}

				sprintf((char *)(_options+ strlen(_options)), ",23,\"%d\"", _trx->coap.Lwm2mGet.block2Code);
				_optionsCount++;

				sprintf((char *)pucTxBuffer, "AT"
						"+QCOAPHEAD=1;"
						"+QCOAPOPTION="
						"%d"
						"%s"
						"\r\n",
						_optionsCount,
						_options);

				if(_trx->coap.Lwm2mGet.block2Code>= 100)
				{
					BC66PHY_TxRxJob(pucTxBuffer, 1, rx_ERROR, 5, 1);/*we expect bc66 return ERROR for block2 more >= 100, but still pass*/
				}
				else
				{
					BC66PHY_TxRxJob(pucTxBuffer, 1, rx_OK, 5, 1);
				}
				_trx->state= 4;
			}
			break;

		case 4:
			_trx->state= 5;

		case 5:
			{
				//SYS_Delay(1000);/*temp cos option keep return error*/
				sprintf((char *)pucTxBuffer, "AT"
						"+QCOAPSEND="
						"0,"/*type*///CON_Bc66LinkCoapMsgType
						"1,"/*method/rspCode*///GET_Bc66LinkCoapMethod
						"%s,"/*ipAddr*/
						"%d,"/*port*/
						"0"/*length*/
						"\r\n",
						_trx->coap.Lwm2mGet.serverIP,
						_trx->coap.Lwm2mGet.serverPort
						);

				sBC66Link.urc.QCOAPSEND= 0xFF;
				sBC66Link.urc.QCOAPURC.rsp.rspCode= 0xFFFF;
				BC66PHY_TxRxJob(pucTxBuffer, 1, rx_OK, 5, 0);
				_trx->state= 6;
			}

		case 6:
			if(/*(0== sBC66Link.urc.QCOAPSEND)&& */
					(203== sBC66Link.urc.QCOAPURC.rsp.rspCode)/*valid*/
					|| (205== sBC66Link.urc.QCOAPURC.rsp.rspCode)/*content*/
					)
			{
				if(0!= sBC66Link.urc.QCOAPURC.rsp.len)
				{
					UTILI_HexStringToBytes(_trx->coap.Lwm2mGet.rxData, sBC66Link.urc.QCOAPURC.rsp.data, sBC66Link.urc.QCOAPURC.rsp.len);
					_trx->coap.Lwm2mGet.rxLength= sBC66Link.urc.QCOAPURC.rsp.len;
					_trx->status= SUCCESS_TransactionStatus;
					_trx->txSent= true;
					_trx->rxReceived= true;
					_trx->transactionInProgress= false;
					_trx->transactionCompleted= true;
					_trx->state= 7;
					DBG_Print("#stat:nb__: reply-success >\r\n");
				}
				else
				{
					_trx->status= ERROR_TransactionStatus;
					_trx->txSent= true;
					_trx->rxReceived= false;
					_trx->transactionInProgress= false;
					_trx->transactionCompleted= true;
					_trx->state= 7;
					DBG_Print("#stat:nb__: reply-wronglencrc >\r\n");
				}
			}
			else if(/*(1== sBC66Link.urc.QCOAPSEND)|| */((0xFFFF!= sBC66Link.urc.QCOAPURC.rsp.rspCode)&& (203!= sBC66Link.urc.QCOAPURC.rsp.rspCode)))
			{
				_trx->status= ERROR_TransactionStatus;
				_trx->txSent= true;
				_trx->rxReceived= false;
				_trx->transactionInProgress= false;
				_trx->transactionCompleted= true;
				_trx->state= 7;
			}
			break;

		case 7:
			break;
	}
}

void BC66LINK_LWM2M_Diag(DIAG_DCode_t _dcode, uint32_t _objectId, uint32_t _instanceId, uint32_t _resourceId, uint32_t _param, uint32_t _status)
{
	DIAG_Code(_dcode,
			(_status<< 28)|
			(_param<< 24)|
			(_resourceId<< 20)|
			(_instanceId<< 16)|
			((_objectId)& 0x0000FFFF));
}

void BC66LINK_LWM2M_Configure(void *_param)
{
	//static bool _deviceObjectConfigured= false;
	BC66LINK_Transaction_t *_trx= (BC66LINK_Transaction_t *)_param;

	switch(_trx->state)
	{
		case 0:
//		case 1:
//			BC66PHY_TxRxJob("AT+QLWDELOBJ=3303\r\n", 1, rx_OK, 5, 2);
//			_trx->state= 2;
//			break;
//
//		case 2:
//			BC66PHY_TxRxJob("AT+QLWDELOBJ=10266\r\n", 1, rx_OK, 5, 2);
//			_trx->state= 3;
//			break;
//
//		case 3:
//			BC66PHY_TxRxJob("AT+QLWDELOBJ=10278\r\n", 1, rx_OK, 5, 2);
//			_trx->state= 4;
//			break;
//
//		case 4:
//			BC66PHY_TxRxJob("AT+QLWDELOBJ=10376\r\n", 1, rx_OK, 5, 2);
//			_trx->state= 5;
//			break;
//
//		case 5:
//			BC66PHY_TxRxJob("AT+QLWDELOBJ=10377\r\n", 1, rx_OK, 5, 2);
//			_trx->state= 6;
//			break;
//
//		case 6:
//			BC66PHY_TxRxJob("AT+QLWDELOBJ=9\r\n", 1, rx_OK, 5, 2);
//			_trx->state= 20;
//			break;
//		case 7:
//			if(false== _deviceObjectConfigured)
//			{
//				_deviceObjectConfigured= true;
//				//BC66PHY_TxRxJob("AT+QLWCFG=”CR”,3,0,2,”PUB1234567”\r\n", 1, rx_OK, 5, 2);/*Change LwM2M Device Object's resource*/
//				//BC66PHY_TxRxJob("AT+QLWCFG="device",”George Kent”,”NIUBV0L”,”BoardA.1”,”v1.C.N.abcdefg”,”QuectelFwVersion”,”Water Meter”\r\n", 1, rx_OK, 5, 2);/*Change LwM2M device object*/
//				//AT+QLWCFG="device"[,<manufacturer>,<model_no>,<hw_version>,<sw_version>,<fw_version>,<device_type>]
//				sprintf((char *)pucTxBuffer, "AT+QLWCFG=\"CR\",3,0,0,\"George Kent(M) Berhad\";"
//						"+QLWCFG=\"CR\",3,0,1,\"%s\";"
//						//"+QLWCFG=\"CR\",3,0,2,\"%s\";"
//						"+QLWCFG=\"CR\",3,0,17,\"%s:%s\";"
//						"+QLWCFG=\"CR\",3,0,18,\"%s\";"
//						"+QLWCFG=\"CR\",3,0,19,\"%s\""
//						"\r\n",
//						"NRP-B0LV",//config.system.meterSerialNo,
//						//config.system.serialNo,
//						config.system.meterSerialNo, config.system.serialNo,
//						config.system.hwVersion,
//						config.system.fwVersion
//						);
//				BC66PHY_TxRxJob(pucTxBuffer, 1, rx_OK, 5, 2);
//				_trx->state= 20;/*a bit delay for the config to take effect?*/
//			}
//			else
//			{
//				BC66PHY_JobTransition();/*to not sleep, since no job now*/
//				_trx->state= 21;
//			}
//			break;
//
//		case 20:
//			BC66PHY_PauseJob_s(1);/*emulate 1s delay*//*a bit delay for the config to take effect?*/
//			_trx->state= 21;
//			break;

		case 21:
			if(PSK_Bc66LinkLwm2mSecurityMode== _trx->lwm2m.Configure.securityMode)
			{
				sprintf((char *)pucTxBuffer, "AT"
						"+QLWCONFIG=%d,"/*enableBootstrap*/
						"\"%s\",\"%d\","/*serverIP, port*/
						"\"%s\","		/*endpointName*/
						"%d,"			/*lifetime*/
						"%d,"			/*securityMode*/
						"\"%s\","		/*pskId*/
						"\"%s\""		/*psk*/
						"\r\n",
						(false== _trx->lwm2m.Configure.enableBootstrap)? 0: 1,
						_trx->lwm2m.Configure.serverIP, _trx->lwm2m.Configure.serverPort,
						_trx->lwm2m.Configure.endpointName,
						_trx->lwm2m.Configure.lifetime,
						_trx->lwm2m.Configure.securityMode,
						_trx->lwm2m.Configure.pskId,
						_trx->lwm2m.Configure.psk
						);
			}
			else
			{
				sprintf((char *)pucTxBuffer, "AT"
						"+QLWCONFIG=%d,"/*enableBootstrap*/
						"\"%s\",\"%d\","/*serverIP, port*/
						"\"%s\","		/*endpointName*/
						"%d,"			/*lifetime*/
						"%d"			/*securityMode*/
						"\r\n",
						(false== _trx->lwm2m.Configure.enableBootstrap)? 0: 1,
						_trx->lwm2m.Configure.serverIP, _trx->lwm2m.Configure.serverPort,
						_trx->lwm2m.Configure.endpointName,
						_trx->lwm2m.Configure.lifetime,
						_trx->lwm2m.Configure.securityMode
						);
			}

			BC66PHY_TxRxJob(pucTxBuffer, 1, rx_OK, 5, 2);
			_trx->state= 22;
			break;

		case 22:
			_trx->status= (SUCCESS_Bc66PhyJobstatus== BC66PHY_GetJobStatus())?
					SUCCESS_TransactionStatus: ERROR_TransactionStatus;
			_trx->txSent= true;
			_trx->rxReceived= true;
			_trx->transactionInProgress= false;
			_trx->transactionCompleted= true;
			_trx->state= 23;
			break;

		case 23:
			break;

	}
}

void BC66LINK_LWM2M_Register(void *_param)
{
	BC66LINK_Transaction_t *_trx= (BC66LINK_Transaction_t *)_param;

	switch(_trx->state)
	{
		case 0:
		case 1:
			sBC66Link.urc.QLWREG= ONGOING_Lwm2mRegStatus;/*unset the indication of registration*/
			BC66PHY_TxRxJob("AT+QLWREG\r\n", 1, rx_OK, 5, 2);
			_trx->state= 2;
			break;

		case 2:
			_trx->status= (SUCCESS_Bc66PhyJobstatus== BC66PHY_GetJobStatus())?
					SUCCESS_TransactionStatus: ERROR_TransactionStatus;
			_trx->txSent= true;
			_trx->rxReceived= true;
			_trx->transactionInProgress= false;
			_trx->transactionCompleted= true;
			_trx->state= 3;
			break;

		case 3:
			break;

	}
}

void BC66LINK_LWM2M_PostRegister(void *_param)
{
	BC66LINK_Transaction_t *_trx= (BC66LINK_Transaction_t *)_param;

	switch(_trx->state)
	{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			BC66PHY_TxRxJob("AT+QLWCONFIG?;"/*get bootstrap server info*/
					"+QLWCFG?;"/*get config quectel*/
					//"+QLWUPDATE=1,1;"/*update queue mode jic*//*blocked, no need to change server's setting*/
					"+QENG=0;+QENG=1;+QENG=2;+QENG=3"/*get network info*/
					"\r\n", 1, rx_OK, 5, 1);
			_trx->state= 6;
			break;

		case 6:
			sprintf((char *)pucTxBuffer, "AT"
					"+QPING=%d,"/*for TTL information*/
					"\"%s\""/*serverIP*/
					"\r\n",
					1,
					_trx->lwm2m.PostRegister.pingServerIP
					);
			BC66PHY_TxRxJob(pucTxBuffer, 1, rx_OK, 5, 2);
			_trx->state= 7;
			break;

		case 7:
			_trx->status= (SUCCESS_Bc66PhyJobstatus== BC66PHY_GetJobStatus())?
					SUCCESS_TransactionStatus: ERROR_TransactionStatus;
			_trx->txSent= true;
			_trx->rxReceived= true;
			_trx->transactionInProgress= false;
			_trx->transactionCompleted= true;
			_trx->state= 8;
			break;

		case 8:
			break;

	}
}

void BC66LINK_LWM2M_Update(void *_param)
{
	BC66LINK_Transaction_t *_trx= (BC66LINK_Transaction_t *)_param;
	_trx->status= SUCCESS_TransactionStatus;
	_trx->txSent= true;
	_trx->rxReceived= true;
	_trx->transactionInProgress= false;
	_trx->transactionCompleted= true;
}

void BC66LINK_LWM2M_Deregister(void *_param)
{
	BC66LINK_Transaction_t *_trx= (BC66LINK_Transaction_t *)_param;

	switch(_trx->state)
	{
		case 0:
		case 1:
			sprintf((char *)pucTxBuffer, "AT+QLWDEREG"
					"\r\n"
					);

			sBC66Link.urc.QLWDEREG= 0xFF;
			sBC66Link.status.cmeErrorReceived= false;
			sBC66Link.status.cmeErrorType= 0;
			BC66PHY_TxRxJob(pucTxBuffer, 1, rx_OK, 5, 2);
			_trx->state= 2;
		break;

		case 2:
			if(((true== sBC66Link.status.cmeErrorReceived)&& (4== sBC66Link.status.cmeErrorType)) /*not register error*/
				||(0== sBC66Link.urc.QLWDEREG))/*lwm2m successfully de-registered*/
			{
				/*we need some delay before we can reregister again*/
				BC66PHY_PauseJob_s(1);/*emulate 1s delay*/
				_trx->state= 3;
			}
			else if(1== sBC66Link.urc.QLWDEREG)/*lwm2m failed to de-register*/
			{
				_trx->status= ERROR_TransactionStatus;
				_trx->txSent= true;
				_trx->rxReceived= true;
				_trx->transactionInProgress= false;
				_trx->transactionCompleted= true;
				//config.nbiot.transmitFailed++;
				_trx->state= 4;
			}
			break;

		case 3:
			_trx->status= SUCCESS_TransactionStatus;
			_trx->txSent= true;
			_trx->rxReceived= true;
			_trx->transactionInProgress= false;
			_trx->transactionCompleted= true;
			_trx->state= 4;
			break;

		case 4:
			break;

	}
}

void BC66LINK_LWM2M_AddObject(void *_param)
{
	BC66LINK_Transaction_t *_trx= (BC66LINK_Transaction_t *)_param;

	switch(_trx->state)
	{
		case 0:
		case 1:/*delete first cos we always change object definition*/
			/*commented cos cannot delete specific instance, stoopid*/
//			sprintf((char *)pucTxBuffer, "AT"
//					"+QLWDELOBJ=%d"/*objectId*/
//					"\r\n",
//					_trx->lwm2m.AddObject.objectId
//					);
//			BC66PHY_TxRxJob(pucTxBuffer, 1, rx_OK, 5, 2);
//			_trx->state= 2;
//			break;

		case 2:
			sprintf((char *)pucTxBuffer, "AT"
					"+QLWADDOBJ=%d,"/*objectId*/
					"%d,"			/*instanceId*/
					"%d",			/*noOfResource*/
					_trx->lwm2m.AddObject.objectId,
					_trx->lwm2m.AddObject.instanceId,
					_trx->lwm2m.AddObject.noOfResource
					);
			uint32_t _strLen= strlen((char *)pucTxBuffer);
			for(int i= 0; i<_trx->lwm2m.AddObject.noOfResource; i++)
			{
				sprintf((char *)(pucTxBuffer+ _strLen), ",%d", _trx->lwm2m.AddObject.resourceId[i]);
				_strLen= strlen((char *)pucTxBuffer);
			}
			sprintf((char *)(pucTxBuffer+ _strLen), "\r\n");

			BC66PHY_TxRxJob(pucTxBuffer, 1, rx_OK, 5, 2);
			_trx->state= 3;
		break;

		case 3:
			sBC66Link.urc.QLWADDOBJCount= 0;
			BC66PHY_TxRxJob("AT+QLWADDOBJ?\r\n", 1, rx_OK, 5, 2);
			_trx->state= 4;
			break;

		case 4:
			for(int i= 0; i< sBC66Link.urc.QLWADDOBJCount; i++)
			{
				if(_trx->lwm2m.AddObject.objectId== sBC66Link.urc.QLWADDOBJ[i])/*successfully added*/
				{
					_trx->status= SUCCESS_TransactionStatus;
					_trx->txSent= true;
					_trx->rxReceived= true;
					_trx->transactionInProgress= false;
					_trx->transactionCompleted= true;
					_trx->state= 5;
					break;
				}
			}
			break;

		case 5:
			break;

	}
}

void BC66LINK_LWM2M_DeleteObject(void *_param)
{
	BC66LINK_Transaction_t *_trx= (BC66LINK_Transaction_t *)_param;
	_trx->status= SUCCESS_TransactionStatus;
	_trx->txSent= true;
	_trx->rxReceived= true;
	_trx->transactionInProgress= false;
	_trx->transactionCompleted= true;
}

void BC66LINK_LWM2M_WriteResponse(void *_param)
{
	BC66LINK_Transaction_t *_trx= (BC66LINK_Transaction_t *)_param;

	switch(_trx->state)
	{
		case 0:
		case 1:
			sprintf((char *)pucTxBuffer, "AT+QLWWRRSP="
					"%d,"/*messageId*/
					"%d"			/*result*/
					"\r\n",
					_trx->lwm2m.WriteResponse.messageId,
					(uint8_t)_trx->lwm2m.WriteResponse.result
					);

			sBC66Link.urc.QLWWRRSP= 0xFF;
			sBC66Link.status.cmeErrorReceived= false;
			sBC66Link.status.cmeErrorType= 0;
			BC66PHY_TxRxJob(pucTxBuffer, 1, rx_OK, 5, 2);
			_trx->state= 2;
		break;

		case 2:

		case 3:
			if((WRITE_RESPONSE_Bc66LinkLwm2m== _trx->lwm2mType)&& (0== sBC66Link.urc.QLWWRRSP))
			{
				_trx->status= SUCCESS_TransactionStatus;
				_trx->txSent= true;
				_trx->rxReceived= true;
				_trx->state= 4;
			}
			else if((true== sBC66Link.status.cmeErrorReceived)&& (3== sBC66Link.status.cmeErrorType)) /*parameter value error*/
			{
				_trx->status= ERROR_TransactionStatus;
				_trx->txSent= true;
				_trx->rxReceived= true;
				//config.nbiot.transmitFailed++;
				_trx->state= 4;
			}
			break;

		case 4:
			_trx->transactionInProgress= false;
			_trx->transactionCompleted= true;

			BC66LINK_LWM2M_Diag(WRITE_RESPONSE_NbiotLwm2mDCode,
					_trx->lwm2m.WriteResponse.objectId,
					_trx->lwm2m.WriteResponse.instanceId, _trx->lwm2m.WriteResponse.resourceId,
					_trx->lwm2m.WriteResponse.result, sBC66Link.urc.QLWWRRSP);
			_trx->state= 5;
			break;

		case 5:
			break;

	}
}

void BC66LINK_LWM2M_ReadResponse(void *_param)
{
	BC66LINK_Transaction_t *_trx= (BC66LINK_Transaction_t *)_param;

	switch(_trx->state)
	{
		case 0:
		case 1:
			sprintf((char *)pucTxBuffer, "AT+QLWRDRSP="
					"%d,"/*messageId*/
					"%d,"			/*result*/
					"%d,"			/*objectId*/
					"%d,"			/*instanceId*/
					"%d,"			/*resourceId*/
					"%d,"			/*valueType*/
					"%d,"			/*len*/
					"%s,"			/*value*/
					"%d"			/*index*/
					"\r\n",
					_trx->lwm2m.ReadResponse.messageId,
					(uint8_t)_trx->lwm2m.ReadResponse.result,
					_trx->lwm2m.ReadResponse.objectId,
					_trx->lwm2m.ReadResponse.instanceId,
					_trx->lwm2m.ReadResponse.resourceId,
					_trx->lwm2m.ReadResponse.valueType,
					_trx->lwm2m.ReadResponse.len,
					_trx->lwm2m.ReadResponse.value,
					_trx->lwm2m.ReadResponse.index
					);

			sBC66Link.urc.QLWRDRSP= 0xFF;
			sBC66Link.status.cmeErrorReceived= false;
			sBC66Link.status.cmeErrorType= 0;
			BC66PHY_TxRxJob(pucTxBuffer, 1, rx_OK, 5, 2);
			_trx->state= 2;
		break;

		case 2:

		case 3:
			if(0!= _trx->lwm2m.ReadResponse.index)/*not the last transaction, thus cannot expect QLWRDRSP urc.*/
			{
				_trx->status= SUCCESS_TransactionStatus;
				_trx->txSent= true;
				_trx->rxReceived= true;
				_trx->state= 4;
			}
			else if((READ_RESPONSE_Bc66LinkLwm2m== _trx->lwm2mType)&& (0== sBC66Link.urc.QLWRDRSP))
			{
				_trx->status= SUCCESS_TransactionStatus;
				_trx->txSent= true;
				_trx->rxReceived= true;
				_trx->state= 4;
			}
			else if((true== sBC66Link.status.cmeErrorReceived)&& (3== sBC66Link.status.cmeErrorType)) /*parameter value error*/
			{
				_trx->status= ERROR_TransactionStatus;
				_trx->txSent= true;
				_trx->rxReceived= true;
				//config.nbiot.transmitFailed++;
				_trx->state= 4;
			}
			break;

		case 4:
			_trx->transactionInProgress= false;
			_trx->transactionCompleted= true;

			BC66LINK_LWM2M_Diag(READ_RESPONSE_NbiotLwm2mDCode,
					_trx->lwm2m.ReadResponse.objectId,
					_trx->lwm2m.ReadResponse.instanceId, _trx->lwm2m.ReadResponse.resourceId,
					_trx->lwm2m.ReadResponse.index, sBC66Link.urc.QLWRDRSP);
			_trx->state= 5;
			break;

		case 5:
			break;

	}
}

void BC66LINK_LWM2M_ExecuteResponse(void *_param)
{
	BC66LINK_Transaction_t *_trx= (BC66LINK_Transaction_t *)_param;

	switch(_trx->state)
	{
		case 0:
		case 1:
			sprintf((char *)pucTxBuffer, "AT+QLWEXERSP="
					"%d,"/*messageId*/
					"%d"/*result*/
					"\r\n",
					_trx->lwm2m.ExecuteResponse.messageId,
					(uint8_t)_trx->lwm2m.ExecuteResponse.result
					);

			sBC66Link.urc.QLWEXERSP= 0xFF;
			sBC66Link.status.cmeErrorReceived= false;
			sBC66Link.status.cmeErrorType= 0;
			BC66PHY_TxRxJob(pucTxBuffer, 1, rx_OK, 5, 2);
			_trx->state= 2;
		break;

		case 2:

		case 3:
			if((EXECUTE_RESPONSE_Bc66LinkLwm2m== _trx->lwm2mType)&& (0== sBC66Link.urc.QLWEXERSP))
			{
				_trx->status= SUCCESS_TransactionStatus;
				_trx->txSent= true;
				_trx->rxReceived= true;
				_trx->state= 4;
			}
			else if((true== sBC66Link.status.cmeErrorReceived)&& (3== sBC66Link.status.cmeErrorType)) /*parameter value error*/
			{
				_trx->status= ERROR_TransactionStatus;
				_trx->txSent= true;
				_trx->rxReceived= true;
				//config.nbiot.transmitFailed++;
				_trx->state= 4;
			}
			break;

		case 4:
			_trx->transactionInProgress= false;
			_trx->transactionCompleted= true;

			BC66LINK_LWM2M_Diag(EXECUTE_RESPONSE_NbiotLwm2mDCode,
					_trx->lwm2m.ExecuteResponse.objectId,
					_trx->lwm2m.ExecuteResponse.instanceId, _trx->lwm2m.ExecuteResponse.resourceId,
					_trx->lwm2m.ExecuteResponse.result, sBC66Link.urc.QLWWRRSP);
			_trx->state= 5;
			break;

		case 5:
			break;

	}
}

void BC66LINK_LWM2M_ObserveResponse(void *_param)
{
	BC66LINK_Transaction_t *_trx= (BC66LINK_Transaction_t *)_param;

	switch(_trx->state)
	{
		case 0:
		case 1:
			sprintf((char *)pucTxBuffer, "AT+QLWOBSRSP="
					"%d,"/*messageId*/
					"%d,"			/*result*/
					"%d,"			/*objectId*/
					"%d,"			/*instanceId*/
					"%d,"			/*resourceId*/
					"%d,"			/*valueType*/
					"%d,"			/*len*/
					"%s,"			/*value*/
					"%d"			/*index*/
					"\r\n",
					_trx->lwm2m.ReadResponse.messageId,
					(uint8_t)_trx->lwm2m.ReadResponse.result,
					_trx->lwm2m.ReadResponse.objectId,
					_trx->lwm2m.ReadResponse.instanceId,
					_trx->lwm2m.ReadResponse.resourceId,
					_trx->lwm2m.ReadResponse.valueType,
					_trx->lwm2m.ReadResponse.len,
					_trx->lwm2m.ReadResponse.value,
					_trx->lwm2m.ReadResponse.index
					);

			sBC66Link.urc.QLWOBSRSP= 0xFF;
			sBC66Link.status.cmeErrorReceived= false;
			sBC66Link.status.cmeErrorType= 0;
			BC66PHY_TxRxJob(pucTxBuffer, 1, rx_OK, 5, 2);
			_trx->state= 2;
		break;

		case 2:

		case 3:
			if(0!= _trx->lwm2m.ReadResponse.index)/*not the last transaction, thus cannot expect QLWRDRSP urc.*/
			{
				_trx->status= SUCCESS_TransactionStatus;
				_trx->txSent= true;
				_trx->rxReceived= true;
				_trx->state= 4;
			}
			else if((OBSERVE_RESPONSE_Bc66LinkLwm2m== _trx->lwm2mType)&& (0== sBC66Link.urc.QLWOBSRSP))
			{
				_trx->status= SUCCESS_TransactionStatus;
				_trx->txSent= true;
				_trx->rxReceived= true;
				_trx->state= 4;
			}
			else if((true== sBC66Link.status.cmeErrorReceived)&& (3== sBC66Link.status.cmeErrorType)) /*parameter value error*/
			{
				_trx->status= ERROR_TransactionStatus;
				_trx->txSent= true;
				_trx->rxReceived= true;
				//config.nbiot.transmitFailed++;
				_trx->state= 4;
			}
			break;

		case 4:
			_trx->transactionInProgress= false;
			_trx->transactionCompleted= true;

			BC66LINK_LWM2M_Diag(OBSERVE_RESPONSE_NbiotLwm2mDCode,
					_trx->lwm2m.ReadResponse.objectId,
					_trx->lwm2m.ReadResponse.instanceId, _trx->lwm2m.ReadResponse.resourceId,
					_trx->lwm2m.ReadResponse.index, sBC66Link.urc.QLWRDRSP);
			_trx->state= 5;
			break;

		case 5:
			break;

	}
}

void BC66LINK_LWM2M_Notify(void *_param)
{
	BC66LINK_Transaction_t *_trx= (BC66LINK_Transaction_t *)_param;
	static int8_t _recoveryWaitCountdown;

	switch(_trx->state)
	{
		case 0:
			_recoveryWaitCountdown= BC66LINK_CFG_LWM2M_RECOVERY_WAIT_S;
			sBC66Link.status.qlwregReceived= false;/*we may get recovered or reg in the middle*/
			_trx->state= 0x0F;
		case 0x0F:
			if((0== _recoveryWaitCountdown)||
					(true== sBC66Link.status.qlwregReceived)||
					//blocked cos we want to allow the modem to complete its auto registration when recovered = 3((0!= sBC66Link.urc.QLWURC.recovered)&& (0xFF!= sBC66Link.urc.QLWURC.recovered))|| /*we may get failed recovery after we forced update*/
					(true== sBC66Link.status.inPSM))
			{
				BC66PHY_JobTransition();/*to not sleep, since no job now*/
				_trx->state= 10;
				break;
			}
			else if(0!= sBC66Link.urc.QLWURC.recovered)
			{
				BC66PHY_PauseJob_s(1);/*emulate delay*/
				--_recoveryWaitCountdown;
				break;
			}
		case 1:
			if(true== sBC66Link.status.sendLwUpdate)
			{
				sBC66Link.status.sendLwUpdate= false;
				sprintf((char *)pucTxBuffer, "AT+QLWUPDATE="
						"0,"/*mode: lifetime*/
						"%d"/*lifetime*/
						"\r\n",
						_trx->lwm2m.Notify.lifetime
						);
				sBC66Link.urc.QLWUPDATE= 0xFF;
				BC66PHY_TxRxJob(pucTxBuffer, 1, rx_OK, 5, 1);
				_trx->state= 10;
			}
			else
			{
				BC66PHY_JobTransition();/*to not sleep, since no job now*/
				_trx->state= 2;
			}
			break;

		case 10:
			if((0== _recoveryWaitCountdown)||
					(true== sBC66Link.status.qlwregReceived)||
					(0!= sBC66Link.urc.QLWURC.recovered)|| /*we may get failed recovery after we forced update*/
					(true== sBC66Link.status.inPSM))/*modem may even go back to sleep babi betul*/
			{
				_trx->status= ERROR_TransactionStatus;
				_trx->txSent= true;/*dummy*/
				_trx->rxReceived= true;/*dummy*/
				_trx->lwm2m.Notify.ackStatus= 2;/*packet not sent.*/
				_trx->state= 5;
				 if((0== _recoveryWaitCountdown)/*|| (0!= sBC66Link.urc.QLWURC.recovered)*/)
				 {
					 /*tricky situation, we always hang here. just reboot?*/
					 BC66LINK_ResetModem();
				 }
			}
			else if(0== sBC66Link.urc.QLWUPDATE)/*decada use this to reconnect mqtt, so we need to wait until server acked this*/
			{
				BC66PHY_PauseJob_s(config.nbiot.downlinkWaitPeriod_s);/*emulate delay*//*decada need some delay to reconnect mqtt*/
				_trx->state= 2;
			}
			else
			{
				--_recoveryWaitCountdown;
				BC66PHY_PauseJob_s(1);/*to avoid mcu sleeping long while waiting*/
			}
			break;

		case 2:
			sprintf((char *)pucTxBuffer, "AT+QLWNOTIFY="
					"%d,"			/*objectId*/
					"%d,"			/*instanceId*/
					"%d,"			/*resourceId*/
					"%d,"			/*valueType*/
					"%d,"			/*len*/
					"%s,"			/*value*/
					"%d,"			/*index*/
					"%d,"			/*ack*/
					"%d"			/*raiFlag*/
					"\r\n",
					_trx->lwm2m.Notify.objectId,
					_trx->lwm2m.Notify.instanceId,
					_trx->lwm2m.Notify.resourceId,
					_trx->lwm2m.Notify.valueType,
					_trx->lwm2m.Notify.len,
					_trx->lwm2m.Notify.value,
					_trx->lwm2m.Notify.index,
					_trx->lwm2m.Notify.ack,
					_trx->lwm2m.Notify.raiFlag
					);

			sBC66Link.urc.QLWNOTIFY= 0xFF;
			sBC66Link.status.cmeErrorReceived= false;
			sBC66Link.status.cmeErrorType= 0;
			sBC66Link.urc.QLWURC.report_ack= false;
			sBC66Link.urc.QLWURC.report_ack_status= 0xFF;
			BC66PHY_TxRxJob(pucTxBuffer, 1, rx_OK, 5, 1);
			_trx->state= 3;
		break;

		case 3:

		case 4:
			if(
					((0== _trx->lwm2m.Notify.ack)&& (0== sBC66Link.urc.QLWNOTIFY)) ||
					((1== _trx->lwm2m.Notify.ack)&& (0== sBC66Link.urc.QLWNOTIFY)&& (true== sBC66Link.urc.QLWURC.report_ack)&& (0== sBC66Link.urc.QLWURC.report_ack_status)))
			{
				_trx->status= SUCCESS_TransactionStatus;
				_trx->txSent= true;
				_trx->rxReceived= true;
				_trx->lwm2m.Notify.ackStatus= sBC66Link.urc.QLWURC.report_ack_status;
				BC66PHY_JobTransition();/*compulsory, otherwise mcu sleep longer*/
				_trx->state= 5;
			}
			else if((1== sBC66Link.urc.QLWNOTIFY)
					||(9== sBC66Link.urc.QLWNOTIFY)
					|| ((true== sBC66Link.status.cmeErrorReceived)&& (3== sBC66Link.status.cmeErrorType))/*parameter value error*/
					|| ((true== sBC66Link.urc.QLWURC.report_ack)&& (0!= sBC66Link.urc.QLWURC.report_ack_status))
					)
			{
				_trx->status= ERROR_TransactionStatus;
				_trx->txSent= true;
				_trx->rxReceived= true;
				if((1== sBC66Link.urc.QLWNOTIFY)|| (9== sBC66Link.urc.QLWNOTIFY))
				{
					_trx->lwm2m.Notify.ackStatus= sBC66Link.urc.QLWNOTIFY;
				}
				else
				{
					_trx->lwm2m.Notify.ackStatus= 2;/*packet not sent.*//*locked cos value 1 already used by qlwnotify urc*///sBC66Link.urc.QLWURC.report_ack_status;
				}
				//config.nbiot.transmitFailed++;
				BC66PHY_JobTransition();/*compulsory, otherwise mcu sleep longer*/
				_trx->state= 5;
			}
			else
			{
				BC66PHY_PauseJob_s(1);/*to avoid mcu sleeping long while waiting*/
			}
			break;

		case 5:
//			if(true== sBC66Link.status.sendLwUpdate)
//			{
//				sBC66Link.status.sendLwUpdate= false;
//				sprintf((char *)pucTxBuffer, "AT+QLWUPDATE="
//						"0,"/*mode: lifetime*/
//						"%d"/*lifetime*/
//						"\r\n",
//						_trx->lwm2m.Notify.lifetime
//						);
//				BC66PHY_TxRxJob(pucTxBuffer, 1, rx_OK, 5, 1);
//			}
			_trx->transactionInProgress= false;
			_trx->transactionCompleted= true;

			BC66LINK_LWM2M_Diag(NOTIFY_NbiotLwm2mDCode,
					_trx->lwm2m.Notify.objectId,
					_trx->lwm2m.Notify.instanceId, _trx->lwm2m.Notify.resourceId,
					_trx->status, _trx->lwm2m.Notify.ackStatus);
			_trx->state= 6;
			break;

		case 6:
			break;

	}
}

void BC66LINK_LWM2M_ReadData(void *_param)
{
	BC66LINK_Transaction_t *_trx= (BC66LINK_Transaction_t *)_param;
	_trx->status= SUCCESS_TransactionStatus;
	_trx->txSent= true;
	_trx->rxReceived= true;
	_trx->transactionInProgress= false;
	_trx->transactionCompleted= true;
}

void BC66LINK_LWM2M_Status(void *_param)
{
	BC66LINK_Transaction_t *_trx= (BC66LINK_Transaction_t *)_param;
	_trx->status= SUCCESS_TransactionStatus;
	_trx->txSent= true;
	_trx->rxReceived= true;
	_trx->transactionInProgress= false;
	_trx->transactionCompleted= true;
}

uint8_t BC66LINK_TaskState(void)
{
	SYS_TaskState_t _taskState= SLEEP_TaskState;

	if((0!= BC66LINK_QueueDepth())&& (sBC66Link.state!= PROCESS_REQUEST_Bc66LinkState))
	{
		if(false==
				((sBC66Link.state== WAIT_REQUEST_Bc66LinkState)&& (false== BC66LINK_NetworkIsRegistered()))) /*if unregistered, we want to wait for re-registration without triggering wdg*/
		{
			_taskState= RUN_TaskState;
		}
	}

	return (_taskState| BC66PHY_TaskState());
}

