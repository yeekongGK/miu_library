/*
 * ioctrl.c
 *
 *  Created on: 15 Dec 2020
 *      Author: muhammad.ahmad@georgekent.net
 */
//#include "common.h"
#include "main.h"
#include "sys.h"
#include "cfg.h"
#include "ioctrl.h"
#include "pulser.h"

void IOCTRL_MainPower_Init(bool _enable)
{
	if(LCSENS_Mode== config.pulser.mode)
	{
		/*shared pin*/
		return;
	}

	LL_GPIO_InitTypeDef GPIO_InitStruct;

	SYS_EnablePortClock(PWR_EN_GPIO_Port);
	IOCTRL_MainPower_Enable(_enable);/*to set default value*/
	GPIO_InitStruct.Pin = PWR_EN_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	LL_GPIO_Init(PWR_EN_GPIO_Port, &GPIO_InitStruct);
}

void IOCTRL_MainPower_Enable(bool _enable)
{
	if(LCSENS_Mode== config.pulser.mode)
	{
		/*shared pin*/
		return;
	}

	if(true== _enable)
	{
		LL_GPIO_SetOutputPin(PWR_EN_GPIO_Port, PWR_EN_Pin);
	}
	else
	{
		LL_GPIO_ResetOutputPin(PWR_EN_GPIO_Port, PWR_EN_Pin);
	}
}

bool IOCTRL_MainPower_IsEnabled(void)
{
	if(LCSENS_Mode== config.pulser.mode)
	{
		/*shared pin*/
		return true;
	}

	return  LL_GPIO_IsOutputPinSet(PWR_EN_GPIO_Port, PWR_EN_Pin);
}

void IOCTRL_NFCPower_Init(bool _enable)
{
	LL_GPIO_InitTypeDef GPIO_InitStruct;

	SYS_EnablePortClock(NFC_PWR_GPIO_Port);
	IOCTRL_NFCPower_Enable(_enable);/*to set default value*/
	GPIO_InitStruct.Pin = NFC_PWR_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	LL_GPIO_Init(NFC_PWR_GPIO_Port, &GPIO_InitStruct);
}

void IOCTRL_NFCPower_Enable(bool _enable)
{
	if(true== _enable)
	{
		LL_GPIO_SetOutputPin(NFC_PWR_GPIO_Port, NFC_PWR_Pin);
	}
	else
	{
		LL_GPIO_ResetOutputPin(NFC_PWR_GPIO_Port, NFC_PWR_Pin);
	}
}

bool IOCTRL_NFCPower_IsEnable(void)
{
	return (1== LL_GPIO_IsOutputPinSet(NFC_PWR_GPIO_Port, NFC_PWR_Pin))? true: false;
}

void IOCTRL_RadioPower_Init(bool _enable)
{
#if CFG_DEVICE_USE_RADIO_SW == 1
	LL_GPIO_InitTypeDef GPIO_InitStruct;

	SYS_EnablePortClock(NB_PWR_EN_GPIO_Port);
	IOCTRL_RadioPower_Enable(_enable);
	GPIO_InitStruct.Pin = NB_PWR_EN_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	LL_GPIO_Init(NB_PWR_EN_GPIO_Port, &GPIO_InitStruct);
#endif
}

void IOCTRL_RadioPower_Enable(bool _enable)
{
#if CFG_DEVICE_USE_RADIO_SW == 1
	if(true== _enable)
	{
		LL_GPIO_SetOutputPin(NB_PWR_EN_GPIO_Port, NB_PWR_EN_Pin);
	}
	else
	{
		LL_GPIO_ResetOutputPin(NB_PWR_EN_GPIO_Port, NB_PWR_EN_Pin);
	}
#endif
}

void IOCTRL_RadioPowerBypass_Init(bool _enable)
{
	LL_GPIO_InitTypeDef GPIO_InitStruct;

	SYS_EnablePortClock(NB_PWR_BYPASS_GPIO_Port);
	IOCTRL_RadioPowerBypass_Enable(_enable);
	GPIO_InitStruct.Pin = NB_PWR_BYPASS_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	LL_GPIO_Init(NB_PWR_BYPASS_GPIO_Port, &GPIO_InitStruct);
}

void IOCTRL_RadioPowerBypass_Enable(bool _enable)
{
	if(true== config.nbiot.bypassBuckBoost)
	{
		_enable= true;
	}

	if(true== _enable)
	{
		LL_GPIO_SetOutputPin(NB_PWR_BYPASS_GPIO_Port, NB_PWR_BYPASS_Pin);
	}
	else
	{
		LL_GPIO_ResetOutputPin(NB_PWR_BYPASS_GPIO_Port, NB_PWR_BYPASS_Pin);
	}
}

void IOCTRL_RadioPowerSignal_Init(bool _enable)
{
	LL_GPIO_InitTypeDef GPIO_InitStruct;

	SYS_EnablePortClock(NB_ONOFF_GPIO_Port);
	IOCTRL_RadioPowerSignal_Enable(_enable);
	GPIO_InitStruct.Pin = NB_ONOFF_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	LL_GPIO_Init(NB_ONOFF_GPIO_Port, &GPIO_InitStruct);
}

void IOCTRL_RadioPowerSignal_Enable(bool _enable)
{
	if(true== _enable)
	{
		LL_GPIO_SetOutputPin(NB_ONOFF_GPIO_Port, NB_ONOFF_Pin);
	}
	else
	{
		LL_GPIO_ResetOutputPin(NB_ONOFF_GPIO_Port, NB_ONOFF_Pin);
	}
}

