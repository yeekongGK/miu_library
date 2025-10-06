/******************************************************************************
 * File:        lcsens.c
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file implements the driver for a 3-coil LC (Inductor-Capacitor)
 *   proximity sensor, used for detecting rotational movement. It configures
 *   and uses various peripherals including LPTIM, COMP, DAC, and GPIOs to
 *   drive the LC circuits and count oscillations to determine the rotor's
 *   position. The driver includes logic for sampling the sensors, processing
 *   the resulting state transitions to determine direction and revolutions,
 *   and a calibration routine to tune sensor thresholds.
 *
 * Notes:
 *   - The driver is highly dependent on specific hardware connections and
 *     peripheral configurations (LPTIM1, LPTIM2, COMP2, DAC1).
 *   - Implements both static and dynamic calibration routines.
 *
 * To Do:
 *   - The calibration logic is complex and could be simplified or better
 *     documented.
 *
 ******************************************************************************/

#include "common.h"
#include "lcsens.h"
#include "lptim1.h"
#include "lptim2.h"

#pragma GCC push_options
#pragma GCC optimize ("O0")

bool LCSENS_Calibration(uint8_t _sensorIndex);
void LCSENS_DoCalibration(void);
void LCSENS_Sample(void);
void LCSENS_Tempo_Task(void);
void LCSENS_TempoSync_Task(void);

__IO static LCSENS_t *pConfig= NULL;

static void LCSENS_IO_Init(void)
{
	LL_GPIO_InitTypeDef GPIO_InitStruct;

	/*these pins are where the oscillating signals are
	 * activated and then sampled by comparator*/
	SYS_EnablePortClock(LC1_PIN_GPIO_Port);
    GPIO_InitStruct.Pin = LC1_PIN_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.OutputType= LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(LC1_PIN_GPIO_Port, &GPIO_InitStruct);
    LL_GPIO_ResetOutputPin(LC1_PIN_GPIO_Port, LC1_PIN_Pin);/*default level*/
	LL_GPIO_SetPinMode(LC1_PIN_GPIO_Port, LC1_PIN_Pin, LL_GPIO_MODE_ANALOG);/*default mode*/

	SYS_EnablePortClock(LC2_PIN_GPIO_Port);
    GPIO_InitStruct.Pin = LC2_PIN_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.OutputType= LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(LC2_PIN_GPIO_Port, &GPIO_InitStruct);
    LL_GPIO_ResetOutputPin(LC2_PIN_GPIO_Port, LC2_PIN_Pin);/*default level*/
	LL_GPIO_SetPinMode(LC2_PIN_GPIO_Port, LC2_PIN_Pin, LL_GPIO_MODE_ANALOG);/*default mode*/

	SYS_EnablePortClock(LC3_PIN_GPIO_Port);
    GPIO_InitStruct.Pin = LC3_PIN_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.OutputType= LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(LC3_PIN_GPIO_Port, &GPIO_InitStruct);
    LL_GPIO_ResetOutputPin(LC3_PIN_GPIO_Port, LC3_PIN_Pin);/*default level*/
	LL_GPIO_SetPinMode(LC3_PIN_GPIO_Port, LC3_PIN_Pin, LL_GPIO_MODE_ANALOG);/*default mode*/

	/*these pins control the switch that disconnect the inductors
	 * from interfering with neighbors signals*/
	SYS_EnablePortClock(CN1_PIN_GPIO_Port);
	LL_GPIO_ResetOutputPin(CN1_PIN_GPIO_Port, CN1_PIN_Pin);
    GPIO_InitStruct.Pin = CN1_PIN_Pin;/*LC_CN1*/
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType= LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(CN1_PIN_GPIO_Port, &GPIO_InitStruct);

	SYS_EnablePortClock(CN2_PIN_GPIO_Port);
	LL_GPIO_ResetOutputPin(CN2_PIN_GPIO_Port, CN2_PIN_Pin);
    GPIO_InitStruct.Pin = CN2_PIN_Pin;/*LC_CN2*/
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType= LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(CN2_PIN_GPIO_Port, &GPIO_InitStruct);

	SYS_EnablePortClock(CN3_PIN_GPIO_Port);
	LL_GPIO_ResetOutputPin(CN3_PIN_GPIO_Port, CN3_PIN_Pin);
    GPIO_InitStruct.Pin = CN3_PIN_Pin;/*LC_CN3*/
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType= LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(CN3_PIN_GPIO_Port, &GPIO_InitStruct);
}

