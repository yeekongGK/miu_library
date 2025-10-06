/******************************************************************************
 * File:        sys.c
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file implements the main system control functions, including task
 *   scheduling, power management, and system reset handling. It provides a
 *   centralized point for managing system-wide operations, such as entering
 *   low-power modes, handling system requests (e.g., reboot, shutdown), and
 *   dispatching TLV commands to various modules.
 *
 * Notes:
 *   - The `SYS_Task` function orchestrates the main application loop, including
 *     the sleep/wake cycle.
 *
 * To Do:
 *   - The `SYS_GetTaskIdName` function is commented out and could be enabled
 *     for better debugging.
 *
 ******************************************************************************/

//#include "common.h"
#include "main.h"
#include "cfg.h"

#include "sys.h"
#include "lptim2.h"
#include "sensor.h"
#include "logger.h"
#include "nfctag.h"
#include "pulser.h"
#include "rtc.h"
#include "syssleep.h"
#include "msg.h"
#include "bc66link.h"
#include "failsafe.h"
#include "ioctrl.h"
#include "security.h"
#include "../alarm/alarm.h"

SYS_TaskInfo_t pSYS_TaskInfo[MAX_TaskId];
bool bKeepSleep= true;

static inline void SYS_JumpToMain(uint32_t programStartAddress)
{
	typedef void(*pMainApp)(void);
	pMainApp mainApplication;
	uint32_t mainAppAddr =  (uint32_t)programStartAddress;
	uint32_t mainAppStack = (uint32_t)*((uint32_t*)mainAppAddr);
	mainApplication = (pMainApp)*(uint32_t*)(mainAppAddr + 4); // Corrected!!!

	__disable_irq();
	__set_MSP(mainAppStack);
	SCB->VTOR = mainAppAddr;
	__enable_irq();

	mainApplication();
}

static inline void SYS_JumpToProgram(uint32_t programStartAddress, uint32_t programLength, uint32_t programChecksum)
{
	__IO uint32_t _calculatedChecksum;

	if(
			((((uint32_t)&main)>= programStartAddress)&& (((uint32_t)&main)< (programStartAddress+ programLength)))
			|| (programLength> CFG_MCU_FLASH_SIZE)
		)
	{
		/*no need to jump*/
		//DBG_Print("Staying in address %d.\r\n", programStartAddress);
		//__DIAG__(DCODE_STAY_ON_CURRENT_BANK);
	}
	else
	{
		_calculatedChecksum=  CFG_GetFWUChecksum(0x0000, (uint8_t *)programStartAddress, programLength);

	    /*jump if the CRC is valid*/
	    if(_calculatedChecksum== programChecksum)
	    {
	    	SYS_JumpToMain(programStartAddress);
	    }
	}
}

uint64_t SYS_GetTimestamp_ms()
{
	return SYSCLK_GetTimestamp_ms();
}

uint32_t SYS_GetTimestamp_s()
{
	return SYSCLK_GetTimestamp_s();
}

uint32_t SYS_GetTimestampUTC_s()
{
	return (SYSCLK_GetTimestamp_s()- (config.system.utc* 15* 60));
}

bool SYS_IsAwake(SYS_TaskId_t _taskId)
{
	return SYSSLEEP_IsAwake(_taskId);
}

void SYS_Sleep(SYS_TaskId_t _taskId, uint32_t _period_ms)
{
	SYSSLEEP_RequestSleep(_taskId, _period_ms);
	//DBG_Print("%s Request sleep for %d ms. curr:%llu next:%llu Timestamp: %d\r\n",
	//		SYS_GetTaskIdName(_taskId), _period_ms, SYSSLEEP_GetCurrSleepRequest(), SYSSLEEP_GetNextSleepRequest(),  SYSCLK_GetTimestamp_ms());
}

void SYS_Wakeup(void)
{
	bKeepSleep= false;
}

bool SYS_IsTimeout(SYS_TaskId_t _taskId)
{
	return SYSSLEEP_IsAwake(_taskId);
}

void SYS_SetTimeout(SYS_TaskId_t _taskId, uint32_t _period_ms)
{
	SYSSLEEP_RequestSleep(_taskId, _period_ms);
}

void SYS_Delay(uint32_t _period_ms)
{
	LL_mDelay(_period_ms);
}

void SYS_EnableModemPower(bool _enable)
{
	SYSPWR_EnableModem(_enable);
	SENSOR_HibernateBattSensor(!_enable);/*adjust fuel gauge gain according to expected power consumed*/
}

