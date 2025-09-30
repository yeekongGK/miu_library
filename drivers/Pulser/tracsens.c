/*
 * tracsens.c
 *
 *  Created on: 13 Jan 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#include "common.h"
#include "tracsens.h"
#include "lptim1.h"

static TRACSENS_t *pConfig;
static PULSER_CounterMode_t eMode= NORMAL_CounterMode;
__IO static int32_t lCntrErrorReading= 0;
__IO static int32_t lCntrMultiplier= 0;
__IO static int32_t lCntrBackwardFlowMarker= 0;
__IO static bool bAutoReloadArmed= true;
__IO static uint16_t uwCompareValue;
__IO static PULSER_CounterDirection_t eCounterDirection= UNKNOWN_CounterDirection;
__IO static PULSER_CounterDirection_t eExpectedDirection= UNKNOWN_CounterDirection;

static void TRACSENS_Power(bool _enable);

void TRACSENS_Init(TRACSENS_t *_config)
{
	LL_GPIO_InitTypeDef GPIO_InitStruct;

	pConfig= _config;
	eMode= (TRACSENSi_Mode== config.pulser.mode)? INVERT_CounterMode: NORMAL_CounterMode;

	SYS_EnablePortClock(LPTIM1_IN1_GPIO_Port);
    GPIO_InitStruct.Pin = LPTIM1_IN1_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.OutputType= LL_GPIO_OUTPUT_OPENDRAIN;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
    LL_GPIO_Init(LPTIM1_IN1_GPIO_Port, &GPIO_InitStruct);

	SYS_EnablePortClock(LPTIM1_IN2_GPIO_Port);
    GPIO_InitStruct.Pin = LPTIM1_IN2_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.OutputType= LL_GPIO_OUTPUT_OPENDRAIN;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
    LL_GPIO_Init(LPTIM1_IN2_GPIO_Port, &GPIO_InitStruct);

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
    //LL_LPTIM_ConfigClock(LPTIM1, LL_LPTIM_CLK_FILTER_8, LL_LPTIM_CLK_POLARITY_FALLING);

    LL_LPTIM_TrigSw(LPTIM1);
}

static void TRACSENS_Power(bool _enable)
{
}

static int32_t TRACSENS_GetCounter(void)
{
	int32_t _value;
    do/* 2 consecutive readings need to be the same*/
    {
    	_value= LL_LPTIM_GetCounter(LPTIM1);
    }while(LL_LPTIM_GetCounter(LPTIM1)!= _value);

	if(INVERT_CounterMode== eMode)
	{
		_value= 0xFFFF& ((TRACSENS_CFG_AUTORELOAD_VALUE+ 1)- _value);
	}

    _value+= (lCntrMultiplier* (TRACSENS_CFG_AUTORELOAD_VALUE+ 1));

    return _value;
}

void TRACSENS_StartCounting()
{
	TRACSENS_Power(true);

	if(NORMAL_CounterMode== eMode)
	{
		LPTIM1_SetCompareCallback(TRACSENS_CompareCallback);
		LPTIM1_SetAutoReloadMatchCallback(TRACSENS_AutoReloadMatchCallback);
		LPTIM1_SetCounterChangedToUpCallback(TRACSENS_CounterChangedToUpCallback);
		LPTIM1_SetCounterChangedToDownCallback(TRACSENS_CounterChangedToDownCallback);
	}
	else if(INVERT_CounterMode== eMode)
	{
		LPTIM1_SetCompareCallback(TRACSENS_AutoReloadMatchCallback);
		LPTIM1_SetAutoReloadMatchCallback(TRACSENS_CompareCallback);
		LPTIM1_SetCounterChangedToUpCallback(TRACSENS_CounterChangedToDownCallback);
		LPTIM1_SetCounterChangedToDownCallback(TRACSENS_CounterChangedToUpCallback);
	}

	uwCompareValue=0x01;/*used once to detect direction*/

	LL_LPTIM_EnableIT_CMPM(LPTIM1);
	LL_LPTIM_EnableIT_ARRM(LPTIM1);
	LL_LPTIM_EnableIT_UP(LPTIM1);/*direction change from down to up*/
	LL_LPTIM_EnableIT_DOWN(LPTIM1);/*direction change from up to down*/

	LL_LPTIM_SetCompare(LPTIM1, uwCompareValue);/*we need this to know the initial pulse direction(which we don't know after reset)*/

	LL_LPTIM_SetEncoderMode(LPTIM1, LL_LPTIM_ENCODER_MODE_RISING_FALLING);
    LL_LPTIM_EnableEncoderMode(LPTIM1);
    LL_LPTIM_Enable(LPTIM1);
	LL_LPTIM_SetAutoReload(LPTIM1, TRACSENS_CFG_AUTORELOAD_VALUE);
    LL_LPTIM_StartCounter(LPTIM1, LL_LPTIM_OPERATING_MODE_CONTINUOUS);
    //LPTIM_FeedExternalClock();/*needed when using external clock in counter mode*/

    /*this is needed during power up as we always get extra pulse a bit while after start counting*/
	UTILI_usDelay(1);/*when reboot, we get extra pulse*/

	do/* 2 consecutive readings need to be the same*/
    {
    	lCntrErrorReading= LL_LPTIM_GetCounter(LPTIM1);
    }while(LL_LPTIM_GetCounter(LPTIM1)!= lCntrErrorReading);

	//DBG_Print("lCntrErrorReading: %d.\r\n", lCntrErrorReading);
}

