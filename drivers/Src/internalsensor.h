/*
 * tempsensor.h
 *
 *  Created on: 18 Jan 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef SENSOR_INTERNALSENSOR_H_
#define SENSOR_INTERNALSENSOR_H_

#include "main.h"

int32_t INTSENSOR_Temperature_Get(void);
int32_t INTSENSOR_Temperature_GetMin(void);
int32_t INTSENSOR_Temperature_GetMax(void);
uint32_t INTSENSOR_Voltage_Get(void);
uint32_t INTSENSOR_Voltage_GetMin(void);
uint32_t INTSENSOR_Voltage_GetMax(void);
bool INTSENSOR_Temperature_GetFlag(void);
void INTSENSOR_Temperature_SetFlag(bool _flag);
bool INTSENSOR_Voltage_GetFlag(void);
void INTSENSOR_Voltage_SetFlag(bool _flag);
void INTSENSOR_Init(void);
void INTSENSOR_Task(void);
uint8_t INTSENSOR_TaskState(void);

#endif /* SENSOR_INTERNALSENSOR_H_ */