void SYS_EnablePortClock(GPIO_TypeDef *_gpioPort)
{
	switch((uint32_t)_gpioPort)
	{
		case (uint32_t)GPIOA:
			LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
			break;
		case (uint32_t)GPIOB:
			LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
			break;
		case (uint32_t)GPIOC:
			LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);
			break;
		case (uint32_t)GPIOD:
			LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOD);
			break;
		case (uint32_t)GPIOE:
			LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOE);
			break;
		case (uint32_t)GPIOH:
			LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOH);
			break;
	}
}

void SYS_SystemReset(void)
{
#if PARTITION== 2
		SYS_JumpToMain(CFG_PARTITION_2_ADDR);
#else
		SYS_JumpToMain(CFG_PARTITION_1_ADDR);
#endif
}

void SYS_Reset(void)
{
	SYS_Save();
	SYS_SystemReset();/*without bootloader we need to use this instead of NVIC_SystemReset. This also good for debugging since it doesnt break debug mode.*/
	//NVIC_SystemReset();/*in bootloader we didint implement power latch, experiment first.*/
}

void SYS_SetResetConfigFlag(bool _reset)
{
	uint32_t _operationalFlags= LL_RTC_BAK_GetRegister(RTC, OPERATIONAL_FLAGS_BKPReg);
	LL_RTC_BAK_SetRegister(RTC, OPERATIONAL_FLAGS_BKPReg, _operationalFlags| ((true== _reset)? 0b1: 0b0));
}

void SYS_Shutdown(bool _softShutdown)
{
	config.failsafe.rteResetFlags|= SHUTDOWN_Reset;
	BC66LINK_ResetModem();/*jic*/SYS_EnableModemPower(false);
	if(true== _softShutdown)
	{
		CFG_Store(&config);
	}
	else
	{
		SYS_SetResetConfigFlag(true);
	}
	SYS_Save();
	SENSOR_HibernateBattSensor(true);/*this will hibernate current sensor to 6u, otherwise it might stay 19u.*/
	SYS_Delay(700);/*jic*/

	FAILSAFE_PVD_DeInit();/*needed as we don't want to trigger PVD reboot*/
	__disable_irq();
	__disable_fault_irq();
	/*enable or disable wake up pins here if used*/
	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);/*clear wakeup pin state*/

	IOCTRL_MainPower_Enable(false);
	while(1)
	{
		HAL_PWREx_EnterSHUTDOWNMode();
	}
	/*at this point power will be drained by the modem.*/
}

void SYS_FailureHandler(void)
{

}

void SYS_FailureHandler_Critical(void)
{
	CFG_Store(&config);
	SYS_Reset();
}

void SYS_FailureHandler_MidCritical(void)
{
	SYS_Reset();
}

void SYS_FailureHandler_HyperCritical(void)
{
	SYS_SystemReset();//NVIC_SystemReset();
}

void SYS_DateTime_Update(uint8_t _weekday, uint8_t _date, uint8_t _month, uint8_t  _year, uint8_t _hour, uint8_t _minute, uint8_t _second, uint8_t _utc)
{
	RTC_DateTime_Update(_weekday, _date, _month, _year, _hour, _minute, _second);
	config.system.utc= _utc;
	SYSCLK_SyncTick();
	SYSCLK_SyncTick();
}

void SYS_PreInit(void)
{
	/* MCU Configuration--------------------------------------------------------*/
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
	NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

	SYSPWR_InitPower();

	/* Configure the system clock */
	SYSCLK_InitSystemClock(config.system.mcuFrequency);

	/* Configure 1ms systick */
	HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);
	//__DIAG__(DCODE_PREINIT_DONE);
}

ErrorStatus SYS_Request(SYS_SysRequest_t _request)
{
	ErrorStatus _status= SUCCESS;
	switch(_request)
	{
		case ENABLE_POWER_SysRequest:
			IOCTRL_MainPower_Enable(true);
			break;
		case DISABLE_POWER_SysRequest:
			IOCTRL_MainPower_Enable(false);
			break;
		case ENABLE_SLEEP_SysRequest:
			config.system.rteDisableSleep= false;
			break;
		case DISABLE_SLEEP_SysRequest:
			config.system.rteDisableSleep= true;
			break;
		default:
			config.system.rteSysRequest= _request;
			break;
	}

	return _status;
}