void TRACSENS_CompareCallback(void)
{
	if(UNKNOWN_CounterDirection== eCounterDirection)/*initially we don't know. we choose forward cos this compare confirming it forward*/
	{
		eCounterDirection= FORWARD_CounterDirection;
	}

	//DBG_Print("CompareCallback:%d, multiplier:%d \r\n", eCounterDirection, lCntrMultiplier);
}

void TRACSENS_AutoReloadMatchCallback(void)
{
	if(UNKNOWN_CounterDirection== eCounterDirection)/*initially we don't know. we choose backward cos we have a compare int to choose forward*/
	{
		eCounterDirection= BACKWARD_CounterDirection;
	}

	/*check and set cos sometimes direction interrupt will occur simultaneously with ARR interrupt, but served after ARR*/
	if(LL_LPTIM_IsActiveFlag_UP(LPTIM1))
	{
		if(NORMAL_CounterMode== eMode)
		{
			eCounterDirection= FORWARD_CounterDirection;
		}
		else if(INVERT_CounterMode== eMode)
		{
			eCounterDirection= BACKWARD_CounterDirection;
		}
	}
	else if(LL_LPTIM_IsActiveFlag_DOWN(LPTIM1))
	{
		if(NORMAL_CounterMode== eMode)
		{
			eCounterDirection= BACKWARD_CounterDirection;
		}
		else if(INVERT_CounterMode== eMode)
		{
			eCounterDirection= FORWARD_CounterDirection;
		}
	}

	if(FORWARD_CounterDirection== eCounterDirection)
	{
		lCntrMultiplier++;
	}
	else if(BACKWARD_CounterDirection== eCounterDirection)
	{
		lCntrMultiplier--;
	}

	//DBG_Print("AutoReloadMatchCallback:%d, multiplier:%d \r\n", eCounterDirection, lCntrMultiplier);
}

void TRACSENS_ChangedToUpErrorHandling(void)
{
	if(BWD_EXPECTING_FWD_CounterErrorState== pConfig->rteErrorPatternState)
	{
		pConfig->rteErrorPatternState= FWD_EXPECTING_BWD_END_CounterErrorState;
	}

}

