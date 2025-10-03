/******************************************************************************
 * File:        failsafe.c
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file implements the failsafe mechanisms for the device, including
 *   software and hardware watchdogs (SWDG, IWDG, WWDG), power voltage
 *   detection (PVD), and brown-out reset (BOR) handling. It is responsible
 *   for initializing these hardware features, checking for various reset
 *   conditions upon startup, logging reset events, and performing periodic
 *   checks to ensure system stability.
 *
 * Notes:
 *   - It uses RTC backup registers to persist reset flags across reboots.
 *
 * To Do:
 *   - The reset checking logic in `FAILSAFE_Reset_Check` could be simplified.
 *
 ******************************************************************************/

//#include "common.h"
#include "failsafe.h"
//#include "batterysensor.h"

static FAILSAFE_t *pConfig;
uint32_t FAILSAFE_ulSWDGCounter= 0;

#define DBG_Print(...)

bool FAILSAFE_SWDGTimeout(void) /*we use systick to detect software hang*/
{
	if(pConfig->SWDGTimeout_ms== (++(FAILSAFE_ulSWDGCounter)))
	{
		FAILSAFE_SWDG_Clear();
		return true;
	}
	return false;
}

void FAILSAFE_CFG_Check(void)
{
	if((config.flash.failsafeWordA!= FAILSAVE_CFG_WORD_A)|| (pConfig->failsafeWordB!= FAILSAVE_CFG_WORD_B))
	{
		pConfig->rteResetFlags|= FS_CORR_CFG_Reset;
		SYS_Reset();
	}
}

bool FAILSAFE_CFG_IsCorrupted(void)
{
	return (0!= (FS_CORR_CFG_Reset& LL_RTC_BAK_GetRegister(RTC, FS_RST_FLAGS_BKPReg)))? true: false;
}

void FAILSAFE_SWDG_Init(void)
{

}

void FAILSAFE_SWDG_Clear(void)
{
	FAILSAFE_ulSWDGCounter= 0;
}

void FAILSAFE_IWDG_Init(void)
{
	if(false== pConfig->IWDGEnable)
	{
		return;
	}

	LL_IWDG_Enable(IWDG);
	LL_IWDG_EnableWriteAccess(IWDG);
	LL_IWDG_SetPrescaler(IWDG, pConfig->IWDGPrescaler);
	LL_IWDG_SetReloadCounter(IWDG, pConfig->IWDGReloadCounter);
	while(1!= LL_IWDG_IsReady(IWDG))
	{
	}

	LL_IWDG_ReloadCounter(IWDG);
}

void FAILSAFE_IWDG_Clear(void)
{
	LL_IWDG_ReloadCounter(IWDG);
}

void FAILSAFE_WWDG_Init(void)
{
	if(false== pConfig->WWDGEnable)
	{
		return;
	}

	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_WWDG);

	NVIC_SetPriority(WWDG_IRQn, SYS_CFG_WATCHDOG_PRIORITY);
	NVIC_EnableIRQ(WWDG_IRQn);

	LL_WWDG_SetCounter(WWDG, pConfig->WWDGWindow);
	LL_WWDG_Enable(WWDG);
	LL_WWDG_SetPrescaler(WWDG, pConfig->WWDGPrescaler);
	LL_WWDG_SetWindow(WWDG, pConfig->WWDGWindow);
	LL_WWDG_EnableIT_EWKUP(WWDG);
}

void FAILSAFE_WWDG_Clear(void)
{
	LL_WWDG_SetCounter(WWDG, pConfig->WWDGWindow);
}

void FAILSAFE_PVD_Init(void)
{
	if(false== pConfig->PVDEnable)
	{
		LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
		LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

		NVIC_DisableIRQ(PVD_PVM_IRQn);
		LL_PWR_DisablePVD();
	}
	else
	{
		LL_EXTI_InitTypeDef EXTI_InitStruct;

		LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
		LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

		EXTI_InitStruct.Line_0_31= LL_EXTI_LINE_16; /*PVD generate interrupt on this line*/
		EXTI_InitStruct.LineCommand= ENABLE;
		EXTI_InitStruct.Mode= LL_EXTI_MODE_IT;
		EXTI_InitStruct.Trigger= LL_EXTI_TRIGGER_RISING;/*rising will give interrupt when voltage is dropping*/
		LL_EXTI_Init(&EXTI_InitStruct);

		NVIC_SetPriority(PVD_PVM_IRQn, SYS_CFG_PVD_PRIORITY);
		NVIC_EnableIRQ(PVD_PVM_IRQn);

		LL_PWR_SetPVDLevel(pConfig->PVDLevel);
		LL_PWR_EnablePVD();
	}
}