void SYS_ExecuteRequest(SYS_SysRequest_t _request)
{
	switch(_request)
	{
		case SOFT_REBOOT_SysRequest:
			config.failsafe.rteResetFlags|= USER_Reset;
			CFG_Store(&config);
			SYS_Reset();
			break;
		case HARD_REBOOT_SysRequest:
			config.failsafe.rteResetFlags|= USER_Reset;
			SYS_SystemReset();//NVIC_SystemReset();
			break;
		case SOFT_SHUTDOWN_SysRequest:
			SYS_Shutdown(true);
			break;
		case HARD_SHUTDOWN_SysRequest:
			SYS_Shutdown(false);
			break;
		case PVD_BOR_SHUTDOWN_SysRequest:
			/*IWDG*/
			/*BC66LINK_ResetModem();
			IOCTRL_MainPower_Enable(false);
			while(1)
			{
				//HAL_PWREx_EnterSHUTDOWNMode();
				SYSSLEEP_EnterDeepSleep();
			}*/
			/*Constant BOR*/
			BC66LINK_ResetModem();
			IOCTRL_MainPower_Enable(false);
			while(1)
			{
				BC66LINK_ResetModem();
				FAILSAFE_SWDG_Clear();/*clear software watchdog in every sleep*/
				SYS_Delay(500);
				//CFG_Store(&config);
				//DBG_Print("-------------- VCell: %u mV, Vint: %u mV.\r\n", (int32_t)BATTSENSOR_GetVCell(), (int32_t)INTSENSOR_Voltage_Get());
				//DBG_Task();
				//SENSOR_Task();
			}
			break;
		case SWITCH_PARTITION_SysRequest:
			config.failsafe.rteResetFlags|= FWU_Reset;
			CFG_Store(&config);
			SYS_Save();
			SYS_JumpToProgram(config.flash.partitionStartAddress, config.flash.partitionLength, config.flash.partitionChecksum);
			break;
		case DIV0_SysRequest:
			SCB->CCR |= 0x10;/*enable DIV_0_TRP*/
			__IO uint8_t _0= 0;
			_0= 1/ _0;
			break;
		case STALL_SysRequest:
			while(3);
			break;
		case PREPARE_REBOOT_SysRequest:
			CFG_Store(&config);
			break;
		default:
			break;
	}
}

static uint8_t *SYS_GetKeyPtr(uint8_t _keyNo)
{
	uint8_t *_keyPtr= NULL;

	switch(_keyNo)
	{
		case 0:
			_keyPtr= config.system.key.master;
			break;
		case 1:
			_keyPtr= config.system.key.cfg.user;
			break;
		case 2:
			_keyPtr= config.system.key.opr.a;
			break;
		case 3:
			_keyPtr= config.system.key.opr.b;
			break;
		case 4:
			_keyPtr= config.system.key.cfg.prod;
			break;
		case 5:
			_keyPtr= config.system.key.cfg.dev;
			break;
		default:
			break;
	}

	return _keyPtr;
}

