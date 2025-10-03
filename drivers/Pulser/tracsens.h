/******************************************************************************
 * File:        tracsens.h
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file defines the public interface for the TRACSENS pulser driver. It
 *   includes the configuration structure `TRACSENS_t`, enumerations for error
 *   states, and function prototypes for initializing the driver, controlling
 *   the counting process, and retrieving pulse data, direction, and error
 *   status.
 *
 * Notes:
 *   - -
 *
 * To Do:
 *   - -
 *
 ******************************************************************************/

#ifndef PULSECNTR_TRACSENS_H_
#define PULSECNTR_TRACSENS_H_

#include "main.h"

#define TRACSENS_CFG_AUTORELOAD_VALUE 0xFFFF

typedef enum
{
	NONE_CounterErrorState= 0,
	FWD_EXPECTING_BWD_CounterErrorState,
	BWD_EXPECTING_FWD_CounterErrorState,
	FWD_EXPECTING_BWD_END_CounterErrorState,
	BWD_EXPECTING_FWD_END_CounterErrorState,
}TRACSENS_CounterErrorState_t;

typedef struct
{
	bool enableErrorPatternCheck;
	bool useCompensatedValue;
	uint16_t errorPatternConfirmationCount;

	int32_t rteOffsetValue;
	int32_t rteLastSavedValue;
	uint32_t rteErrorPatternCount;
	bool rteErrorPatternCompensationStarted;
	TRACSENS_CounterErrorState_t rteErrorPatternState;
	int32_t rteErrorPatternPreviousPulse;
	bool rteErrorPatternJustStarted;
}TRACSENS_t;

void TRACSENS_Init(TRACSENS_t *_config);
void TRACSENS_StartCounting(void);
void TRACSENS_AutoReloadMatchCallback(void);
void TRACSENS_CounterChangedToUpCallback(void);
void TRACSENS_CounterChangedToDownCallback(void);
void TRACSENS_CompareCallback(void);
void TRACSENS_StopCounting(void);
uint32_t TRACSENS_GetValue(void);
void TRACSENS_SetValue(int32_t _value);
uint8_t TRACSENS_GetDirection(void);
bool TRACSENS_ErrorDetected(void);
void TRACSENS_ClearError(void);

#endif /* PULSECNTR_TRACSENS_H_ */
