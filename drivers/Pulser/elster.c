/******************************************************************************
 * File:        elster.c
 * Author:      Firmware Team
 * Created:     03-10-2025
 * Last Update: -
 *
 * Description:
 *   This file implements the driver for an Elster-type pulser sensor. It uses
 *   the LPTIM1 peripheral to count pulses and an external GPIO to determine
 *   the flow direction (forward or backward). The driver provides functions to
 *   initialize the sensor, start/stop counting, and get the net pulse value.
 *
 * Notes:
 *   - Relies on LPTIM1 for pulse counting and an EXTI line for direction sensing.
 *   - Handles counter overflow via the auto-reload interrupt.
 *
 * To Do:
 *   - The `ELSTER_Power` and `ELSTER_StopCounting` functions are placeholders
 *     and may require implementation.
 *
 ******************************************************************************/

#include "common.h"
#include "elster.h"
#include "lptim1.h"

extern void (*fpAutoReloadMatchCallback)(void);

static ELSTER_t *pConfig;
static int32_t lCntrErrorReading= 0;
__IO static int32_t lCntrMultiplier= 0;
__IO static int32_t lBackwardFlowMarker= 0;
__IO static int32_t lBackwardPulse= 0;
__IO static bool bIsBackward= false;

static void ELSTER_Power(uint8_t _enable);
void ELSTER_AutoReloadMatchCallback(void);

void ELSTER_Init(ELSTER_t *_config)
{
	pConfig= _config;

	/*DIRECTION pin config*/
	LL_EXTI_InitTypeDef EXTI_InitStruct;
	SYS_EnablePortClock(LPTIM1_IN2_GPIO_Port);

	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
	LL_SYSCFG_SetEXTISource(LPTIM1_IN2_SYCFG_EXTI_Port, LPTIM1_IN2_SYCFG_EXTI_Line);

	LL_GPIO_SetPinPull(LPTIM1_IN2_GPIO_Port, LPTIM1_IN2_Pin, LL_GPIO_PULL_NO);
	LL_GPIO_SetPinMode(LPTIM1_IN2_GPIO_Port, LPTIM1_IN2_Pin, LL_GPIO_MODE_INPUT);
	//LL_GPIO_SetPinSpeed(LPTIM1_IN2_GPIO_Port, LPTIM1_IN2_Pin, LL_GPIO_SPEED_FREQ_VERY_HIGH);

	EXTI_InitStruct.Line_0_31= LPTIM1_IN2_EXTI_Line;
	EXTI_InitStruct.LineCommand= ENABLE;
	EXTI_InitStruct.Mode= LL_EXTI_MODE_IT;
	EXTI_InitStruct.Trigger= LL_EXTI_TRIGGER_RISING_FALLING;
	LL_EXTI_Init(&EXTI_InitStruct);

	NVIC_SetPriority(LPTIM1_IN2_EXTI_IRQn, SYS_CFG_EXTI_PRIORITY);
	NVIC_EnableIRQ(LPTIM1_IN2_EXTI_IRQn);


    /*PULSER pin config*/
	LL_GPIO_InitTypeDef GPIO_InitStruct;
	SYS_EnablePortClock(LPTIM1_IN1_GPIO_Port);
    GPIO_InitStruct.Pin = LPTIM1_IN1_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
    LL_GPIO_Init(LPTIM1_IN1_GPIO_Port, &GPIO_InitStruct);
    NVIC_SetPriority(LPTIM1_IRQn, SYS_CFG_PULSE_CNTR_PRIORITY); /* LPTIM1 interrupt Init */
    NVIC_EnableIRQ(LPTIM1_IRQn);

	LL_RCC_SetLPTIMClockSource(LL_RCC_LPTIM1_CLKSOURCE_LSE);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_LPTIM1);
    LL_LPTIM_DeInit(LPTIM1);/*force lptim reset. p/s register wont reset when we do soft reboot without MCU reset*/
    LL_LPTIM_SetClockSource(LPTIM1, LL_LPTIM_CLK_SOURCE_INTERNAL);
    LL_LPTIM_SetPrescaler(LPTIM1, LL_LPTIM_PRESCALER_DIV1);
    LL_LPTIM_SetPolarity(LPTIM1, LL_LPTIM_OUTPUT_POLARITY_REGULAR);
    LL_LPTIM_SetUpdateMode(LPTIM1, LL_LPTIM_UPDATE_MODE_IMMEDIATE);
    LL_LPTIM_SetCounterMode(LPTIM1, LL_LPTIM_COUNTER_MODE_EXTERNAL);

    /*TODO: check if need to apply glitch filter*/
    LL_LPTIM_ConfigClock(LPTIM1, LL_LPTIM_CLK_FILTER_8, LL_LPTIM_CLK_POLARITY_RISING);

    LL_LPTIM_TrigSw(LPTIM1);
}

