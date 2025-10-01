/*
 * ioctrl.h
 *
 *  Created on: 15 Dec 2018
 *      Author: muhammad.ahmad@georgekent.net
 */

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
