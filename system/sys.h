/*
 * sys.h
 *
 *  Created on: 18 Dec 2020
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef INC_SYS_H_
#define INC_SYS_H_

#include "rtc.h"
#include "main.h"

#include "sysclk.h"
#include "syspwr.h"
#include <stdbool.h>

typedef enum
{
	NO_TASK_TaskId= 0,
	SYS_TaskId,
	M95M01_TaskId,
	BC66_PHY_TaskId,
	BC66_LINK_TaskId,
	PULSER_TaskId,
	EXTI_TaskId,
	METER_Log_TaskId,
	NBIOT_TaskId,
	NBIOT_GKCOAP_TaskId,/*this is actually a subtask*/
	NBIOT_GKCOAP_REPORT_TaskId,/*this is actually a subtask of a subtask*/
	NBIOT_GKCOAP_PKT_TaskId,/*this is actually a subtask*/
	NBIOT_LWOBJ_TaskId,/*this is actually a subtask*/
	NFC_TaskId,
	RTC_TaskId,
	MSG_TaskId,
	WMBUS_TaskId,
	DBG_TaskId,
	SENSOR_TaskId,
	FLOWSENSOR_TaskId,
	MCUADC_TaskId,
	DIAG_TaskId,
	ALARM_TaskId,
	FAILSAFE_TaskId,
	LOGGER_TaskId,
	NBIOT_LWM2M_TaskId,/*this is actually a subtask*/
	MAX_TaskId
}SYS_TaskId_t;

typedef enum
{
	FS_RST_PC_BKPReg= LL_RTC_BKP_DR0,/*failsafe reset program counter*/
	FS_RST_FLAGS_BKPReg= LL_RTC_BKP_DR1,/*failsafe reset flags(type(s))*/
	FS_RST_TIMESTAMP_BKPReg= LL_RTC_BKP_DR2,/*failsafe reset timestamp*/
	FS_TASK_SLEEP_STATUS_BKPReg= LL_RTC_BKP_DR3,/*SYS sleep status, can check which task isnt sleeping.*/
	RTC_SYNC_BKPReg= LL_RTC_BKP_DR4, /*RTC sync word*/
	PULSER_VALUE_BKPReg= LL_RTC_BKP_DR5,/*current pulser value*/
	PULSER_ERRPATT_CNT_BKPReg= LL_RTC_BKP_DR6,/*pulser error pattern count (for tracsens)*/
	PULSER_CNTR_DIR_BKPReg= LL_RTC_BKP_DR7,/*pulser counter direction(for tracsens)*/
	OPERATIONAL_FLAGS_BKPReg= LL_RTC_BKP_DR8,/*general operational flags*/
	SNSR_QH_PREV_BKPReg= LL_RTC_BKP_DR9,/*battery sensor qh overflow monitoring flag*/
	SNSR_QH_MULT_BKPReg= LL_RTC_BKP_DR10,/*battery sensor qh multiplier*/
//	_BKPReg= LL_RTC_BKP_DR11,
//	_BKPReg= LL_RTC_BKP_DR12,
//	_BKPReg= LL_RTC_BKP_DR13,
//	_BKPReg= LL_RTC_BKP_DR14,
//	_BKPReg= LL_RTC_BKP_DR15,
//	_BKPReg= LL_RTC_BKP_DR16,
//	_BKPReg= LL_RTC_BKP_DR17,
//	_BKPReg= LL_RTC_BKP_DR18,
//	_BKPReg= LL_RTC_BKP_DR19,
//	_BKPReg= LL_RTC_BKP_DR20,
//	_BKPReg= LL_RTC_BKP_DR21,
//	_BKPReg= LL_RTC_BKP_DR22,
//	_BKPReg= LL_RTC_BKP_DR23,
//	_BKPReg= LL_RTC_BKP_DR24,
//	_BKPReg= LL_RTC_BKP_DR25,
//	_BKPReg= LL_RTC_BKP_DR26,
//	_BKPReg= LL_RTC_BKP_DR27,
//	_BKPReg= LL_RTC_BKP_DR28,
//	_BKPReg= LL_RTC_BKP_DR29,
//	_BKPReg= LL_RTC_BKP_DR30,
//	_BKPReg= LL_RTC_BKP_DR31,
}SYS_BKPReg_t;

