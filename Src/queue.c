/*
 * queue.c
 *
 *  Created on: 30 Aug 2021
 *      Author: muhammad.ahmad@georgekent.net
 */
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
