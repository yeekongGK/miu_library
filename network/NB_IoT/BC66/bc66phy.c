/*
 * bc66.c
 *
 *  Created on: 30 Mar 2020
 *      Author: muhammad.ahmad@georgekent.net
 */

#include <BC66/bc66phy.h>
#include "common.h"
#include "lpuart1.h"
#include "ioctrl.h"
#include "batterysensor.h"

//#define DBG_PrintByte(x)

static void BC66PHY_SetTimeout(uint16_t _seconds);
static bool BC66PHY_IsTimeout(void);

__IO static bool bTxInProgress= false;
__IO static uint32_t uwTimeoutValue= 0;

__IO static char 		pcReplyMsg	[BC66PHY_CFG_MAX_REPLY_MSG_ARRAY][BC66PHY_CFG_MAX_REPLY_MSG_LEN];
__IO static char 		pcURCMsg	[BC66PHY_CFG_MAX_URC_MSG_ARRAY  ][BC66PHY_CFG_MAX_URC_MSG_LEN  ];
__IO static uint8_t		ucCurrByte;
__IO static uint8_t		ucPrevByte= 0;
__IO static bool   		ubIsURCMsg= false;
__IO static uint16_t 	ubReplyMsgIndex= 0;
__IO static uint8_t 	ubReplyArrayHead= 0;
__IO static uint8_t 	ubReplyArrayTail= 0;
__IO static uint16_t 	ubURCMsgIndex= 0;
__IO static uint8_t 	ubURCArrayHead= 0;
__IO static uint8_t 	ubURCArrayTail= 0;
__IO static bool 		bInhibitDiag= false;
__IO static bool 		bInhibitErrorReset= false;

__IO static bool 		bHardReset= false;

static BC66PHY_Job_t eBC66Job = {
		.status= SUCCESS_Bc66PhyJobstatus,
		.state= IDLE_Bc66PhyJobState,
		.txString= NULL,
		.expectedRxCnt= 0,
		.expectedRx= NULL,
		.timeout= 0,
		.noOfRetry= 0,
		.rxBuffer= NULL,
};

void BC66PHY_Init(void)
{
	IOCTRL_RadioPower_Init(false);
	IOCTRL_RadioPowerBypass_Init(false);/*never set to 1 when RadioPower is off, cos the pin will suck current*/
	IOCTRL_RadioPowerSignal_Init(false);
	IOCTRL_RadioReset_Init(false);
	IOCTRL_RadioPSM_Init(false, IOCTRL_RadioPSMPin_NBIOT);
	IOCTRL_SIM_Init();

	LPUART1_Init();
}

void BC66PHY_InitBaudrate(uint32_t _baudrate)
{
	LPUART1_ReInit(_baudrate);
}

uint32_t BC66PHY_GetBaudrate(void)
{
	return LPUART1_GetBaudrate();
}

void BC66PHY_SetRxTxPinOD(bool _rxPin, bool _txPin)
{
	LPUART1_SetPinoutsOpenDrain(_rxPin, _txPin);
}

void BC66PHY_PowerState(BC66PHY_Power _state)
{
	/*when transmitting we want to enable switching regulator(3.3V output) but this consume 30uA. thus need to bypass(back to 3.6V battery volt) after use.*/
	switch(_state)
	{
		case DISABLED_Bc66PhyPower:
		case SLEEP_Bc66PhyPower:
			SYS_EnableModemPower(false);
			break;

		case ACTIVE_Bc66PhyPower:
			SYS_EnableModemPower(true);
			break;

		default:
			SYS_FailureHandler();
			break;
	}
}

void BC66PHY_PowerSignalPin(uint8_t _state)
{
	if(0== _state)
	{
		IOCTRL_RadioPowerSignal_Enable(false);
	}
	else
	{
		IOCTRL_RadioPowerSignal_Enable(true);
	}
}

void BC66PHY_ResetPin(uint8_t _state)
{
	if(0== _state)
	{
		IOCTRL_RadioReset_Enable(false);
	}
	else
	{
		IOCTRL_RadioReset_Enable(true);
	}
}

void BC66PHY_PSMPin(uint8_t _state)
{
	if(0== _state)
	{
		IOCTRL_RadioPSM_Enable(false, IOCTRL_RadioPSMPin_NBIOT);
	}
	else
	{
		IOCTRL_RadioPSM_Enable(true, IOCTRL_RadioPSMPin_NBIOT);
	}
}

