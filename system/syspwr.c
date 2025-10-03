/******************************************************************************
 * File:        syspwr.c
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file implements system-level power management functions. It includes
 *   routines for initializing GPIO pins to a low-power state (analog mode)
 *   and for controlling the power supply to various peripherals, such as the
 *   radio modem.
 *
 * Notes:
 *   - The power-up sequence considers the state of the magnetic tamper sensor
 *     to determine if the main power should be latched on.
 *
 * To Do:
 *   - The logic for handling power-on after a shutdown or BOR needs to be
 *     reviewed, as it's currently commented out.
 *
 ******************************************************************************/

#include "main.h"
#include "syspwr.h"
#include "ioctrl.h"
#include "digitalsensor.h"

void SYSPWR_InitGPIO(void)
{
	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Mode= LL_GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull= LL_GPIO_PULL_NO;

	/*except debug swdio(a13) and swclk(a14)*/
	GPIO_InitStruct.Pin= (	LL_GPIO_PIN_0| LL_GPIO_PIN_1| LL_GPIO_PIN_2| LL_GPIO_PIN_3| LL_GPIO_PIN_4|
							LL_GPIO_PIN_5| LL_GPIO_PIN_6| LL_GPIO_PIN_7| LL_GPIO_PIN_8| LL_GPIO_PIN_9|
							LL_GPIO_PIN_10|	LL_GPIO_PIN_11| LL_GPIO_PIN_12| LL_GPIO_PIN_15);
	//GPIO_InitStruct.Pin= LL_GPIO_PIN_ALL;
	LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
	LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	GPIO_InitStruct.Pin= LL_GPIO_PIN_ALL;
	LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
	LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*except debug LSE pins. Port C only have 3 pins*/
	GPIO_InitStruct.Pin= LL_GPIO_PIN_13;
	//GPIO_InitStruct.Pin= LL_GPIO_PIN_ALL;
	LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);
	LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	GPIO_InitStruct.Pin= LL_GPIO_PIN_ALL;
	LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOH);
	LL_GPIO_Init(GPIOH, &GPIO_InitStruct);
}

void SYSPWR_InitPower(void)
{
	//SYSPWR_InitGPIO();/*set pins to analog, do we need this?*/
	IOCTRL_MainPower_Init(true);

	IOCTRL_RadioPower_Init(false);
	//IOCTRL_RadioPowerBypass_Init(false);/*never set to 1 when RadioPower is off, cos the pin will suck current*/
	//IOCTRL_NFCPower_Init(false);
	//IOCTRL_SEOnOff_Init(false);

	//uint32_t _flags= LL_RTC_BAK_GetRegister(RTC, FS_RST_FLAGS_BKPReg);
	//if((0== _flags)|| (0!= (SHUTDOWN_Reset& _flags)))
	{
		/*power enable latched if only powered by magnet/reboot(not nfc powered)*/
		//DIGISENSOR_PBMag_Init();
		//IOCTRL_MainPower_Enable(DIGISENSOR_PBMag_IsActive());
		//DIGISENSOR_PBMag_WaitActive();
	}
}

void SYSPWR_EnableModem(bool _enable)
{
	if(true== _enable)
	{
		IOCTRL_RadioPower_Enable(true);
		IOCTRL_RadioPowerBypass_Enable(false);
	}
	else
	{
		IOCTRL_RadioPower_Enable(true);
		IOCTRL_RadioPowerBypass_Enable(true);
	}
}