typedef enum
{
	NONE_SysTLVTag= 0,
	ENABLE_POWER_SysTLVTag,
	DISABLE_POWER_SysTLVTag,
	SOFT_REBOOT_SysTLVTag,
	HARD_REBOOT_SysTLVTag,
	SOFT_SHUTDOWN_SysTLVTag,
	HARD_SHUTDOWN_SysTLVTag,
	PVD_BOR_SHUTDOWN_SysTLVTag,
	SWITCH_PARTITION_SysTLVTag,
	ENABLE_SLEEP_SysTLVTag,
	DISABLE_SLEEP_SysTLVTag,
	DIV0_SysTLVTag,
	STALL_SysTLVTag,
	PREPARE_REBOOT_SysTLVTag,
	GET_DATETIME_SysTLVTag,
	SET_DATETIME_SysTLVTag,
	GET_INFO_SysTLVTag,
	SET_INFO_SysTLVTag,
	GET_METER_MODEL_SysTLVTag,
	READ_CONFIG_SysTLVTag,
	WRITE_CONFIG_SysTLVTag,
	RESET_CONFIG_SysTLVTag,
	SAVE_CONFIG_SysTLVTag,
	ERASE_PARTITION_SysTLVTag,
	WRITE_FLASH_SysTLVTag,
	SWITCH_FLASH_SysTLVTag,
	REQUEST_TIME_DELAY_SysTLVTag,
	SET_RTC_OUTPUT_SysTLVTag,
	READ_KEY_SysTLVTag,
	WRITE_KEY_SysTLVTag,
	MAX_SysTLVTag,
}SYS_SysTLVTag_t;

typedef enum
{
	FW_VERSION_InfoTLVTag= 0,
	HW_VERSION_InfoTLVTag,
	BOARD_ID_InfoTLVTag,
	MFC_DATE_InfoTLVTag,
	UID_InfoTLVTag,
	SERIAL_NO_InfoTLVTag,
	NAME_InfoTLVTag,
	ADDRESS_InfoTLVTag,
	NOTE_InfoTLVTag,
	METER_SERIAL_NO_InfoTLVTag,
	CONFIG_InfoTLVTag,
	PARTITION_InfoTLVTag,
	LATLONG_InfoTLVTag,
	METER_MODEL_InfoTLVTag,
	MAX_InfoTLVTag,
}SYS_SysInfo_t;

typedef enum
{
	NONE_SysRequest= NONE_SysTLVTag,
	ENABLE_POWER_SysRequest= ENABLE_POWER_SysTLVTag,
	DISABLE_POWER_SysRequest= DISABLE_POWER_SysTLVTag,
	SOFT_REBOOT_SysRequest= SOFT_REBOOT_SysTLVTag,
	HARD_REBOOT_SysRequest= HARD_REBOOT_SysTLVTag,
	SOFT_SHUTDOWN_SysRequest= SOFT_SHUTDOWN_SysTLVTag,
	HARD_SHUTDOWN_SysRequest= HARD_SHUTDOWN_SysTLVTag,
	PVD_BOR_SHUTDOWN_SysRequest= PVD_BOR_SHUTDOWN_SysTLVTag,
	SWITCH_PARTITION_SysRequest= SWITCH_PARTITION_SysTLVTag,
	ENABLE_SLEEP_SysRequest= ENABLE_SLEEP_SysTLVTag,
	DISABLE_SLEEP_SysRequest= DISABLE_SLEEP_SysTLVTag,
	DIV0_SysRequest= DIV0_SysTLVTag,
	STALL_SysRequest= STALL_SysTLVTag,
	PREPARE_REBOOT_SysRequest= PREPARE_REBOOT_SysTLVTag
}SYS_SysRequest_t;