static void LCSENS_OPAMP_Init(void)
{
	LL_GPIO_InitTypeDef GPIO_InitStruct;

	/*this pin control the opamp on/off to save power*/
	SYS_EnablePortClock(OPAMP_PIN_GPIO_Port);
	LL_GPIO_ResetOutputPin(OPAMP_PIN_GPIO_Port, OPAMP_PIN_Pin);
	GPIO_InitStruct.Pin = OPAMP_PIN_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;//Open drain + pull up will leak some current. Open drain + pull down won't work.
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	LL_GPIO_Init(OPAMP_PIN_GPIO_Port, &GPIO_InitStruct);
}

static void LCSENS_DAC_Init(void)
{
	LL_GPIO_InitTypeDef GPIO_InitStruct;

	SYS_EnablePortClock(DAC1_OUT1_GPIO_Port);
	GPIO_InitStruct.Pin = DAC1_OUT1_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	LL_GPIO_Init(DAC1_OUT1_GPIO_Port, &GPIO_InitStruct);

	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_DAC1);

//	LL_DAC_DisableTrigger(DAC1, LL_DAC_CHANNEL_1);
//	LL_DAC_SetOutputConnection(DAC1, LL_DAC_CHANNEL_1, LL_DAC_OUTPUT_CONNECT_INTERNAL);
//	LL_DAC_SetOutputBuffer(DAC1, LL_DAC_CHANNEL_1, LL_DAC_OUTPUT_BUFFER_ENABLE);/*to output to gpio*/

	LL_DAC_InitTypeDef DAC_InitStruct = {0};

	DAC_InitStruct.TriggerSource = LL_DAC_TRIG_SOFTWARE;
	DAC_InitStruct.WaveAutoGeneration = LL_DAC_WAVE_AUTO_GENERATION_NONE;
	DAC_InitStruct.OutputBuffer = LL_DAC_OUTPUT_BUFFER_ENABLE;//LL_DAC_OUTPUT_BUFFER_ENABLE;
	DAC_InitStruct.OutputConnection = LL_DAC_OUTPUT_CONNECT_INTERNAL;
	DAC_InitStruct.OutputMode = LL_DAC_OUTPUT_MODE_SAMPLE_AND_HOLD;
	LL_DAC_Init(DAC1, LL_DAC_CHANNEL_1, &DAC_InitStruct);
	LL_DAC_DisableTrigger(DAC1, LL_DAC_CHANNEL_1);
	LL_DAC_SetSampleAndHoldSampleTime(DAC1, LL_DAC_CHANNEL_1, 1);
	LL_DAC_SetSampleAndHoldHoldTime(DAC1, LL_DAC_CHANNEL_1, 1);
	LL_DAC_SetSampleAndHoldRefreshTime(DAC1, LL_DAC_CHANNEL_1, 1);
}