void FAILSAFE_PVD_DeInit(void)
{
	LL_PWR_DisablePVD();
}

void FAILSAFE_BOR_Init(void)
{
	FLASH_OBProgramInitTypeDef _config= {0};

	HAL_FLASHEx_OBGetConfig(&_config );

	if((FLASH_OPTR_BOR_LEV& _config.USERConfig)== pConfig->BORLevel)
	{
		return;
	}

	/*no escape, must reboot to set new option bytes*/
	SYS_ExecuteRequest(PREPARE_REBOOT_SysRequest);

	HAL_FLASH_Unlock();/*HAL_FLASH_Unlock is need immediately before HAL_FLASH_OB_Unlock !*/
	if(HAL_OK== HAL_FLASH_OB_Unlock())
	{
		_config.OptionType= OPTIONBYTE_USER;
		_config.USERType= OB_USER_BOR_LEV;
		_config.USERConfig= pConfig->BORLevel;
		HAL_FLASHEx_OBProgram(&_config);
		//HAL_FLASH_OB_Lock();
		HAL_FLASH_OB_Launch();/*this will reset the mcu*/
	}
	HAL_FLASH_Lock();
}

void FAILSAFE_Reset_Check(void)/*this can be optimized/simplified*/
{
	bool _latchPower= true;/*for BOR and shutdown we should not latch power*/

	pConfig->rtePC= LL_RTC_BAK_GetRegister(RTC, FS_RST_PC_BKPReg);
	pConfig->rteResetFlags= LL_RTC_BAK_GetRegister(RTC, FS_RST_FLAGS_BKPReg);
	uint32_t _timestamp= LL_RTC_BAK_GetRegister(RTC, FS_RST_TIMESTAMP_BKPReg);
	uint32_t _taskSleepStatus= LL_RTC_BAK_GetRegister(RTC, FS_TASK_SLEEP_STATUS_BKPReg);
	/*p/s: timestamp and PC might not be valid for some resets*/

	if(1== LL_RCC_IsActiveFlag_FWRST())
	{
		pConfig->rteResetFlags|= FIREWALL_Reset;
	}

	if(1== LL_RCC_IsActiveFlag_OBLRST())
	{
		pConfig->rteResetFlags|= OB_LOAD_Reset;
	}

	if(1== LL_RCC_IsActiveFlag_PINRST())
	{
		/*blocked due to redundant*///pConfig->rteResetFlags|= NRST_PIN_Reset;
	}

	if(1== LL_RCC_IsActiveFlag_BORRST())
	{
		pConfig->rteResetFlags|= BOR_Reset;
	}

	if(1== LL_RCC_IsActiveFlag_SFTRST())
	{
		/*other reset*/
	}

	if(1== LL_RCC_IsActiveFlag_IWDGRST())
	{
		pConfig->rteResetFlags|= IWDG_Reset;
	}

	if(1== LL_RCC_IsActiveFlag_WWDGRST())
	{
		pConfig->rteResetFlags|= WWDG_Reset;
	}

	if(1== LL_RCC_IsActiveFlag_LPWRRST())
	{
		pConfig->rteResetFlags|= LOW_POWER_Reset;
	}

	for(int i= 0; i< 32; i++)
	{
		uint32_t _flag= (pConfig->rteResetFlags& (0b1<< i));
		if(0!= _flag)
		{
			uint8_t _counter= 0;
			if(pConfig->rteResetLogCounter>= FAILSAVE_CFG_RESET_LOG_COUNT)
			{
				for (; _counter< (FAILSAVE_CFG_RESET_LOG_COUNT- 1); _counter++)
				{
					pConfig->resetLog[_counter]= pConfig->resetLog[_counter+ 1]; /*move up one place*/
				}
			}
			else
			{
				_counter= pConfig->rteResetLogCounter;
			}

			pConfig->resetLog[_counter].PC= pConfig->rtePC;
			pConfig->resetLog[_counter].flag= _flag;
			pConfig->resetLog[_counter].timestamp= _timestamp;
			pConfig->rteResetLogCounter++;
			DIAG_Code(NMI_RESET_FailsafeDCode+ i, pConfig->rtePC);
		}
	}

	const char _reseTypeSoftwareString[]= "Reset Type: SOFTWARE:";
	if(0!= (NMI_Reset& pConfig->rteResetFlags)){DBG_Print("%s: NMI\r\n", _reseTypeSoftwareString);}
	if(0!= (HARDFAULT_Reset& pConfig->rteResetFlags)){DBG_Print("%s: HARDFAULT\r\n", _reseTypeSoftwareString);}
	//if(0!= (MEMANAGE_Reset& pConfig->rteResetFlags)){DBG_Print("%s: MEMANAGE\r\n", _reseTypeSoftwareString);}
	//if(0!= (BUSFAULT_Reset& pConfig->rteResetFlags)){DBG_Print("%s: BUSFAULT\r\n", _reseTypeSoftwareString);}
	//if(0!= (USAGEFAULT_Reset& pConfig->rteResetFlags)){DBG_Print("%s: USAGEFAULT\r\n", _reseTypeSoftwareString);}
	//if(0!= (SVC_HANDLER_Reset& pConfig->rteResetFlags)){DBG_Print("%s: SVC_HANDLER\r\n", _reseTypeSoftwareString);}
	//if(0!= (DEBUG_MON_Reset& pConfig->rteResetFlags)){DBG_Print("%s: DEBUG_MON\r\n", _reseTypeSoftwareString);}
	//if(0!= (PENDSV_Reset& pConfig->rteResetFlags)){DBG_Print("%s: PENDSV\r\n", _reseTypeSoftwareString);}
	if(0!= (SWDG_Reset& pConfig->rteResetFlags))
	{
		config.diagnostic.rteFailsafeRebootCount++;
		config.nbiot.stats.failsafeRebootCount= config.diagnostic.rteFailsafeRebootCount;
		DIAG_Code(TASK_SLEEP_STATUS_FailsafeDCode, _taskSleepStatus);
		DBG_Print("%s: SWDG\r\n", _reseTypeSoftwareString);
//		for(int _i= 0; _i< (sizeof(uint32_t)* 8); _i++)
//		{
//			if(0!= (_taskSleepStatus& (1UL<< _i)))
//			{
//				DBG_Print("Task %s didn't sleep.\r\n", SYS_GetTaskIdName(_i));
//			}
//		}
	}
	if(0!= (USER_Reset& pConfig->rteResetFlags)){DBG_Print("%s: USER\r\n", _reseTypeSoftwareString);}
	if(0!= (SHUTDOWN_Reset& pConfig->rteResetFlags))
	{
		//_latchPower= false;/*this has become an indicator rather than flag because we place while loop when shutdown rather than reset*/
		DBG_Print("%s: SHUTDOWN\r\n", _reseTypeSoftwareString);
	}
	if(0!= (FWU_Reset& pConfig->rteResetFlags)){DBG_Print("%s: FWU\r\n", _reseTypeSoftwareString);}
	if(0!= (FS_CORR_CFG_Reset& pConfig->rteResetFlags)){DBG_Print("%s: FS_CORR_CFG\r\n", _reseTypeSoftwareString);}
	if(0!= (PVD_PVM_Reset& pConfig->rteResetFlags))
	{
		config.diagnostic.rtePVDRebootCount++;
		config.nbiot.stats.PVDRebootCount= config.diagnostic.rtePVDRebootCount;
		DBG_Print("%s: PVD_PVM\r\n", _reseTypeSoftwareString);
		config.nbiot.modemMode= BACKOFF_ModemMode;
	}
	//if(0!= (FIREWALL_Reset& pConfig->rteResetFlags)){DBG_Print("Reset Type: FIREWALL\r\n");}
	//if(0!= (OB_LOAD_Reset& pConfig->rteResetFlags)){DBG_Print("Reset Type: OPTION BYTES LOADER\r\n");}
	if(0!= (NRST_PIN_Reset& pConfig->rteResetFlags)){DBG_Print("Reset Type: NRST PIN\r\n");}
	if(0!= (BOR_Reset& pConfig->rteResetFlags))
	{
		config.diagnostic.rteBORCount++;
		config.nbiot.stats.BORRebootCount= config.diagnostic.rteBORCount;
		//_latchPower= IOCTRL_MainPower_IsEnabled();/*if already turned on by magnet*/
		DBG_Print("Reset Type: BOR\r\n");
		if(NCTAG_RFFieldIsPresent())
		{
			//DBG_Print("#stat:nfc_: RF Field Present, Unlatch power>\r\n");
			_latchPower= false;
		}
		else
		{
			//DBG_Print("#stat:nfc_: RF Field NOT Present>\r\n");
		}
	}
	if(0!= (IWDG_Reset& pConfig->rteResetFlags)){DBG_Print("Reset Type: IWDG\r\n");}
	if(0!= (WWDG_Reset& pConfig->rteResetFlags)){DBG_Print("Reset Type: WWDG\r\n");}
	if(0!= (LOW_POWER_Reset& pConfig->rteResetFlags)){DBG_Print("Reset Type: LOW POWER\r\n");}
	if(0!= (RTC_DRIFT_Reset& pConfig->rteResetFlags)){DBG_Print("Reset Type: RTC_DRIFT_Reset\r\n");}

	LL_RCC_ClearResetFlags();
	pConfig->rteResetFlags= 0;
	LL_RTC_BAK_SetRegister(RTC, FS_RST_PC_BKPReg, 0);
	LL_RTC_BAK_SetRegister(RTC, FS_RST_FLAGS_BKPReg, 0);
	LL_RTC_BAK_SetRegister(RTC, FS_RST_TIMESTAMP_BKPReg, 0);
	LL_RTC_BAK_SetRegister(RTC, FS_TASK_SLEEP_STATUS_BKPReg, 0);

	IOCTRL_MainPower_Enable(_latchPower);
}

