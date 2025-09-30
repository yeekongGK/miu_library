/*
 * clcokmgt.c
 *
 *  Created on: 5 Feb 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#include "common.h"
#include "sysclk.h"

void (*SYSCLK_InitWakeupClock)(void)= NULL;

__IO uint64_t SYSCLK_rteSysTick= 0;

void SYSCLK_WaitPLL(void)
{
	//LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_MSI, LL_RCC_PLLM_DIV_1, 40, LL_RCC_PLLR_DIV_2);
	LL_RCC_PLL_Enable();
	LL_RCC_PLL_EnableDomain_SYS();
	while(LL_RCC_PLL_IsReady()!= 1)
	{
	}

	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
	while(LL_RCC_GetSysClkSource()!= LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
	{
	}
}

void SYSCLK_InitSystemClock(uint32_t _frequency)
{
	SYSCLK_t _clockType;
	uint32_t _flashLatency;
	uint32_t _msiRange= LL_RCC_MSIRANGE_6;
	uint32_t _ahbPrescaler= LL_RCC_SYSCLK_DIV_1;
	uint32_t _apb1Prescaler= LL_RCC_APB1_DIV_1;
	uint32_t _apb2Prescaler= LL_RCC_APB2_DIV_1;

	switch(_frequency)
	{
		case 80000000:/*PLL from msi, cant wakeup from stop2 mode properly*/
			_clockType= SYSCLK_MSI_PLL;
			_flashLatency= LL_FLASH_LATENCY_4;
			_apb1Prescaler= LL_RCC_APB1_DIV_2;/*i2c1 clock cannot use 80Mhz*, so we need to scale down.*/
			SYSCLK_InitWakeupClock= SYSCLK_WaitPLL;
			break;

		case 48000000:/*MSI highest range, able to wakeup from stop2 mode*/
			_clockType= SYSCLK_MSI;
			_flashLatency= LL_FLASH_LATENCY_2;
			_msiRange= LL_RCC_MSIRANGE_11;
			_apb1Prescaler= LL_RCC_APB1_DIV_2;/*i2c1 clock cannot use 48Mhz*, so we need to scale down.*/
			break;

		case 24000000:/*MSI ST LC demo clock, able to wakeup from stop2 mode*/
			_clockType= SYSCLK_MSI;
			_flashLatency= LL_FLASH_LATENCY_2;
			_msiRange= LL_RCC_MSIRANGE_9;
			break;

		case 16000000:/*HSI16, able to wakeup from stop2 mode*/
			_clockType= SYSCLK_HSI16;
			_flashLatency= LL_FLASH_LATENCY_0;
			break;

		default:/*MSI, able to wakeup from stop2 mode*/
			_frequency= 4000000;
		case 4000000:
			_clockType= SYSCLK_MSI;
			_flashLatency= LL_FLASH_LATENCY_0;
			_msiRange= LL_RCC_MSIRANGE_6;
			break;
	}

  LL_FLASH_SetLatency(_flashLatency);
  while(LL_FLASH_GetLatency()!= _flashLatency)
  {
  }

  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);

  if((SYSCLK_MSI== _clockType)|| (SYSCLK_MSI_PLL== _clockType))
  {
	  LL_RCC_MSI_Enable();
	  while(LL_RCC_MSI_IsReady() != 1)/* Wait till MSI is ready */
	  {
	  }

	  if(SYSCLK_MSI== _clockType)
	  {
		  LL_RCC_MSI_EnablePLLMode();
		  LL_RCC_MSI_EnableRangeSelection();
		  LL_RCC_MSI_SetRange(_msiRange);
		  LL_RCC_MSI_SetCalibTrimming(16);

		  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_MSI);
		  while(LL_RCC_GetSysClkSource()!= LL_RCC_SYS_CLKSOURCE_STATUS_MSI)/* Wait till System clock is ready */
		  {
		  }
	  }
	  else if((SYSCLK_MSI_PLL== _clockType)&& (80000000== _frequency))/*TODO: generic PLL selection for other frequencies*/
	  {
		  /* Main PLL configuration and activation */
		  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_MSI, LL_RCC_PLLM_DIV_1, 40, LL_RCC_PLLR_DIV_2);
		  LL_RCC_PLL_Enable();
		  LL_RCC_PLL_EnableDomain_SYS();
		  while(LL_RCC_PLL_IsReady()!= 1)
		  {
		  }

		  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
		  while(LL_RCC_GetSysClkSource()!= LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
		  {
		  }
	  }

	  LL_RCC_SetClkAfterWakeFromStop(LL_RCC_STOP_WAKEUPCLOCK_MSI);
  }
  else /*HSI16 clock*//*TODO HSI16 PLL*/
  {
	  LL_RCC_HSI_Enable();

	  while(LL_RCC_HSI_IsReady()!= 1)/* Wait till HSI is ready */
	  {
	  }
	  LL_RCC_HSI_SetCalibTrimming(16);
	  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);
	  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI)
	  {
	  }

	  LL_RCC_SetClkAfterWakeFromStop(LL_RCC_STOP_WAKEUPCLOCK_HSI);
  }

  LL_RCC_SetAHBPrescaler(_ahbPrescaler);
  LL_RCC_SetAPB1Prescaler(_apb1Prescaler);
  LL_RCC_SetAPB2Prescaler(_apb2Prescaler);
  LL_SetSystemCoreClock(_frequency);

  LL_PWR_EnableBkUpAccess();/*Access to RTC, RTC Backup and RCC CSR registers*/
  while(0== LL_PWR_IsEnabledBkUpAccess())
  {
  }
  //LL_RCC_ForceBackupDomainReset();/*don't reset backup domain*/
  //LL_RCC_ReleaseBackupDomainReset();/*don't reset backup domain*/
  if(0== LL_RCC_LSE_IsReady())
  {
	  LL_RCC_LSE_SetDriveCapability(LL_RCC_LSEDRIVE_LOW);
	  LL_RCC_LSE_Enable();
	  while(1!= LL_RCC_LSE_IsReady())
	  {
	  }
	  LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSE);
	  LL_RCC_EnableRTC();
  }

  LL_RCC_SetLPTIMClockSource(LL_RCC_LPTIM1_CLKSOURCE_LSE);

  LL_RCC_SetI2CClockSource(LL_RCC_I2C1_CLKSOURCE_PCLK1);
  LL_RCC_SetI2CClockSource(LL_RCC_I2C2_CLKSOURCE_PCLK1);
}

void HAL_IncTick(void) /*overwrite __weak funct*/
{
	/*config.system.*/++SYSCLK_rteSysTick;
}

uint32_t HAL_GetTick(void) /*overwrite __weak funct*/
{
	/*note: Most ARM lib using HAL_GetTick took accounts of the wrap:
	 * eg:
	 * delta = (current - start); // The math works and accounts for the wrap
	 * DON'T USE end = (start + x) type constructs.
	 * */
	return (uint32_t)/*config.system.*/SYSCLK_rteSysTick;
}

uint64_t SYSCLK_GetTimestamp_ms(void)
{
	return SYSCLK_rteSysTick;
}

uint32_t SYSCLK_GetTimestamp_s(void)
{
	return (uint32_t)(/*config.system.*/SYSCLK_rteSysTick/ 1000);
}

void SYSCLK_SyncTick(void)
{
	/*config.system.*/SYSCLK_rteSysTick= RTC_GetTick_ms();
}


