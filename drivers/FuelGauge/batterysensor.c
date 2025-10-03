/******************************************************************************
 * File:        batterysensor.c
 * Author:      Firmware Team
 * Created:     03-10-2025
 * Last Update: -
 *
 * Description:
 *   This file implements the driver for the battery sensor, which is based on
 *   the MAX17260 fuel gauge IC. It provides an interface to initialize the
 *   sensor, read various battery metrics such as state of charge (SOC),
 *   voltage, current, and temperature, and handle operational states like
 *   hibernate mode. It also includes logic for managing the coulomb counter
 *   and logging diagnostic data.
 *
 * Notes:
 *   - This driver interacts directly with the MAX17260 hardware driver.
 *   - It relies on RTC backup registers to maintain state, such as the
 *     coulomb counter overflow multiplier, across device resets.
 *
 * To Do:
 *   - The initialization sequence `BATTSENSOR_LWInit` contains many magic
 *     numbers that could be defined as named constants for clarity.
 *
 ******************************************************************************/

#include "common.h"
#include "batterysensor.h"
#include "max17260.h"

SENSOR_t *pConfig_SENS;

#ifdef COMMENT
static void BATTSENSOR_DisplayRegisters(void)
{
	/*debug*/
	int16_t _value;
	uint32_t _totalTime= 0;
	uint8_t _temp[2];
	DBG_Print("#stat:snsr>>>>>>>>>>>>>>>>>>>>>>\r\n");
	MAX17260_Register_ReadSingle(SOFT_WAKEUP, (uint16_t*)&_value);
	_temp[0]= _value>> 8, _temp[1]= _value;
	DBG_Print("#stat:snsr:SOFT_WAKEUP> 0x%s \r\n", UTILI_BytesToHexString(&_temp, 2, NULL));
	MAX17260_Register_ReadSingle(HIB_CFG, (uint16_t*)&_value);
	_temp[0]= _value>> 8, _temp[1]= _value;
	DBG_Print("#stat:snsr:HIB_CFG> 0x%s \r\n", UTILI_BytesToHexString(&_temp, 2, NULL));
	MAX17260_Register_ReadSingle(DESIGN_CAP, (uint16_t*)&_value);
	_temp[0]= _value>> 8, _temp[1]= _value;
	DBG_Print("#stat:snsr:DESIGN_CAP> 0x%s \r\n", UTILI_BytesToHexString(&_temp, 2, NULL));
	MAX17260_Register_ReadSingle(I_CHG_TERM, (uint16_t*)&_value);
	_temp[0]= _value>> 8, _temp[1]= _value;
	DBG_Print("#stat:snsr:I_CHG_TERM> 0x%s \r\n", UTILI_BytesToHexString(&_temp, 2, NULL));
	MAX17260_Register_ReadSingle(V_EMPTY, (uint16_t*)&_value);
	_temp[0]= _value>> 8, _temp[1]= _value;
	DBG_Print("#stat:snsr:V_EMPTY> 0x%s \r\n", UTILI_BytesToHexString(&_temp, 2, NULL));
	MAX17260_Register_ReadSingle(LEARN_CFG, (uint16_t*)&_value);
	_temp[0]= _value>> 8, _temp[1]= _value;
	DBG_Print("#stat:snsr:LEARN_CFG> 0x%s \r\n", UTILI_BytesToHexString(&_temp, 2, NULL));
	MAX17260_Register_ReadSingle(FILTER_CFG, (uint16_t*)&_value);
	_temp[0]= _value>> 8, _temp[1]= _value;
	DBG_Print("#stat:snsr:FILTER_CFG> 0x%s \r\n", UTILI_BytesToHexString(&_temp, 2, NULL));
	MAX17260_Register_ReadSingle(C_OFF, (uint16_t*)&_value);
	_temp[0]= _value>> 8, _temp[1]= _value;
	DBG_Print("#stat:snsr:C_OFF> 0x%s \r\n", UTILI_BytesToHexString(&_temp, 2, NULL));
	MAX17260_Register_ReadSingle(C_GAIN, (uint16_t*)&_value);
	_temp[0]= _value>> 8, _temp[1]= _value;
	DBG_Print("#stat:snsr:C_GAIN> 0x%s \r\n", UTILI_BytesToHexString(&_temp, 2, NULL));
	MAX17260_Register_ReadSingle(FULL_CAP, (uint16_t*)&_value);
	_temp[0]= _value>> 8, _temp[1]= _value;
	DBG_Print("#stat:snsr:FULL_CAP> 0x%s \r\n", UTILI_BytesToHexString(&_temp, 2, NULL));
	MAX17260_Register_ReadSingle(R_CELL, (uint16_t*)&_value);
	_temp[0]= _value>> 8, _temp[1]= _value;
	DBG_Print("#stat:snsr:R_CELL> 0x%s \r\n", UTILI_BytesToHexString(&_temp, 2, NULL));
	MAX17260_Register_ReadSingle(TIMER, (uint16_t*)&_value);
	_temp[0]= _value>> 8, _temp[1]= _value;
	DBG_Print("#stat:snsr:TIMER> 0x%s \r\n", UTILI_BytesToHexString(&_temp, 2, NULL));
	_totalTime= 0xFFFF& _value;
	MAX17260_Register_ReadSingle(TIMER_H, (uint16_t*)&_value);
	_temp[0]= _value>> 8, _temp[1]= _value;
	DBG_Print("#stat:snsr:TIMER_H> 0x%s \r\n", UTILI_BytesToHexString(&_temp, 2, NULL));
	_totalTime|= _value<< 16;
	MAX17260_Register_ReadSingle(CONFIG, (uint16_t*)&_value);
	_temp[0]= _value>> 8, _temp[1]= _value;
	DBG_Print("#stat:snsr:CONFIG> 0x%s \r\n", UTILI_BytesToHexString(&_temp, 2, NULL));
	MAX17260_Register_ReadSingle(CONFIG2, (uint16_t*)&_value);
	_temp[0]= _value>> 8, _temp[1]= _value;
	DBG_Print("#stat:snsr:CONFIG2> 0x%s \r\n", UTILI_BytesToHexString(&_temp, 2, NULL));
	MAX17260_Register_ReadSingle(MODEL_CFG, (uint16_t*)&_value);
	_temp[0]= _value>> 8, _temp[1]= _value;
	DBG_Print("#stat:snsr:MODEL_CFG> 0x%s \r\n", UTILI_BytesToHexString(&_temp, 2, NULL));
	DBG_Print("#stat:snsr: -- \r\n", UTILI_BytesToHexString(&_temp, 2, NULL));
	MAX17260_Register_ReadSingle(QH, (uint16_t*)&_value);
	DBG_Print("#stat:snsr:QH> %d \r\n", _value);
	MAX17260_Register_ReadSingle(QH, (uint16_t*)&_value);
	float _totalTime_f= ((float)_totalTime/ 1.0f)* 0.1758f;
	DBG_Print("#stat:snsr:Total time> %f seconds.\r\n", _totalTime_f );
	DBG_Print("#stat:snsr:Total time> %f minutes.\r\n", _totalTime_f/ 60);
	DBG_Print("#stat:snsr:Total time> %f hours.\r\n", _totalTime_f/ 3600);
	DBG_Print("#stat:snsr:Total time> %f days.\r\n", _totalTime_f/ (3600* 24));
	DBG_Print("#stat:snsr:Total time> %f months.\r\n", _totalTime_f/ (30* 3600* 24));
	DBG_Print("#stat:snsr>>>>>>>>>>>>>>>>>>>>>>\r\n");
}
#endif