static void LCSENS_COMP_Init(void)
{
	LL_COMP_InitTypeDef COMP_InitStruct;
	/*
	 * when used with 4Mz and below,
	 *  - set PowerMode to  LL_COMP_POWERMODE_MEDIUMSPEED, this can compesate DAC stabilization after wakeup.
	 *  - LL_PWR_EnableFastWakeUp() can be enabled.
	 * when used with 16Mhz and below,
	 * 	- set PowerMode to LL_COMP_POWERMODE_MEDIUMSPEED, LL_COMP_POWERMODE_ULTRALOWPOWER will give inaccurate pulse reading but won't save current much.
	 * 	- LL_PWR_EnableFastWakeUp() has to be disabled cos unstable reading between run and stop mode.*/
	COMP_InitStruct.PowerMode= LL_COMP_POWERMODE_MEDIUMSPEED;/*LL_COMP_POWERMODE_ULTRALOWPOWER produce different results for stopmode vs runmode*/
	COMP_InitStruct.InputPlus= LL_COMP_INPUT_PLUS_IO2;
	COMP_InitStruct.InputMinus= LL_COMP_INPUT_MINUS_DAC1_CH1;
	COMP_InitStruct.InputHysteresis= LL_COMP_HYSTERESIS_NONE;
	COMP_InitStruct.OutputPolarity= LL_COMP_OUTPUTPOL_NONINVERTED;
	COMP_InitStruct.OutputBlankingSource= LL_COMP_BLANKINGSRC_NONE;
	LL_COMP_Init(COMP2, &COMP_InitStruct);

	LL_COMP_SetCommonWindowMode(__LL_COMP_COMMON_INSTANCE(COMP2), LL_COMP_WINDOWMODE_DISABLE);
	//LL_SYSCFG_VREFINT_EnableCOMP();

	/* Wait loop initialization and execution */
	/* Note: Variable divided by 2 to compensate partially CPU processing cycles */
	__IO uint32_t wait_loop_index = 0;
	wait_loop_index = (LL_COMP_DELAY_VOLTAGE_SCALER_STAB_US * (SystemCoreClock / (1000000 * 2)));
	while(wait_loop_index != 0)
	{
		wait_loop_index--;
	}
}

static void LCSENS_LPTIM2_Init(void)
{
	LL_RCC_SetLPTIMClockSource(LL_RCC_LPTIM2_CLKSOURCE_LSE);

    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_LPTIM2);
    LL_LPTIM_DeInit(LPTIM2);/*force lptim reset. p/s register wont reset when we do soft reboot without MCU reset*/
    LL_LPTIM_SetClockSource(LPTIM2, LL_LPTIM_CLK_SOURCE_EXTERNAL);/*using external clock, will lose 5 initial pulses*/
    LL_LPTIM_SetPrescaler(LPTIM2, LL_LPTIM_PRESCALER_DIV1);
    LL_LPTIM_SetPolarity(LPTIM2, LL_LPTIM_OUTPUT_POLARITY_REGULAR);
    LL_LPTIM_SetUpdateMode(LPTIM2, LL_LPTIM_UPDATE_MODE_IMMEDIATE);
    LL_LPTIM_SetCounterMode(LPTIM2, LL_LPTIM_COUNTER_MODE_EXTERNAL);
    LL_LPTIM_ConfigClock(LPTIM2, LL_LPTIM_CLK_FILTER_NONE, LL_LPTIM_CLK_POLARITY_RISING);
    LL_LPTIM_TrigSw(LPTIM2);
    LL_LPTIM_SetInput1Src(LPTIM2, LL_LPTIM_INPUT1_SRC_COMP2);

    //LL_LPTIM_ConfigTrigger(LPTIM2, LL_LPTIM_TRIG_SOURCE_COMP2, LL_LPTIM_TRIG_FILTER_NONE,LL_LPTIM_TRIG_POLARITY_RISING);

    LL_LPTIM_Enable(LPTIM2);
	LL_LPTIM_SetAutoReload(LPTIM2, LCSENS_CFG_LPTIM2_AUTORELOAD_VALUE);
    LL_LPTIM_StartCounter(LPTIM2, LL_LPTIM_OPERATING_MODE_CONTINUOUS);
}

static void LCSENS_TIM6_Init(void)
{
    //NVIC_SetPriority(TIM6_DAC_IRQn, SYS_CFG_PULSE_CNTR_PRIORITY);
    //NVIC_EnableIRQ(TIM6_DAC_IRQn);

	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM6);
	LL_TIM_SetPrescaler(TIM6, 0);
	LL_TIM_SetCounterMode(TIM6, LL_TIM_COUNTERMODE_DOWN);
	LL_TIM_SetOnePulseMode(TIM6, LL_TIM_ONEPULSEMODE_SINGLE);
	LL_TIM_SetAutoReload(TIM6, 100);
	LL_TIM_ClearFlag_UPDATE(TIM6);
	LL_TIM_EnableIT_UPDATE(TIM6);

	//NVIC_ClearPendingIRQ(TIM6_DAC_IRQn);
	//LL_TIM_EnableUpdateEvent(TIM6);
	//HAL_PWR_EnableSEVOnPend();
}

