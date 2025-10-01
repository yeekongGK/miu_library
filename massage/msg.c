/*
 * msg.c
 *
 *  Created on: 24 Jan 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

//#include "common.h"
#include "msg.h"
#include "logger.h"

__IO static MSG_t		eMsgQueue[MSG_CFG_MAX_MSG_ARRAY];
__IO static uint16_t 	ubMsgIndex= 0;
__IO static uint8_t 	ubMsgArrayHead= 0;
__IO static uint8_t 	ubMsgArrayTail= 0;

__IO MSG_t		eCurrMsg;

uint8_t MSG_Msg_Depth(void)
{
	if(ubMsgArrayTail< ubMsgArrayHead)
	{
		return ubMsgArrayHead- ubMsgArrayTail;
	}
	else if(ubMsgArrayTail> ubMsgArrayHead)
	{
		return (MSG_CFG_MAX_MSG_ARRAY+ ubMsgArrayHead)- ubMsgArrayTail;
	}
	else
	{
		return 0;
	}
}

ErrorStatus MSG_Msg_Enqueue(MSG_t _msg)
{
	if(MSG_CFG_MAX_MSG_ARRAY> MSG_Msg_Depth())
	{
		eMsgQueue[ubMsgArrayHead]= _msg;
		ubMsgArrayHead= (ubMsgArrayHead+ 1)% MSG_CFG_MAX_MSG_ARRAY;
		//DBG_Print("MSG> ubMsgArrayHead: %d ubMsgArrayTail: %d \r\n", ubMsgArrayHead, ubMsgArrayTail);
		return SUCCESS;
	}
	return ERROR;
}

MSG_t MSG_Msg_Dequeue(void)
{
	MSG_t _msg= {0};
	if(ubMsgArrayTail!= ubMsgArrayHead)
	{
		_msg= eMsgQueue[ubMsgArrayTail];
		ubMsgArrayTail= (ubMsgArrayTail+ 1)% MSG_CFG_MAX_MSG_ARRAY;
	}
	return _msg;
}

MSG_t MSG_Msg_Peek(void)
{
	MSG_t _msg= {0};
	if(ubMsgArrayTail!= ubMsgArrayHead)
	{
		_msg= eMsgQueue[ubMsgArrayTail];
	}

	return _msg;
}

SYS_TaskId_t MSG_Msg_TaskId_Peek(void)
{
	return eMsgQueue[ubMsgArrayTail].taskId;
}

void MSG_SyncTask(MSG_t *_msg)
{

	uint8_t *_pBuf= _msg->buffer;
	uint8_t *_pBufMax= _msg->buffer+ _msg->bufferLen;
	uint8_t _pResp[MSG_CFG_MAX_BUFFER_SIZE+ 50];/*add a bit buffer to avoid mem overflow*/
	uint16_t _respLen= 0;

	while(_pBuf< _pBufMax)
	{
		TLV_t _tlv= {
				.Tg= _pBuf[0],
				.t=  _pBuf[1],
				.l=  _pBuf[2],
				.v= &_pBuf[3],
				.rl= 0,       /*this would be modified by the called xxx_TLVRequest()*/
				.rv= &_pResp[_respLen+ 3]
		};

		_pBuf+= (3+ _tlv.l);

		switch(_tlv.Tg)
		{
			case SYS_TLVTag:
				SYS_TLVRequest(&_tlv);
				break;
			case LOG_TLVTag:
				LOGGER_TLVRequest(&_tlv);
				break;
			case NBIOT_TLVTag:
				NBIOT_TLVRequest(&_tlv);
				break;
			case PULSECNTR_TLVTag:
				PULSER_TLVRequest(&_tlv);
				break;
			case SENSOR_TLVTag:
				SENSOR_TLVRequest(&_tlv);
				break;
				/* blocked to save code size
			case ALARM_TLVTag:
				ALARM_TLVRequest(&_tlv);
				break;
				*/
			case DIAGNOSTIC_TLVTag:
				DIAG_TLVRequest(&_tlv);
				break;
			case FAILSAFE_TLVTag:
				FAILSAFE_TLVRequest(&_tlv);
				break;
			case DEVELOPER_TLVTag:
				break;
			case PRODUCTION_TLVTag:
				break;
			default:
				_tlv.rl= 1;
				_tlv.rv[0]= ERROR;
				break;
		}

		if(MSG_CFG_MAX_BUFFER_SIZE>= (_respLen+ 3+ _tlv.rl))/*less than buffer size*/
		{
			_pResp[_respLen++]= _tlv.Tg;
			_pResp[_respLen++]= _tlv.t;
			_pResp[_respLen++]= _tlv.rl;
			_respLen+= _tlv.rl;
		}
		else
		{
			break;/*more than supported buffer size*/
		}
	}

	if(true== _msg->sendResponse)
	{
		memcpy(_msg->buffer, _pResp, _respLen);
		_msg->bufferLen= _respLen;
		_msg->taskId= _msg->responseTaskId;
	}
}

void MSG_Init(void)
{

}

void MSG_Task(void)
{
	if(false== ((0!= MSG_Msg_Depth())&& (MSG_TaskId== MSG_Msg_TaskId_Peek())))
	{
		return;
	}

	eCurrMsg= MSG_Msg_Dequeue();
	MSG_SyncTask(&eCurrMsg);
	MSG_Msg_Enqueue(eCurrMsg);
}

uint8_t MSG_TaskState(void)
{
	if(0!= MSG_Msg_Depth())
	{
		return true;
	}
	return false;
}