void FAILSAFE_TLVRequest(TLV_t *_tlv)
{
	_tlv->rv[0]= SUCCESS;
	_tlv->rl= 1;

	switch(_tlv->t)
	{
		case GET_CONFIG_FailsafeTLVTag:
			{
				_tlv->rl+= UTILI_Array_Copy16(_tlv->rv+ _tlv->rl, pConfig->failsafeWordB);
				_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->SWDGTimeout_ms);
				_tlv->rv[_tlv->rl++]= pConfig->IWDGEnable;
				_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->IWDGPrescaler);
				_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->IWDGReloadCounter);
				_tlv->rv[_tlv->rl++]= pConfig->WWDGEnable;
				_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->WWDGPrescaler);
				_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->WWDGWindow);
				_tlv->rv[_tlv->rl++]= pConfig->PVDEnable;
				_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->PVDLevel);
				_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->BORLevel);
				_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->periodicCheckInterval_s);
				_tlv->rl+= UTILI_Array_Copy16(_tlv->rv+ _tlv->rl, pConfig->rteResetLogCounter);
				_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->rtePC);
				_tlv->rl+= UTILI_Array_Copy32(_tlv->rv+ _tlv->rl, pConfig->rteResetFlags);
			}
			break;
		case SET_CONFIG_FailsafeTLVTag:
			{
				uint16_t _index= 0;
				_index+= UTILI_Array_Copy16_Ptr(&(pConfig->failsafeWordB), _tlv->v+ _index);
				_index+= UTILI_Array_Copy32_Ptr(&(pConfig->SWDGTimeout_ms), _tlv->v+ _index);
				pConfig->IWDGEnable= _tlv->v[_index++];
				_index+= UTILI_Array_Copy32_Ptr(&(pConfig->IWDGPrescaler), _tlv->v+ _index);
				_index+= UTILI_Array_Copy32_Ptr(&(pConfig->IWDGReloadCounter), _tlv->v+ _index);
				pConfig->WWDGEnable= _tlv->v[_index++];
				_index+= UTILI_Array_Copy32_Ptr(&(pConfig->WWDGPrescaler), _tlv->v+ _index);
				_index+= UTILI_Array_Copy32_Ptr(&(pConfig->WWDGWindow), _tlv->v+ _index);
				pConfig->PVDEnable= _tlv->v[_index++];
				_index+= UTILI_Array_Copy32_Ptr(&(pConfig->PVDLevel), _tlv->v+ _index);
				_index+= UTILI_Array_Copy32_Ptr(&(pConfig->BORLevel), _tlv->v+ _index);
				_index+= UTILI_Array_Copy32_Ptr(&(pConfig->periodicCheckInterval_s), _tlv->v+ _index);
				_index+= UTILI_Array_Copy16_Ptr(&(pConfig->rteResetLogCounter), _tlv->v+ _index);
				_index+= UTILI_Array_Copy32_Ptr(&(pConfig->rtePC), _tlv->v+ _index);
				_index+= UTILI_Array_Copy32_Ptr(&(pConfig->rteResetFlags), _tlv->v+ _index);
			}
			break;
		case GET_LOG_FailsafeTLVTag:
			{
				_tlv->rl+= UTILI_Array_Copy(_tlv->rv+ _tlv->rl, pConfig->resetLog, sizeof(FAILSAVE_ResetInfo_t)* FAILSAVE_CFG_RESET_LOG_COUNT);
			}
			break;
		default:
			_tlv->rv[0]= ERROR;
			break;
	}
}