static void LCSENS_Tempo_Init(void)
{
	LCSENS_TIM6_Init();/*used as activation timer*/

	LL_RCC_SetLPTIMClockSource(LL_RCC_LPTIM1_CLKSOURCE_LSE);

    NVIC_SetPriority(LPTIM1_IRQn, SYS_CFG_PULSE_CNTR_PRIORITY);
    NVIC_EnableIRQ(LPTIM1_IRQn);

    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_LPTIM1);
    LL_LPTIM_SetClockSource(LPTIM1, LL_LPTIM_CLK_SOURCE_INTERNAL);
    LL_LPTIM_SetPrescaler(LPTIM1, LL_LPTIM_PRESCALER_DIV1);
    LL_LPTIM_SetPolarity(LPTIM1, LL_LPTIM_OUTPUT_POLARITY_REGULAR);
    LL_LPTIM_SetUpdateMode(LPTIM1, LL_LPTIM_UPDATE_MODE_IMMEDIATE);
    LL_LPTIM_SetCounterMode(LPTIM1, LL_LPTIM_COUNTER_MODE_INTERNAL);
	LL_LPTIM_EnableIT_ARRM(LPTIM1);
    LL_LPTIM_TrigSw(LPTIM1);
    LL_LPTIM_Disable(LPTIM1);

	LPTIM1_SetAutoReloadMatchCallback(LCSENS_Tempo_Task);
}

__STATIC_INLINE void LCSENS_Tempo_Stop(void)
{
    LL_LPTIM_Disable(LPTIM1);
}

__STATIC_INLINE void LCSENS_Tempo_Set(uint16_t _period)
{
    LL_LPTIM_Disable(LPTIM1);
	LL_LPTIM_ClearFLAG_ARRM(LPTIM1);
    LL_LPTIM_Enable(LPTIM1);
	LL_LPTIM_SetAutoReload(LPTIM1, _period);
    LL_LPTIM_StartCounter(LPTIM1, LL_LPTIM_OPERATING_MODE_CONTINUOUS);
}

static void LCSENS_Sampling_Init(void)
{
	LCSENS_IO_Init();
	LCSENS_OPAMP_Init();
	LCSENS_DAC_Init();
	LCSENS_COMP_Init();
	LCSENS_LPTIM2_Init();/*as counter*/
}

__STATIC_INLINE void LCSENS_Sampling_Initial(void)
{
	LCSENS_DAC_Enable();
	LCSENS_COMP_Enable();
	LCSENS_OPAMP_Enable();

	pConfig->prevBitmap= pConfig->currBitmap;
	pConfig->prevTransitionVal= pConfig->currTransitionVal;
	pConfig->currBitmap= 0;
}

__STATIC_INLINE void LCSENS_Sampling_LC1(void)
{
	LCSENS_DAC_SetValue(pConfig->sensor[0].dacLevel[0]);
	LCSENS_COMP_SetInput_LC1();//LCSENS_COMP_SetInput(LL_COMP_INPUT_PLUS_IO3);
	LCSENS_IO_LC1_Enable();
	LCSENS_IO_LC1_Activate();
	LCSENS_IO_Wait();
	LCSENS_IO_LC1_Sample();

	__LC_DELAY(pConfig->sensor[0].dacOnDelay);/*useless pulse that cannot be counted*/
	LCSENS_SaveCount();/*save initial count*/
	__LC_DELAY(pConfig->sensor[0].activationTime[0]);

	if(MULTI_DAC_SamplingStrategy== pConfig->sensor[0].samplingStrategy)
	{
		for(int i= 1; i<LCSENS_CFG_MAX_DAC_STEP; i++)
		{
			LCSENS_DAC_SetValue(pConfig->sensor[0].dacLevel[i]);
			__LC_DELAY(pConfig->sensor[0].activationTime[i]);
		}
	}

	pConfig->sensor[0].currCount= LCSENS_GetCount();
	LCSENS_IO_LC1_Disable();
	pConfig->currBitmap|= (pConfig->sensor[0].currCount<= pConfig->sensor[0].detectThreshold)? 0b100: 0b000;
}

