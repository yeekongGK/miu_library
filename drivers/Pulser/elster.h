/*
 * elster.h
 *
 *  Created on: 3 Aug 2022
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef PULSER_ELSTER_H_
#define PULSER_ELSTER_H_

#include "main.h"

#define ELSTER_CFG_AUTORELOAD_VALUE 0xFFFF

typedef struct
{
	int32_t rteOffsetValue;
	int32_t rteLastSavedValue;
}ELSTER_t;

void ELSTER_Init(ELSTER_t *_config);
void ELSTER_StartCounting(void);
void ELSTER_StopCounting(void);
uint32_t ELSTER_GetValue(void);
void ELSTER_SetValue(int32_t _value);
uint8_t ELSTER_GetDirection(void);

#endif /* PULSER_ELSTER_H_ */
