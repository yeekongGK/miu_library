/*
 * digitalsensor.h
 *
 *  Created on: 16 Jan 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

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
