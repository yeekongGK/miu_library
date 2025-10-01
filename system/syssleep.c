/*
 * rtcwkup.c
 *
 *  Created on: 2 Feb 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

//#include "common.h"
#include "syssleep.h"
#include "sysclk.h"

static void SYSSLEEP_SetMinSleepPeriod_ms(void);
static void SYSSLEEP_SetSleepPeriod_ms(uint32_t _period_ms);
static void SYSSLEEP_RequestAwakeTick(uint32_t _requestMs);

#define SYSSLEEP_GetTick_ms SYS_GetTimestamp_ms

__IO static uint64_t ulTickMultiplier= 0;//10518984(10 years of 30secs);
__IO static uint64_t ulMinPeriod= 0;
__IO static uint64_t ulCurrRequest= 0x10000;
__IO static uint64_t ulCurrRequestTimestamp= 0;
__IO static uint64_t ulNextRequest= 0x10000;
__IO static uint64_t tSleepAllowedTime= 0;

static void SYSSLEEP_SetMinSleepPeriod_ms(void)
{
	ulMinPeriod= SYSSLEEP_GetTick_ms();
	ulCurrRequest= 0x1000UL;
	SYSSLEEP_RequestAwakeTick(ulCurrRequest- 1);/*dummy request to activate the function call*/
	ulMinPeriod= SYSSLEEP_GetTick_ms()- ulMinPeriod;
	ulMinPeriod= (SYSSLEEP_CFG_MIN_PERIOD>= ulMinPeriod)? SYSSLEEP_CFG_MIN_PERIOD: ulMinPeriod;
}

static void __attribute__((optimize("O0"))) SYSSLEEP_InhibitSleepPeriod_ms(uint16_t _period_ms)
{
	tSleepAllowedTime= SYSSLEEP_GetTick_ms() + _period_ms;
}

bool SYSSLEEP_SleepAllowed(void)
{
	return (SYSSLEEP_GetTick_ms()> tSleepAllowedTime)? true: false;
}

static void SYSSLEEP_SetSleepPeriod_ms(uint32_t _period_ms)
{
	uint64_t _autoReloadValue= ((_period_ms* 32768UL)/ (16/*LL_RTC_WAKEUPCLOCK_DIV_16*/* 1000UL))- 1;
	if(0xFFFF< _autoReloadValue)
	{
		_autoReloadValue= 0xFFFF;
	}

	/*TODO: scrutinize this as this was ported from L0 mcu*/
	if(_autoReloadValue> (0x10000- 5))
	{
		_autoReloadValue= (0x10000- 5); /*for 16Mhz, if we set more than (0x10000- 5), mcu might not go to sleep for (_period- 0xFFFF)*/
	}
	else if(4>= _autoReloadValue)
	{
		_autoReloadValue= 5;/*for div16, resolution is 488.28us, but minimum time base is 2*(488.28us), but on 4Mhz if we set less than 5, the mcu might not even wakeup. maybe timeout occur even while setting up the clock.*/
	}

	LL_RTC_DisableWriteProtection(RTC);

	LL_RTC_WAKEUP_Disable(RTC);/* Disable wake up timer to modify it */
	while(1!= LL_RTC_IsActiveFlag_WUTW(RTC))/* Wait until it is allow to modify wake up reload value */
	{
	}
	LL_RTC_WAKEUP_SetAutoReload(RTC, _autoReloadValue);
	LL_RTC_WAKEUP_SetClock(RTC, LL_RTC_WAKEUPCLOCK_DIV_16);
	/* Enable wake up counter and wake up interrupt */
	LL_RTC_WAKEUP_Enable(RTC);
	LL_RTC_EnableIT_WUT(RTC);
	LL_RTC_ClearFlag_WUT(RTC);

	LL_RTC_EnableWriteProtection(RTC);

	ulCurrRequestTimestamp= SYSSLEEP_GetTick_ms();
	SYSSLEEP_InhibitSleepPeriod_ms(9);/*delay to make sure WKUP has enabled and run before going to sleep*/
	//DBG_Print("_autoReloadValue : %u.\r\n", (uint32_t)_autoReloadValue);
}

static void SYSSLEEP_RequestAwakeTick(uint32_t _requestMs)
{
	uint64_t ulCurrRequestRemaining= (ulCurrRequest- (SYSSLEEP_GetTick_ms()- ulCurrRequestTimestamp));

	if((_requestMs< ulCurrRequestRemaining)|| (ulMinPeriod>= ulCurrRequestRemaining))
	{
		ulCurrRequest= _requestMs;

		SYSSLEEP_SetSleepPeriod_ms(ulCurrRequest);
	}
	else
	{
		if((_requestMs- ulCurrRequestRemaining)< ulNextRequest)
		{
			ulNextRequest= (_requestMs- ulCurrRequestRemaining);
			//DBG_Print("Req: ulCurrRequest : %u, ulNextRequest : %u .\r\n", (uint32_t)ulCurrRequest, (uint32_t)ulNextRequest);
		}
	}
}