void SYS_TLVRequest(TLV_t *_tlv)
{
	_tlv->rv[0]= SUCCESS;
	_tlv->rl= 1;

	switch(_tlv->t)
	{
		case NONE_SysTLVTag:
		case ENABLE_POWER_SysTLVTag:
		case DISABLE_POWER_SysTLVTag:
		case SOFT_REBOOT_SysTLVTag:
		case HARD_REBOOT_SysTLVTag:
		case SOFT_SHUTDOWN_SysTLVTag:
		case HARD_SHUTDOWN_SysTLVTag:
		case PVD_BOR_SHUTDOWN_SysTLVTag:
		case SWITCH_PARTITION_SysTLVTag:
		case ENABLE_SLEEP_SysTLVTag:
		case DISABLE_SLEEP_SysTLVTag:
		case DIV0_SysTLVTag:
		case STALL_SysTLVTag:
		case PREPARE_REBOOT_SysTLVTag:
			{
				_tlv->rv[0]= SYS_Request((SYS_SysRequest_t)_tlv->t);
			}
			break;

		case GET_DATETIME_SysTLVTag:
			{
				UTILI_WaitRTCSync();/*wait for sync*/
				_tlv->rv[_tlv->rl++]= UTILI_BCDToBin(LL_RTC_DATE_GetWeekDay(RTC) );//_date.WeekDay;
				_tlv->rv[_tlv->rl++]= UTILI_BCDToBin(LL_RTC_DATE_GetDay(RTC) );//_date.Date;
				_tlv->rv[_tlv->rl++]= UTILI_BCDToBin(LL_RTC_DATE_GetMonth(RTC) );//_date.Month;
				_tlv->rv[_tlv->rl++]= UTILI_BCDToBin(LL_RTC_DATE_GetYear(RTC) );//_date.Year;
				_tlv->rv[_tlv->rl++]= UTILI_BCDToBin(LL_RTC_TIME_GetHour(RTC) );//_time.Hours;
				_tlv->rv[_tlv->rl++]= UTILI_BCDToBin(LL_RTC_TIME_GetMinute(RTC) );//_time.Minutes;
				_tlv->rv[_tlv->rl++]= UTILI_BCDToBin(LL_RTC_TIME_GetSecond(RTC) );//_time.Seconds;
				_tlv->rv[_tlv->rl++]= config.system.utc;//_time.Seconds;
				DBG_Print("SYSCLK_GetTick_ms:%u , %u \r\n", (uint32_t)(SYS_GetTimestamp_ms()>> 32), (uint32_t)SYS_GetTimestamp_ms());
			}
			break;

		case SET_DATETIME_SysTLVTag:
			{
				SYS_DateTime_Update(UTILI_BinToBCD(_tlv->v[0]), /*weekday*/
									UTILI_BinToBCD(_tlv->v[1]), UTILI_BinToBCD(_tlv->v[2]), UTILI_BinToBCD(_tlv->v[3]),/*ddMMyy*/
									UTILI_BinToBCD(_tlv->v[4]), UTILI_BinToBCD(_tlv->v[5]), UTILI_BinToBCD(_tlv->v[6]),/*hhmmss*/
									_tlv->v[7]);/*utc*/
			}
			break;

		case GET_INFO_SysTLVTag:
			{
				uint8_t _len= 0;
				char *_string= NULL;
				_tlv->rv[_tlv->rl++]= _tlv->v[0];
				switch(_tlv->v[0])
				{
					case FW_VERSION_InfoTLVTag:
						_string= config.system.fwVersion;
						break;
					case HW_VERSION_InfoTLVTag:
						_string= config.system.hwVersion;
						break;
					case BOARD_ID_InfoTLVTag:
						_string= config.system.boardId;
						break;
					case MFC_DATE_InfoTLVTag:
						_string= config.system.mfcDate;
						break;
					case UID_InfoTLVTag:
						_string= config.system.uid;
						break;
					case SERIAL_NO_InfoTLVTag:
						_string= config.system.serialNo;
						break;
					case NAME_InfoTLVTag:
						_string= config.system.name;
						break;
					case ADDRESS_InfoTLVTag:
						_string= config.system.address;
						break;
					case NOTE_InfoTLVTag:
						_string= config.system.note;
						break;
					case METER_SERIAL_NO_InfoTLVTag:
						_string= config.system.meterSerialNo;
						break;
					case CONFIG_InfoTLVTag:
						{
							uint16_t _length= sizeof(Config_t);

							memcpy(_tlv->rv+ _tlv->rl, &_length, 2);
							_tlv->rl+= 2;
							memcpy(_tlv->rv+ _tlv->rl, (uint8_t *)&(config.flash.configVersion), 2);
							_tlv->rl+= 2;
						}
						break;
					case PARTITION_InfoTLVTag:
						//DBG_Print("FLASH_InfoTLVTag\r\n");
						_tlv->rv[_tlv->rl++]= (uint8_t)config.flash.type;
						_tlv->rv[_tlv->rl++]= (uint8_t)config.flash.partition;
						memcpy(&(_tlv->rv[_tlv->rl]), (uint8_t *)config.flash.partitionStartAddress, 4);
						_tlv->rl+= 4;
						memcpy(&(_tlv->rv[_tlv->rl]), (uint8_t *)config.flash.partitionLength, 4);
						_tlv->rl+= 4;
						memcpy(&(_tlv->rv[_tlv->rl]), (uint8_t *)config.flash.partitionChecksum, 4);
						_tlv->rl+= 4;
						break;
					case LATLONG_InfoTLVTag:
						memcpy(_tlv->rv+ _tlv->rl, (uint8_t *)&(config.system.latitude), 4);
						memcpy(_tlv->rv+ _tlv->rl+ 4, (uint8_t *)&(config.system.longitude), 4);
						_tlv->rl+= 8;
						break;
					case METER_MODEL_InfoTLVTag:
						memcpy(_tlv->rv+ _tlv->rl, (uint8_t *)&(config.system.meterModel), 4);
						_tlv->rl+= 4;
						break;
					default:
						_tlv->rv[0]= ERROR;
						return;
				}

				if(NULL!= _string)
				{
					_len= strlen(_string);
					memcpy(_tlv->rv+ _tlv->rl, _string, _len);
					_tlv->rl+= _len;
					_tlv->rv[_tlv->rl++]= '\0';
				}
			}
			break;

		case SET_INFO_SysTLVTag:
			{
				uint8_t _maxLen= 0;
				uint8_t _len= 0;
				char *_string= NULL;
				switch(_tlv->v[0])
				{
					case FW_VERSION_InfoTLVTag:
						_tlv->rv[0]= ERROR;
						break;
					case HW_VERSION_InfoTLVTag:
						_string= config.system.hwVersion;
						_maxLen= CFG_HW_VERSION_LEN;
						break;
					case BOARD_ID_InfoTLVTag:
						_string= config.system.boardId;
						_maxLen= CFG_BOARD_ID_LEN;
						break;
					case MFC_DATE_InfoTLVTag:
						_string= config.system.mfcDate;
						_maxLen= CFG_MFC_DATE_LEN;
						break;
					case UID_InfoTLVTag:
						_string= config.system.uid;
						_maxLen= CFG_UID_LEN;
						break;
					case SERIAL_NO_InfoTLVTag:
						_string= config.system.serialNo;
						_maxLen= CFG_SERIAL_NO_LEN;
						break;
					case NAME_InfoTLVTag:
						_string= config.system.name;
						_maxLen= CFG_NAME_LEN;
						break;
					case ADDRESS_InfoTLVTag:
						_string= config.system.address;
						_maxLen= CFG_ADDRESS_LEN;
						break;
					case NOTE_InfoTLVTag:
						_string= config.system.note;
						_maxLen= CFG_NOTE_LEN;
						break;
					case METER_SERIAL_NO_InfoTLVTag:
						_string= config.system.meterSerialNo;
						_maxLen= CFG_SERIAL_NO_LEN;

						config.nbiot.lwm2m.diversifyPractDispatchMask= true;/*special: diversification relies on sno*/
						break;
					case CONFIG_InfoTLVTag:
						_tlv->rv[0]= ERROR;
						break;
					case PARTITION_InfoTLVTag:
						_tlv->rv[0]= ERROR;
						break;
					case LATLONG_InfoTLVTag:
						memcpy((uint8_t *)&(config.system.latitude), &( _tlv->v[1]), 4);
						memcpy((uint8_t *)&(config.system.longitude), &( _tlv->v[5]), 4);
						break;
					case METER_MODEL_InfoTLVTag:
						memcpy((uint8_t *)&(config.system.meterModel), &( _tlv->v[1]), 4);
						break;
					default:
						_tlv->rv[0]= ERROR;
						break;
				}

				if(NULL!= _string)
				{
					_len= _tlv->l - 1;
					_len= (_maxLen>= _len)? _len: _maxLen;
					memcpy(_string, (char *)&(_tlv->v[1]), _len);
					_string[_len]= '\0';
				}
			}
			break;

		case READ_CONFIG_SysTLVTag:
			{
				uint16_t _offset= MAKEWORD(_tlv->v[1], _tlv->v[0]);
				uint16_t _length= MAKEWORD(_tlv->v[3], _tlv->v[2]);

				memcpy(_tlv->rv+ _tlv->rl, ((uint8_t *)&config)+ _offset, _length);
				_tlv->rl+= _length;
			}
			break;

		case WRITE_CONFIG_SysTLVTag:
			{
				uint16_t _offset= MAKEWORD(_tlv->v[1], _tlv->v[0]);
				uint16_t _length= MAKEWORD(_tlv->v[3], _tlv->v[2]);

				if((_tlv->l- 4)!= _length)
				{
					_tlv->rv[0]= ERROR;
				}
				else
				{
					memcpy(((uint8_t *)&config)+ _offset, &_tlv->v[4], _length);
				}
			}
			break;

		case RESET_CONFIG_SysTLVTag:
			{
				SYS_SetResetConfigFlag(true);
				SYS_Request(SOFT_REBOOT_SysRequest);
			}
			break;

		case SAVE_CONFIG_SysTLVTag:
			{
				_tlv->rv[0]= CFG_Store(&config);
			}
			break;

		case ERASE_PARTITION_SysTLVTag:
			{
				//DBG_Print("ERASE_PARTITION_SysTLVTag: %d \r\n", _tlv->v[0]);
				if((CFG_FlashPartition_t)_tlv->v[0]== config.flash.partition)
				{
					_tlv->rv[0]= ERROR;
				}
				else
				{
					_tlv->rv[0]= CFG_ErasePartition((CFG_FlashPartition_t)_tlv->v[0]);
				}
			}
			break;

		case WRITE_FLASH_SysTLVTag:
			{
				uint32_t _offset= MAKELONG(_tlv->v[3], _tlv->v[2], _tlv->v[1], _tlv->v[0]);
				uint16_t _length= MAKEWORD(_tlv->v[5], _tlv->v[4]);
				uint32_t _address= (config.flash.partition== CFG_PARTITION_1)? CFG_PARTITION_2_ADDR: CFG_PARTITION_1_ADDR;
				_address+= _offset;

				//DBG_Print("WRITE_FLASH_SysTLVTag: %d / %d \r\n", _offset, _length);
				if( (false== CFG_IsInPartition(config.flash.partition, _address, _length))
						||(false== CFG_IsInPartition(PARTITION_CONFIG_FlashPartition, _address, _length))
						)
				{
					_tlv->rv[0]= CFG_WriteProgram(_address, &( _tlv->v[6]), _length);
					SYSCLK_SyncTick();/*rtc sync is needed since writing to flash suspend the mcu*/
					SYSCLK_SyncTick();
				}
			}
			break;

		case SWITCH_FLASH_SysTLVTag:
			{
				//DBG_Print("SWITCH_FLASH_SysTLVTag: %d \r\n", _tlv->v[0]);
				CFG_FlashPartition_t _partitionToSwitch= (CFG_FlashPartition_t)(_tlv->v[0]);
				uint32_t _length= MAKELONG(_tlv->v[4], _tlv->v[3], _tlv->v[2], _tlv->v[1]);
				uint16_t _checksum= MAKEWORD(_tlv->v[6], _tlv->v[5]);
				uint32_t _startAddress= (_partitionToSwitch== CFG_PARTITION_1)? CFG_PARTITION_1_ADDR: CFG_PARTITION_2_ADDR;

				if((_partitionToSwitch== config.flash.partition)|| (0== _length))
				{
					_tlv->rv[0]= ERROR;
				}
				else
				{
					if(0!= (_checksum^ CFG_GetFWUChecksum(0x0000, (uint8_t *)_startAddress, _length)))
					{
						_tlv->rv[0]= ERROR;
					}
					else
					{
						config.flash.partition= _partitionToSwitch;
						config.flash.partitionStartAddress= _startAddress;
						config.flash.partitionLength= _length;
						config.flash.partitionChecksum= _checksum;
						SYS_Request(SWITCH_PARTITION_SysRequest);
					}
				}
			}
			break;

		case REQUEST_TIME_DELAY_SysTLVTag:
			{
				time_t _period_s;
				memcpy((uint8_t *)&(_period_s), &( _tlv->v[0]), 4);
				config.system.rteSysRequestTimeDelay= SYS_GetTimestamp_s()+ _period_s;
			}
			break;

		case SET_RTC_OUTPUT_SysTLVTag:
			{
				_tlv->rv[_tlv->rl]= _tlv->v[0];
				switch(_tlv->v[0])
				{
					case 0:
					{
						IOCTRL_RadioPSM_Init(false, IOCTRL_RadioPSMPin_RTCOUT);
					}
					break;

					case 1:
					{
						IOCTRL_RadioPSM_Init(true, IOCTRL_RadioPSMPin_RTCOUT);	/*will output 512Hz*/
					}
					break;
				}
			}
			break;

		case READ_KEY_SysTLVTag:
			{
				uint8_t _keyNo= _tlv->v[0];
				uint16_t _keyLen=  MAKEWORD(_tlv->v[2], _tlv->v[1]);
				uint8_t *_key= &(_tlv->v[3]);
				uint8_t *_keyPtr= SYS_GetKeyPtr(_keyNo);

				if(
						(16!= _keyLen)
						/*either key0 is provided(key read) or same key is provided(key confirmation)*/
						|| ((false== UTILI_IsArrayTheSame(&config.system.key.master[0], _key, _keyLen))&& (false== UTILI_IsArrayTheSame(_keyPtr, _key, _keyLen)))
				  )
				{
					_tlv->rv[0]= ERROR;
				}
				else
				{
					_tlv->rv[_tlv->rl++]= _keyNo;
					_tlv->rl+= UTILI_Array_Copy16(_tlv->rv+ _tlv->rl, _keyLen);
					_tlv->rl+= UTILI_Array_Copy(_tlv->rv+ _tlv->rl, _keyPtr, _keyLen);
				}
			}
			break;

		case WRITE_KEY_SysTLVTag:
			{
				uint8_t _keyNo= _tlv->v[0];
				uint16_t _keyLen=  MAKEWORD(_tlv->v[2], _tlv->v[1]);
				uint8_t *_currKey= &(_tlv->v[3]);
				uint8_t *_newKey= _currKey+ _keyLen;
				uint8_t *_keyPtr= SYS_GetKeyPtr(_keyNo);

				if((NULL== _keyPtr)
						|| ((false== UTILI_IsArrayTheSame(&config.system.key.master[0], _currKey, _keyLen))&& (false== UTILI_IsArrayTheSame(_keyPtr, _currKey, _keyLen) ))
						)
				{
					_tlv->rv[0]= ERROR;
				}
				else
				{
					memcpy(_keyPtr, _newKey, _keyLen);
					SECURE_KeyChange(_keyNo, _keyPtr, _keyLen);
				}
			}
			break;

		default:
			_tlv->rv[0]= ERROR;
			break;
	}
}

