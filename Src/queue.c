/******************************************************************************
 * File:        queue.c
 * Author:      Firmware Team
 * Created:     03-10-2025
 * Last Update: -
 *
 * Description:
 *   This file implements a generic FIFO (First-In, First-Out) queue data
 *   structure. It provides functions to initialize a queue, push elements
 *   onto it, and pop elements from it. It also includes an `UnPop`
 *   functionality to revert a pop operation. The queue is implemented as a
 *   circular buffer.
 *
 * Notes:
 *   - The queue operates on a user-provided storage buffer.
 *
 * To Do:
 *   - -
 *
 ******************************************************************************/
#include "queue.h"

void QUEUE_FIFO_Init(QUEUE_FIFO_t *_queue, uint8_t *_storage, uint16_t _elementSize, uint16_t _elementMax)
{
	_queue->storage= _storage;
	_queue->head= 0;
	_queue->elementCount= 0;
	_queue->elementPopped= 0;
	_queue->elementSize= _elementSize;
	_queue->elementMax= _elementMax;
}

void QUEUE_FIFO_Push(QUEUE_FIFO_t *_queue, void *_element)
{
	memcpy(_queue->storage+ (_queue->elementSize* _queue->head), (uint8_t *)_element, _queue->elementSize);
	_queue->head= (_queue->head+ 1)% _queue->elementMax;
	_queue->elementCount= (_queue->elementCount== _queue->elementMax)? _queue->elementMax: _queue->elementCount+ 1;
}

void* QUEUE_FIFO_Pop(QUEUE_FIFO_t *_queue)
{
	void* _element= NULL;

	if(0!= _queue->elementCount)
	{
		int32_t _tail= _queue->head- _queue->elementCount;
		if(0> _tail)
		{
			_tail+= _queue->elementMax;
		}
		_element= _queue->storage+ (_queue->elementSize* _tail);
		_queue->elementCount-- ;
		_queue->elementPopped++;
	}

	return _element;
}

uint16_t QUEUE_FIFO_UnPop(QUEUE_FIFO_t *_queue, uint16_t _backcount)
{
	if(0!= _queue->elementPopped)
	{

		if(_backcount> _queue->elementPopped)
		{
			_backcount= _queue->elementPopped;
		}

		if((_backcount+ _queue->elementCount)> _queue->elementMax)
		{
			_backcount= _queue->elementMax- _queue->elementCount;
		}

		_queue->elementPopped-= _backcount;
		_queue->elementCount+= _backcount;
	}
	else
	{
		_backcount= 0;
	}

	return _backcount;
}