void FAILSAFE_Save(void)
{
	LL_RTC_BAK_SetRegister(RTC, FS_RST_PC_BKPReg, pConfig->rtePC);
	LL_RTC_BAK_SetRegister(RTC, FS_RST_FLAGS_BKPReg, pConfig->rteResetFlags);
	LL_RTC_BAK_SetRegister(RTC, FS_RST_TIMESTAMP_BKPReg, SYSCLK_GetTimestamp_s());
	LL_RTC_BAK_SetRegister(RTC, FS_TASK_SLEEP_STATUS_BKPReg, SYS_GetTaskSleepStatusBitmap());
	LL_RTC_BAK_SetRegister(RTC, MODEM_STATE_NbiotDCode, (BC66LINK_GetState()<< 8)| BC66LINK_GetSubState());/*due diligence, most likely stucked here when swdg triggered*/
}

static uint64_t ulPrevTs_s;
void FAILSAFE_RTCDrift_Check_Init(void)
{
	ulPrevTs_s= SYS_GetTimestamp_s();
}

void FAILSAFE_RTCDrift_Check(void)
{
	uint32_t _currTs_s= SYS_GetTimestamp_s();
	uint32_t _drift_s= (ulPrevTs_s> _currTs_s)? (ulPrevTs_s- _currTs_s): (_currTs_s- ulPrevTs_s);

	if(FAILSAVE_CFG_RTC_DRIFT_MAX_S< _drift_s)
	{
		DIAG_Code(RTC_DRIFT_S_FailsafeDCode, _drift_s);
	}

	if(pConfig->periodicCheckInterval_s< _drift_s)
	{
		/*critical, should not drift this much*/
		pConfig->rteResetFlags|= RTC_DRIFT_Reset;
		SYS_Reset();
	}

	ulPrevTs_s= _currTs_s;
}