__STATIC_INLINE void LCSENS_Sampling_LC2(void)
{
	LCSENS_DAC_SetValue(pConfig->sensor[1].dacLevel[0]);
	LCSENS_COMP_SetInput_LC2();//LCSENS_COMP_SetInput(LL_COMP_INPUT_PLUS_IO1);
	LCSENS_IO_LC2_Enable();
	LCSENS_IO_LC2_Activate();
	LCSENS_IO_Wait();
	LCSENS_IO_LC2_Sample();

	__LC_DELAY(pConfig->sensor[1].dacOnDelay);/*useless pulse that cannot be counted*/
	LCSENS_SaveCount();/*save initial count*/
	__LC_DELAY(pConfig->sensor[1].activationTime[0]);

	if(MULTI_DAC_SamplingStrategy== pConfig->sensor[1].samplingStrategy)
	{
		for(int i= 1; i<LCSENS_CFG_MAX_DAC_STEP; i++)
		{
			LCSENS_DAC_SetValue(pConfig->sensor[1].dacLevel[i]);
			__LC_DELAY(pConfig->sensor[1].activationTime[i]);
		}
	}

	pConfig->sensor[1].currCount= LCSENS_GetCount();
	LCSENS_IO_LC2_Disable();
	pConfig->currBitmap|= (pConfig->sensor[1].currCount<= pConfig->sensor[1].detectThreshold)? 0b010: 0b000;
}

__STATIC_INLINE void LCSENS_Sampling_LC3(void)
{
	LCSENS_DAC_SetValue(pConfig->sensor[2].dacLevel[0]);
	LCSENS_COMP_SetInput_LC3();//LCSENS_COMP_SetInput(LL_COMP_INPUT_PLUS_IO2);
	LCSENS_IO_LC3_Enable();
	LCSENS_IO_LC3_Activate();
	LCSENS_IO_Wait();
	LCSENS_IO_LC3_Sample();

	__LC_DELAY(pConfig->sensor[2].dacOnDelay);/*useless pulse that cannot be counted*/
	LCSENS_SaveCount();/*save initial count*/
	__LC_DELAY(pConfig->sensor[2].activationTime[0]);

	if(MULTI_DAC_SamplingStrategy== pConfig->sensor[2].samplingStrategy)
	{
		for(int i= 1; i<LCSENS_CFG_MAX_DAC_STEP; i++)
		{
			LCSENS_DAC_SetValue(pConfig->sensor[2].dacLevel[i]);
			__LC_DELAY(pConfig->sensor[2].activationTime[i]);
		}
	}

	pConfig->sensor[2].currCount= LCSENS_GetCount();
	LCSENS_IO_LC3_Disable();
	pConfig->currBitmap|= (pConfig->sensor[2].currCount<= pConfig->sensor[2].detectThreshold)? 0b001: 0b000;
}

__STATIC_INLINE void LCSENS_Sampling_Final(void)
{
	LCSENS_OPAMP_Disable();
	LCSENS_DAC_Disable();
	LCSENS_COMP_Disable();
}

