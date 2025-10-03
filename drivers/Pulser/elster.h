/******************************************************************************
 * File:        elster.h
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file defines the public interface for the Elster-type pulser driver.
 *   It includes the configuration structure `ELSTER_t` and function
 *   prototypes for initializing the driver, controlling the counting process,
 *   and retrieving pulse data and flow direction.
 *
 * Notes:
 *   - -
 *
 * To Do:
 *   - -
 *
 ******************************************************************************/

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