void SYS_Save(void)
{
	FAILSAFE_Save();
	PULSER_Save();
}

SYS_TaskState_t SYS_GetTaskState(void)
{
	SYS_TaskState_t _taskState= 0;

	_taskState= 0
			| PULSER_TaskState()
			| NBIOT_TaskState()
			| SENSOR_TaskState()
			//| ALARM_TaskState()
			| LOGGER_TaskState()
			| NFCTAG_TaskState()
			| MSG_TaskState()
			| DIAG_TaskState()
			| FAILSAFE_TaskState()
			| DBG_TaskState()
			;

	return _taskState;
}

uint32_t SYS_GetTaskSleepStatusBitmap(void)
{
	uint32_t _status= 0;

	_status|= ((SLEEP_TaskState== PULSER_TaskState())? 0: 1)<< PULSER_TaskId;
	_status|= ((SLEEP_TaskState== NBIOT_TaskState())? 0: 1)<< NBIOT_TaskId;
	_status|= ((SLEEP_TaskState== LWM2M_TaskState())? 0: 1)<< NBIOT_LWOBJ_TaskId;
	_status|= ((SLEEP_TaskState== GKCOAP_TaskState())? 0: 1)<< NBIOT_GKCOAP_TaskId;
	_status|= ((SLEEP_TaskState== SENSOR_TaskState())? 0: 1)<< SENSOR_TaskId;
	//_status|= ((SLEEP_TaskState== ALARM_TaskState())? 0: 1)<< ALARM_TaskId;
	_status|= ((SLEEP_TaskState== LOGGER_TaskState())? 0: 1)<< LOGGER_TaskId;
	_status|= ((SLEEP_TaskState== NFCTAG_TaskState())? 0: 1)<< NFC_TaskId;
	_status|= ((SLEEP_TaskState== MSG_TaskState())? 0: 1)<< MSG_TaskId;
	_status|= ((SLEEP_TaskState== DIAG_TaskState())? 0: 1)<< DIAG_TaskId;
	_status|= ((SLEEP_TaskState== FAILSAFE_TaskState())? 0: 1)<< FAILSAFE_TaskId;
	_status|= ((SLEEP_TaskState== DBG_TaskState())? 0: 1)<< DBG_TaskId;

	return _status;
}