__STATIC_INLINE void LCSENS_Process(void)
{
	/*start processing*/
	pConfig->currTransitionVal= pConfig->transitionTable[pConfig->currBitmap];

	if((7== pConfig->currTransitionVal)|| (0xFF== pConfig->currTransitionVal))//error or sensors broke
	{
		if(0!= pConfig->prevTransitionVal)
		{
			pConfig->currTransitionVal= pConfig->prevTransitionVal;/*not necessarily sensor broke, for janz meter sometimes we just cant detect during transition., so we assign as previous value.*/
		}
		else
		{
			//pConfig->currTransitionVal = 4;
		}
	}

	if((1== pConfig->currTransitionVal)&& (6== pConfig->prevTransitionVal))
	{
		pConfig->fwdTransitionCnt++;
		if(pConfig->initialTransitionVal== pConfig->currTransitionVal)
		{
			pConfig->fwdRevolutionCnt++;
		}
		pConfig->currDirection= FORWARD_CounterDirection;
	}
	else if((6== pConfig->currTransitionVal)&& (1== pConfig->prevTransitionVal))
	{
		pConfig->bwdTransitionCnt++;
		if(pConfig->initialTransitionVal== pConfig->prevTransitionVal)
		{
			pConfig->bwdRevolutionCnt++;
		}
		pConfig->currDirection= BACKWARD_CounterDirection;
	}
	else if(pConfig->currTransitionVal> pConfig->prevTransitionVal)
	{
		pConfig->fwdTransitionCnt++;
		if(pConfig->initialTransitionVal== pConfig->currTransitionVal)
		{
			pConfig->fwdRevolutionCnt++;
		}
		pConfig->currDirection= FORWARD_CounterDirection;
	}
	else if(pConfig->currTransitionVal< pConfig->prevTransitionVal)
	{
		pConfig->bwdTransitionCnt++;
		if(pConfig->initialTransitionVal== pConfig->prevTransitionVal)
		{
			pConfig->bwdRevolutionCnt++;
		}
		pConfig->currDirection= BACKWARD_CounterDirection;
	}
}

__IO uint16_t samplingCount= 0;
void LCSENS_Tempo_Task(void)
{
	LCSENS_Sampling_Initial();
	LCSENS_Sampling_LC1();
	LCSENS_Sampling_LC2();
	LCSENS_Sampling_LC3();
	LCSENS_Sampling_Final();
	LCSENS_Process();
	LCSENS_Calibration_Task();

	samplingCount++;
	if(0== (samplingCount% pConfig->samplingTempo_hz)&& (samplingCount))
	{
		samplingCount= 0;
		DBG_Print("LCCount [%d] [%d] [%d]\r\n", pConfig->sensor[0].currCount, pConfig->sensor[1].currCount, pConfig->sensor[2].currCount);
		DBG_Print("LCBitmap [%c%c%c%c]\r\n", NIBBLE_TO_BINARY(pConfig->currBitmap));
		DBG_Print("LCRev [%d] [%d]\r\n", pConfig->fwdRevolutionCnt, pConfig->bwdRevolutionCnt);
		//SYS_Wakeup();/*3wakeup to print*/
	}
}

void LCSENS_ScheduleCalibration(uint8_t _sensor)
{
	pConfig->sensor[_sensor].doCalib= true;
}

void LCSENS_StaticCalibration(void)
{
	LCSENS_ScheduleCalibration(0);
	LCSENS_ScheduleCalibration(1);
	LCSENS_ScheduleCalibration(2);
	pConfig->calib= 1;
}

void LCSENS_DynamicCalibration(void)
{
	pConfig->dynamicCalibBitmap= 0b111;
	pConfig->calib= 2;
}

void LCSENS_Calibration_Task(void)
{
	if(0== pConfig->calib)
	{
		return;
	}

	if(2== pConfig->calib)
	{
		if(0!= (0b1& pConfig->dynamicCalibBitmap))
		{
			if((4== pConfig->prevTransitionVal)&& (6== pConfig->currTransitionVal))
			{
				pConfig->sensor[0].doCalib= 1;
				pConfig->dynamicCalibBitmap&= 0b110;
				DBG_Print("LC1 DynamiCalibration\r\n");
			}
		}

		if(0!= (0b10& pConfig->dynamicCalibBitmap))
		{
			if((6== pConfig->prevTransitionVal)&& (1== pConfig->currTransitionVal))
			{
				pConfig->sensor[1].doCalib= 1;
				pConfig->dynamicCalibBitmap&= 0b101;
				DBG_Print("LC2 DynamiCalibration\r\n");
			}
		}

		if(0!= (0b100& pConfig->dynamicCalibBitmap))
		{
			if((1== pConfig->prevTransitionVal)&& (4== pConfig->currTransitionVal))
			{
				pConfig->sensor[2].doCalib= 1;
				pConfig->dynamicCalibBitmap&= 0b011;
				DBG_Print("LC3 DynamiCalibration\r\n");
			}
		}

		if(0== pConfig->dynamicCalibBitmap)
		{
			pConfig->calib= 0;
		}
	}

	uint8_t _sensorToCalibCnt= 0;
	for(int i= 0; i< LCSENS_CFG_SENSOR_COUNT; i++)
	{
		_sensorToCalibCnt+= (true== pConfig->sensor[i].doCalib)? 1: 0;
	}

	if(0== _sensorToCalibCnt)
	{
		return;
	}

	do{
		LCSENS_Sampling_Initial();
		LCSENS_Sampling_LC1();
		LCSENS_Sampling_LC2();
		LCSENS_Sampling_LC3();
		LCSENS_Sampling_Final();

		for(int i= 0; i< LCSENS_CFG_SENSOR_COUNT; i++)
		{
			if(false== pConfig->sensor[i].doCalib)
			{
				continue;
			}
			if(true== LCSENS_Calibration(i))
			{
				_sensorToCalibCnt--;
			}
		}
	}while(0!= _sensorToCalibCnt);

	pConfig->calib= 0;
}