#ifdef COMMENT
static void BATTSENSOR_SetCoulombCounter(void)
{
	/*init MAX17260 as coulomb counter*/
	uint16_t _value= 0;

	/*3. Set maximum design capacity.*/
	_value= 0xFFFF;
	MAX17260_Register_WriteSingle(DESIGN_CAP, _value);
	MAX17260_Register_ReadSingle(DESIGN_CAP, &_value);

	/*1. Write 0x07 to LearnCfg to let coulomb counter dominate.*/
	MAX17260_Register_ReadSingle(LEARN_CFG, &_value);
	_value&= ~(0b111<< 4);
	_value|= (0x7<< 4);
	MAX17260_Register_WriteSingle(LEARN_CFG, _value);

	/*2. Set SOC % threshold for alert(notify as overflow, increase multiplication counter).*/
	_value= (uint16_t)(98<< 8)| 2;/*if the percentage more than 99*/
	MAX17260_Register_WriteSingle(S_ALRT_TH, _value);

	/*3. Set alert for RepSOC register.*/
	MAX17260_Register_ReadSingle(MISC_CFG, &_value);
	_value&= ~(0b11<< 0);
	_value|= (0x00<< 0);
	MAX17260_Register_WriteSingle(MISC_CFG, _value);

	/* Enable Alert*/
	MAX17260_Register_ReadSingle(CONFIG, &_value);
	_value&= ~(0b1<< 3);
	_value|= (0x1<< 3);
	MAX17260_Register_WriteSingle(CONFIG, _value);
}
#endif

