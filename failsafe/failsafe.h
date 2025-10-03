/******************************************************************************
 * File:        failsafe.h
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file defines the public interface for the failsafe module. It includes
 *   the main configuration structure `FAILSAFE_t`, enumerations for reset
 *   reasons (`FAILSAVE_Reset_t`) and TLV tags, and function prototypes for
 *   initializing and managing various failsafe mechanisms like watchdogs and
 *   power monitoring.
 *
 * Notes:
 *   - The `FAILSAFE_t` structure contains both configuration and runtime data.
 *
 * To Do:
 *   - -
 *
 ******************************************************************************/

#ifndef FAILSAFE_FAILSAFE_H_
#define FAILSAFE_FAILSAFE_H_

#include "main.h"
//#include "rtcalarm.h"

#define FAILSAFE_CFG_VERSION			0x01
#define FAILSAVE_CFG_RESET_LOG_COUNT	16
#define FAILSAVE_CFG_WORD_A				0xB4B1
#define FAILSAVE_CFG_WORD_B				0x8A81
#define FAILSAVE_CFG_RTC_DRIFT_MAX_S	300

typedef enum
{
	NMI_Reset= 			0b1,
	HARDFAULT_Reset= 	0b10,
	MEMANAGE_Reset= 	0b100,
	BUSFAULT_Reset= 	0b1000,
	USAGEFAULT_Reset=	0b10000,
	SVC_HANDLER_Reset=	0b100000,
	DEBUG_MON_Reset= 	0b1000000,
	PENDSV_Reset= 		0b10000000,
	SWDG_Reset= 		0b100000000,/*software watchdog*/
	USER_Reset=			0b1000000000,/*user reset*/
	SHUTDOWN_Reset=		0b10000000000,/*user shutdown*/
	FWU_Reset=			0b100000000000,/*firmware upgrade reset*/
	FS_CORR_CFG_Reset=	0b1000000000000,/*failsafe stored config corrupted*/
	PVD_PVM_Reset=		0b10000000000000,
	FIREWALL_Reset=		0b100000000000000,
	OB_LOAD_Reset=		0b1000000000000000,
	NRST_PIN_Reset=		0b10000000000000000,
	BOR_Reset=			0b100000000000000000,
	IWDG_Reset=			0b1000000000000000000,
	WWDG_Reset=			0b10000000000000000000,
	LOW_POWER_Reset=	0b100000000000000000000,
	RTC_DRIFT_Reset=	0b1000000000000000000000,
}FAILSAVE_Reset_t;

typedef struct
{
	uint32_t PC;
	uint32_t flag;
	uint32_t timestamp;
}FAILSAVE_ResetInfo_t;

/*TO EXPORT OUT TO CFG*/
typedef struct __attribute__((aligned(8)))/*compulsory alignment*/
{
	uint32_t checksum;/*compulsory checksum*/
	uint8_t  version;

	uint16_t failsafeWordB;
	uint32_t SWDGTimeout_ms;
	bool 	 IWDGEnable;
	uint32_t IWDGPrescaler;
	uint32_t IWDGReloadCounter;
	bool 	 WWDGEnable;
	uint32_t WWDGPrescaler;
	uint32_t WWDGWindow;
	bool 	 PVDEnable;
	uint32_t PVDLevel;
	uint32_t BORLevel;
	uint32_t periodicCheckInterval_s;

	FAILSAVE_ResetInfo_t resetLog[FAILSAVE_CFG_RESET_LOG_COUNT];
	uint16_t rteResetLogCounter;
	uint32_t rtePC;
	uint32_t rteResetFlags;

	uint32_t saveConfigInterval_s;

	uint16_t lwm2mIgnoreCycle;
	uint16_t lwm2mAdditionMargin;/*error margin before we reboot the modem*/
	uint16_t lwm2mMultiplierMargin;/*error margin before we reboot the mcu*/

	uint8_t reserve[118];/*to add more param reduce this, thus no need to do backward compatible thing. remember to set the default value after config*/
}FAILSAFE_t;

typedef enum
{
	NONE_FailsafeTLVTag= 0,
	GET_CONFIG_FailsafeTLVTag,
	SET_CONFIG_FailsafeTLVTag,
	GET_LOG_FailsafeTLVTag,
	MAX_FailsafeTLVTag,
}FAILSAFE_FailsafeTLVTag_t;

void FAILSAFE_PVD_DeInit(void);
bool FAILSAFE_CFG_IsCorrupted(void);
void FAILSAFE_SWDG_Clear(void);
void FAILSAFE_IWDG_Clear(void);
void FAILSAFE_TLVRequest(TLV_t *_tlv);
void FAILSAFE_Save(void);
void FAILSAFE_Init(FAILSAFE_t *_config);
void FAILSAFE_Task(void);
uint8_t FAILSAFE_TaskState(void);

#endif /* FAILSAFE_FAILSAFE_H_ */
