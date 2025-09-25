/*
 * positionsensor.h
 *
 *  Created on: 16 Jan 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef SENSOR_ACCELEROMETER_H_
#define SENSOR_ACCELEROMETER_H_

#include "main.h"

typedef enum
{
	POSSENSOR_MODE_CUSTOM= 0,
	POSSENSOR_MODE_6D_EXTENDED= 1,
}POSSENSOR_Mode_Typedef;

void POSSENSOR_XYZ_Get(uint16_t *_x, uint16_t *_y, uint16_t *_z);
uint16_t POSSENSOR_X_Get(void);
uint16_t POSSENSOR_Y_Get(void);
uint16_t POSSENSOR_Z_Get(void);
uint32_t POSSENSOR_Tilt_GetCount(void);
uint32_t POSSENSOR_Tilt_GetTotalCount(void);
void POSSENSOR_Tilt_ClearCount(void);
void POSSENSOR_Tilt_ClearTotal(void);
bool POSSENSOR_Tilt_GetFlag(void);
void POSSENSOR_Tilt_SetFlag(bool _flag);
void POSSENSOR_Init(void);
void POSSENSOR_Task(void);
uint8_t POSSENSOR_TaskState(void);

#endif /* SENSOR_ACCELEROMETER_H_ */