void FAILSAFE_PeriodicCheck_LogSensor(void)
{
	static uint8_t _skip= 0;
	float _aveCurr= SENSOR_GetValue(AVECURRENT_Sensor);

	if((100< _aveCurr)|| (0== _skip))/*to not overcrowded the log with this message, only log if MIU not sleep*/
	{
		DIAG_Code(VOLT_TEMP_MV_C_SensorDCode, ((uint32_t)SENSOR_GetValue(INTERNAL_VOLTAGE_Sensor)<< 16)| ((uint32_t)SENSOR_GetValue(INTERNAL_TEMPERATURE_Sensor)));
		DIAG_Code_f(AVECURRENT_UA_SensorDCode, _aveCurr);
	}

	if(12== ++_skip)
	{
		_skip= 0;
	}
}

static bool bSaveConfigAllowed= true;
void FAILSAFE_PeriodicCheck_CFGStoreAllowance(void)
{
	/*typically important during brown out reset*/
	static uint32_t _nextSaveTime= 0;
	uint32_t _currTime= SYS_GetTimestamp_s();

	if(_nextSaveTime<= _currTime)
	{
		_nextSaveTime= _currTime+ pConfig->saveConfigInterval_s;
		bSaveConfigAllowed= true;
	}
}

void FAILSAFE_CFGStore(void)
{
	if(true== bSaveConfigAllowed)
	{
		CFG_Store(&config);
		bSaveConfigAllowed= false;
	}
}