bool BC66PHY_SIMDetected(void)
{
	return IOCTRL_SIM_Detected();
}

uint32_t BC66PHY_DiagCode(uint8_t _code)
{
	return (_code<< 24)| ((uint32_t)SENSOR_GetValue(INTERNAL_VOLTAGE_Sensor)<< 8)| ((uint32_t)SENSOR_GetValue(INTERNAL_TEMPERATURE_Sensor));
}

void BC66PHY_PowerDown(void)
{
	BC66PHY_SetRxTxPinOD(true, true);
	BC66PHY_PowerState(DISABLED_Bc66PhyPower);
	BC66PHY_PowerSignalPin(0);
	BC66PHY_ResetPin(0);
	SYS_Delay(20);

	DIAG_Code(MODEM_POWER_STATE_NbiotDCode, BC66PHY_DiagCode(DISABLED_Bc66PhyPower));
}

void BC66PHY_PowerUp(void)
{
	BC66PHY_SetRxTxPinOD(false, false);
	BC66PHY_PowerState(ACTIVE_Bc66PhyPower);
	BC66PHY_PowerSignalPin(1);
	SYS_Delay(500);/*500ms atleast*/
	BC66PHY_PowerSignalPin(0);

	DIAG_Code(MODEM_POWER_STATE_NbiotDCode, BC66PHY_DiagCode(ACTIVE_Bc66PhyPower));
}

void BC66PHY_Sleep(void)
{
	BC66PHY_PowerState(SLEEP_Bc66PhyPower);

	DIAG_Code(MODEM_POWER_STATE_NbiotDCode, BC66PHY_DiagCode(SLEEP_Bc66PhyPower));
}

void BC66PHY_Wakeup(void)
{
	BC66PHY_PowerState(ACTIVE_Bc66PhyPower);

	DIAG_Code(MODEM_POWER_STATE_NbiotDCode, BC66PHY_DiagCode(ACTIVE_Bc66PhyPower));
}

void BC66PHY_Reset(void)
{
	BC66PHY_ResetPin(1);
	SYS_Delay(50);/*50ms atleast*/
	BC66PHY_ResetPin(0);
	bHardReset= true;
}

void BC66PHY_ExitPSM(void)
{
	BC66PHY_PSMPin(1);
	SYS_Delay(5);/*4ms atleast*/
	BC66PHY_PSMPin(0);
}

bool BC66PHY_IsTxInProgress(void)
{
	return LPUART1_IsTransmitInProgress();
}

static void BC66PHY_SetTimeout(uint16_t _seconds)
{
	if(0!= _seconds)
	{
		SYS_Sleep(BC66_PHY_TaskId, _seconds* 1000);
	}
}

static bool BC66PHY_IsTimeout(void)
{
	return (true== SYS_IsAwake(BC66_PHY_TaskId));
}

ErrorStatus BC66PHY_PrepareRx(void)
{
	LPUART1_RequestReceiveDataFlush();

	ucPrevByte= 0;
	ubIsURCMsg= false;
	ubReplyMsgIndex= 0;
	ubReplyArrayHead= 0;
	ubURCMsgIndex= 0;
	ubURCArrayHead= 0;
	ubReplyArrayTail= 0;
	ubURCArrayTail= 0;
	return SUCCESS;
}