void IOCTRL_RadioReset_Init(bool _enable)
{
	LL_GPIO_InitTypeDef GPIO_InitStruct;

	SYS_EnablePortClock(NB_RESET_GPIO_Port);
	IOCTRL_RadioReset_Enable(_enable);
	GPIO_InitStruct.Pin = NB_RESET_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	LL_GPIO_Init(NB_RESET_GPIO_Port, &GPIO_InitStruct);
}

void IOCTRL_RadioReset_Enable(bool _enable)
{
	if(true== _enable)
	{
		LL_GPIO_SetOutputPin(NB_RESET_GPIO_Port, NB_RESET_Pin);
	}
	else
	{
		LL_GPIO_ResetOutputPin(NB_RESET_GPIO_Port, NB_RESET_Pin);
	}
}

void IOCTRL_RadioPSM_Init(bool _enable, IOCTRL_RadioPSMPin_t _type)
{
	LL_GPIO_InitTypeDef GPIO_InitStruct;

	switch(_type)
	{
		case IOCTRL_RadioPSMPin_NBIOT:
			SYS_EnablePortClock(NB_PSM_EINT_GPIO_Port);
			IOCTRL_RadioPSM_Enable(_enable, IOCTRL_RadioPSMPin_NBIOT);
			GPIO_InitStruct.Pin = NB_PSM_EINT_Pin;
			GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
			GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
			GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
			GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
			LL_GPIO_Init(NB_PSM_EINT_GPIO_Port, &GPIO_InitStruct);
			break;

		case IOCTRL_RadioPSMPin_RTCOUT:
			SYS_EnablePortClock(NB_PSM_EINT_GPIO_Port);
			IOCTRL_RadioPSM_Enable(_enable, IOCTRL_RadioPSMPin_RTCOUT);
			GPIO_InitStruct.Pin = NB_PSM_EINT_Pin;
			GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
			GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
			GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
			GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
			LL_GPIO_Init(NB_PSM_EINT_GPIO_Port, &GPIO_InitStruct);
			break;

		default:
			break;
	}
}

void IOCTRL_RadioPSM_Enable(bool _enable, IOCTRL_RadioPSMPin_t _type)
{
	switch(_type)
	{
		case IOCTRL_RadioPSMPin_NBIOT:
			if(true== _enable)
			{
				LL_GPIO_SetOutputPin(NB_PSM_EINT_GPIO_Port, NB_PSM_EINT_Pin);
			}
			else
			{
				LL_GPIO_ResetOutputPin(NB_PSM_EINT_GPIO_Port, NB_PSM_EINT_Pin);
			}
			break;

		case IOCTRL_RadioPSMPin_RTCOUT:
			LL_RTC_DisableWriteProtection(RTC);/* needed as RTC registers are write protected */
			if(true== _enable)
			{
				LL_RTC_CAL_SetOutputFreq(RTC, LL_RTC_CALIB_OUTPUT_1HZ);
			}
			else
			{
				LL_RTC_CAL_SetOutputFreq(RTC, LL_RTC_CALIB_OUTPUT_NONE);
			}
			LL_RTC_EnableWriteProtection(RTC);
			break;

		default:
			break;
	}
}

void IOCTRL_SEOnOff_Init(bool _enable)
{
	LL_GPIO_InitTypeDef GPIO_InitStruct;

	SYS_EnablePortClock(VBATT_ADC_GPIO_Port);
	IOCTRL_SEOnOff_Enable(_enable);
	GPIO_InitStruct.Pin = VBATT_ADC_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	LL_GPIO_Init(VBATT_ADC_GPIO_Port, &GPIO_InitStruct);
}

void IOCTRL_SEOnOff_Enable(bool _enable)
{
	if(true== _enable)
	{
		LL_GPIO_SetOutputPin(VBATT_ADC_GPIO_Port, VBATT_ADC_Pin);
	}
	else
	{
		LL_GPIO_ResetOutputPin(VBATT_ADC_GPIO_Port, VBATT_ADC_Pin);
	}
}

void IOCTRL_SEReset_Init(bool _enable)
{
	if(LCSENS_Mode== config.pulser.mode)
	{
		/*shared pin*/
		return;
	}

//	LL_GPIO_InitTypeDef GPIO_InitStruct;
//
//	SYS_EnablePortClock(DAC1_OUT1_GPIO_Port);
//	IOCTRL_SEReset_Enable(_enable);
//	GPIO_InitStruct.Pin = DAC1_OUT1_Pin;
//	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
//	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
//	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
//	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
//	LL_GPIO_Init(DAC1_OUT1_GPIO_Port, &GPIO_InitStruct);
}

void IOCTRL_SEReset_Enable(bool _enable)
{
	if(LCSENS_Mode== config.pulser.mode)
	{
		/*shared pin*/
		return;
	}

//	if(true== _enable)
//	{
//		LL_GPIO_SetOutputPin(DAC1_OUT1_GPIO_Port, DAC1_OUT1_Pin);
//	}
//	else
//	{
//		LL_GPIO_ResetOutputPin(DAC1_OUT1_GPIO_Port, DAC1_OUT1_Pin);
//	}
}

void IOCTRL_SIM_Init(void)
{
	LL_GPIO_InitTypeDef GPIO_InitStruct;

	SYS_EnablePortClock(SIM_DT_GPIO_Port);
	GPIO_InitStruct.Pin = SIM_DT_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	LL_GPIO_Init(SIM_DT_GPIO_Port, &GPIO_InitStruct);
}

bool IOCTRL_SIM_Detected(void)
{
	return (false== LL_GPIO_IsInputPinSet(SIM_DT_GPIO_Port, SIM_DT_Pin));
}

void IOCTRL_Init(void)
{

}

void IOCTRL_Task(void)
{

}

uint8_t IOCTRL_TaskState(void)
{
	return false;
}



