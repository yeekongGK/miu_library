/******************************************************************************
 * File:        internalsensor.h
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file defines the public interface for the internal sensor driver. It
 *   provides function prototypes for initializing the driver and retrieving
 *   data from the MCU's internal temperature sensor and voltage reference. It
 *   also includes functions for managing status flags related to these sensors.
 *
 * Notes:
 *   - -
 *
 * To Do:
 *   - -
 *
 ******************************************************************************/

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
