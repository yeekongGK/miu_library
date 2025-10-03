/******************************************************************************
 * File:        lcsens.h
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file defines the public interface for the LC (Inductor-Capacitor)
 *   proximity sensor driver. It includes hardware-specific macros for direct
 *   peripheral control, data structures (`LCSENS_t`, `LCSENS_Sensor_t`) for
 *   sensor configuration and state management, and function prototypes for
 *   initialization, calibration, and data retrieval.
 *
 * Notes:
 *   - The file contains many low-level hardware control macros that are
 *     performance-sensitive.
 *
 * To Do:
 *   - The macros could be replaced with inline functions for better type
 *     safety and readability.
 *
 ******************************************************************************/

#ifndef PULSER_LCSENS_H_
#define PULSER_LCSENS_H_

#include "main.h"

#define LCSENS_CFG_SENSOR_COUNT 				3
#define LCSENS_CFG_MAX_DAC_STEP					10
#define LCSENS_CFG_LPTIM1_AUTORELOAD_VALUE		0xFFFF
#define LCSENS_CFG_LPTIM2_AUTORELOAD_VALUE		0xFFFF

#define LC1_PIN_Pin			COMP2_INP_CH1_Pin
#define LC1_PIN_GPIO_Port	COMP2_INP_CH1_GPIO_Port
#define LC2_PIN_Pin			COMP2_INP_CH2_Pin
#define LC2_PIN_GPIO_Port	COMP2_INP_CH2_GPIO_Port
#define LC3_PIN_Pin			PWR_EN_Pin
#define LC3_PIN_GPIO_Port	PWR_EN_GPIO_Port
#define CN1_PIN_Pin			LPTIM1_IN1_Pin
#define CN1_PIN_GPIO_Port	LPTIM1_IN1_GPIO_Port
#define CN2_PIN_Pin			LPTIM1_IN2_Pin
#define CN2_PIN_GPIO_Port	LPTIM1_IN2_GPIO_Port
#define CN3_PIN_Pin			TAMPER_IN_Pin
#define CN3_PIN_GPIO_Port	TAMPER_IN_GPIO_Port
#define OPAMP_PIN_Pin		USART1_RX_Pin
#define OPAMP_PIN_GPIO_Port	USART1_RX_GPIO_Port

#define LCSENS_IO_LC1_Enable()		LL_GPIO_SetOutputPin(CN1_PIN_GPIO_Port, CN1_PIN_Pin)
#define LCSENS_IO_LC2_Enable()		LL_GPIO_SetOutputPin(CN2_PIN_GPIO_Port, CN2_PIN_Pin)
#define LCSENS_IO_LC3_Enable()		LL_GPIO_SetOutputPin(CN3_PIN_GPIO_Port, CN3_PIN_Pin)
#define LCSENS_IO_LC1_Disable()		LL_GPIO_ResetOutputPin(CN1_PIN_GPIO_Port, CN1_PIN_Pin)
#define LCSENS_IO_LC2_Disable()		LL_GPIO_ResetOutputPin(CN2_PIN_GPIO_Port, CN2_PIN_Pin)
#define LCSENS_IO_LC3_Disable()		LL_GPIO_ResetOutputPin(CN3_PIN_GPIO_Port, CN3_PIN_Pin)
#define LCSENS_IO_LC1_Activate()	{LC1_PIN_GPIO_Port->MODER&= 0xFFFFFF3F; LC1_PIN_GPIO_Port->MODER|= 0b01000000;}//LL_GPIO_SetPinMode(LC1_PIN_GPIO_Port, LC1_PIN_Pin, LL_GPIO_MODE_OUTPUT)
#define LCSENS_IO_LC2_Activate()	{LC2_PIN_GPIO_Port->MODER&= 0xFFFFFCFF; LC2_PIN_GPIO_Port->MODER|= 0b0100000000;}//LL_GPIO_SetPinMode(LC2_PIN_GPIO_Port, LC2_PIN_Pin, LL_GPIO_MODE_OUTPUT)
#define LCSENS_IO_LC3_Activate()	{LC3_PIN_GPIO_Port->MODER&= 0xFFFFCFFF; LC3_PIN_GPIO_Port->MODER|= 0b01000000000000;}//LL_GPIO_SetPinMode(LC3_PIN_GPIO_Port, LC3_PIN_Pin, LL_GPIO_MODE_OUTPUT)
#define LCSENS_IO_Wait()			{__DSB();__DSB();__DSB();__DSB();__DSB();__DSB();__DSB();__DSB();\
										__DSB();__DSB();__DSB();__DSB();__DSB();__DSB();__DSB();__DSB();}