void SYS_Init(void)
{
	SYSSLEEP_Init();
	config.system.rteSysRequest= NONE_SysRequest;
	config.system.rteSysRequestTimeDelay= 0;

	RTC_Init();
	SYSCLK_SyncTick();
	SYSCLK_SyncTick();
}

void SYS_Task(void)
{
	/*Extra awake cycle is needed because KeepAwake and Task are called independently.
	 * KeepAwake might already approved sleep condition but Task might just finish sleep inhibited job and switching to next job.
	 * We want to give the Task to check for next job before we sleep.*/

	static uint8_t _extraCycle= SYS_CFG_EXTRA_AWAKE_CYCLE;
	SYS_TaskState_t _taskState= SYS_GetTaskState();

	if(
			(0== _extraCycle--)
			&&(RUN_TaskState!= _taskState)
			&&(true== SYSSLEEP_SleepAllowed())/*delay to make sure WKUP has enabled and run*/
		)
	{
		FAILSAFE_SWDG_Clear();/*clear software watchdog in every sleep*/

		if(NONE_SysRequest!= config.system.rteSysRequest)
		{
			if(SYS_GetTimestamp_s()>= config.system.rteSysRequestTimeDelay)
			{
				SYS_SysRequest_t _request= config.system.rteSysRequest;
				config.system.rteSysRequest= NONE_SysRequest;/*always reset request*/
				config.system.rteSysRequestTimeDelay= 0;
				SYS_ExecuteRequest(_request);/*execute system request when tasks are idle*/
			}
		}

		uint64_t _timestamp= SYS_GetTimestamp_ms(), _timestamp2, _timestamp3;
		if(false== config.system.rteDisableSleep)
		{
			bKeepSleep= true;
			while(true== bKeepSleep)
			{
				SYSSLEEP_EnterDeepSleep();
			}
		}
		/*sync immediately after waking up seems to failed, thus we sync twice(sue me!)*/
		SYSCLK_SyncTick();
		SYSCLK_SyncTick();

		_timestamp2= SYS_GetTimestamp_ms();
		_timestamp3= _timestamp2-_timestamp;
//		if(_timestamp3>= 500)
//			DIAG_Code(AVECURRENT_UA_SensorDCode, SENSOR_GetValue(AVECURRENT_Sensor));
	//	DBG_Print("SYS Slept for: %d ms (%u - %u).\r\n", _timestamp3, _timestamp2, _timestamp);
	//	DBG_Print("Timestamp: %d sec\r\n", SYSCLK_GetTimestamp_s());

		if(NULL!= SYSCLK_InitWakeupClock)
		{
			SYSCLK_InitWakeupClock();
		}
		_extraCycle= SYS_CFG_EXTRA_AWAKE_CYCLE;
	}
}

