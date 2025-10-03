/******************************************************************************
 * File:        pulser.h
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file defines the public interface for the generic pulser driver, which
 *   acts as an abstraction layer for various pulser sensor types (e.g.,
 *   TRACSENS, LCSENS, ELSTER). It includes the main configuration structure
 *   `PULSER_t`, enumerations for different modes and directions, and function
 *   prototypes for initializing the driver, getting/setting pulse values, and
 *   converting between pulses and physical units.
 *
 * Notes:
 *   - The active pulser driver is selected at runtime based on the
 *     `PULSER_Mode_t` setting.
 *
 * To Do:
 *   - -
 *
 ******************************************************************************/

#ifndef PULSECNTR_PULSECNTR_H_
#define PULSECNTR_PULSECNTR_H_

#include "main.h"
#include "tracsens.h"
#include "lcsens.h"
#include "elster.h"

#define PULSER_CFG_VERSION 0x01

typedef enum
{
  RESERVE0_Mode= 0,
  RESERVE1_Mode= 1,
  RESERVE2_Mode= 2,
  TRACSENS_Mode= 3,
  TRACSENSi_Mode= 4,/*TRACSENS but inverted*/
  LCSENS_Mode= 5,
  ELSTER_Mode= 6,
  PULSECNTR_UNSUPPORTED_Mode,
} PULSER_Mode_t;

typedef enum
{
	UNKNOWN_CounterDirection,
	FORWARD_CounterDirection,
	BACKWARD_CounterDirection,
}PULSER_CounterDirection_t;

typedef enum
{
	NORMAL_CounterMode,
	INVERT_CounterMode,
}PULSER_CounterMode_t;

typedef enum
{
	NONE_PulserTLVTag= 0,
	GET_PARAMS_PulserTLVTag,
	SET_PARAMS_PulserTLVTag,
	GET_ERRPATT_PARAMS_PulserTLVTag,
	SET_ERRPATT_PARAMS_PulserTLVTag,
	GET_ERRPATT_COUNT_PulserTLVTag,
	SET_ERRPATT_COUNT_PulserTLVTag,
	SET_ERRPATT_USE_COMPENSATED_VALUE_PulserTLVTag,
	GET_PULSE_PulserTLVTag,
	SET_PULSE_PulserTLVTag,
}PULSER_PulserTLVTag_t;

/*TO EXPORT OUT TO CFG*/
typedef struct __attribute__((aligned(8)))/*compulsory alignment*/
{
	uint32_t checksum;/*compulsory checksum*/
	uint8_t	 version;

	PULSER_Mode_t mode;
	float weight_liter;
	float rtePrevValue_liter;/*used as var to calculate hourly consumptions by lwm2m task*/
	float rtePrevValue2_liter;/*used as var to calculate monthly consumptions by lwm2m task*/

	TRACSENS_t tracsens;
	LCSENS_t lcsens;
	ELSTER_t elster;

	uint32_t rtePrevErrorPatternCount;/*used as var to meter faulty alarm by sensor task for lwm2m*/
	uint8_t reserve[116];/*to add more param reduce this, thus no need to do backward compatible thing. remember to set the default value after config*/
}PULSER_t;

PULSER_Mode_t PULSER_Mode_Get(void);
uint32_t PULSER_Value_Get(void);
void PULSER_Value_Set(int32_t _value);
float PULSER_ValueInLiter_Get(void);
void PULSER_ValueInLiter_Set(float _value);
float PULSER_PrevValueInLiter_Get(void);
void PULSER_PrevValueInLiter_Set(float _value);
float PULSER_PrevValue2InLiter_Get(void);
void PULSER_PrevValue2InLiter_Set(float _value);
PULSER_CounterDirection_t PULSER_Direction(void);
float PULSER_ConvertToMeterReading(uint32_t _pulseValue);
int32_t PULSER_ConvertToPulseValue(float _meterReading);
void PULSER_TLVRequest(TLV_t *_tlv);
void PULSER_Save(void);
void PULSER_Restore(void);
void PULSER_Init(PULSER_t *_config);
void PULSER_Task(void);
uint8_t PULSER_TaskState(void);

#endif /* PULSECNTR_PULSECNTR_H_ */