static void BATTSENSOR_LWInit(void)
{
	int16_t _value;
	// SOFT_WAKEUP `0x0F` → Sends a **soft-wakeup command** (used to bring the device out of hibernate mode).
	MAX17260_Register_WriteSingle(0x60, 0x0F);		

	// CONFIG2 Regester Bits:
	// D13: DPEn: Turns power calculations on or off. If off, you can use the power registers as extra memory.
	// D8-D12: POWR: Sets the averaging speed for power readings.
	// D7: dSOCen: Enables or disables the alert for battery percentage changes.
	// D5: TAlrten: Enables or disables temperature alerts.
	// D4: LDMdl: A flag used to load a new battery profile. Set 1 clears it to 0 when finished.
	// D1-D2: DRCfg: Sets how long the device must "rest" before entering deep sleep.
	// D1: CPMode: Switches time-to-empty math between two load types:
	// 	0: For loads with constant current (like an LED).  time-to-empty
	// 		Example: “I’m drawing 500 mA right now → at this rate, I’ll be empty in X hours.”
	// 	1: For loads with constant power (like a smartphone processor).
	// 		Example: A DC-DC converter load usually demands the same wattage regardless of battery voltage 
	// 		→ as battery voltage drops, current increases.
	MAX17260_Register_WriteSingle(CONFIG2, 0x01);		

	HAL_Delay(500);		//

	/*exit hibernate mode sequence*/
	// To wake and exit hibernate:
	// 	1. Write HibCfg = 0x0000.
	// 	2. Soft-Wakeup Command. Write Command Register (SOFT_WAKEUP) to 0x0090.
	// 	3. Clear Command. Write Command Register (SOFT_WAKEUP) to 0x0000.
	// 	4. Eventually restore HibCfg to again allow automatic hibernate decisions.
	MAX17260_Register_WriteSingle(HIB_CFG, 0x00);		// `0x00` → Clear hibernate configuration, ensures device is in **active mode**.
	MAX17260_Register_WriteSingle(SOFT_WAKEUP, 0x90);	// `0x90` → Forces the gauge out of hibernate temporarily.
	MAX17260_Register_WriteSingle(SOFT_WAKEUP, 0x00);	// `0x00` → Ends the wakeup command sequence.


	/*Temporary Initialization for Model Configuration */
	MAX17260_Register_WriteSingle(DESIGN_CAP, 0x7FF8);		// `0x7FF8` (~32760 mAh if LSB=0.5mAh) Temporary capacity placeholder (maximum).
	MAX17260_Register_WriteSingle(I_CHG_TERM, 0x0000);		// `0x0000`	Charge termination current disabled temporarily.

	/*V_EMPTY Regester Bit Fields*/
	// D15..D8 → VE (Empty Voltage Target)
	// 	Resolution: 10mV/LSB
	// 	Range: 0–5.11V
	// 	Default = 3.3V → The gauge declares 0% SOC when the cell under load falls to 3.3V.
	// D7..D0 → VR (Recovery Voltage)
	// 	Resolution: 40mV/LSB
	// 	Range: 0–5.08V
	// 	Default = 3.88V → Once the cell voltage recovers above 3.88V, the gauge clears the “empty” 
	// 					  condition and resumes normal SOC tracking.
	// Example: 0x9661 = 1001 0110 0110 0001b
	// 		VE (upper 8 bits, D15..D8) = 0x96 = 150 decimal
	// 				VE = 150 × 10mV = 1500mV = 1.50V
	// 		VR (lower 8 bits, D7..D0) = 0x61 = 97 decimal
	// 				VR = 97 × 40mV = 3880mV = 3.88V
	MAX17260_Register_WriteSingle(V_EMPTY, 0x9661);			// `0x9661`	Sets **empty & recovery thresholds** (custom config).
	MAX17260_Register_WriteSingle(AT_Q_RESIDUAL, 0x8000);	// `0x8000`	Forces AtRate residual capacity estimate reset.

	/*Wait for AtQResidual to Clear*/
	do{
		MAX17260_Register_ReadSingle(AT_Q_RESIDUAL, &_value);		//
	}while(0x00!= _value);		// 0 → means model reload sequence completed.

	/*Re-enable Hibernate after Model Load*/
	// 0x870C
	// Hibernate Entry Time : 0.000 – 2.812 seconds
	// Hibernate Threshold  : 3750.00 mA (FullCap=2000 mAh)
	// Task Period in Hibernate: 5.616 seconds
	// Hibernate Exit Time  : 786.2 seconds (13.1 minutes)
	MAX17260_Register_WriteSingle(HIB_CFG, 0x870C);	// `0x870C`	Restores hibernate configuration (automatic entry/exit conditions enabled).

	/*Status Reset*/
	// STATUS Register Bits:
	// D1: POR (Power-On Reset): This flag is 1 if the chip just powered up or was reset. You need to clear it to 0 to detect the next reset.
	// D2 & D6: Imn / Imx (Current Alerts): These flags are set to 1 if the battery current is too low (Imn) or too high (Imx).
	// D3: Bst (Battery Status): Shows if a battery is connected.
	// 	0 = Battery is present.
	// 	1 = Battery is absent.
	// D7: dSOCi (SOC 1% Change): This flag is set to 1 every time the battery percentage changes by a whole number (e.g., from 88.9% to 89.0%). You need to clear it after reading.
	// D8 & D12: Vmn / Vmx (Voltage Alerts): These flags are set to 1 if the battery voltage is too low (Vmn) or too high (Vmx).
	// D9 & D13: Tmn / Tmx (Temperature Alerts): These flags are set to 1 if the temperature is too low (Tmn) or too high (Tmx).
	// D10 & D14: Smn / Smx (State of Charge Alerts): These flags are set to 1 if the battery's charge percentage is too low (Smn) or too high (Smx).
	// D11: Bi (Battery Insertion): This flag is set to 1 when a battery is plugged in. You need to clear it to detect the next insertion.
	// D15: Br (Battery Removal): This flag is set to 1 when a battery is removed. You need to clear it to detect the next removal.
	MAX17260_Register_ReadSingle(STATUS, &_value);	// read → get initial flags.
	MAX17260_Register_WriteSingle(STATUS, 0x8080);	// `0x8080` → Clears **POR (power-on reset)** and alert flags to start fresh.

	/*Final Configuration Values*/
	MAX17260_Register_WriteSingle(DESIGN_CAP, 0x7FF8);	//– Set actual design capacity (~32760 mAh if LSB=0.5mAh)
	
	// I_CHG_TERM 
	// 0x1900 = 6400 (decimal)
	// LSB = 1.5625e-6 / 0.1 = 0.0015625 A = 15.625 µA
	// IChgTerm = 6400 × 15.625 µA
    //       ≈ 0.1 A (charge termination current threshold.)
	MAX17260_Register_WriteSingle(I_CHG_TERM, 0x1900);	// 0x1900 → sets charge termination current (depends on Rsense).

	// V_EMPTY: 0x783F = 0111 1000 0011 1111b
	// VE (Empty Voltage Target): 2.400 V
  	// VR (Recovery Voltage)    : 2.520 V
	MAX17260_Register_WriteSingle(V_EMPTY, 0x783F);		// → new **empty voltage thresholds**.
	MAX17260_Register_WriteSingle(LEARN_CFG, 0x44F6);	// = 0x44F6 = Coulomb counter dominates, 0x4486 = Voltage model dominates
	MAX17260_Register_WriteSingle(FILTER_CFG, 0xCEA4);	// = configures averaging filters for voltage, current, SOC, temp.
	MAX17260_Register_WriteSingle(C_OFF, 0xFFFF);		// = Current offset calibration. `0xFFFF` means adjust calibration baseline.
	MAX17260_Register_WriteSingle(C_GAIN, 0x0400);		// = Current gain calibration. Default 0x0400 = unity gain.
	MAX17260_Register_WriteSingle(R_CELL, 0x0290);		// = Initial internal resistance value (~160 mΩ).
	MAX17260_Register_WriteSingle(TIMER, 0x0000);		// = reset the general-purpose 32-bit timer.
	MAX17260_Register_WriteSingle(TIMER_H, 0x0000);		// = reset the general-purpose 32-bit timer.
	MAX17260_Register_WriteSingle(CONFIG, 0x2210);		// = main configuration (alerts enable, temp source, SOC change interrupt).
	MAX17260_Register_WriteSingle(CONFIG2, 0x0658);		// = secondary configuration (AtRate enable, DPEn, serial number features).
	MAX17260_Register_WriteSingle(MODEL_CFG, 0x8000); 	// `0x8000` reloads the EZ battery model into memory.	This step is **mandatory after parameter configuration**.
	MAX17260_Register_WriteSingle(HIB_CFG, 0x8F0C);		//= final hibernate configuration for normal operation.

	// ✅ **Summary**
	// This sequence:

	// 1. Wakes device from hibernate
	// 2. Clears POR flags
	// 3. Loads temporary values
	// 4. Triggers model reload via `MODEL_CFG = 0x8000`
	// 5. Writes battery design parameters (capacity, termination current, empty voltage)
	// 6. Configures filters & learning algorithm
	// 7. Restores hibernate config

//	/*debug*/
//	uint8_t _temp[2];
//	MAX17260_Register_WriteSingle(I_CHG_TERM, 0);
//	MAX17260_Register_WriteSingle(FULL_SOC_THR, 0x5000);
//	MAX17260_Register_WriteSingle(FULL_CAP_REP, 0x9474);
//	MAX17260_Register_ReadSingle(FULL_CAP_REP, (uint16_t*)&_value);
//
//	MAX17260_Register_WriteSingle(FULL_CAP_NOM, 0x9474);
//	MAX17260_Register_ReadSingle(FULL_CAP_NOM, (uint16_t*)&_value);
//
//	MAX17260_Register_WriteSingle(FULL_CAP, 0x9474);
//	MAX17260_Register_ReadSingle(FULL_CAP, (uint16_t*)&_value);
//
//	_temp[0]= _value>> 8, _temp[1]= _value;
//	DBG_Print("#stat:snsr:FULL_CAP> 0x%s \r\n", UTILI_BytesToHexString(&_temp, 2, NULL));
//
//	MAX17260_Register_WriteSingle(DESIGN_CAP, BATTSENSOR_QH_OVERFLOW_VALUE- 1);
//	MAX17260_Register_WriteSingle(I_CHG_TERM, 0x1900);
//	MAX17260_Register_WriteSingle(V_EMPTY, 0x783F);
//	MAX17260_Register_WriteSingle(LEARN_CFG, 0x44F6);
//	MAX17260_Register_WriteSingle(FILTER_CFG, 0xCEA4);
//	MAX17260_Register_WriteSingle(C_OFF, 0xFFFF);
//	MAX17260_Register_WriteSingle(C_GAIN, 0x0400);
//	MAX17260_Register_WriteSingle(FULL_CAP, 0x9474);
//	MAX17260_Register_WriteSingle(R_CELL, 0x0290);
//	MAX17260_Register_WriteSingle(TIMER, 0x0000);
//	MAX17260_Register_WriteSingle(TIMER_H, 0x0000);
//	MAX17260_Register_WriteSingle(CONFIG, 0x2210);
//	MAX17260_Register_WriteSingle(CONFIG2, 0x0658);
//	MAX17260_Register_WriteSingle(MODEL_CFG, 0x8000);
//	MAX17260_Register_WriteSingle(HIB_CFG, 0x8F0C);
//
//	/*reset*/
//	MAX17260_Register_WriteSingle(0x60, 0x0F);
//	MAX17260_Register_WriteSingle(0xBB, 0x01);
//
//	HAL_Delay(500);
//
//	MAX17260_Register_WriteSingle(0x60, 0x90);
//	MAX17260_Register_WriteSingle(0xBA, 0x00);
//	MAX17260_Register_WriteSingle(0x60, 0x00);
//	MAX17260_Register_WriteSingle(0x18, 0x7FF8);
//	MAX17260_Register_WriteSingle(0x1E, 0x00);
//	MAX17260_Register_WriteSingle(0x3A, 0x9661);
//	MAX17260_Register_WriteSingle(0xDB, 0x8000);
//	do{
//		MAX17260_Register_ReadSingle(0xDB, &_value);
//	}while(0x00!= _value);
//	MAX17260_Register_WriteSingle(0xBA, 0x870C);
//	MAX17260_Register_ReadSingle(0x00, &_value);
//	MAX17260_Register_WriteSingle(0x00, 0x8080);
}