const char* SYS_GetTaskIdName(SYS_TaskId_t _taskId)
{
	 switch(_taskId)
	 {
//		 case NO_TASK_TaskId:
//			 return "NoTask";
//			 break;
//		case M95M01_TaskId:
//			 return "M95M01";
//			 break;
//		case BC66_PHY_TaskId:
//			 return "BC66_Phy";
//			 break;
//		case BC66_LINK_TaskId:
//			 return "BC66_Link";
//			 break;
//		case PULSER_TaskId:
//			 return "Pulse_Cntr";
//			 break;
//		case EXTI_TaskId:
//			 return "Exti";
//			 break;
//		case METER_Log_TaskId:
//			 return "Meter_Log";
//			 break;
//		case NBIOT_TaskId:
//			 return "Nbiot";
//			 break;
//		case NBIOT_GKCOAP_TaskId:
//			 return "Nbiot_GKCOAP";
//			 break;
//		case NBIOT_GKCOAP_REPORT_TaskId:
//			 return "Nbiot_GKCOAP_REPORT";
//			 break;
//		case NBIOT_LWOBJ_TaskId:
//			 return "Nbiot_LwM2M";
//			 break;
//		case NFC_TaskId:
//			 return "Nfc";
//			 break;
//		case RTC_TaskId:
//			 return "Rtc";
//			 break;
//		case MSG_TaskId:
//			 return "Msg";
//			 break;
//		case WMBUS_TaskId:
//			 return "Wmbus";
//			 break;
//		case DBG_TaskId:
//			 return "Dbg";
//			 break;
//		case SENSOR_TaskId:
//			 return "Sensor";
//			 break;
//		case FLOWSENSOR_TaskId:
//			 return "FlowSensor";
//			break;
//		case MCUADC_TaskId:
//			 return "McuAdc";
//			 break;
//		case DIAG_TaskId:
//			return "Diag";
//			break;
//		case ALARM_TaskId:
//			return "Alarm";
//			break;
//		case FAILSAFE_TaskId:
//			return "Failsafe";
//			break;
//		case LOGGER_TaskId:
//			return "Logger";
//			break;
//		case MAX_TaskId:
//			 return "MAX";
//			 break;
		default:
			 return "";
			 break;
	 }
}
