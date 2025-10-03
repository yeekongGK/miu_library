/******************************************************************************
 * File:        digitalsensor.c
 * Author:      Firmware Team
 * Created:     03-10-2025
 * Last Update: -
 *
 * Description:
 *   This file implements the driver for digital input sensors, including
 *   magnetic (reed switch) and tamper sensors. It configures GPIO pins as
 *   external interrupts to detect events such as magnet presence and tamper
 *   activation. The driver maintains counters for each sensor event and
 *   provides functions to initialize the sensors, retrieve event counts, and
 *   manage status flags.
 *
 * Notes:
 *   - The driver uses EXTI interrupts to handle sensor events asynchronously.
 *   - The `DIGISENSOR_Task` function should be called periodically to process
 *     pending interrupts.
 *
 * To Do:
 *   - Consolidate the repetitive logic for counter and flag management into a
 *     more generic structure or function.
 *
 ******************************************************************************/

#include "common.h"
#include "digitalsensor.h"

__IO ITStatus DIGISENSOR_PBMag_Interrupt= RESET;
__IO ITStatus DIGISENSOR_TamperIn_Interrupt= RESET;
__IO ITStatus DIGISENSOR_RSTamper_Interrupt= RESET;

static uint32_t DIGISENSOR_PBMag_Counter= 0;
static uint32_t DIGISENSOR_TamperIn_Counter= 0;
static uint32_t DIGISENSOR_RSTamper_Counter= 0;

static uint32_t DIGISENSOR_PBMag_TotalCounter= 0;
static uint32_t DIGISENSOR_TamperIn_TotalCounter= 0;
static uint32_t DIGISENSOR_RSTamper_TotalCounter= 0;

static bool bPBMagFlag= false;/*this flag has to be manuallay cleared*/
static bool bTamperInFlag= false;/*this flag has to be manuallay cleared*/
static bool bRSTamperFlag= false;/*this flag has to be manuallay cleared*/

void DIGISENSOR_PBMag_Init(void)
{
	LL_EXTI_InitTypeDef EXTI_InitStruct;

	SYS_EnablePortClock(PWR_BY_MAG_GPIO_Port);

	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
	LL_SYSCFG_SetEXTISource(PWR_BY_MAG_SYCFG_EXTI_Port, PWR_BY_MAG_SYCFG_EXTI_Line);

	LL_GPIO_SetPinPull(PWR_BY_MAG_GPIO_Port, PWR_BY_MAG_Pin, LL_GPIO_PULL_NO);
	LL_GPIO_SetPinMode(PWR_BY_MAG_GPIO_Port, PWR_BY_MAG_Pin, LL_GPIO_MODE_INPUT);

	EXTI_InitStruct.Line_0_31= PWR_BY_MAG_EXTI_Line;
	EXTI_InitStruct.LineCommand= ENABLE;
	EXTI_InitStruct.Mode= LL_EXTI_MODE_IT;
	EXTI_InitStruct.Trigger= LL_EXTI_TRIGGER_RISING;
	LL_EXTI_Init(&EXTI_InitStruct);

	NVIC_SetPriority(PWR_BY_MAG_EXTI_IRQn, SYS_CFG_EXTI_PRIORITY);
	NVIC_EnableIRQ(PWR_BY_MAG_EXTI_IRQn);
}

bool DIGISENSOR_PBMag_IsActive(void)
{
	return (false== LL_GPIO_IsInputPinSet(PWR_BY_MAG_GPIO_Port, PWR_BY_MAG_Pin));
}

void DIGISENSOR_PBMag_WaitActive(void)
{
	while(true== DIGISENSOR_PBMag_IsActive())/*wait until magnet switch is lifted*/
	{
	}
}

uint32_t DIGISENSOR_PBMag_GetCount(void)
{
	return DIGISENSOR_PBMag_TotalCounter- DIGISENSOR_PBMag_Counter;
}

uint32_t DIGISENSOR_PBMag_GetTotalCount(void)
{
	return DIGISENSOR_PBMag_TotalCounter;
}

void DIGISENSOR_PBMag_ClearCount(void)
{
	DIGISENSOR_PBMag_Counter= DIGISENSOR_PBMag_TotalCounter;
}

void DIGISENSOR_PBMag_ClearTotal(void)
{
	DIGISENSOR_PBMag_TotalCounter= 0;
}

bool DIGISENSOR_PBMag_GetFlag(void)
{
	return bPBMagFlag;
}

void DIGISENSOR_PBMag_SetFlag(bool _flag)
{
	bPBMagFlag= _flag;
}

void DIGISENSOR_TamperIn_Init(void)
{
	if(LCSENS_Mode== config.pulser.mode)
	{
		/*shared pin*/
		return;
	}

	LL_EXTI_InitTypeDef EXTI_InitStruct;

	SYS_EnablePortClock(TAMPER_IN_GPIO_Port);

	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
	LL_SYSCFG_SetEXTISource(TAMPER_IN_SYCFG_EXTI_Port, TAMPER_IN_SYCFG_EXTI_Line);

	LL_GPIO_SetPinPull(TAMPER_IN_GPIO_Port, TAMPER_IN_Pin, LL_GPIO_PULL_NO);
	LL_GPIO_SetPinMode(TAMPER_IN_GPIO_Port, TAMPER_IN_Pin, LL_GPIO_MODE_INPUT);

	EXTI_InitStruct.Line_0_31= TAMPER_IN_EXTI_Line;
	EXTI_InitStruct.LineCommand= ENABLE;
	EXTI_InitStruct.Mode= LL_EXTI_MODE_IT;
	EXTI_InitStruct.Trigger= LL_EXTI_TRIGGER_RISING;
	LL_EXTI_Init(&EXTI_InitStruct);

	NVIC_SetPriority(TAMPER_IN_EXTI_IRQn, SYS_CFG_EXTI_PRIORITY);
	NVIC_EnableIRQ(TAMPER_IN_EXTI_IRQn);
}

