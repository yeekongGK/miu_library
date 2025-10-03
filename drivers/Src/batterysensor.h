/******************************************************************************
 * File:        batterysensor.h
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file defines the public interface for the battery sensor driver. It
 *   provides function prototypes for initializing the sensor, reading various
 *   battery metrics (e.g., capacity, voltage, current, temperature), and
 *   controlling its operational modes like hibernate.
 *
 * Notes:
 *   - The driver relies on configuration values defined in the main `config`
 *     structure.
 *
 * To Do:
 *   - -
 *
 ******************************************************************************/

#ifndef SENSOR_BATTERYSENSOR_H_
#define SENSOR_BATTERYSENSOR_H_

#include "main.h"

#define BATTSENSOR_CFG_DESIGN_CAPACITY 	(config.sensors.battery.designCapacity_Ah)//19.0/*19AH*/
#define BATTSENSOR_CFG_RSENSE_VALUE 	(config.sensors.battery.RSense)//0.1/*0.1Ohm*/

typedef enum
{
	LOW_BattSensorGain,
	HIGH_BattSensorGain,
}BATTSENSOR_BattSensorGain_t;

bool BATTSENSOR_IsPOR(void);
void BATTSENSOR_ClearPOR(void);
bool BATTSENSOR_IsDataNotReady(void);
uint16_t BATTSENSOR_GetQH(void);
uint32_t BATTSENSOR_GetTotalQH(void);
uint16_t BATTSENSOR_GetRepCap(void);
uint16_t BATTSENSOR_GetRepSOC(void);
float BATTSENSOR_GetVCell(void);
float BATTSENSOR_GetAvgVCell(void);
float BATTSENSOR_GetCurrent(void);
float BATTSENSOR_GetAvgCurrent(void);
float BATTSENSOR_GetMinCurrent(void);
float BATTSENSOR_GetMaxCurrent(void);
float BATTSENSOR_GetCurrentUsed(void);
float BATTSENSOR_GetCapacity(void);
float BATTSENSOR_GetCapacityPercentage(void);
float BATTSENSOR_GetTemperature(void);
float BATTSENSOR_GetAvgTemperature(void);
float BATTSENSOR_GetTimer(void);
void BATTSENSOR_Hibernate(bool _enable, BATTSENSOR_BattSensorGain_t _gain);
void BATTSENSOR_LogRegisters(void);
void BATTSENSOR_Init(SENSOR_t *_config);
void BATTSENSOR_Task(void);
uint8_t BATTSENSOR_TaskState(void);

#endif /* SENSOR_BATTERYSENSOR_H_ */
