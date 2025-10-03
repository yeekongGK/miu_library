/******************************************************************************
 * File:        ioctrl.h
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file defines the public interface for the I/O control module. It
 *   provides function prototypes for initializing and controlling various
 *   hardware power signals and control lines, such as main power, NFC power,
 *   radio power, and Secure Element (SE) controls. It also includes functions
 *   for SIM card detection.
 *
 * Notes:
 *   - -
 *
 * To Do:
 *   - -
 *
 ******************************************************************************/

#ifndef SRC_IOCTRL_IOCTRL_H_
#define SRC_IOCTRL_IOCTRL_H_

#include "main.h"

typedef enum
{
	IOCTRL_RadioPSMPin_NBIOT,
	IOCTRL_RadioPSMPin_RTCOUT,
}IOCTRL_RadioPSMPin_t;

void IOCTRL_MainPower_Init(bool _enable);
void IOCTRL_MainPower_Enable(bool _enable);
bool IOCTRL_MainPower_IsEnabled(void);
void IOCTRL_NFCPower_Init(bool _enable);
void IOCTRL_NFCPower_Enable(bool _enable);
bool IOCTRL_NFCPower_IsEnable(void);
void IOCTRL_RadioPower_Init(bool _enable);
void IOCTRL_RadioPower_Enable(bool _enable);
void IOCTRL_RadioPowerBypass_Init(bool _enable);
void IOCTRL_RadioPowerBypass_Enable(bool _enable);
void IOCTRL_RadioPowerSignal_Init(bool _enable);
void IOCTRL_RadioPowerSignal_Enable(bool _enable);
void IOCTRL_RadioReset_Init(bool _enable);
void IOCTRL_RadioReset_Enable(bool _enable);
void IOCTRL_RadioPSM_Init(bool _enable, IOCTRL_RadioPSMPin_t _type);
void IOCTRL_RadioPSM_Enable(bool _enable, IOCTRL_RadioPSMPin_t _type);
void IOCTRL_SEOnOff_Init(bool _enable);
void IOCTRL_SEOnOff_Enable(bool _enable);
void IOCTRL_SEReset_Init(bool _enable);
void IOCTRL_SEReset_Enable(bool _enable);
void IOCTRL_SIM_Init(void);
bool IOCTRL_SIM_Detected(void);
void IOCTRL_Init(void);
void IOCTRL_Task(void);
uint8_t IOCTRL_TaskState(void);

#endif /* SRC_IOCTRL_IOCTRL_H_ */