void LPUART1_ReceiveCallback(uint8_t _data8)
{
	ucCurrByte= _data8;

	DBG_PrintByte( ucCurrByte);

	if(((true!= ubIsURCMsg)&& (ubReplyMsgIndex== 0))&&
			((ucCurrByte== '\r')|| (ucCurrByte== '\n'))) /* "/r/n" at the start of message */
	{
		/*do nothing as we don't want to store this*/
	}
	else if((ucPrevByte== '\r')&& (ucCurrByte== '\n')) /* "/r/n" at the end of message */
	{
		if(true== ubIsURCMsg)
		{
			pcURCMsg[ubURCArrayHead][ubURCMsgIndex-1]= '\0'; /*replace \r with \0 - terminate string*/
			/*change buffer for next message to arrive:*/
			ubURCMsgIndex= 0;
			ubURCArrayHead= (ubURCArrayHead+ 1)% BC66PHY_CFG_MAX_URC_MSG_ARRAY;

			ubIsURCMsg= false;
		}
		else
		{
			pcReplyMsg[ubReplyArrayHead][ubReplyMsgIndex-1]= '\0';/*replace \r with \0 - terminate string*/
			/*change buffer for next message to arrive:*/
			ubReplyMsgIndex= 0;
			ubReplyArrayHead= (ubReplyArrayHead+ 1)% BC66PHY_CFG_MAX_REPLY_MSG_ARRAY;
		}
		//TODO check overrun
	}
	else
	{
		if(ucCurrByte== '+')
		{
			ubIsURCMsg= true;
		}

		if(true== ubIsURCMsg)
		{
			if((ucCurrByte== '"')|| (ucCurrByte== ' '))/*ignore space & " characters*/
			{
				/*do nothing as we don't want to store this*/
			}
			else if(ubURCMsgIndex< BC66PHY_CFG_MAX_URC_MSG_LEN)/*truncate long messages.*/
			{
				pcURCMsg[ubURCArrayHead][ubURCMsgIndex++]= ucCurrByte;
			}
		}
		else
		{
			if(ubReplyMsgIndex< BC66PHY_CFG_MAX_REPLY_MSG_LEN)/*truncate long messages.*/
			{
				pcReplyMsg[ubReplyArrayHead][ubReplyMsgIndex++]= ucCurrByte;
			}
		}
	}

	ucPrevByte= ucCurrByte;
}

uint8_t BC66PHY_ReplyMsg_Depth(void)
{
	if(ubReplyArrayTail< ubReplyArrayHead)
	{
		return ubReplyArrayHead- ubReplyArrayTail;
	}
	else if(ubReplyArrayTail> ubReplyArrayHead)
	{
		return (BC66PHY_CFG_MAX_REPLY_MSG_ARRAY+ ubReplyArrayHead)- ubReplyArrayTail;
	}
	else
	{
		return 0;
	}
}

char* BC66PHY_ReplyMsg_Dequeue(void)
{
	char* _msg= NULL;
	if(ubReplyArrayTail!= ubReplyArrayHead)
	{
		_msg= (char *)pcReplyMsg[ubReplyArrayTail];
		ubReplyArrayTail= (ubReplyArrayTail+ 1)% BC66PHY_CFG_MAX_REPLY_MSG_ARRAY;
	}

	return _msg;
}

char* BC66PHY_ReplyMsg_Peek(void)
{
	char* _msg= NULL;
	if(ubReplyArrayTail!= ubReplyArrayHead)
	{
		_msg= (char *)pcReplyMsg[ubReplyArrayTail];
	}

	return _msg;
}

uint8_t BC66PHY_URCMsg_Depth(void)
{
	if(ubURCArrayTail< ubURCArrayHead)
	{
		return ubURCArrayHead- ubURCArrayTail;
	}
	else if(ubURCArrayTail> ubURCArrayHead)
	{
		return (BC66PHY_CFG_MAX_URC_MSG_ARRAY+ ubURCArrayHead)- ubURCArrayTail;
	}
	else
	{
		return 0;
	}
}

char* BC66PHY_URCMsg_Dequeue(void)
{
	char* _msg= NULL;
	if(ubURCArrayTail!= ubURCArrayHead)
	{
		_msg= (char *)pcURCMsg[ubURCArrayTail];
		ubURCArrayTail= (ubURCArrayTail+ 1)% BC66PHY_CFG_MAX_URC_MSG_ARRAY;
	}

	return _msg;
}

char* BC66PHY_URCMsg_Peek(void)
{
	char* _msg= NULL;
	if(ubURCArrayTail!= ubURCArrayHead)
	{
		_msg= (char *)pcURCMsg[ubURCArrayTail];
	}

	return _msg;
}