void BATTSENSOR_QHOverflowCallback(void)
{
	pConfig_SENS->battery.rteQHMultiplier++;
	LL_RTC_BAK_SetRegister(RTC, SNSR_QH_MULT_BKPReg, pConfig_SENS->battery.rteQHMultiplier);

	//DBG_Print("#stat:snsr:rteQHMultiplier> %u.\r\n", pConfig_SENS->battery.rteQHMultiplier);

	DIAG_Code(QH_MULTIPLIER_SensorDCode, pConfig_SENS->battery.rteQHMultiplier);

	BATTSENSOR_LogRegisters();
}

bool BATTSENSOR_IsPOR(void)
{
	uint16_t _value;

	MAX17260_Register_ReadSingle(STATUS, &_value);

	return ((_value& 0b10)!= 0)? true: false;
}

void BATTSENSOR_ClearPOR(void)
{
	uint16_t _value;

	MAX17260_Register_ReadSingle(STATUS, &_value);
	MAX17260_Register_WriteSingle(STATUS, _value& (~(0b10)));
}

bool BATTSENSOR_IsDataNotReady(void)
{
	uint16_t _value;

	MAX17260_Register_ReadSingle(F_STAT, &_value);

	return ((_value& 0b1)!= 0)? true: false;
}

uint16_t BATTSENSOR_GetQH(void)
{
	static uint16_t _value= 0;
	static uint32_t _lastTs= 0;
	uint32_t _currTS= SYS_GetTimestamp_s();

	if(_currTS== _lastTs) /*simple backoff to reduce load on shared i2c line. NFC writing to mailbox may fail when I2C busy.*/
	{
		return _value;
	}
	_lastTs= _currTS;

	MAX17260_Register_ReadSingle(QH, &_value);

	if((pConfig_SENS->battery.rtePrevQH!= _value)&& (pConfig_SENS->battery.rtePrevQH< _value))
	{
		DBG_Print("DH Overflow: rtePrevQH(%d) _value(%d)\r\n", pConfig_SENS->battery.rtePrevQH, _value);
		DIAG_Code(QH_OVERFLOWED_SensorDCode, (_value<< 16)| pConfig_SENS->battery.rtePrevQH);

		BATTSENSOR_QHOverflowCallback(); /*manually detecting overflow*/
	}

	pConfig_SENS->battery.rtePrevQH= _value;
	LL_RTC_BAK_SetRegister(RTC, SNSR_QH_PREV_BKPReg, (0== _value)? 1: pConfig_SENS->battery.rtePrevQH);/*we are not writing zero to detect if RTC memory reset*/

	return _value;
}