void TRACSENS_ChangedToDownErrorHandling(void)
{
	int32_t _curr= TRACSENS_GetCounter();
	if(0!= (_curr- pConfig->rteErrorPatternPreviousPulse))
	{
		pConfig->rteErrorPatternState= NONE_CounterErrorState; /*reset if we get real pulse inbetween error pattern*/
		pConfig->rteErrorPatternJustStarted= false;
		if(false== pConfig->rteErrorPatternCompensationStarted)
		{
			pConfig->rteErrorPatternCount= 0;/*we need consecutive error pattern to mark the meter as erroneous that we can handle*/
			//DBG_Print("#stat:mag2: errorPatternCountCleared >\r\n");
		}
	}
	if(NONE_CounterErrorState== pConfig->rteErrorPatternState)
	{
		pConfig->rteErrorPatternJustStarted= true;
		pConfig->rteErrorPatternState= BWD_EXPECTING_FWD_CounterErrorState;
	}
	else if(FWD_EXPECTING_BWD_CounterErrorState== pConfig->rteErrorPatternState)
	{
		pConfig->rteErrorPatternState= BWD_EXPECTING_FWD_END_CounterErrorState;
	}
	else if(FWD_EXPECTING_BWD_END_CounterErrorState== pConfig->rteErrorPatternState)
	{
		pConfig->rteErrorPatternCount++;
		if(true== pConfig->rteErrorPatternJustStarted)
		{
			pConfig->rteErrorPatternJustStarted= false;
			pConfig->rteErrorPatternCount++;
		}

		if((false== pConfig->rteErrorPatternCompensationStarted)&& (pConfig->errorPatternConfirmationCount<= pConfig->rteErrorPatternCount))
		{
			pConfig->rteErrorPatternCompensationStarted= true;
			//DBG_Print("#stat:mag2: errorPatternCompensationStarted > \r\n");
		}
		pConfig->rteErrorPatternState= BWD_EXPECTING_FWD_CounterErrorState;
	}
	pConfig->rteErrorPatternPreviousPulse= _curr;
}

void TRACSENS_CounterChangedToUpCallback(void)
{
	eCounterDirection= FORWARD_CounterDirection;

	if(true== pConfig->enableErrorPatternCheck)
	{
		TRACSENS_ChangedToUpErrorHandling();
	}

	//DBG_Print("CounterChangedToUpCallback:%d, multiplier:%d \r\n", eCounterDirection, lCntrMultiplier);
}

void TRACSENS_CounterChangedToDownCallback(void)
{
	eCounterDirection= BACKWARD_CounterDirection;

	if(true== pConfig->enableErrorPatternCheck)
	{
		TRACSENS_ChangedToDownErrorHandling();
	}

	//DBG_Print("CounterChangedToDownCallback:%d, multiplier:%d \r\n", eCounterDirection, lCntrMultiplier);
}

void TRACSENS_StopCounting(void)
{
	TRACSENS_Power(false);
}

void TRACSENS_DisplayInfo(void)
{
	static int32_t _prevPulse= 0;
	int32_t _currPulse= TRACSENS_GetCounter();
	if(_prevPulse!= _currPulse)
	{
		_prevPulse= _currPulse;

		DBG_Print("#stat:mag2: display > curr: %d errCnt: %d errState: %d errPulse: %d\r\n",
				_currPulse, pConfig->rteErrorPatternCount, pConfig->rteErrorPatternState, pConfig->rteErrorPatternPreviousPulse);
	}
}

uint32_t TRACSENS_GetValue(void)
{
	int32_t _value= TRACSENS_GetCounter();

	_value-= lCntrErrorReading;

	_value+= (pConfig->rteOffsetValue);

	if((true== pConfig->useCompensatedValue)&& (true== pConfig->enableErrorPatternCheck)&& (true== pConfig->rteErrorPatternCompensationStarted))
	{
		_value+= (pConfig->rteErrorPatternCount* 4);
	}

	//TRACSENS_DisplayInfo();

	return (uint32_t)_value;
}

void TRACSENS_SetValue(int32_t _value)
{
	_value-= TRACSENS_GetValue();
	pConfig->rteOffsetValue+= _value;
}

uint8_t TRACSENS_GetDirection(void)
{
	if(NONE_CounterErrorState!= pConfig->rteErrorPatternState)
	{
		if(0== (TRACSENS_GetCounter()- pConfig->rteErrorPatternPreviousPulse))
		{
			return FORWARD_CounterDirection;
		}
	}
	return eCounterDirection;
}

bool TRACSENS_ErrorDetected(void)
{
	if((true== pConfig->rteErrorPatternCompensationStarted)&&
			(config.pulser.rtePrevErrorPatternCount!= pConfig->rteErrorPatternCount))
	{
		return true;
	}

	return false;
}

void TRACSENS_ClearError(void)
{
	config.pulser.rtePrevErrorPatternCount= pConfig->rteErrorPatternCount;
}