#define LCSENS_IO_LC1_Sample()		(LC1_PIN_GPIO_Port->MODER|= 0b11000000)//LL_GPIO_SetPinMode(LC1_PIN_GPIO_Port, LC1_PIN_Pin, LL_GPIO_MODE_ANALOG)
#define LCSENS_IO_LC2_Sample()		(LC2_PIN_GPIO_Port->MODER|= 0b1100000000)//LL_GPIO_SetPinMode(LC2_PIN_GPIO_Port, LC2_PIN_Pin, LL_GPIO_MODE_ANALOG)
#define LCSENS_IO_LC3_Sample()		(LC3_PIN_GPIO_Port->MODER|= 0b11000000000000)//LL_GPIO_SetPinMode(LC3_PIN_GPIO_Port, LC3_PIN_Pin, LL_GPIO_MODE_ANALOG)
#define LCSENS_OPAMP_Enable()		LL_GPIO_SetOutputPin(OPAMP_PIN_GPIO_Port, OPAMP_PIN_Pin)
#define LCSENS_OPAMP_Disable()		LL_GPIO_ResetOutputPin(OPAMP_PIN_GPIO_Port, OPAMP_PIN_Pin)
#define LCSENS_COMP_Enable()		(COMP2->CSR|= 0b1)//LL_COMP_Enable(COMP2)
#define LCSENS_COMP_Disable()		(COMP2->CSR&= 0xFFFFFFFE)//LL_COMP_Disable(COMP2)
#define LCSENS_COMP_SetInput_LC1()	{COMP2->CSR&= 0xFFFFFE7F; COMP2->CSR|= 0b100000000;}//LL_COMP_SetInputPlus(COMP2, LL_COMP_INPUT_PLUS_IO3)
#define LCSENS_COMP_SetInput_LC2()	{COMP2->CSR&= 0xFFFFFE7F; COMP2->CSR|= 0b000000000;}//LL_COMP_SetInputPlus(COMP2, LL_COMP_INPUT_PLUS_IO1)
#define LCSENS_COMP_SetInput_LC3()	{COMP2->CSR&= 0xFFFFFE7F; COMP2->CSR|= 0b010000000;}//LL_COMP_SetInputPlus(COMP2, LL_COMP_INPUT_PLUS_IO2)
#define LCSENS_DAC_Enable()			(DAC1->CR|= 0b1)//LL_DAC_Enable(DAC1, LL_DAC_CHANNEL_1)
#define LCSENS_DAC_Disable()		(DAC1->CR&= 0xFFFFFFFE)//LL_DAC_Disable(DAC1, LL_DAC_CHANNEL_1)
#define LCSENS_DAC_SetValue(x)		(DAC1->DHR12R1= x<< 4)//LL_DAC_ConvertData8RightAligned(DAC1, LL_DAC_CHANNEL_1, x)
#define LCSENS_SaveCount()			(pConfig->tempCount= (LPTIM2->CNT))
#define LCSENS_GetCount()			(((LPTIM2->CNT)>= pConfig->tempCount)? ((LPTIM2->CNT)- pConfig->tempCount): ((LPTIM2->CNT)+ (LCSENS_CFG_LPTIM2_AUTORELOAD_VALUE- pConfig->tempCount)))
#define LCSENS_GetThreshold(x)		((uint8_t)(70*x/100))

#define __LC_DELAY(__TIME__) \
  do {\
    TIM6->ARR= (__TIME__);          /* Reload ARR value*/\
	TIM6->SR&= 0xFFFFFFFE;      	/* Clear 1st dummy IT when starting the counter */\
    TIM6->CR1|= 0b1;       			/* Start the counter (single-shot) */\
	while(!(TIM6->SR& 0b1))\
	{\
	}\
  } while(0)

typedef enum
{
	SINGLE_DAC_SamplingStrategy= 0,
	MULTI_DAC_SamplingStrategy,
	MAX_SamplingStrategy,
}LCSENS_SamplingStrategy_t;

typedef struct
{
	LCSENS_SamplingStrategy_t samplingStrategy;
	uint32_t dacOnDelay;
	uint8_t dacLevel[LCSENS_CFG_MAX_DAC_STEP];
	uint32_t activationTime[LCSENS_CFG_MAX_DAC_STEP];
	uint8_t prevCount;
	uint8_t currCount;
	uint8_t detectPercentage;
	uint8_t detectThreshold;

	/*for calibration*/
	bool doCalib;
	uint8_t calibStep;
	uint8_t dacStep;
}LCSENS_Sensor_t;

typedef struct
{
	int32_t rteOffsetValue;
	int32_t rteLastSavedValue;
	uint16_t samplingTempo_hz;
	uint16_t samplingTempo;
	uint32_t tempCount;/*for calculation*/
	uint8_t  currBitmap;
	uint8_t  prevBitmap;
	uint8_t	 transitionTable[8];
	uint8_t  initialTransitionVal;
	uint8_t  currTransitionVal;
	uint8_t  prevTransitionVal;
	uint32_t fwdTransitionCnt;
	uint32_t bwdTransitionCnt;
	uint32_t fwdRevolutionCnt;
	uint32_t bwdRevolutionCnt;
	uint8_t currDirection;
	uint8_t calib;
	uint8_t dynamicCalibBitmap;
	__IO LCSENS_Sensor_t sensor[LCSENS_CFG_SENSOR_COUNT];
}LCSENS_t;


void LCSENS_ScheduleCalibration(uint8_t _sensor);
void LCSENS_StaticCalibration(void);
uint32_t LCSENS_GetValue(void);
void LCSENS_SetValue(int32_t _value);
uint8_t LCSENS_GetDirection(void);
void LCSENS_StartCounting(void);
void LCSENS_Init(LCSENS_t *_config);
void LCSENS_Task(void);
uint8_t LCSENS_TaskState(void);

#endif /* PULSER_LCCSENS_H_ */
