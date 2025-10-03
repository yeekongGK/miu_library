/******************************************************************************
 * File:        digitalsensor.h
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file defines the public interface for the digital sensor driver. It
 *   provides function prototypes for initializing various digital input
 *   sensors, such as magnetic and tamper sensors, and for retrieving their
 *   status, event counts, and flags. The interface allows higher-level modules
 *   to interact with these sensors and respond to their events.
 *
 * Notes:
 *   - -
 *
 * To Do:
 *   - -
 *
 ******************************************************************************/

#ifndef SENSOR_DIGITALSENSOR_H_
#define SENSOR_DIGITALSENSOR_H_

#include "main.h"

void DIGISENSOR_PBMag_Init(void);
bool DIGISENSOR_PBMag_IsActive(void);
void DIGISENSOR_PBMag_WaitActive(void);
uint32_t DIGISENSOR_PBMag_GetCount(void);
uint32_t DIGISENSOR_PBMag_GetTotalCount(void);
void DIGISENSOR_PBMag_ClearCount(void);
void DIGISENSOR_PBMag_ClearTotal(void);
bool DIGISENSOR_PBMag_GetFlag(void);
void DIGISENSOR_PBMag_SetFlag(bool _flag);
void DIGISENSOR_TamperIn_Init(void);
uint32_t DIGISENSOR_TamperIN_GetCount(void);
uint32_t DIGISENSOR_TamperIN_GetTotalCount(void);
void DIGISENSOR_TamperIN_ClearCount(void);
void DIGISENSOR_TamperIN_ClearTotal(void);
bool DIGISENSOR_TamperIN_GetFlag(void);
void DIGISENSOR_TamperIN_SetFlag(bool _flag);
void DIGISENSOR_RSTamper_Init(void);
uint32_t DIGISENSOR_RSTamper_GetCount(void);
uint32_t DIGISENSOR_RSTamper_GetTotalCount(void);
void DIGISENSOR_RSTamper_ClearCount(void);
void  DIGISENSOR_RSTamper_ClearTotal(void);
bool DIGISENSOR_RSTamper_GetFlag(void);
void DIGISENSOR_RSTamper_SetFlag(bool _flag);
void DIGISENSOR_Init(void);
void DIGISENSOR_Task(void);
uint8_t DIGISENSOR_TaskState(void);

#endif /* SENSOR_DIGITALSENSOR_H_ */