uint32_t BATTSENSOR_GetTotalQH(void)
{
	uint16_t _value= BATTSENSOR_GetQH();

	return (0x10000* pConfig_SENS->battery.rteQHMultiplier)+ (0x10000- _value);
}

uint16_t BATTSENSOR_GetRepCap(void)
{
	uint16_t _value;

	MAX17260_Register_ReadSingle(REP_CAP, &_value);

	return _value;
}

uint16_t BATTSENSOR_GetRepSOC(void)
{
	uint16_t _value;

	MAX17260_Register_ReadSingle(REP_SOC, &_value);

	return _value;
}

float BATTSENSOR_GetVCell(void)
{
	uint16_t _value;
	float _oneUnit= 0.000078125;

	MAX17260_Register_ReadSingle(V_CELL, &_value);

	return ((_value* _oneUnit)* 1000);
}

float BATTSENSOR_GetAvgVCell(void)
{
	uint16_t _value;
	float _oneUnit= 0.000078125;

	MAX17260_Register_ReadSingle(AVG_V_CELL, &_value);

	return ((_value* _oneUnit)* 1000);
}

float BATTSENSOR_GetCurrent(void)
{
	int16_t _value;
	float _oneUnit= (-0.0000015625/ BATTSENSOR_CFG_RSENSE_VALUE);/*5uV / RSense*/

	MAX17260_Register_ReadSingle(CURRENT, (uint16_t*)&_value);

	return ((_value* _oneUnit)* 1000000);
}