bool SYSSLEEP_IsAwake(SYS_TaskId_t _taskId)
{
	uint64_t _currTick= SYSSLEEP_GetTick_ms();

	if(_currTick>= (pSYS_TaskInfo[_taskId].sleepTimestamp+ pSYS_TaskInfo[_taskId].sleepPeriod))
	{
		return true;
	}
	else
	{
		SYSSLEEP_RequestAwakeTick(pSYS_TaskInfo[_taskId].sleepPeriod- (_currTick- pSYS_TaskInfo[_taskId].sleepTimestamp));
	}
	return false;
}

void SYSSLEEP_RequestSleep(SYS_TaskId_t _taskId, uint32_t _period_ms)
{
	pSYS_TaskInfo[_taskId].sleepTimestamp= SYSSLEEP_GetTick_ms();
	pSYS_TaskInfo[_taskId].sleepPeriod= _period_ms;
	(void)SYSSLEEP_IsAwake(_taskId); /*call once to initiate sleep*/
}

uint64_t SYSSLEEP_GetCurrSleepRequest(void)
{
	return ulCurrRequest;
}
uint64_t SYSSLEEP_GetNextSleepRequest(void)
{
	return ulNextRequest;
}

void SYSSLEEP_EventCallback(void)
{
	SYSSLEEP_Disable();
	//DBG_Print("ulCurrRequest : %u, ulNextRequest : %u .\r\n", (uint32_t)ulCurrRequest, (uint32_t)ulNextRequest);
	ulCurrRequest= (ulNextRequest!= 0)? ulNextRequest: SYSSLEEP_CFG_DEFAULT_NEXT_REQUEST;
	ulNextRequest= SYSSLEEP_CFG_DEFAULT_NEXT_REQUEST;
	SYSSLEEP_SetSleepPeriod_ms(ulCurrRequest);
}

void SYSSLEEP_Disable(void)
{
	LL_RTC_DisableWriteProtection(RTC);/* needed as RTC registers are write protected */
	LL_RTC_WAKEUP_Disable(RTC);
	LL_RTC_EnableWriteProtection(RTC);
}

void SYSSLEEP_Init(void)
{
	LL_DBGMCU_DisableDBGStopMode();
	//LL_DBGMCU_EnableDBGStopMode();
	//LL_DBGMCU_APB1_GRP1_FreezePeriph(LL_DBGMCU_APB1_GRP1_RTC_STOP);

	RTC_Init();

	/*WKUP RTC interrupt Init */
	LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_20);
	LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_20);
	NVIC_SetPriority(RTC_WKUP_IRQn, SYS_CFG_SYSSLEEP_PRIORITY);
	NVIC_EnableIRQ(RTC_WKUP_IRQn);

	SYSSLEEP_SetMinSleepPeriod_ms();
}

void SYSSLEEP_EnterLightSleep(void)
{
    LL_PWR_SetPowerMode(LL_PWR_MODE_STOP0); /* Enter STOP 2 mode */
    LL_LPM_EnableDeepSleep();  /* Set SLEEPDEEP bit of Cortex System Control Register */
    __WFI();/* Request Wait For Interrupt */

//	LL_LPM_EnableSleep();/*clear deep sleep flag*/
//    //RCC->CFGR = RCC_CFGR_HPRE_DIV64;    /* Decrease the HCLK clock during sleep */
//    __WFI();                            /* Sleep */
//    //RCC->CFGR = RCC_CFGR_HPRE_DIV1;     /* Go back to full speed */
}

void SYSSLEEP_EnterDeepSleep(void)
{
//	if(ulCurrRequest>= 500)
//		DIAG_Code(AVECURRENT_UA_SensorDCode, SENSOR_GetValue(AVECURRENT_Sensor));

	//DIAG_Code(AVECURRENT_UA_SensorDCode, SENSOR_GetValue(AVECURRENT_Sensor));
    LL_PWR_SetPowerMode(LL_PWR_MODE_STOP2); /* Enter STOP 2 mode */
    LL_LPM_EnableDeepSleep();  /* Set SLEEPDEEP bit of Cortex System Control Register */
    __WFI();/* Request Wait For Interrupt */
}