__IO uint8_t _dacStep= 1;
__IO int8_t _countPerStep= 1;
__IO uint8_t _dacStabilityMargin= 2;
__IO uint8_t _timeStep= 5;//10;
__IO uint8_t _minActTime= 2;//5;
__IO uint8_t _maxActTime= 1/*_countPerStep*/* 30;

bool LCSENS_Calibration(uint8_t _sensorIndex)
{
	__IO uint8_t _dacIndex= pConfig->sensor[_sensorIndex].dacStep;
	__IO int16_t _countDiff= pConfig->sensor[_sensorIndex].currCount- pConfig->sensor[_sensorIndex].prevCount;

	switch(pConfig->sensor[_sensorIndex].calibStep)
	{
		case 0:
			for(int j= 0; j< LCSENS_CFG_MAX_DAC_STEP; j++)
			{
				pConfig->sensor[_sensorIndex].dacLevel[j]= 0;
				//pConfig->sensor[_sensorIndex].dacLevel[j]= 200;
				pConfig->sensor[_sensorIndex].activationTime[j]= 15;
			}
			pConfig->sensor[_sensorIndex].prevCount= 0;
			pConfig->sensor[_sensorIndex].dacOnDelay= 15;
			pConfig->sensor[_sensorIndex].dacStep= 0;
			pConfig->sensor[_sensorIndex].calibStep++;
			break;
		case 1:
			if(0!= pConfig->sensor[_sensorIndex].currCount)
			{
				/*we want to eliminate the pulses that falls below DAC level 0*/
				pConfig->sensor[_sensorIndex].dacOnDelay+= _timeStep;
				break;
			}
			pConfig->sensor[_sensorIndex].dacOnDelay+= 2; /*for stability*/
			pConfig->sensor[_sensorIndex].calibStep++;
		case 2:
			if(pConfig->sensor[_sensorIndex].dacLevel[_dacIndex]>= 125)/*maximum dac value*/
			//if(pConfig->sensor[_sensorIndex].dacLevel[_dacIndex]<= 130)/*minimum dac value*/
			{
				pConfig->sensor[_sensorIndex].calibStep= 0;
			}
//			else if(0> _countDiff)
//			{
//				pConfig->sensor[_sensorIndex].calibStep= 0;
//			}
			else if(_countPerStep> _countDiff)
			{
				/*not found yet*/
				if(_maxActTime> pConfig->sensor[_sensorIndex].activationTime[_dacIndex])
				{
					pConfig->sensor[_sensorIndex].activationTime[_dacIndex]+= _timeStep;/*increase time*/
				}
				else
				{
					pConfig->sensor[_sensorIndex].activationTime[_dacIndex]= _minActTime;
					pConfig->sensor[_sensorIndex].dacLevel[_dacIndex]+= _dacStep;
					//pConfig->sensor[_sensorIndex].dacLevel[_dacIndex]-= _dacStep;
				}
			}
			else
			{
				pConfig->sensor[_sensorIndex].dacLevel[_dacIndex]+= _dacStabilityMargin;
				//pConfig->sensor[_sensorIndex].dacLevel[_dacIndex]-= _dacStabilityMargin;
				_dacIndex++;
				pConfig->sensor[_sensorIndex].dacStep++;
				pConfig->sensor[_sensorIndex].prevCount+= _countPerStep;//pConfig->sensor[_sensorIndex].currCount;
				if(LCSENS_CFG_MAX_DAC_STEP> _dacIndex)
				{
					pConfig->sensor[_sensorIndex].dacLevel[_dacIndex]=
							pConfig->sensor[_sensorIndex].dacLevel[_dacIndex- 1];
				}
				else
				{
					pConfig->sensor[_sensorIndex].detectThreshold= LCSENS_GetThreshold(LCSENS_CFG_MAX_DAC_STEP);
					pConfig->sensor[_sensorIndex].calibStep= 0;
					pConfig->sensor[_sensorIndex].doCalib= false;
					return true;
				}
			}
			break;
	}

	return false;
}