float BATTSENSOR_GetAvgCurrent(void)
{
	int16_t _value;
	float _oneUnit= (-0.0000015625/ BATTSENSOR_CFG_RSENSE_VALUE);/*5uV / RSense*/
	float _current;

	MAX17260_Register_ReadSingle(AVG_CURRENT, (uint16_t*)&_value);

	_current= ((_value* _oneUnit)* 1000000);

	if(0< _current)
	{
		if(pConfig_SENS->battery.rteMinCurrent> _current)
		{
			pConfig_SENS->battery.rteMinCurrent= _current;
		}
		else if(pConfig_SENS->battery.rteMaxCurrent< _current)
		{
			pConfig_SENS->battery.rteMaxCurrent= _current;
		}
	}

	return _current;
}

float BATTSENSOR_GetMinCurrent(void)
{
	return pConfig_SENS->battery.rteMinCurrent;
}

float BATTSENSOR_GetMaxCurrent(void)
{
	return pConfig_SENS->battery.rteMaxCurrent;
}

float BATTSENSOR_GetCurrentUsed(void)
{
	float _counter= BATTSENSOR_GetTotalQH()/ 1.0;
	float _oneUnit= (0.000005/ BATTSENSOR_CFG_RSENSE_VALUE);/*5uV / RSense*/

	return (_counter* _oneUnit);
}