static uint8_t ucPrevBc66State, ucCurrBc66State;
void FAILSAFE_PeriodicCheck_Nbiot_Init(void)
{
	ucPrevBc66State= ucCurrBc66State= (BC66LINK_GetState()<< 8)| BC66LINK_GetSubState();

	if(0== ucPrevBc66State)/*since we call periodic task straight away, we don't want to simply re-reset BC66(taht starts with state 0*/
	{
		ucPrevBc66State= 1;
	}
}

void FAILSAFE_PeriodicCheck_Nbiot(void)
{
	if((ucCurrBc66State== ucPrevBc66State)
			&&(ucCurrBc66State!= (WAIT_REQUEST_Bc66LinkState<< 8))
			&&(ucCurrBc66State!= ((DISABLE_Bc66LinkState<< 8)| CHECK_FOR_ENABLE_DisableState)))/*can further check this state with restart timer*/
	{
		/*state unchanged*/
		DIAG_Code(BC66_STATE_UNCHANGED_FailsafeDCode, ucCurrBc66State);
		BC66LINK_ResetModem();
	}

	ucPrevBc66State= ucCurrBc66State= (BC66LINK_GetState()<< 8)| BC66LINK_GetSubState();
}

void FAILSAFE_PeriodicCheck_NbiotGKCOAP(void)
{
	static uint32_t ulNoOfLogReported= 0;
	static uint32_t tsLastLogReported= 0;

	if(true== config.nbiot.gkcoap.enabled)
	{
		if(SYS_GetTimestamp_s()> config.nbiot.gkcoap.rteStartTime)
		{
			if(SYS_GetTimestamp_s()> (config.nbiot.gkcoap.rteNextTime+ 300))
			{
				DIAG_Code(GKCOAP_HANG_FailsafeDCode, ucCurrBc66State);
				BC66LINK_ResetModem();
			}
		}

		if(SYS_GetTimestamp_s()> (tsLastLogReported+ (config.nbiot.gkcoap.reportInterval_s* 4)))
		{
			if((0!= config.nbiot.gkcoap.rte.report.noOfLogsReported)&&
					(ulNoOfLogReported== config.nbiot.gkcoap.rte.report.noOfLogsReported))
			{
				DIAG_Code(GKCOAP_HANG_FailsafeDCode, ucCurrBc66State);
				SYS_FailureHandler_Critical();
			}
			else
			{
				ulNoOfLogReported= config.nbiot.gkcoap.rte.report.noOfLogsReported;
				tsLastLogReported= SYS_GetTimestamp_s();
			}
		}
	}
}

static uint32_t ulLogInterval= 3600;
void FAILSAFE_PeriodicCheck_NbiotLWM2M_SetLogAlarm(void)
{
	/*to ensure we wake up the device on time for logging*/
	if((true== config.nbiot.lwm2m.enabled)&& (false== config.nbiot.gkcoap.enabled))
	{
		RTC_AlarmStartMarker_t _startMarker= {0x00, 0x00, 0x00, 0x01};
		RTC_TickType_t _tickType;
		LWOBJ_Obj_t *_readings= &(config.nbiot.lwm2m.lwObj[GET_READING_PractInstance]);
		ulLogInterval= _readings->resource[PERIODIC_INTERVAL_PractResource].value.integer;

		if(0== (ulLogInterval% (60* 60* 24)))/*day*/
		{
			_tickType= DAY_TickType;
		}
		else if(0== (ulLogInterval% (60* 60)))/*hour*/
		{
			_tickType= HOUR_TickType;
		}
		else if(0== (ulLogInterval% (60)))/*minute*/
		{
			_tickType= MINUTE_TickType;
		}
		else
		{
			_tickType= SECOND_TickType;
		}
		RTCALARM_A_Enable(_tickType, _startMarker);
	}
}

static uint16_t uwIgnoreCycle= 12;
void FAILSAFE_PeriodicCheck_NbiotLWM2M_Init(void)
{
	FAILSAFE_PeriodicCheck_NbiotLWM2M_SetLogAlarm();
	uwIgnoreCycle= pConfig->lwm2mIgnoreCycle;
}