static void ELSTER_Power(uint8_t _enable)
{

}

static int32_t ELSTER_GetCounter(void)
{
	int32_t _value;
    do/* 2 consecutive readings need to be the same*/
    {
    	_value= LL_LPTIM_GetCounter(LPTIM1);
    }while(LL_LPTIM_GetCounter(LPTIM1)!= _value);

    _value+= (lCntrMultiplier* (ELSTER_CFG_AUTORELOAD_VALUE+ 1));

    return _value;
}

static bool ELSTER_IsBackward(void)
{
	return (false== LL_GPIO_IsInputPinSet(LPTIM1_IN2_GPIO_Port, LPTIM1_IN2_Pin))? true: false;
}

void ELSTER_StartCounting(void)
{
	ELSTER_Power(SET);

	LPTIM1_SetAutoReloadMatchCallback(ELSTER_AutoReloadMatchCallback);

	LL_LPTIM_EnableIT_ARRM(LPTIM1);
    LL_LPTIM_Enable(LPTIM1);
	LL_LPTIM_SetAutoReload(LPTIM1, ELSTER_CFG_AUTORELOAD_VALUE);
    LL_LPTIM_StartCounter(LPTIM1, LL_LPTIM_OPERATING_MODE_CONTINUOUS);

    /*this is needed during power up as we always get extra pulse a bit while after start counting*/
	UTILI_usDelay(250);/*reboot needs shorter delay somehow*/

    do/* 2 consecutive readings need to be the same*/
    {
    	lCntrErrorReading= LL_LPTIM_GetCounter(LPTIM1);
    }while(LL_LPTIM_GetCounter(LPTIM1)!= lCntrErrorReading);

    /*determine if we are in backflow*/

	if(true== ELSTER_IsBackward())
	{
		lBackwardFlowMarker= ELSTER_GetCounter();
		bIsBackward= true;
	}
}

void ELSTER_AutoReloadMatchCallback(void)
{
	lCntrMultiplier++;
}

void ELSTER_CH1DPinCallback(void)
{
	int32_t _value= ELSTER_GetCounter();

	if(false== ELSTER_IsBackward())
	{
		lBackwardPulse+= (_value- lBackwardFlowMarker);
		bIsBackward= false;
	}
	else
	{
		lBackwardFlowMarker= _value;
		bIsBackward= true;
	}
	//DBG_Print("-------------- bIsBackward(%d). lBackwardPulse(%d). _value(%d). lBackwardFlowMarker(%d)\r\n", bIsBackward, lBackwardPulse, _value, lBackwardFlowMarker);
}

void ELSTER_StopCounting(void)
{
	ELSTER_Power(RESET);
}

uint32_t ELSTER_GetValue(void)
{
	int32_t _value= ELSTER_GetCounter();
	uint32_t _templCntrBackFlow;

    if(true== bIsBackward)/*currently meter is running backward*/
    {
    	_templCntrBackFlow= lBackwardPulse+ (_value- lBackwardFlowMarker);
	}
    else
    {
    	_templCntrBackFlow= lBackwardPulse;
    }
    _value-= (_templCntrBackFlow* 2);//each pulse during backward flow
    _value-= lCntrErrorReading;
    _value+= (pConfig->rteOffsetValue);

	return (uint32_t)_value;
}

void ELSTER_SetValue(int32_t _value)
{
	_value-= ELSTER_GetValue();
	pConfig->rteOffsetValue+= _value;
}

uint8_t ELSTER_GetDirection(void)
{
	return (true== bIsBackward)? BACKWARD_CounterDirection: FORWARD_CounterDirection;
}