uint32_t LCSENS_GetValue(void)
{
	int32_t _value= (pConfig->fwdRevolutionCnt- pConfig->bwdRevolutionCnt);

	_value+= pConfig->rteOffsetValue;

	return (uint32_t)_value;
}

void LCSENS_SetValue(int32_t _value)
{
	_value-= LCSENS_GetValue();
	pConfig->rteOffsetValue+= _value;
}

uint8_t LCSENS_GetDirection(void)
{
	return pConfig->currDirection;
}

void LCSENS_StartCounting(void)
{
	LCSENS_Tempo_Set(pConfig->samplingTempo);
}

void LCSENS_Init(LCSENS_t *_config)
{
	pConfig= _config;

	//use 0 for min
	pConfig->transitionTable[0b100]= 1;
	pConfig->transitionTable[0b110]= 1;
	pConfig->transitionTable[0b010]= 4;
	pConfig->transitionTable[0b011]= 4;
	pConfig->transitionTable[0b001]= 6;
	pConfig->transitionTable[0b101]= 6;
	pConfig->transitionTable[0b000]= 7;//2 sensors broke
	//use 8 for max
	pConfig->transitionTable[0b111]= 0xFF;//all sensors broke

	pConfig->sensor[0].samplingStrategy= MULTI_DAC_SamplingStrategy;
	pConfig->sensor[1].samplingStrategy= MULTI_DAC_SamplingStrategy;
	pConfig->sensor[2].samplingStrategy= MULTI_DAC_SamplingStrategy;
	pConfig->samplingTempo_hz= 32;//32;//16;//15;
	pConfig->samplingTempo= HZ_TO_32768HZ_COUNTER(pConfig->samplingTempo_hz);

	LCSENS_Sampling_Init();
	LCSENS_Tempo_Init();

	if(0== pConfig->sensor[0].dacOnDelay)
	{
		for(int i= 0; i< LCSENS_CFG_SENSOR_COUNT; i++)
		{
			uint8_t _dacLevel= 0;
			pConfig->sensor[i].dacOnDelay= 250;
			pConfig->sensor[i].detectThreshold= LCSENS_GetThreshold(LCSENS_CFG_MAX_DAC_STEP);

			for(int j= 0; j< LCSENS_CFG_MAX_DAC_STEP; j++)
			{
				_dacLevel+= 8;
				pConfig->sensor[i].dacLevel[j]= _dacLevel;
				pConfig->sensor[i].activationTime[j]= 15;/*minimum is 15*/
			}
		}
		pConfig->initialTransitionVal= 4;
		LCSENS_StaticCalibration();
		LCSENS_Calibration_Task();
		LCSENS_DynamicCalibration();
	}
	else
	{
		pConfig->initialTransitionVal= 4;
		LCSENS_StaticCalibration();
		LCSENS_Calibration_Task();
	}
}

void LCSENS_Task(void)
{
}

uint8_t LCSENS_TaskState(void)
{
	return SLEEP_TaskState;
}

#pragma GCC pop_options