float BATTSENSOR_GetCapacity(void)
{
	return (BATTSENSOR_CFG_DESIGN_CAPACITY- BATTSENSOR_GetCurrentUsed());
}

float BATTSENSOR_GetCapacityPercentage(void)
{
	//BATTSENSOR_DisplayRegisters();/*for debug only*/
	return ((BATTSENSOR_GetCapacity()/ BATTSENSOR_CFG_DESIGN_CAPACITY)* 100);
}

float BATTSENSOR_GetTemperature(void)
{
	int16_t _value;
	float _oneUnit= 1/ 256.0;

	MAX17260_Register_ReadSingle(TEMP, (uint16_t*)&_value);

	return (_value* _oneUnit);
}

float BATTSENSOR_GetAvgTemperature(void)
{
	int16_t _value;
	float _oneUnit= 1/ 256.0;

	MAX17260_Register_ReadSingle(AVG_TA, (uint16_t*)&_value);

	return (_value* _oneUnit);
}

float BATTSENSOR_GetTimer(void)
{
	uint16_t _value;
	uint32_t _totalTime= 0;

	MAX17260_Register_ReadSingle(TIMER, &_value);
	_totalTime= 0xFFFF& _value;

	MAX17260_Register_ReadSingle(TIMER_H, &_value);
	_totalTime|= _value<< 16;

	return (_totalTime/ 1.0f)* 0.1758f;
}

void BATTSENSOR_Hibernate(bool _enable, BATTSENSOR_BattSensorGain_t _gain)
{
	MAX17260_Register_WriteSingle(HIB_CFG, (true== _enable)? 0x8F0C: 0x810C);
	MAX17260_Register_WriteSingle(C_GAIN, (LOW_BattSensorGain== _gain)? 0x02BC: 0x0400);
}