typedef enum
{
	/*make sure the state can be OR-ed to indicate higher priority state to not sleep*/
	SLEEP_TaskState= 0b0,/*STOP2 Mode*/
	//BACKGROUND_RUN_TaskState= 0b1,/*in this state we need to enable Vrefint during stop mode(disable ULP), so we can keep our Power Voltage Monitoring(PVD).*/
	LIGHT_SLEEP_TaskState= 0b1,/*STOP1 Mode*/
	RUN_TaskState= 0b11,
}SYS_TaskState_t;

typedef struct
{
	uint64_t sleepTimestamp;
	uint64_t sleepPeriod;
}SYS_TaskInfo_t;

#define SYS_CFG_LPTIM2_PRIORITY		 		0
#define SYS_CFG_PULSE_CNTR_PRIORITY 		0
#define SYS_CFG_PVD_PRIORITY		 		0
#define SYS_CFG_WATCHDOG_PRIORITY	 		0
#define SYS_CFG_EXTI_PRIORITY 				2
#define SYS_CFG_SENSOR_PRIORITY 			2
#define SYS_CFG_SENSOR_DMA_PRIORITY 		3/*ADC DMA priority must be lower than ADC itself*/
#define SYS_CFG_EEPROM_PRIORITY 			2
#define SYS_CFG_NBIOT_PRIORITY	 			2
#define SYS_CFG_WMBUS_PRIORITY				2
#define SYS_CFG_NFC_PRIORITY 				2
#define SYS_CFG_RTCALARM_PRIORITY			1
#define SYS_CFG_SYSSLEEP_PRIORITY			1
#define SYS_CFG_SYSTIC_PRIORITY 			1

#define SYS_CFG_EXTRA_AWAKE_CYCLE 			6//8//6
											/*This is important to be adjusted depending on task case statement after wakeup.
 	 	 	 	 	 	 	 	 	 	 	 For example, task bc66link(delay between csq) failed to proceed to next case because mcu immediately fell asleep after woke up.
 	 	 	 	 	 	 	 	 	 	 	 The task need at least 4 cycle to evaluate next case.*/

extern SYS_TaskInfo_t pSYS_TaskInfo[MAX_TaskId];

uint32_t SYS_GetTick_ms(void);
uint64_t SYS_GetTimestamp_ms();
uint32_t SYS_GetTimestamp_s();
uint32_t SYS_GetTimestampUTC_s();
bool SYS_IsAwake(SYS_TaskId_t _taskId);
void SYS_Sleep(SYS_TaskId_t _taskId, uint32_t _period_ms);
void SYS_Wakeup(void);
bool SYS_IsTimeout(SYS_TaskId_t _taskId);
void SYS_SetTimeout(SYS_TaskId_t _taskId, uint32_t _period_ms);
void SYS_Delay(uint32_t _period_ms);
void SYS_EnableModemPower(bool _enable);
void SYS_EnablePortClock(GPIO_TypeDef *_gpioPort);
const char* SYS_GetTaskIdName(SYS_TaskId_t _taskId);
ErrorStatus SYS_Request(SYS_SysRequest_t _request);
void SYS_ExecuteRequest(SYS_SysRequest_t _request);
ErrorStatus SYS_SaveContext(void);
ErrorStatus SYS_SaveContext_Minimal(void);
void SYS_Reset(void);
void SYS_SetResetConfigFlag(bool _reset);
void SYS_FailureHandler(void);
void SYS_FailureHandler_Critical(void);
void SYS_DateTime_Update(uint8_t _weekday, uint8_t _date, uint8_t _month, uint8_t  _year, uint8_t _hour, uint8_t _minute, uint8_t _second, uint8_t _utc);
void SYS_PreInit(void);
void SYS_TLVRequest(TLV_t *_tlv);
void SYS_Save(void);
uint32_t SYS_GetTaskSleepStatusBitmap(void);
void SYS_Init(void);
void SYS_Task(void);

#endif /* INC_SYS_H_ */