void BC66PHY_Job(void)
{
	static uint8_t _ERRORCount= 0, _timeoutCount= 0;

	switch(eBC66Job.state)
	{
		case IDLE_Bc66PhyJobState:/*#messy*/
			while(0!= BC66PHY_ReplyMsg_Depth())/*need to empty queue to go to sleep*/
			{
				if(NULL!= strstr(BC66PHY_ReplyMsg_Peek(), "RDY"))/*The only bc66 Reset message that we recognize at baudrate 9600*/
				{
					/*don't dequeue this, it need to be handled by resethandler*/
					break;
				}
				else
				{
					BC66PHY_ReplyMsg_Dequeue();//empty queue
				}
			}
			break;

		case PREPARING_Bc66PhyJobState:
			while(BC66PHY_ReplyMsg_Dequeue()){}//empty queue/
			eBC66Job.status= BUSY_Bc66PhyJobstatus;
			BC66PHY_SetTimeout(eBC66Job.timeout);
			if(NULL!= eBC66Job.txString)
			{
				SYS_Delay(20);/*sara-n2 good practice at least 20ms before transmission*/
				LPUART1_Transmit(eBC66Job.txString);
			}
			eBC66Job.state= TRANSMITTING_Bc66PhyJobState;
			break;

		case TRANSMITTING_Bc66PhyJobState:
			if(false== LPUART1_IsTransmitInProgress())
			{
				SYS_Delay(20);/*sleep inhibition to avoid cmeerror*/
				eBC66Job.state= RECEIVING_Bc66PhyJobState;
			}
			else if(true== BC66PHY_IsTimeout())
			{
				DBG_Print("BC66PHY Tx Timeout, retry:%d, data:%s\r\n", eBC66Job.noOfRetry, eBC66Job.txString);
				LPUART1_ReStart();/*Important, Tx may failed and unrecoverable, thus need to re-init.*/
				if(0!= eBC66Job.noOfRetry)
				{
					eBC66Job.noOfRetry--;
					eBC66Job.state= PREPARING_Bc66PhyJobState;
					break;
				}
				else
				{
					eBC66Job.status= TX_TIMEOUT_Bc66PhyJobstatus;
					eBC66Job.state= IDLE_Bc66PhyJobState;
				}
			}
			break;

		case RECEIVING_Bc66PhyJobState:
			if(eBC66Job.expectedRxCnt<= BC66PHY_ReplyMsg_Depth())
			{
				for(int i= 0; i< eBC66Job.expectedRxCnt; i++)
				{
					if(RESPONSE_ERROR_Bc66PhyJobstatus!= eBC66Job.status)/*if previous reply is error, we just dequeue all remaining*/
					{
						if(NULL!= eBC66Job.expectedRx[i])/*if no string to be compared we dont check the result*/
						{
							if(0== strstr(BC66PHY_ReplyMsg_Peek(), eBC66Job.expectedRx[i]))
							{
								if(NULL!= strstr(BC66PHY_ReplyMsg_Peek(), "ERROR"))/*could be transmission error*/
								{
									(void *)BC66PHY_ReplyMsg_Dequeue();

									_ERRORCount++;

									//DBG_Print("_ERRORCount: %d.\r\n", _ERRORCount);
									if(4<= _ERRORCount)
									{
										/*this could be transmission error, we need to reboot the modem and call Recovery for Coap and Lwm2m in reset handler.*/
										_ERRORCount= 0;
										BC66PHY_Reset();
										eBC66Job.status= RESPONSE_ERROR_Bc66PhyJobstatus;
										//DBG_Print("Reset modem\r\n");

										if(false== bInhibitDiag)
										{
											DIAG_Code(MODEM_ERROR_NbiotDCode, BC66PHY_DiagCode(ERROR_RECEIVED_Bc66PhyError));
										}
									}
									else
									{
										goto bc66phy_job_resend;/*resend*/
									}
								}
								else
								{
									eBC66Job.status= RESPONSE_ERROR_Bc66PhyJobstatus;

									if(false== bInhibitDiag)
									{
										DIAG_Code(MODEM_ERROR_NbiotDCode, BC66PHY_DiagCode(RESP_ERR_Bc66PhyError));
									}
								}
							}
						}
						else if(NULL!= eBC66Job.rxBuffer)/*if rxbuffer provided we copy the result*/
						{
							char *_rxString=  BC66PHY_ReplyMsg_Peek();
							memcpy(eBC66Job.rxBuffer, _rxString, strlen(_rxString));
						}
					}
					(void *)BC66PHY_ReplyMsg_Dequeue();
				}

				if(RESPONSE_ERROR_Bc66PhyJobstatus!= eBC66Job.status)
				{
					//_ERRORCount= 0;/*this is blocked because network command may be forever failed but if non network command passed, it will reset. so no point*/
					_timeoutCount= 0;
					eBC66Job.status= SUCCESS_Bc66PhyJobstatus;
					//DBG_Print("BC66PHY_STATUS_SUCCESS.\r\n");
				}
				eBC66Job.state= IDLE_Bc66PhyJobState;
			}
			else if((true== BC66PHY_IsTimeout())
					||((0!= BC66PHY_URCMsg_Depth())&& (NULL!= strstr(BC66PHY_URCMsg_Peek(), "+CMEERROR"))))/* CME ERROR might indicate unrecognizeable command, which could be noise*/
			{
bc66phy_job_resend:
				if(0!= eBC66Job.noOfRetry)
				{
					eBC66Job.noOfRetry--;
					eBC66Job.state= PREPARING_Bc66PhyJobState;
					//DBG_Print("noOfRetry: %d.\r\n", eBC66Job.noOfRetry);
					break;
				}
				else
				{
					if(NULL!= eBC66Job.txString)/*for pause job, we set tx=null, not count as error*/
					{
						_timeoutCount++;
					}
					if(3<= _timeoutCount)
					{
						/*this could be voltage dip, or transmission during PSM(erroneous).*/
						_timeoutCount= 0;
						if(false== bInhibitErrorReset)
						{
							BC66PHY_Reset();
						}
					}
					eBC66Job.status= RESPONSE_TIMEOUT_Bc66PhyJobstatus;
					eBC66Job.state= IDLE_Bc66PhyJobState;
					//DBG_Print("BC66PHY_STATUS_RESPONSE_TIMEOUT.\r\n");

					if((false== bInhibitDiag)&& (NULL!= eBC66Job.txString))
					{
						DIAG_Code(MODEM_ERROR_NbiotDCode, BC66PHY_DiagCode(RESP_TIMEOUT_Bc66PhyError));
					}
				}
			}
			break;

		default:
			SYS_FailureHandler();
			break;
	}
}