void FAILSAFE_PeriodicCheck_NbiotLWM2M(void)
{
	if(true== config.nbiot.lwm2m.enabled)
	{
		LWOBJ_Obj_t *_readings= &(config.nbiot.lwm2m.lwObj[PRACT_GET_READING_ObjName]);

		if(0== uwIgnoreCycle)/*dont want to keep rebooting*/
		{
			uint32_t _expectedDispatch= _readings->resource[RECORD_DISPATCH_PERIODIC_INTERVAL_PractResource].value.integer/ _readings->resource[PERIODIC_INTERVAL_PractResource].value.integer;
			int32_t _toDispatch= _readings->resource[RECORD_HEAD_PractResource].value.integer- _readings->resource[RECORD_READ_PractResource].value.integer;

			if((0!= pConfig->lwm2mMultiplierMargin)&&
					((_expectedDispatch* (uint32_t)(pConfig->lwm2mMultiplierMargin))<= _toDispatch))/*check if we're EXTREMELY behind on logs*/
			{
				uwIgnoreCycle= pConfig->lwm2mIgnoreCycle;/*jic*/
				DIAG_Code(LWM2M_READINGS_NOT_DISPATCHED_FailsafeDCode, ucCurrBc66State);
				SYS_FailureHandler_Critical();
			}
			else if((0!= pConfig->lwm2mAdditionMargin)&&
					((_expectedDispatch+ (uint32_t)(pConfig->lwm2mAdditionMargin))<= _toDispatch))/*check if we're behind on logs*/
			{
				uwIgnoreCycle= pConfig->lwm2mIgnoreCycle;/*to not always rebooting modem*/
				/*this can be cause of:
				 * 1. We did not receive observe on readings
				 * 2. We are not registered to server*/
				DIAG_Code(LWM2M_READINGS_NOT_DISPATCHED_FailsafeDCode, ucCurrBc66State);
				BC66LINK_ResetModem();/*this would initiate registration*/
			}
		}
		else
		{
			uwIgnoreCycle-- ;
		}

		if(ulLogInterval!= _readings->resource[PERIODIC_INTERVAL_PractResource].value.integer)
		{
			/*log interval has changed*/
			FAILSAFE_PeriodicCheck_NbiotLWM2M_SetLogAlarm();
		}
	}
}

void FAILSAFE_PeriodicCheck_Init(void)
{
	FAILSAFE_PeriodicCheck_Nbiot_Init();
	FAILSAFE_PeriodicCheck_NbiotLWM2M_Init();
}

void FAILSAFE_PeriodicCheck(void)
{
	FAILSAFE_PeriodicCheck_LogSensor();
	FAILSAFE_PeriodicCheck_CFGStoreAllowance();
	FAILSAFE_PeriodicCheck_Nbiot();
	FAILSAFE_PeriodicCheck_NbiotGKCOAP();
	FAILSAFE_PeriodicCheck_NbiotLWM2M();
}

static uint32_t ulLastAwake_s= 0;
void FAILSAFE_Init(FAILSAFE_t *_config)
{
	pConfig= _config;
	ulLastAwake_s= SYS_GetTimestamp_s();

	FAILSAFE_CFG_Check();
	FAILSAFE_Reset_Check();
	FAILSAFE_SWDG_Init();
	FAILSAFE_IWDG_Init();
	FAILSAFE_WWDG_Init();
	FAILSAFE_PVD_Init();
	FAILSAFE_BOR_Init();
	FAILSAFE_RTCDrift_Check_Init();
	FAILSAFE_PeriodicCheck_Init();
}

void FAILSAFE_Task(void)
{
	FAILSAFE_RTCDrift_Check();

	if(true== SYS_IsAwake(FAILSAFE_TaskId))
	{
		//DBG_Print("FAILSAFE_PeriodicCheck.\r\n");
		FAILSAFE_PeriodicCheck();
		SYS_Sleep(FAILSAFE_TaskId, pConfig->periodicCheckInterval_s* 1000);
	}

	ucCurrBc66State|= BC66LINK_GetState();
	FAILSAFE_IWDG_Clear();
}

uint8_t FAILSAFE_TaskState(void)
{
	return SLEEP_TaskState;
}
