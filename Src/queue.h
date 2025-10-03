/******************************************************************************
 * File:        queue.h
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file defines the public interface for a generic FIFO (First-In,
 *   First-Out) queue. It includes the `QUEUE_FIFO_t` structure, which holds
 *   the queue's state, and declares functions for initializing, pushing,
 *   popping, and un-popping elements.
 *
 * Notes:
 *   - -
 *
 * To Do:
 *   - -
 *
 ******************************************************************************/

#ifndef QUEUE_H_
#define QUEUE_H_

#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef struct
{
	uint16_t head;
	uint16_t elementCount;
	uint16_t elementPopped;
	uint16_t elementMax;
	uint16_t elementSize;
	uint8_t *storage;
}QUEUE_FIFO_t;

void QUEUE_FIFO_Init(QUEUE_FIFO_t *_queue, uint8_t *_storage, uint16_t _elementSize, uint16_t _elementMax);
void QUEUE_FIFO_Push(QUEUE_FIFO_t *_queue, void *_element);
void* QUEUE_FIFO_Pop(QUEUE_FIFO_t *_queue);
uint16_t QUEUE_FIFO_UnPop(QUEUE_FIFO_t *_queue, uint16_t _backcount);

#endif /* QUEUE_H_ */