uint32_t DIGISENSOR_TamperIN_GetCount(void)
{
	return DIGISENSOR_TamperIn_TotalCounter- DIGISENSOR_TamperIn_Counter;
}

uint32_t DIGISENSOR_TamperIN_GetTotalCount(void)
{
	return DIGISENSOR_TamperIn_TotalCounter;
}

void DIGISENSOR_TamperIN_ClearCount(void)
{
	DIGISENSOR_TamperIn_Counter= DIGISENSOR_TamperIn_TotalCounter;
}

void DIGISENSOR_TamperIN_ClearTotal(void)
{
	DIGISENSOR_TamperIn_TotalCounter= 0;
}

bool DIGISENSOR_TamperIN_GetFlag(void)
{
	return bTamperInFlag;
}

void DIGISENSOR_TamperIN_SetFlag(bool _flag)
{
	bTamperInFlag= _flag;
}

void DIGISENSOR_RSTamper_Init(void)
{
	LL_EXTI_InitTypeDef EXTI_InitStruct;

	SYS_EnablePortClock(RS_TAMPER_GPIO_Port);

	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
	LL_SYSCFG_SetEXTISource(RS_TAMPER_SYCFG_EXTI_Port, RS_TAMPER_SYCFG_EXTI_Line);

	LL_GPIO_SetPinPull(RS_TAMPER_GPIO_Port, RS_TAMPER_Pin, LL_GPIO_PULL_UP);
	LL_GPIO_SetPinMode(RS_TAMPER_GPIO_Port, RS_TAMPER_Pin, LL_GPIO_MODE_INPUT);

	EXTI_InitStruct.Line_0_31= RS_TAMPER_EXTI_Line;
	EXTI_InitStruct.LineCommand= ENABLE;
	EXTI_InitStruct.Mode= LL_EXTI_MODE_IT;
	EXTI_InitStruct.Trigger= LL_EXTI_TRIGGER_FALLING;
	LL_EXTI_Init(&EXTI_InitStruct);

	NVIC_SetPriority(RS_TAMPER_EXTI_IRQn, SYS_CFG_EXTI_PRIORITY);
	NVIC_EnableIRQ(RS_TAMPER_EXTI_IRQn);
}

uint32_t DIGISENSOR_RSTamper_GetCount(void)
{
	return DIGISENSOR_RSTamper_TotalCounter- DIGISENSOR_RSTamper_Counter;
}

uint32_t DIGISENSOR_RSTamper_GetTotalCount(void)
{
	return DIGISENSOR_RSTamper_TotalCounter;
}

void DIGISENSOR_RSTamper_ClearCount(void)
{
	DIGISENSOR_RSTamper_Counter= DIGISENSOR_RSTamper_TotalCounter;
}

void DIGISENSOR_RSTamper_ClearTotal(void)
{
	DIGISENSOR_RSTamper_TotalCounter= 0;
}

bool DIGISENSOR_RSTamper_GetFlag(void)
{
	return bRSTamperFlag;
}

void DIGISENSOR_RSTamper_SetFlag(bool _flag)
{
	bRSTamperFlag= _flag;
}

void DIGISENSOR_Init(void)
{
	DIGISENSOR_PBMag_Init();
	DIGISENSOR_TamperIn_Init();
	DIGISENSOR_RSTamper_Init();
}

void DIGISENSOR_Task(void)
{
	if(SET== DIGISENSOR_PBMag_Interrupt)
	{
		DIGISENSOR_PBMag_Interrupt= RESET;
		DIGISENSOR_PBMag_TotalCounter++;
		bPBMagFlag= true;
		//DBG_Print("#stat:DIGISENSOR_PBMag_Interrupt >\r\n");
	}

	if(true== DIGISENSOR_PBMag_IsActive())
	{
		bPBMagFlag= true;
	}

	if(SET== DIGISENSOR_TamperIn_Interrupt)
	{
		DIGISENSOR_TamperIn_Interrupt= RESET;
		DIGISENSOR_TamperIn_TotalCounter++;
		bTamperInFlag= true;
		//DBG_Print("#stat:DIGISENSOR_TamperIn_Interrupt >\r\n");
	}

	if(SET== DIGISENSOR_RSTamper_Interrupt)
	{
		DIGISENSOR_RSTamper_Interrupt= RESET;
		DIGISENSOR_RSTamper_TotalCounter++;
		bRSTamperFlag= true;
		//DBG_Print("#stat:DIGISENSOR_RSTamper_Interrupt >\r\n");
	}
}

uint8_t DIGISENSOR_TaskState(void)
{
	return false;
}