void BATTSENSOR_LogRegisters(void)
{
	DIAG_Code(SOFT_WAKEUP_HIB_CFG_SensorDCode, (MAX17260_Register_ReadSingleFast(SOFT_WAKEUP)<< 16)| MAX17260_Register_ReadSingleFast(HIB_CFG));
	DIAG_Code(DESIGN_CAP_I_CHG_TERM_SensorDCode, (MAX17260_Register_ReadSingleFast(DESIGN_CAP)<< 16)| MAX17260_Register_ReadSingleFast(I_CHG_TERM));
	DIAG_Code(V_EMPTY_LEARN_CFG_SensorDCode, (MAX17260_Register_ReadSingleFast(V_EMPTY)<< 16)| MAX17260_Register_ReadSingleFast(LEARN_CFG));
	DIAG_Code(FILTER_CFG_C_OFF_SensorDCode, (MAX17260_Register_ReadSingleFast(FILTER_CFG)<< 16)| MAX17260_Register_ReadSingleFast(C_OFF));
	DIAG_Code(C_GAIN_FULL_CAP_SensorDCode, (MAX17260_Register_ReadSingleFast(C_GAIN)<< 16)| MAX17260_Register_ReadSingleFast(FULL_CAP));
	DIAG_Code(R_CELL_SensorDCode, MAX17260_Register_ReadSingleFast(R_CELL));
	DIAG_Code(TIMER_TIMER_H_SensorDCode, (MAX17260_Register_ReadSingleFast(TIMER)<< 16)| MAX17260_Register_ReadSingleFast(TIMER_H));
	DIAG_Code(CONFIG_CONFIG2_SensorDCode, (MAX17260_Register_ReadSingleFast(CONFIG)<< 16)| MAX17260_Register_ReadSingleFast(CONFIG2));
	DIAG_Code(MODEL_CFG_QH_SensorDCode, (MAX17260_Register_ReadSingleFast(MODEL_CFG)<< 16)| MAX17260_Register_ReadSingleFast(QH));
}

void BATTSENSOR_Init(SENSOR_t *_config)
{
	pConfig_SENS= _config;

	MAX17260_Init();
	if(true== BATTSENSOR_IsPOR())
	{
		UTILI_WaitUntil(false, BATTSENSOR_IsDataNotReady(), NULL, 1000UL);
		//BATTSENSOR_SetCoulombCounter();
		BATTSENSOR_LWInit();
		BATTSENSOR_ClearPOR();
	}

	/*we use RTC, but it was also not reliable, thus we also use config*/
	/*update, below also failed (seems like the sensor also reset). So we get prev from init*/
//	if(0!= LL_RTC_BAK_GetRegister(RTC, SNSR_QH_PREV_BKPReg))/*if 0 we assume MCU RTC reset, cos we're not writing 0*/
//	{
//		pConfig_SENS->battery.rtePrevQH= LL_RTC_BAK_GetRegister(RTC, SNSR_QH_PREV_BKPReg);
//	}
	MAX17260_Register_ReadSingle(QH, &(pConfig_SENS->battery.rtePrevQH));
	uint32_t _savedMultiplier= pConfig_SENS->battery.rteQHMultiplier;
	if(0!= LL_RTC_BAK_GetRegister(RTC, SNSR_QH_MULT_BKPReg))
	{
		pConfig_SENS->battery.rteQHMultiplier= LL_RTC_BAK_GetRegister(RTC, SNSR_QH_MULT_BKPReg);
		if(0>= BATTSENSOR_GetCapacityPercentage())/*battery less than 0, possibly the RTC register not valid*/
		{
			pConfig_SENS->battery.rteQHMultiplier= _savedMultiplier;
		}
	}

	if(0>= BATTSENSOR_GetCapacityPercentage())/*still not valid*/
	{
		pConfig_SENS->battery.rteQHMultiplier= 1;/*set dummy*/
	}
}

void BATTSENSOR_Task(void)
{
	MAX17260_Task();
}

uint8_t BATTSENSOR_TaskState(void)
{
	return MAX17260_TaskState();
}