void BC66PHY_InhibitDiag(void)
{
	bInhibitDiag= true;
}

void BC66PHY_InhibitErrorReset(bool _true)
{
	bInhibitErrorReset= _true;
}

void BC66PHY_TxRxJob(__IO char *_txString, uint8_t _expectedRxCnt,  const char **_expectedRx, uint16_t _timeout, uint8_t _noOfRetry)
{
	eBC66Job.state= PREPARING_Bc66PhyJobState;
	eBC66Job.txString=  (char *)_txString;
	eBC66Job.expectedRxCnt= _expectedRxCnt;
	eBC66Job.expectedRx= _expectedRx;
	eBC66Job.timeout= _timeout;
	eBC66Job.noOfRetry= _noOfRetry;
	eBC66Job.rxBuffer= NULL;

	bInhibitDiag= false;
}

void BC66PHY_TxRxJob2(__IO char *_txString, uint8_t _expectedRxCnt,  const char **_expectedRx, uint16_t _timeout, uint8_t _noOfRetry, char* _rxBuffer)
{
	BC66PHY_TxRxJob(_txString, _expectedRxCnt, _expectedRx, _timeout, _noOfRetry);
	eBC66Job.rxBuffer= _rxBuffer;
}

void BC66PHY_PauseJob_s(uint16_t _period)
{
	BC66PHY_TxRxJob(NULL, 5, rx_OK, _period, 0);
	bInhibitDiag= true;
}

void BC66PHY_WakeupUsingAT(uint16_t _waitPeriod)
{
	BC66PHY_TxRxJob("AT\r\n", 1, rx_OK, _waitPeriod, 2);
	bInhibitDiag= true;
}

void BC66PHY_JobTransition(void)
{
	if(PREPARING_Bc66PhyJobState!= eBC66Job.state)
	{
		BC66PHY_TxRxJob(NULL, 0, NULL, 0, 0);/*to prevent task from sleep when we are moving to next state/substate.*/
	}
}

void BC66PHY_ResetJob(void)
{
	eBC66Job.state= IDLE_Bc66PhyJobState;
	eBC66Job.status= RESPONSE_ERROR_Bc66PhyJobstatus;/*BC66PHY_STATUS_SUCCESS;*/
}

bool BC66PHY_IsIdle(void)
{
	return (IDLE_Bc66PhyJobState== eBC66Job.state);
}

bool BC66PHY_IsHardReset(void)
{
	return bHardReset;
}

void BC66PHY_ClearHardResetFlag(void)
{
	bHardReset= false;
}

BC66PHY_Job_Status BC66PHY_GetJobStatus(void)
{
	return eBC66Job.status;
}

uint8_t BC66PHY_TaskState(void)
{
	if(
		(PREPARING_Bc66PhyJobState== eBC66Job.state)
		|| (true== BC66PHY_IsTxInProgress())
		|| (0!= BC66PHY_ReplyMsg_Depth())
		|| (0!= BC66PHY_URCMsg_Depth())
	  )
	{
		return RUN_TaskState;
	}

	return SLEEP_TaskState;
}



