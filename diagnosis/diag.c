/******************************************************************************
 * File:        diag.c
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file implements the diagnostic logging system. It provides functions
 *   to record diagnostic codes with integer or float values into a FIFO
 *   queue. It also includes functions to pop and un-pop log entries, and a
 *   TLV handler for external communication to retrieve diagnostic data.
 *
 * Notes:
 *   - Diagnostic logs are stored in a queue located in a dedicated RAM
 *     section (.ram2).
 *   - Depends on the Queue and System modules.
 *
 * To Do:
 *   - -
 *
 ******************************************************************************/

//#include "common.h"
#include "main.h"
#include "sys.h"
#include "diag.h"

static DIAG_t *pConfig;
__IO static uint8_t __attribute__((section(".ram2"))) pucBuffer[DIAG_CFG_BUFFER_SIZE]; /*need to remove at flash in linker script, otherwise it will be loaded inside flash and waste memory:*/// >RAM2 /*AT> FLASH*/
__IO static uint8_t *pBuffer;
__IO static uint8_t *pBufferMax;

void DIAG_Code(uint16_t _dcode, uint32_t _value)
{
	uint8_t _buf[DIAG_CFG_ENTRY_SIZE];
	*((uint16_t *)  (_buf))= _dcode;
	*((uint32_t *)  (_buf+ 2))= SYS_GetTimestamp_s();
	*((uint32_t *) 	(_buf+ 2+ 4))= _value;

	QUEUE_FIFO_Push(&pConfig->queue, _buf);
}

void DIAG_Code_f(uint16_t _dcode, float _value)
{
	uint8_t _buf[DIAG_CFG_ENTRY_SIZE];
	*((uint16_t *)  (_buf))= (0x4000| _dcode); //deprecated
	*((uint16_t *)  (_buf))= _dcode;
	*((uint32_t *)  (_buf+ 2))= SYS_GetTimestamp_s();
	*((float *) 	(_buf+ 2+ 4))= _value;

	QUEUE_FIFO_Push(&pConfig->queue, _buf);
}

uint16_t DIAG_Code_Pop(uint8_t *_buffer, uint16_t _count)
{
	uint16_t _popped;
	for(_popped= 0; _popped< _count; _popped++)
	{
		void * _element= QUEUE_FIFO_Pop(&pConfig->queue);
		if(NULL== _element)
		{
			break;
		}
		memcpy(_buffer+ (_popped* DIAG_CFG_ENTRY_SIZE), _element, DIAG_CFG_ENTRY_SIZE);
	}
	//DBG_Print("Diag popped: %d.\r\n", _popped);
	return _popped;
}

uint16_t DIAG_Code_UnPop(uint16_t _backcount)
{
	uint16_t _unPopped= QUEUE_FIFO_UnPop(&pConfig->queue, _backcount);

	//DBG_Print("Diag unPopped: %d.\r\n", _unPopped);
	return _unPopped;
}

void DIAG_TLVRequest(TLV_t *_tlv)
{
	_tlv->rv[0]= SUCCESS;
	_tlv->rl= 1;

	switch(_tlv->t)
	{
		case CODE_DiagTLVTag:
			_tlv->rv[_tlv->rl++]= _tlv->v[0];
			switch(_tlv->v[0])
			{
				case 0:/*pop*/
					{
						_tlv->rl+= (DIAG_CFG_ENTRY_SIZE* DIAG_Code_Pop(_tlv->rv+ _tlv->rl, (249/ DIAG_CFG_ENTRY_SIZE)- 2));
					}
					break;
				case 1:/*unpop*/
					{
						uint16_t _backcount= MAKEWORD(_tlv->v[2], _tlv->v[1]);
						_backcount= DIAG_Code_UnPop(_backcount);
						_tlv->rl+= UTILI_Array_Copy16(_tlv->rv+ _tlv->rl, _backcount);
					}
					break;
				default:
					_tlv->rv[0]= ERROR;
					return;
			}
			break;

		default:
			_tlv->rv[0]= ERROR;
			break;
	}
}

void DIAG_Init(DIAG_t *_config)
{
	pConfig= _config;

	if((pucBuffer!= pConfig->queue.storage)|| (1== LL_RCC_IsActiveFlag_BORRST()))
	{
		QUEUE_FIFO_Init(&(pConfig->queue), pucBuffer, DIAG_CFG_ENTRY_SIZE, DIAG_CFG_MAX_ENTRY);
	}
}

void DIAG_Task(void)
{
}

uint8_t DIAG_TaskState(void)
{
	return SLEEP_TaskState;
}
