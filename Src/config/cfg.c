/*
 * configuration.c
 *
 *  Created on: 28 Dec 2020
 *      Author: muhammad.ahmad@georgekent.net
 */

#include "common.h"
#include "cfg.h"
#include "cfg_keys.h"
#include "dbg.h"

//#define DBG_PrintLine(...)
//#define DBG_Print(...)

static uint16_t 	CFG_GetConfigVersion(uint32_t _eepromAddress);
static void 		CFG_ReadStruct(Config_t *_cfg, uint32_t _eepromAddress);
static ErrorStatus 	CFG_WriteStruct(uint8_t *_struct, uint16_t _length, uint32_t _address);

Config_t config;

void CFG_ApplyDefaults(Config_t *_cfg)
{
    _cfg->flash.configVersion= CFG_CONFIG_VERSION;
		_cfg->flash.useDefaultConfig= false;
		_cfg->flash.type= APP1_APP2_CFG_FlashType;

#if PARTITION== 2
		_cfg->flash.partition= PARTITION_2_FlashPartition;
		_cfg->flash.partitionStartAddress= CFG_PARTITION_2_ADDR;
#else
		_cfg->flash.partition= PARTITION_1_FlashPartition;
		_cfg->flash.partitionStartAddress= CFG_PARTITION_1_ADDR;
#endif
		_cfg->flash.partitionLength= CFG_PARTITION_SIZE;
		_cfg->flash.partitionChecksum= 0x00000000;
		_cfg->flash.failsafeWordA= FAILSAVE_CFG_WORD_A;

		_cfg->system.checksum= 0x09abcdef;
		_cfg->system.mcuFrequency= 24000000; /*4000000 16000000 24000000 48000000 80000000*/
		_cfg->system.transmissionType= NBIOT_Transmission;
		_cfg->system.hours= 0x00;
		_cfg->system.minutes= 0x00;
		_cfg->system.seconds= 0x00;
		_cfg->system.weekday= LL_RTC_WEEKDAY_TUESDAY;
		_cfg->system.day= 0x12;
		_cfg->system.month= LL_RTC_MONTH_OCTOBER;
		_cfg->system.year= 0x21;
		_cfg->system.utc= ((8* 60)/ 15);/*UTC+8 in term of 15 minutes*/

		// _cfg->system.key.master= CFG_KEYS_KEY_MASTER;
		// _cfg->system.key.cfg.prod= CFG_KEYS_KEY_CFG_PROD;
		// _cfg->system.key.cfg.dev= CFG_KEYS_KEY_CFG_DEV;
		// _cfg->system.key.cfg.user= CFG_KEYS_KEY_CFG_USER;
		// _cfg->system.key.opr.a= CFG_KEYS_KEY_OPR_A;
		// _cfg->system.key.opr.b= CFG_KEYS_KEY_OPR_B;
		// _cfg->system.fwVersion= CFG_DEVICE_FIRMWARE_VERSION;
		// _cfg->system.hwVersion= CFG_DEVICE_HARDWARE_VERSION;

		static const char CFG_KEYS_KEY_MASTER_temp[] = CFG_KEYS_KEY_MASTER;		
		static const char CFG_KEYS_KEY_CFG_PROD_temp[] = CFG_KEYS_KEY_CFG_PROD;		
		static const char CFG_KEYS_KEY_CFG_DEV_temp[] = CFG_KEYS_KEY_CFG_DEV;		
		static const char CFG_KEYS_KEY_CFG_USER_temp[] = CFG_KEYS_KEY_CFG_USER;		
		static const char CFG_KEYS_KEY_OPR_A_temp[] = CFG_KEYS_KEY_OPR_A;		
		static const char CFG_KEYS_KEY_OPR_B_temp[] = CFG_KEYS_KEY_OPR_B;		
		static const char CFG_DEVICE_FIRMWARE_VERSION_temp[] = CFG_DEVICE_FIRMWARE_VERSION;		
		static const char CFG_DEVICE_HARDWARE_VERSION_temp[] = CFG_DEVICE_HARDWARE_VERSION;		
		memcpy(_cfg->system.key.master, CFG_KEYS_KEY_MASTER_temp, sizeof(_cfg->system.key.master));
		memcpy(_cfg->system.key.cfg.prod, CFG_KEYS_KEY_CFG_PROD_temp, sizeof(_cfg->system.key.cfg.prod));
		memcpy(_cfg->system.key.cfg.dev, CFG_KEYS_KEY_CFG_DEV_temp, sizeof(_cfg->system.key.cfg.dev));
		memcpy(_cfg->system.key.cfg.user, CFG_KEYS_KEY_CFG_USER_temp, sizeof(_cfg->system.key.cfg.user));
		memcpy(_cfg->system.key.opr.a, CFG_KEYS_KEY_OPR_A_temp, sizeof(_cfg->system.key.opr.a));
		memcpy(_cfg->system.key.opr.b, CFG_KEYS_KEY_OPR_B_temp, sizeof(_cfg->system.key.opr.b));
		memcpy(_cfg->system.fwVersion, CFG_DEVICE_FIRMWARE_VERSION, sizeof(_cfg->system.fwVersion));
		memcpy(_cfg->system.hwVersion, CFG_DEVICE_HARDWARE_VERSION, sizeof(_cfg->system.hwVersion));

		// _cfg->system.mfcDate= EMPTY_STRING;
		memset(_cfg->system.mfcDate, 0, sizeof(_cfg->system.mfcDate));
		// _cfg->system.uid= "00000001";
		static const char STSTEM_UID_temp[] = "00000001";
		memcpy(_cfg->system.uid, STSTEM_UID_temp, sizeof(STSTEM_UID_temp));	// system.uid[CFG_UID_LEN (35)]
		// _cfg->system.serialNo= EMPTY_STRING;
		memset(_cfg->system.serialNo, 0, sizeof(_cfg->system.serialNo));
		// _cfg->system.name= NAME_STRING;
		static const char SYSTEM_NAME_temp[] = NAME_STRING;
		memcpy(_cfg->system.name, SYSTEM_NAME_temp, sizeof(SYSTEM_NAME_temp));	// nbiot.apn[NBIOT_CFG_APN_LEN 32 + 1]
		// _cfg->system.address= EMPTY_STRING;
		memset(_cfg->system.address, 0, sizeof(_cfg->system.address));
		// _cfg->system.note= EMPTY_STRING;
		memset(_cfg->system.note, 0, sizeof(_cfg->system.note));
		// _cfg->system.meterSerialNo= EMPTY_STRING;
		memset(_cfg->system.meterSerialNo, 0, sizeof(_cfg->system.meterSerialNo));
		_cfg->system.meterModel= 254- 1;
		_cfg->system.latitude= 0;
		_cfg->system.longitude= 0;
		_cfg->system.rteDisableSleep= false;
		_cfg->system.rteSysTick= 0;
		_cfg->system.rteSysRequest= NONE_SysRequest;
		_cfg->system.rteIsTurnedOn= false;
		_cfg->system.rteSysRequestTimeDelay= 0;

		_cfg->failsafe.failsafeWordB= FAILSAVE_CFG_WORD_B;
		_cfg->failsafe.SWDGTimeout_ms= 25000,//25000,/*IWDG will force reset maximum at about 32sec, SWDG must timeout before that so that we can save config *;
		_cfg->failsafe.IWDGEnable= true;
		_cfg->failsafe.IWDGPrescaler= LL_IWDG_PRESCALER_256;
		_cfg->failsafe.IWDGReloadCounter= 0x0FFF;
		_cfg->failsafe.WWDGEnable= false;
		_cfg->failsafe.WWDGPrescaler= LL_WWDG_PRESCALER_8;
		_cfg->failsafe.WWDGWindow= 0x7E;
		_cfg->failsafe.PVDEnable= true;
		_cfg->failsafe.PVDLevel= PWR_PVDLEVEL_3,//PWR_PVDLEVEL_3;
		_cfg->failsafe.BORLevel= OB_BOR_LEVEL_1;
		_cfg->failsafe.periodicCheckInterval_s= 3600;
		// _cfg->failsafe.resetLog= {{0}};
		memset(_cfg->failsafe.resetLog, 0, sizeof(_cfg->failsafe.resetLog));
		_cfg->failsafe.rteResetLogCounter= 0;
		_cfg->failsafe.rtePC= 0;
		_cfg->failsafe.rteResetFlags= 0;
		_cfg->failsafe.saveConfigInterval_s= HOUR_TO_SECONDS(24);/*stm32l4 guaranteed write cycle max 10K*/
		_cfg->failsafe.lwm2mIgnoreCycle= 12;
		_cfg->failsafe.lwm2mAdditionMargin= 3;/*decada time window is within 1am to 4am*/
		_cfg->failsafe.lwm2mMultiplierMargin= 3;/*decada KPI is within maximum 3 days*/

		_cfg->diagnostic.queue.storage= 0;
		_cfg->diagnostic.queue.head= 0;
		_cfg->diagnostic.queue.elementCount= 0;
		_cfg->diagnostic.queue.elementPopped= 0;
		_cfg->diagnostic.queue.elementSize= DIAG_CFG_ENTRY_SIZE;
		_cfg->diagnostic.queue.elementMax= DIAG_CFG_MAX_ENTRY;
		_cfg->diagnostic.rteHardfaultRebootCount= 0;
		_cfg->diagnostic.rteFailsafeRebootCount= 0;
		_cfg->diagnostic.rtePVDRebootCount= 0;
		_cfg->diagnostic.rteBORCount= 0;
		_cfg->diagnostic.rteWatchdogRebootCount= 0;
		_cfg->diagnostic.rteShutdownCount= 0;
		_cfg->diagnostic.rteRebootCount= 0;
		_cfg->diagnostic.rteVRefDippedCount= 0;
		_cfg->diagnostic.rteNbModemSelfResetCount= 0;

		_cfg->pulser.mode= TRACSENS_Mode;
		_cfg->pulser.weight_liter= 0.25;
		_cfg->pulser.rtePrevValue_liter= 0;
		_cfg->pulser.rtePrevValue2_liter= 0;
		_cfg->pulser.tracsens.enableErrorPatternCheck= true;
		_cfg->pulser.tracsens.useCompensatedValue= false;
		_cfg->pulser.tracsens.errorPatternConfirmationCount= 36,//6;
		_cfg->pulser.tracsens.rteOffsetValue= 0;
		_cfg->pulser.tracsens.rteLastSavedValue= 0;
		_cfg->pulser.tracsens.rteErrorPatternCount= 0;
		_cfg->pulser.tracsens.rteErrorPatternCompensationStarted= 0;
		_cfg->pulser.tracsens.rteErrorPatternState= 0;
		_cfg->pulser.tracsens.rteErrorPatternPreviousPulse= 0;
		_cfg->pulser.tracsens.rteErrorPatternJustStarted= 0;
		_cfg->pulser.lcsens.currBitmap= 0b0;
		_cfg->pulser.lcsens.prevBitmap= 0b0;
		// _cfg->pulser.lcsens.transitionTable= {0};
		memset(_cfg->pulser.lcsens.transitionTable,0,sizeof(_cfg->pulser.lcsens.transitionTable));
		_cfg->pulser.lcsens.initialTransitionVal= 0b0;
		_cfg->pulser.lcsens.currTransitionVal= 0b0;
		_cfg->pulser.lcsens.prevTransitionVal= 0b0;
		_cfg->pulser.lcsens.fwdTransitionCnt= 0;
		_cfg->pulser.lcsens.bwdTransitionCnt= 0;
		_cfg->pulser.lcsens.fwdRevolutionCnt= 0;
		_cfg->pulser.lcsens.bwdRevolutionCnt= 0;
		_cfg->pulser.elster.rteOffsetValue= 0;
		_cfg->pulser.elster.rteLastSavedValue= 0;
		_cfg->pulser.rtePrevErrorPatternCount= 0;

		_cfg->log.device.contextSaved= false;
		_cfg->log.device.logConfigured= false;
		_cfg->log.device.deviceLogEnabled= false;
		_cfg->log.device.tamperLogEnabled= false;
		_cfg->log.device.tickType= HOUR_TickType;
		_cfg->log.device.tickSize= 1;
		_cfg->log.device.startType= WAITSTARTTIME_StartType;
		_cfg->log.device.startMarker.second= 0x00;/*in BCD*/
		_cfg->log.device.startMarker.minute= 0x00;/*in BCD*/
		_cfg->log.device.startMarker.hour= 0x00;/*in BCD*/
		_cfg->log.device.startMarker.date= 0xFF;/*in BCD*/
		_cfg->log.device.rte.nextTick= 1;
		_cfg->log.device.rte.currentEEPROMAddress= 0;
		_cfg->log.device.rte.logsInEEPROMFloorIndex= 0;
		_cfg->log.device.rte.logsInEEPROMCeilingIndex= 0;
		_cfg->log.device.rte.logCount= 0;
		_cfg->log.device.rte.periodicLogCount= 0;
		_cfg->log.device.rte.tamperLogCount= 0;
		_cfg->log.device.rte.currentLogTimestamp= 0;

		_cfg->sensors.taskInterval_ms= HOUR_TO_MILISECONDS(1);
		_cfg->sensors.ADCSensorsSamplingInterval_ms= HOUR_TO_MILISECONDS(1);
		_cfg->sensors.temperatureOffset= 0;
		_cfg->sensors.battery.RSense= 0.1;/*in ohm*/
		_cfg->sensors.battery.designCapacity_Ah= 14.0;/*Total 19.0. 3% discharge yearly. 12.0(15yrs) 14.0(10yrs)in Ah*/
		_cfg->sensors.battery.lowThreshold= 10.0;
		_cfg->sensors.battery.rtePrevQH= 0xFFFF;/*QH is counting down*/
		_cfg->sensors.battery.rteQHMultiplier= 0;
		_cfg->sensors.battery.rteMinCurrent= 99999;
		_cfg->sensors.battery.rteMaxCurrent= 0;
		_cfg->sensors.flow.enabled= true;
		_cfg->sensors.flow.samplingPeriod_s= 60;
		_cfg->sensors.flow.samplingTimeMultiplier= 1;
		_cfg->sensors.flow.backflowThreshold= 16;
		_cfg->sensors.flow.burstSamplingPeriod_s= 604800;
		_cfg->sensors.flow.burstThreshold= 3500;
		_cfg->sensors.flow.noflowSamplingPeriod_s= 604800;
		_cfg->sensors.flow.leakageThreshold= 16;
		_cfg->sensors.flow.leakageSamplingPeriod_s= 604800;
		_cfg->sensors.flow.status.flowrate= 0;
		_cfg->sensors.flow.status.flowrateMin= 3000;
		_cfg->sensors.flow.status.flowrateMax= 0;
		_cfg->sensors.flow.status.flowrateHourly= 0;
		_cfg->sensors.flow.status.backflow= 0;
		_cfg->sensors.flow.status.backflowMax= 0;
		_cfg->sensors.flow.status.backflowFlag= false;
		_cfg->sensors.flow.status.backflowDetected= false;
		_cfg->sensors.flow.status.burstFlag= false;
		_cfg->sensors.flow.status.burstDetected= false;
		_cfg->sensors.flow.status.noflowFlag= false;
		_cfg->sensors.flow.status.noflowDetected= false;
		_cfg->sensors.flow.status.leakageFlag= false;
		_cfg->sensors.flow.status.leakageDetected= false;
		_cfg->sensors.flow.rte.prevReading_l= 0;
		_cfg->sensors.flow.rte.backflow.prevDirection= 0;
		_cfg->sensors.flow.rte.backflow.currValue= 0;
		_cfg->sensors.flow.rte.backflow.prevValue= 0;
		_cfg->sensors.flow.rte.backflow.prevMarker= 0;
		_cfg->sensors.flow.rte.backflow.totalPulse= 0;
		_cfg->sensors.flow.rte.backflow.valueMarker= 0;
		_cfg->sensors.flow.rte.burst.samplingCount= 0;
		_cfg->sensors.flow.rte.noflow.samplingCount= 0;
		_cfg->sensors.flow.rte.leakage.samplingCount= 0;
		_cfg->sensors.position.intArmedThreshold= 0x36;//0x3E/*80 degree*//*region of which 6D interrupt is generated*/
		_cfg->sensors.position.intDisarmedThreshold= 0x1F;/*30 degree*//*clearance from threshold of which 6d interrupt of the axis need to be disabled*/
		_cfg->sensors.position.intDisarmedThreshold= 0x1F;/*30 degree*//*clearance from threshold of which 6d interrupt of the axis need to be disabled*/

//		_cfg->alarm.backOffPeriod_ms= 60000;
//		_cfg->alarm.object[PBMAGCOUNT_Alarm]= 	{true, PBMAGCOUNT_Sensor, 			ABOVE_AND_EQUAL_AlarmThreshold, 1, AND_AlarmThreshold, NONE_AlarmThreshold, 0, 1, 15000};
//		_cfg->alarm.object[TAMPERINCOUNT_Alarm]= {true, TAMPERINCOUNT_Sensor, 		ABOVE_AND_EQUAL_AlarmThreshold, 1, AND_AlarmThreshold, NONE_AlarmThreshold, 0, 1, 15000};
//		_cfg->alarm.object[RSTAMPERCOUNT_Alarm]= {true, RSTAMPERCOUNT_Sensor, 		ABOVE_AND_EQUAL_AlarmThreshold, 1, AND_AlarmThreshold, NONE_AlarmThreshold, 0, 1, 15000};
//		_cfg->alarm.object[VOLTAGE_Alarm]= 		{true, INTERNAL_VOLTAGE_Sensor, 	ABOVE_AND_EQUAL_AlarmThreshold, 1, AND_AlarmThreshold, NONE_AlarmThreshold, 0, 1, 15000};
//		_cfg->alarm.object[TEMPERATURE_Alarm]= 	{true, INTERNAL_TEMPERATURE_Sensor, ABOVE_AND_EQUAL_AlarmThreshold, 1, AND_AlarmThreshold, NONE_AlarmThreshold, 0, 1, 15000};
//		_cfg->alarm.object[CELLCAPACITY_Alarm]= 	{true, CELLCAPACITY_Sensor, 		ABOVE_AND_EQUAL_AlarmThreshold, 1, AND_AlarmThreshold, NONE_AlarmThreshold, 0, 1, 15000};
//		_cfg->alarm.object[AVECURRENT_Alarm]= 	{true, AVECURRENT_Sensor, 			ABOVE_AND_EQUAL_AlarmThreshold, 1, AND_AlarmThreshold, NONE_AlarmThreshold, 0, 1, 15000};
//		_cfg->alarm.object[POSITION_X_Alarm]= 	{true, POSITION_X_Sensor, 			ABOVE_AND_EQUAL_AlarmThreshold, 1, AND_AlarmThreshold, NONE_AlarmThreshold, 0, 1, 15000};
//		_cfg->alarm.object[POSITION_Y_Alarm]= 	{true, POSITION_Y_Sensor, 			ABOVE_AND_EQUAL_AlarmThreshold, 1, AND_AlarmThreshold, NONE_AlarmThreshold, 0, 1, 15000};
//		_cfg->alarm.object[POSITION_Z_Alarm]= 	{true, POSITION_Z_Sensor, 			ABOVE_AND_EQUAL_AlarmThreshold, 1, AND_AlarmThreshold, NONE_AlarmThreshold, 0, 1, 15000};
//		_cfg->alarm.object[TILTCOUNT_Alarm]= 	{true, TILTCOUNT_Sensor, 			ABOVE_AND_EQUAL_AlarmThreshold, 1, AND_AlarmThreshold, NONE_AlarmThreshold, 0, 1, 15000};
//		_cfg->alarm.object[PIPE_BURST_Alarm]= 	{true, PIPE_FLOWRATE_Sensor, 		ABOVE_AND_EQUAL_AlarmThreshold, 1, AND_AlarmThreshold, NONE_AlarmThreshold, 0, 1, 15000};
//		_cfg->alarm.object[PIPE_EMPTY_Alarm]= 	{true, PIPE_FLOWRATE_Sensor, 		ABOVE_AND_EQUAL_AlarmThreshold, 1, AND_AlarmThreshold, NONE_AlarmThreshold, 0, 1, 15000};
//		_cfg->alarm.object[PIPE_LEAK_Alarm]= 	{true, PIPE_BACKFLOW_Sensor, 		ABOVE_AND_EQUAL_AlarmThreshold, 1, AND_AlarmThreshold, NONE_AlarmThreshold, 0, 1, 15000};
//		_cfg->alarm.object[PIPE_BACKFLOW_Alarm]= {true, INTERNAL_VOLTAGE_Sensor, 	ABOVE_AND_EQUAL_AlarmThreshold, 1, AND_AlarmThreshold, NONE_AlarmThreshold, 0, 1, 15000};

		// _cfg->nbiot.apn= "nbiot";
		static const char NBIOT_APN_temp[] = "nbiot";
		memcpy(_cfg->nbiot.apn, NBIOT_APN_temp, sizeof(NBIOT_APN_temp));	// nbiot.apn[NBIOT_CFG_APN_LEN 32 + 1]
		// _cfg->nbiot.pdpAddress= UNINIT_STRING;
		// _cfg->nbiot.imei= UNINIT_STRING;
		// _cfg->nbiot.imsi= UNINIT_STRING;
		// _cfg->nbiot.iccid= UNINIT_STRING;
		memset(_cfg->nbiot.pdpAddress, 0, sizeof(_cfg->nbiot.pdpAddress));
		memset(_cfg->nbiot.imei, 0, sizeof(_cfg->nbiot.imei));
		memset(_cfg->nbiot.imsi, 0, sizeof(_cfg->nbiot.imsi));
		memset(_cfg->nbiot.iccid, 0, sizeof(_cfg->nbiot.iccid));
		_cfg->nbiot.modemMode= APPLICATION_ModemMode;
		_cfg->nbiot.activeTime= 0;/*rrc drop, no need edrx,*/
		_cfg->nbiot.periodicTau= (3600* 24);/*one day*/
		_cfg->nbiot.edrx= 81920;
		_cfg->nbiot.pagingTime= 2560;
		_cfg->nbiot.delayBetweenCSQ_s= 2;
		_cfg->nbiot.maximumCSQRetry= 240;
		_cfg->nbiot.restartDelay_s= 157680000;/*5 years/off by default*///HOUR_TO_SECONDS(3),//(24* 60* 60)*/
		_cfg->nbiot.maxUnknwonFailureAllowed= 3;
		_cfg->nbiot.rteActiveTime= 0;
		_cfg->nbiot.rtePeriodicTau= 0;
		_cfg->nbiot.rteEdrx= 0;
		_cfg->nbiot.rtePagingTime= 0;
		_cfg->nbiot.stats.noOfTransmission= 0;
		_cfg->nbiot.stats.noOfFailedTransmission= 0;
		_cfg->nbiot.stats.noOfAttach= 0;
		_cfg->nbiot.stats.noOfDisattach= 0;
		_cfg->nbiot.stats.noOfSimError= 0;
		_cfg->nbiot.stats.latency_ms= 0;
		_cfg->nbiot.stats.aveLatency_ms= 0;
		_cfg->nbiot.stats.minLatency_ms= 0xFFFF;
		_cfg->nbiot.stats.maxLatency_ms= 0;
		_cfg->nbiot.stats.pingLatency_ms= 0;
		_cfg->nbiot.stats.failsafeRebootCount= 0;
		_cfg->nbiot.stats.PVDRebootCount= 0;
		_cfg->nbiot.stats.BORRebootCount= 0;
		_cfg->nbiot.stats.rsrp= 0;
		_cfg->nbiot.stats.aveRsrp= 0;
		_cfg->nbiot.stats.minRsrp= 200;
		_cfg->nbiot.stats.maxRsrp= -200;
		_cfg->nbiot.stats.rssi= 0;
		_cfg->nbiot.stats.aveRssi= 0;
		_cfg->nbiot.stats.minRssi= 200;
		_cfg->nbiot.stats.maxRssi= -200;
		_cfg->nbiot.stats.sinr= 0;
		_cfg->nbiot.stats.aveSinr= 0;
		_cfg->nbiot.stats.minSinr= 127;
		_cfg->nbiot.stats.maxSinr= -127;
		_cfg->nbiot.stats.rsrq= 0;
		_cfg->nbiot.stats.aveRsrq= 0;
		_cfg->nbiot.stats.minRsrq= 127;
		_cfg->nbiot.stats.maxRsrq= -127;
		_cfg->nbiot.stats.txPower= 0;
		_cfg->nbiot.stats.aveTxPower= 0;
		_cfg->nbiot.stats.minTxPower= 255;
		_cfg->nbiot.stats.maxTxPower= 0;
		_cfg->nbiot.stats.ceMode= 0;
		_cfg->nbiot.stats.ecl= 0;
		_cfg->nbiot.stats.battVoltage_mV= 3600;
		_cfg->nbiot.rteTotalLatency_ms= 0;
		_cfg->nbiot.rteSignalSampleCount= 0;
		_cfg->nbiot.rteTotalRsrp= 0;
		_cfg->nbiot.rteTotalRssi= 0;
		_cfg->nbiot.rteTotalSinr= 0;
		_cfg->nbiot.rteTotalRsrq= 0;
		_cfg->nbiot.rteTotalTxPower= 0;
		_cfg->nbiot.enableRRCDrop= true;
		_cfg->nbiot.RRCDropPeriod_s= 3;
		_cfg->nbiot.downlinkWaitPeriod_s= 2;/*decada needs at least 2 seconds to reconnet mqtt, 5 seconds to send downlink command*/
		_cfg->nbiot.enableAttachOnMagnetTamper= true;
		_cfg->nbiot.enableAttachOnPulserThreshold= true;
		_cfg->nbiot.pulserThresholdValueForAttach_liter= 500; /*must be more than production flow test value*/
		_cfg->nbiot.enableAttachOnTiltTamper= false;
		_cfg->nbiot.tiltTamperAttachBackoff_s= HOUR_TO_SECONDS(3);/*roughly handling during installation*/
		_cfg->nbiot.enableGkcoapOnAttach= true;
		_cfg->nbiot.enableLwm2mOnAttach= true;
		_cfg->nbiot.backoffOnCereg0= false;
		_cfg->nbiot.bypassBuckBoost= false;
		_cfg->nbiot.fastModemReboot= false;/*don't skip before production done*/
		_cfg->nbiot.reattachWait_s= 30;
		_cfg->nbiot.gkcoap.enabled= false;
		_cfg->nbiot.gkcoap.localPort= 56830;/*for bc66 coap AT commands*/
		//_cfg->nbiot.gkcoap.activeServer= 1;

		// _cfg->nbiot.gkcoap.serverIP= "223.25.247.73";
		// _cfg->nbiot.gkcoap.serverURI= "/device";
		// _cfg->nbiot.gkcoap.fotaServerIP= "223.25.247.73";
		// _cfg->nbiot.gkcoap.fotaServerURI= "/gkm-swmgt/latest";
		static const char NBIOT_GKCOAP_SERVER_IP_temp[] = "223.25.247.73";
		static const char NBIOT_GKCOAP_SERVER_URL_temp[] = "/device";
		static const char NBIOT_GKCOAP_FOTASERVER_IP_temp[] =  "223.25.247.73";
		static const char NBIOT_GKCOAP_FOTASERVER_URL_temp[] = "/gkm-swmgt/latest";
		memcpy(_cfg->nbiot.gkcoap.serverIP, NBIOT_GKCOAP_SERVER_IP_temp, sizeof(NBIOT_GKCOAP_SERVER_IP_temp));	// char serverIP[150]
		memcpy(_cfg->nbiot.gkcoap.serverURI, NBIOT_GKCOAP_SERVER_URL_temp, sizeof(NBIOT_GKCOAP_SERVER_URL_temp));	// char serverURI[35+ 1]
		memcpy(_cfg->nbiot.gkcoap.fotaServerIP, NBIOT_GKCOAP_FOTASERVER_IP_temp, sizeof(NBIOT_GKCOAP_FOTASERVER_IP_temp));	// fotaServerIP[150];
		memcpy(_cfg->nbiot.gkcoap.fotaServerURI, NBIOT_GKCOAP_FOTASERVER_URL_temp, sizeof(NBIOT_GKCOAP_FOTASERVER_URL_temp));	// fotaServerURI[35+ 1]
		_cfg->nbiot.gkcoap.serverPort= 5653;
		_cfg->nbiot.gkcoap.fotaServerPort= 36000;
		//_cfg->nbiot.gkcoap.mode= BC66LINK_COAP_ENCRYPTION_NONE;
		_cfg->nbiot.gkcoap.retryBackoffMin_s= 60;
		_cfg->nbiot.gkcoap.retryBackoffMax_s= 360;
		_cfg->nbiot.gkcoap.retryMax= 3;
		_cfg->nbiot.gkcoap.packetType= 0x32;
		_cfg->nbiot.gkcoap.logTickType= 2;
		_cfg->nbiot.gkcoap.logTickSize= 1;
		_cfg->nbiot.gkcoap.logTickStartMask= 0xFFFF0000;
		_cfg->nbiot.gkcoap.reportStartMask= 0xFFFFFFFF00000000;/*0xYYMMDDhhmmss0000*/
		_cfg->nbiot.gkcoap.reportInterval_s= HOUR_TO_SECONDS(24);
		_cfg->nbiot.gkcoap.reportTriggerType= START_TIME_ReportTrigger;
		_cfg->nbiot.gkcoap.reportTriggerLogCount= 0;
		_cfg->nbiot.gkcoap.rteStartTime= 0;
		_cfg->nbiot.gkcoap.rteNextTime= 0;
		_cfg->nbiot.gkcoap.rte.reg.state= GKCOAP_REG_STATE_IDLE;
		_cfg->nbiot.gkcoap.rte.reg.isRequested= true;
		_cfg->nbiot.gkcoap.rte.reg.isCompleted= false;
		_cfg->nbiot.gkcoap.rte.reg.retryCount= 0;
		_cfg->nbiot.gkcoap.rte.report.state= GKCOAP_REPORT_STATE_IDLE;
		_cfg->nbiot.gkcoap.rte.report.isRequested= false;
		_cfg->nbiot.gkcoap.rte.report.isCompleted= false;
		_cfg->nbiot.gkcoap.rte.report.retryCount= 0;
		_cfg->nbiot.gkcoap.rte.report.noOfLogsToReport= 0;
		_cfg->nbiot.gkcoap.rte.report.noOfLogsReported= 0;
		_cfg->nbiot.gkcoap.rte.fota.state= Idle_FotaState;
		_cfg->nbiot.gkcoap.rte.fota.isRequested= false;
		_cfg->nbiot.gkcoap.rte.fota.isCompleted= false;
		_cfg->nbiot.gkcoap.rte.fota.retryCount= 0;
		_cfg->nbiot.gkcoap.alarmActivePeriod_s= 120;
		_cfg->nbiot.gkcoap.rteAlarmClearTimestamp= 0;
		_cfg->nbiot.lwm2m.enabled= false;
		_cfg->nbiot.lwm2m.enableBootstrap= true;
		// _cfg->nbiot.lwm2m.defaultConnection.serverIP= "18.142.171.195";//"lwm2m.thingsboard.cloud",//"223.25.247.73", /*"leshan.eclipseprojects.io" "demo.gkmetering.com"*/
		static const char NBIOT_LWM2M_DEFAULTCONN_SERVER_IP_temp[] = "18.142.171.195";
		memcpy(_cfg->nbiot.lwm2m.defaultConnection.serverIP, NBIOT_LWM2M_DEFAULTCONN_SERVER_IP_temp, sizeof(NBIOT_LWM2M_DEFAULTCONN_SERVER_IP_temp));	// char serverIP[150]
		_cfg->nbiot.lwm2m.defaultConnection.serverPort= 5784;//36002,//5683,/*5683 5684(psk) 5783(bootstrap) 5784(bootstrap-PSK)*;
		// _cfg->nbiot.lwm2m.defaultConnection.endpointName= "";//"urn:imei:867997039346084",//"urn:imei:867997039345904",//"urn:imei:867997039346084",//"gk-default-endpoint",//"gk-test-00"/
		memset(_cfg->nbiot.lwm2m.defaultConnection.endpointName, 0, sizeof(_cfg->nbiot.lwm2m.defaultConnection.endpointName));
		_cfg->nbiot.lwm2m.defaultConnection.lifetime_s= 86400;
		_cfg->nbiot.lwm2m.defaultConnection.securityMode= PSK_Bc66LinkLwm2mSecurityMode;//NONE_Bc66LinkLwm2mSecurityMode, /*BC66LINK_LWM2M_SECURITY_MODE_NONE BC66LINK_LWM2M_SECURITY_MODE_PSK*/
		// _cfg->nbiot.lwm2m.defaultConnection.pskId= "GK";
		static const char NBIOT_LWM2M_DEFAULTCONN_PSKID_temp[] = "GK";
		memcpy(_cfg->nbiot.lwm2m.defaultConnection.pskId, NBIOT_LWM2M_DEFAULTCONN_PSKID_temp, sizeof(NBIOT_LWM2M_DEFAULTCONN_PSKID_temp));	// char serverIP[150]
		// _cfg->nbiot.lwm2m.defaultConnection.psk= "6B552929767446753678";
		static const char NBIOT_LWM2M_DEFAULTCONN_PSK_temp[] = "6B552929767446753678";
		memcpy(_cfg->nbiot.lwm2m.defaultConnection.psk, NBIOT_LWM2M_DEFAULTCONN_PSK_temp, sizeof(NBIOT_LWM2M_DEFAULTCONN_PSK_temp));	// char serverIP[150]
		// _cfg->nbiot.lwm2m.currentConnection.serverIP= "";
		memset(_cfg->nbiot.lwm2m.currentConnection.serverIP, 0, sizeof(_cfg->nbiot.lwm2m.currentConnection.serverIP));
		_cfg->nbiot.lwm2m.currentConnection.serverPort= 5683;
		// _cfg->nbiot.lwm2m.currentConnection.endpointName= "";
		memset(_cfg->nbiot.lwm2m.currentConnection.endpointName, 0, sizeof(_cfg->nbiot.lwm2m.currentConnection.endpointName));
		_cfg->nbiot.lwm2m.currentConnection.lifetime_s= 86400;
		_cfg->nbiot.lwm2m.currentConnection.securityMode= NONE_Bc66LinkLwm2mSecurityMode;
		// _cfg->nbiot.lwm2m.currentConnection.pskId= "";
		memset(_cfg->nbiot.lwm2m.currentConnection.pskId, 0, sizeof(_cfg->nbiot.lwm2m.currentConnection.pskId));
		// _cfg->nbiot.lwm2m.currentConnection.psk= "";
		memset(_cfg->nbiot.lwm2m.currentConnection.psk, 0, sizeof(_cfg->nbiot.lwm2m.currentConnection.psk));
		_cfg->nbiot.lwm2m.registerBackoff_s= 28800;
		_cfg->nbiot.lwm2m.notifyACK= 1;
		_cfg->nbiot.lwm2m.notifyRAI= 0;/*set to 0 by default because we have separate RAI handling*/
		_cfg->nbiot.lwm2m.reregisterAfterPSM= false;

		// _cfg->nbiot.lwm2m.lwObj[SOFTWARE_MANAGEMENT_ObjName]= {
		// 		.type= SWMGT_ObjType,
		// 		.id= 9,
		// 		.version= 1.0,
		// 		.instance= FOTA_SwMgtInstance,
		// 		.resourceCount= MAX_SwMgtResource,
		// 		.resource= config_cfg->nbiot.lwm2m.swmgt.resource,
		// 		.rte.swMgt= &(config_cfg->nbiot.lwm2m.swmgt.rte),
		// };
		LWOBJ_Obj_t *sw_mgt_obj = &(_cfg->nbiot.lwm2m.lwObj[SOFTWARE_MANAGEMENT_ObjName]);
		sw_mgt_obj->type = SWMGT_ObjType;
		sw_mgt_obj->id = 9;
		sw_mgt_obj->version = 1.0;
		sw_mgt_obj->instance = FOTA_SwMgtInstance;
		sw_mgt_obj->resourceCount = MAX_SwMgtResource;
		sw_mgt_obj->resource = _cfg->nbiot.lwm2m.swmgt.resource;
		sw_mgt_obj->rte.swMgt = &(_cfg->nbiot.lwm2m.swmgt.rte);


		// _cfg->nbiot.lwm2m.lwObj[PRACT_GET_READING_ObjName]= {
		// 		.type= PRACT_ObjType,
		// 		.id= 10376,
		// 		.version = 1.0,
		// 		.instance= GET_READING_PractInstance,
		// 		.resourceCount= MAX_PractResource,
		// 		.resource= config_cfg->nbiot.lwm2m.pract.resource[GET_READING_PractInstance],
		// 		.rte.prAct= &(config_cfg->nbiot.lwm2m.pract.rte[GET_READING_PractInstance]),
		// };
		LWOBJ_Obj_t *pract_reading_obj = &(_cfg->nbiot.lwm2m.lwObj[PRACT_GET_READING_ObjName]);
		pract_reading_obj->type = PRACT_ObjType;
		pract_reading_obj->id = 10376;
		pract_reading_obj->version = 1.0;
		pract_reading_obj->instance= GET_READING_PractInstance;
		pract_reading_obj->resourceCount= MAX_PractResource;
		pract_reading_obj->resource= _cfg->nbiot.lwm2m.pract.resource[GET_READING_PractInstance];
		pract_reading_obj->rte.prAct= &(_cfg->nbiot.lwm2m.pract.rte[GET_READING_PractInstance]);
		
		// _cfg->nbiot.lwm2m.lwObj[PRACT_GET_STATUS_ObjName]= {
		// 		.type= PRACT_ObjType,
		// 		.id= 10376,
		// 		.version = 1.0,
		// 		.instance= GET_STATUS_PractInstance,
		// 		.resourceCount= MAX_PractResource,
		// 		.resource= config_cfg->nbiot.lwm2m.pract.resource[GET_STATUS_PractInstance],
		// 		.rte.prAct= &(config_cfg->nbiot.lwm2m.pract.rte[GET_STATUS_PractInstance]),
		// };
		LWOBJ_Obj_t *pract_status_objname = &(_cfg->nbiot.lwm2m.lwObj[PRACT_GET_STATUS_ObjName]);
		pract_status_objname->type= PRACT_ObjType;
		pract_status_objname->id= 10376;
		pract_status_objname->version = 1.0;
		pract_status_objname->instance= GET_STATUS_PractInstance;
		pract_status_objname->resourceCount= MAX_PractResource;
		pract_status_objname->resource= _cfg->nbiot.lwm2m.pract.resource[GET_STATUS_PractInstance];
		pract_status_objname->rte.prAct= &(_cfg->nbiot.lwm2m.pract.rte[GET_STATUS_PractInstance]);

		// _cfg->nbiot.lwm2m.lwObj[PRACT_QUERY_ObjName]= {
		// 		.type= PRACT_ObjType,
		// 		.id= 10376,
		// 		.version = 1.0,
		// 		.instance= QUERY_PractInstance,
		// 		.resourceCount= MAX_PractResource,
		// 		.resource= config_cfg->nbiot.lwm2m.pract.resource[QUERY_PractInstance],
		// 		.rte.prAct= &(config_cfg->nbiot.lwm2m.pract.rte[QUERY_PractInstance]),
		// };
		LWOBJ_Obj_t *pract_query_objname = &(_cfg->nbiot.lwm2m.lwObj[PRACT_QUERY_ObjName]);
		pract_query_objname->type= PRACT_ObjType;
		pract_query_objname->id= 10376;
		pract_query_objname->version = 1.0;
		pract_query_objname->instance= QUERY_PractInstance;
		pract_query_objname->resourceCount= MAX_PractResource;
		pract_query_objname->resource= _cfg->nbiot.lwm2m.pract.resource[QUERY_PractInstance];
		pract_query_objname->rte.prAct= &(_cfg->nbiot.lwm2m.pract.rte[QUERY_PractInstance]);
		
		// _cfg->nbiot.lwm2m.lwObj[DTMON_ALARMS_ObjName]= {
		// 		.type= DTMON_ObjType,
		// 		.id= 10377,
		// 		.version = 1.0,
		// 		.instance= ALARMS_DtmonInstance,
		// 		.resourceCount= MAX_DtmonResource,
		// 		.resource= config_cfg->nbiot.lwm2m.dtmon.resource[ALARMS_DtmonInstance],
		// 		.rte.dtMon= &(config_cfg->nbiot.lwm2m.dtmon.rte[ALARMS_DtmonInstance]),
		// };
		LWOBJ_Obj_t *dtmon_alarms_objname = &(_cfg->nbiot.lwm2m.lwObj[DTMON_ALARMS_ObjName]);
		dtmon_alarms_objname->type= DTMON_ObjType;
		dtmon_alarms_objname->id= 10377;
		dtmon_alarms_objname->version = 1.0;
		dtmon_alarms_objname->instance= ALARMS_DtmonInstance;
		dtmon_alarms_objname->resourceCount= MAX_DtmonResource;
		dtmon_alarms_objname->resource = _cfg->nbiot.lwm2m.dtmon.resource[ALARMS_DtmonInstance];
		dtmon_alarms_objname->rte.dtMon = &(_cfg->nbiot.lwm2m.dtmon.rte[ALARMS_DtmonInstance]);

		// _cfg->nbiot.lwm2m.lwObj[DTMON_BACKFLOW_ObjName] = {
		// 	.type = DTMON_ObjType,
		// 	.id = 10377,
		// 	.version = 1.0,
		// 	.instance = BACKFLOW_DtmonInstance,
		// 	.resourceCount = MAX_DtmonResource,
		// 	.resource = config_cfg->nbiot.lwm2m.dtmon.resource[BACKFLOW_DtmonInstance],
		// 	.rte.dtMon = &(config_cfg->nbiot.lwm2m.dtmon.rte[BACKFLOW_DtmonInstance]),
		// };

		// _cfg->nbiot.lwm2m.swmgt.resource= {
		// 	{.attr= {PKG_NAME_SwMgtResource, READ_ResourceOperation, STRING_ResourceType}, .observe= true},
		// 	{.attr= {PKG_VERSION_SwMgtResource, READ_ResourceOperation, STRING_ResourceType}, .observe= true},
		// 	{.attr= {PACKAGE_SwMgtResource,  WRITE_ResourceOperation, INTEGER_ResourceType}},
		// 	{.attr= {PACKAGE_URI_SwMgtResource, WRITE_ResourceOperation, STRING_ResourceType}, .value.string= config_cfg->nbiot.lwm2m.swmgt.uri, .valueMaxLen= SWMGT_CFG_URI_MAX_LEN},
		// 	{.attr= {INSTALL_SwMgtResource, EXECUTE_ResourceOperation, NONE_ResourceType}},
		// 	{.attr= {FILLER1_SwMgtResource,  READ_ResourceOperation, INTEGER_ResourceType}},
		// 	{.attr= {UNINSTALL_SwMgtResource, EXECUTE_ResourceOperation, NONE_ResourceType}},
		// 	{.attr= {UPDATE_STATE_SwMgtResource, READ_ResourceOperation, INTEGER_ResourceType}, .observe= true},
		// 	{.attr= {FILLER2_SwMgtResource,  READ_ResourceOperation, INTEGER_ResourceType}},
		// 	{.attr= {UPDATE_RESULT_SwMgtResource, READ_ResourceOperation, INTEGER_ResourceType}, .observe= true},
		// 	{.attr= {ACTIVATE_SwMgtResource, EXECUTE_ResourceOperation, NONE_ResourceType}},
		// 	{.attr= {DEACTIVATE_SwMgtResource, EXECUTE_ResourceOperation, NONE_ResourceType}},
		// 	{.attr= {ACTIVATION_STATE_SwMgtResource, READ_ResourceOperation, BOOLEAN_ResourceType}, .observe= true},
		// };
		static const LWOBJ_Resource_t SWMGT_RESOURCES_DEFAULT[MAX_SwMgtResource] = {
			{.attr = {PKG_NAME_SwMgtResource, READ_ResourceOperation, STRING_ResourceType}, .observe = true},
			{.attr = {PKG_VERSION_SwMgtResource, READ_ResourceOperation, STRING_ResourceType}, .observe = true},
			{.attr = {PACKAGE_SwMgtResource,  WRITE_ResourceOperation, INTEGER_ResourceType}},
			{.attr = {PACKAGE_URI_SwMgtResource, WRITE_ResourceOperation, STRING_ResourceType}, .value.string = NULL, .valueMaxLen = SWMGT_CFG_URI_MAX_LEN}, // Set pointer to NULL for now
			{.attr = {INSTALL_SwMgtResource, EXECUTE_ResourceOperation, NONE_ResourceType}},
			{.attr = {FILLER1_SwMgtResource,  READ_ResourceOperation, INTEGER_ResourceType}},
			{.attr = {UNINSTALL_SwMgtResource, EXECUTE_ResourceOperation, NONE_ResourceType}},
			{.attr = {UPDATE_STATE_SwMgtResource, READ_ResourceOperation, INTEGER_ResourceType}, .observe = true},
			{.attr = {FILLER2_SwMgtResource,  READ_ResourceOperation, INTEGER_ResourceType}},
			{.attr = {UPDATE_RESULT_SwMgtResource, READ_ResourceOperation, INTEGER_ResourceType}, .observe = true},
			{.attr = {ACTIVATE_SwMgtResource, EXECUTE_ResourceOperation, NONE_ResourceType}},
			{.attr = {DEACTIVATE_SwMgtResource, EXECUTE_ResourceOperation, NONE_ResourceType}},
			{.attr = {ACTIVATION_STATE_SwMgtResource, READ_ResourceOperation, BOOLEAN_ResourceType}, .observe = true},
		};
		memcpy(_cfg->nbiot.lwm2m.swmgt.resource, SWMGT_RESOURCES_DEFAULT, sizeof(SWMGT_RESOURCES_DEFAULT));

		// _cfg->nbiot.lwm2m.swmgt.uri= "";
		memset(_cfg->nbiot.lwm2m.swmgt.uri, 0, sizeof(_cfg->nbiot.lwm2m.swmgt.uri));

		// _cfg->nbiot.lwm2m.pract.resource[GET_READING_PractInstance]= {
		// 		{.attr= {NAME_PractResource, READ_ResourceOperation, STRING_ResourceType}},
		// 		{.attr= {DESC_PractResource, READ_ResourceOperation, STRING_ResourceType}},
		// 		{.attr= {SETTINGS_PractResource, READWRITE_ResourceOperation, OPAQUE_ResourceType}, .value.opaque= config_cfg->nbiot.lwm2m.pract.practSetting0, .valueMaxLen= PRACT_CFG_0_SETTINGS_RESOURCE_MAX_LEN},
		// 		{.attr= {START_MASK_PractResource, READWRITE_ResourceOperation, STRING_ResourceType}, .value.string= config_cfg->nbiot.lwm2m.pract.startMask[GET_READING_PractInstance], .valueMaxLen= PRACT_CFG_MASK_RESOURCE_LEN},
		// 		{.attr= {PERIODIC_INTERVAL_PractResource, READWRITE_ResourceOperation, INTEGER_ResourceType}, .value.integer= 3600},
		// 		{.attr= {RUN_PERIOD_PractResource, READWRITE_ResourceOperation, INTEGER_ResourceType}, .value.integer= 0x00FFFFFF},/*if set all ff nanti stopped immediately cos + run period and overflowed*/
		// 		{.attr= {RECORD_PractResource, READ_ResourceOperation, OPAQUE_ResourceType}},
		// 		{.attr= {RECORD_HEAD_PractResource, READ_ResourceOperation, INTEGER_ResourceType}},
		// 		{.attr= {RECORD_TAIL_PractResource, READ_ResourceOperation, INTEGER_ResourceType}},
		// 		{.attr= {RECORD_READ_PractResource, READWRITE_ResourceOperation, INTEGER_ResourceType}},
		// 		{.attr= {RECORD_DISPATCH_START_MASK_PractResource, READWRITE_ResourceOperation, STRING_ResourceType}, .value.string= config_cfg->nbiot.lwm2m.pract.recordDispatchStartMask[GET_READING_PractInstance], .valueMaxLen= PRACT_CFG_MASK_RESOURCE_LEN},
		// 		{.attr= {RECORD_DISPATCH_PERIODIC_INTERVAL_PractResource, READWRITE_ResourceOperation, INTEGER_ResourceType}, .value.integer= 3600},
		// };
		// At the top of cfg.c (file scope)

		static const LWOBJ_Resource_t PRACT_GET_READING_DEFAULTS[MAX_PractResource] = {
		    {.attr = {NAME_PractResource, READ_ResourceOperation, STRING_ResourceType}},
		    {.attr = {DESC_PractResource, READ_ResourceOperation, STRING_ResourceType}},
		    {
		        .attr = {SETTINGS_PractResource, READWRITE_ResourceOperation, OPAQUE_ResourceType},
		        .value.opaque = NULL, // Set pointer to NULL for now
		        .valueMaxLen = PRACT_CFG_0_SETTINGS_RESOURCE_MAX_LEN
		    },
		    {
		        .attr = {START_MASK_PractResource, READWRITE_ResourceOperation, STRING_ResourceType},
		        .value.string = NULL, // Set pointer to NULL for now
		        .valueMaxLen = PRACT_CFG_MASK_RESOURCE_LEN
		    },
		    {.attr = {PERIODIC_INTERVAL_PractResource, READWRITE_ResourceOperation, INTEGER_ResourceType}, .value.integer = 3600},
		    {.attr = {RUN_PERIOD_PractResource, READWRITE_ResourceOperation, INTEGER_ResourceType}, .value.integer = 0x00FFFFFF},
		    {.attr = {RECORD_PractResource, READ_ResourceOperation, OPAQUE_ResourceType}},
		    {.attr = {RECORD_HEAD_PractResource, READ_ResourceOperation, INTEGER_ResourceType}},
		    {.attr = {RECORD_TAIL_PractResource, READ_ResourceOperation, INTEGER_ResourceType}},
		    {.attr = {RECORD_READ_PractResource, READWRITE_ResourceOperation, INTEGER_ResourceType}},
		    {
		        .attr = {RECORD_DISPATCH_START_MASK_PractResource, READWRITE_ResourceOperation, STRING_ResourceType},
		        .value.string = NULL, // Set pointer to NULL for now
		        .valueMaxLen = PRACT_CFG_MASK_RESOURCE_LEN
		    },
		    {.attr = {RECORD_DISPATCH_PERIODIC_INTERVAL_PractResource, READWRITE_ResourceOperation, INTEGER_ResourceType}, .value.integer = 3600},
		};
		// Inside your CFG_ApplyDefaults(Config_t *_cfg) function

		// 1. Copy the default values from the constant array
		memcpy(_cfg->nbiot.lwm2m.pract.resource[GET_READING_PractInstance],
		       PRACT_GET_READING_DEFAULTS,
		       sizeof(PRACT_GET_READING_DEFAULTS));

		// 2. Now, fix the pointers to point to the correct runtime locations inside _cfg
		_cfg->nbiot.lwm2m.pract.resource[GET_READING_PractInstance][SETTINGS_PractResource].value.opaque =
		    _cfg->nbiot.lwm2m.pract.practSetting0;

		_cfg->nbiot.lwm2m.pract.resource[GET_READING_PractInstance][START_MASK_PractResource].value.string =
		    _cfg->nbiot.lwm2m.pract.startMask[GET_READING_PractInstance];

		_cfg->nbiot.lwm2m.pract.resource[GET_READING_PractInstance][RECORD_DISPATCH_START_MASK_PractResource].value.string =
		    _cfg->nbiot.lwm2m.pract.recordDispatchStartMask[GET_READING_PractInstance];


		_cfg->nbiot.lwm2m.pract.rte[GET_READING_PractInstance].runState= STOPPED_PractRunState;
		_cfg->nbiot.lwm2m.pract.rte[GET_READING_PractInstance].log.partition= M95M01_PARTITION_PRACT_0;
		_cfg->nbiot.lwm2m.pract.rte[GET_READING_PractInstance].log.head= 0;
		_cfg->nbiot.lwm2m.pract.rte[GET_READING_PractInstance].log.elementCount= 0;
		_cfg->nbiot.lwm2m.pract.rte[GET_READING_PractInstance].log.elementMax= PRACT_CFG_0_RECORD_RESOURCE_MAX_COUNT;
		_cfg->nbiot.lwm2m.pract.rte[GET_READING_PractInstance].log.elementSize= sizeof(PRACT_GetReadingRecord_t);

		// _cfg->nbiot.lwm2m.pract.startMask[GET_READING_PractInstance]= "--------0000";
		// _cfg->nbiot.lwm2m.pract.recordDispatchStartMask[GET_READING_PractInstance]= "--------0000";
		static const char PRACTINSTANCE_TEMP1[] = "--------0000";
		static const char PRACTINSTANCE_TEMP2[] = "------000000";
		memcpy(_cfg->nbiot.lwm2m.pract.startMask[GET_READING_PractInstance], PRACTINSTANCE_TEMP1, sizeof(PRACTINSTANCE_TEMP1));
		memcpy(_cfg->nbiot.lwm2m.pract.recordDispatchStartMask[GET_READING_PractInstance], PRACTINSTANCE_TEMP1, sizeof(PRACTINSTANCE_TEMP1));

		// _cfg->nbiot.lwm2m.pract.resource[GET_STATUS_PractInstance]= {
		// 		{.attr= {NAME_PractResource, READ_ResourceOperation, STRING_ResourceType}},
		// 		{.attr= {DESC_PractResource, READ_ResourceOperation, STRING_ResourceType}},
		// 		{.attr= {SETTINGS_PractResource, READWRITE_ResourceOperation, OPAQUE_ResourceType}, .value.opaque= config_cfg->nbiot.lwm2m.pract.practSetting1, .valueMaxLen= PRACT_CFG_1_SETTINGS_RESOURCE_MAX_LEN},
		// 		{.attr= {START_MASK_PractResource, READWRITE_ResourceOperation, STRING_ResourceType}, .value.string= config_cfg->nbiot.lwm2m.pract.startMask[GET_STATUS_PractInstance], .valueMaxLen= PRACT_CFG_MASK_RESOURCE_LEN},
		// 		{.attr= {PERIODIC_INTERVAL_PractResource, READWRITE_ResourceOperation, INTEGER_ResourceType}, .value.integer= 86400/2},/*when we have sent this, we have to wait until 12 hours to have a new one sampled*/
		// 		{.attr= {RUN_PERIOD_PractResource, READWRITE_ResourceOperation, INTEGER_ResourceType}, .value.integer= 10000},
		// 		{.attr= {RECORD_PractResource, READ_ResourceOperation, OPAQUE_ResourceType}},
		// 		{.attr= {RECORD_HEAD_PractResource, READ_ResourceOperation, INTEGER_ResourceType}},
		// 		{.attr= {RECORD_TAIL_PractResource, READ_ResourceOperation, INTEGER_ResourceType}},
		// 		{.attr= {RECORD_READ_PractResource, READWRITE_ResourceOperation, INTEGER_ResourceType}},
		// 		{.attr= {RECORD_DISPATCH_START_MASK_PractResource, READWRITE_ResourceOperation, STRING_ResourceType}, .value.string= config_cfg->nbiot.lwm2m.pract.recordDispatchStartMask[GET_STATUS_PractInstance], .valueMaxLen= PRACT_CFG_MASK_RESOURCE_LEN},
		// 		{.attr= {RECORD_DISPATCH_PERIODIC_INTERVAL_PractResource, READWRITE_ResourceOperation, INTEGER_ResourceType}, .value.integer= 86400},
		// };
		// At the top of your cfg.c file

		static const LWOBJ_Resource_t PRACT_GET_STATUS_DEFAULTS[MAX_PractResource] = {
			{.attr = {NAME_PractResource, READ_ResourceOperation, STRING_ResourceType}},
			{.attr = {DESC_PractResource, READ_ResourceOperation, STRING_ResourceType}},
			{
				.attr = {SETTINGS_PractResource, READWRITE_ResourceOperation, OPAQUE_ResourceType}, 
				.value.opaque = NULL, // Set pointer to NULL for now
				.valueMaxLen = PRACT_CFG_1_SETTINGS_RESOURCE_MAX_LEN
			},
			{
				.attr = {START_MASK_PractResource, READWRITE_ResourceOperation, STRING_ResourceType}, 
				.value.string = NULL, // Set pointer to NULL for now
				.valueMaxLen = PRACT_CFG_MASK_RESOURCE_LEN
			},
			{.attr = {PERIODIC_INTERVAL_PractResource, READWRITE_ResourceOperation, INTEGER_ResourceType}, .value.integer = 86400 / 2},
			{.attr = {RUN_PERIOD_PractResource, READWRITE_ResourceOperation, INTEGER_ResourceType}, .value.integer = 10000},
			{.attr = {RECORD_PractResource, READ_ResourceOperation, OPAQUE_ResourceType}},
			{.attr = {RECORD_HEAD_PractResource, READ_ResourceOperation, INTEGER_ResourceType}},
			{.attr = {RECORD_TAIL_PractResource, READ_ResourceOperation, INTEGER_ResourceType}},
			{.attr = {RECORD_READ_PractResource, READWRITE_ResourceOperation, INTEGER_ResourceType}},
			{
				.attr = {RECORD_DISPATCH_START_MASK_PractResource, READWRITE_ResourceOperation, STRING_ResourceType}, 
				.value.string = NULL, // Set pointer to NULL for now
				.valueMaxLen = PRACT_CFG_MASK_RESOURCE_LEN
			},
			{.attr = {RECORD_DISPATCH_PERIODIC_INTERVAL_PractResource, READWRITE_ResourceOperation, INTEGER_ResourceType}, .value.integer = 86400},
		};
		// Inside your CFG_ApplyDefaults(Config_t *_cfg) function

		// 1. Copy the default values from the constant array
		memcpy(_cfg->nbiot.lwm2m.pract.resource[GET_STATUS_PractInstance], 
			PRACT_GET_STATUS_DEFAULTS, 
			sizeof(PRACT_GET_STATUS_DEFAULTS));

		// 2. Fix the pointers to point to the correct runtime locations within _cfg
		_cfg->nbiot.lwm2m.pract.resource[GET_STATUS_PractInstance][SETTINGS_PractResource].value.opaque = 
		_cfg->nbiot.lwm2m.pract.practSetting1;
		
		_cfg->nbiot.lwm2m.pract.resource[GET_STATUS_PractInstance][START_MASK_PractResource].value.string = 
		_cfg->nbiot.lwm2m.pract.startMask[GET_STATUS_PractInstance];
		
		_cfg->nbiot.lwm2m.pract.resource[GET_STATUS_PractInstance][RECORD_DISPATCH_START_MASK_PractResource].value.string = 
		_cfg->nbiot.lwm2m.pract.recordDispatchStartMask[GET_STATUS_PractInstance];


		_cfg->nbiot.lwm2m.pract.rte[GET_STATUS_PractInstance].runState= STOPPED_PractRunState;
		_cfg->nbiot.lwm2m.pract.rte[GET_STATUS_PractInstance].log.partition= M95M01_PARTITION_PRACT_1;
		_cfg->nbiot.lwm2m.pract.rte[GET_STATUS_PractInstance].log.head= 0;
		_cfg->nbiot.lwm2m.pract.rte[GET_STATUS_PractInstance].log.elementCount= 0;
		_cfg->nbiot.lwm2m.pract.rte[GET_STATUS_PractInstance].log.elementMax= PRACT_CFG_1_RECORD_RESOURCE_MAX_COUNT;
		_cfg->nbiot.lwm2m.pract.rte[GET_STATUS_PractInstance].log.elementSize= sizeof(PRACT_GetStatusRecord_t);
		// _cfg->nbiot.lwm2m.pract.startMask[GET_STATUS_PractInstance]= "--------0000";
		// _cfg->nbiot.lwm2m.pract.recordDispatchStartMask[GET_STATUS_PractInstance]= "------000000";
		memcpy(_cfg->nbiot.lwm2m.pract.startMask[GET_STATUS_PractInstance], PRACTINSTANCE_TEMP1, sizeof(PRACTINSTANCE_TEMP1));
		memcpy(_cfg->nbiot.lwm2m.pract.recordDispatchStartMask[GET_STATUS_PractInstance], PRACTINSTANCE_TEMP2, sizeof(PRACTINSTANCE_TEMP2));

		// _cfg->nbiot.lwm2m.pract.resource[QUERY_PractInstance]= {
		// 		{.attr= {NAME_PractResource, READ_ResourceOperation, STRING_ResourceType}},
		// 		{.attr= {DESC_PractResource, READ_ResourceOperation, STRING_ResourceType}},
		// 		{.attr= {SETTINGS_PractResource, READWRITE_ResourceOperation, OPAQUE_ResourceType}, .value.opaque= config_cfg->nbiot.lwm2m.pract.practSetting2, .valueMaxLen= PRACT_CFG_2_SETTINGS_RESOURCE_MAX_LEN},
		// 		{.attr= {START_MASK_PractResource, READWRITE_ResourceOperation, STRING_ResourceType}, .value.string= config_cfg->nbiot.lwm2m.pract.startMask[QUERY_PractInstance], .valueMaxLen= PRACT_CFG_MASK_RESOURCE_LEN},
		// 		{.attr= {PERIODIC_INTERVAL_PractResource, READWRITE_ResourceOperation, INTEGER_ResourceType}, .value.integer= 86400},
		// 		{.attr= {RUN_PERIOD_PractResource, READWRITE_ResourceOperation, INTEGER_ResourceType}, .value.integer= 1},
		// 		{.attr= {RECORD_PractResource, READ_ResourceOperation, OPAQUE_ResourceType}},
		// 		{.attr= {RECORD_HEAD_PractResource, READ_ResourceOperation, INTEGER_ResourceType}},
		// 		{.attr= {RECORD_TAIL_PractResource, READ_ResourceOperation, INTEGER_ResourceType}},
		// 		{.attr= {RECORD_READ_PractResource, READWRITE_ResourceOperation, INTEGER_ResourceType}},
		// 		{.attr= {RECORD_DISPATCH_START_MASK_PractResource, READWRITE_ResourceOperation, STRING_ResourceType}, .observe= false, .value.string= config_cfg->nbiot.lwm2m.pract.recordDispatchStartMask[QUERY_PractInstance], .notifyState= IDLE_NotifyState, .valueMaxLen= PRACT_CFG_MASK_RESOURCE_LEN},
		// 		{.attr= {RECORD_DISPATCH_PERIODIC_INTERVAL_PractResource, READWRITE_ResourceOperation, INTEGER_ResourceType}, .value.integer= 86400},
		// };
		// At the top of your cfg.c file

		static const LWOBJ_Resource_t PRACT_QUERY_DEFAULTS[MAX_PractResource] = {
			{.attr= {NAME_PractResource, READ_ResourceOperation, STRING_ResourceType}},
			{.attr= {DESC_PractResource, READ_ResourceOperation, STRING_ResourceType}},
			{
				.attr= {SETTINGS_PractResource, READWRITE_ResourceOperation, OPAQUE_ResourceType}, 
				.value.opaque= NULL, // Set pointer to NULL for now
				.valueMaxLen= PRACT_CFG_2_SETTINGS_RESOURCE_MAX_LEN
			},
			{
				.attr= {START_MASK_PractResource, READWRITE_ResourceOperation, STRING_ResourceType}, 
				.value.string= NULL, // Set pointer to NULL for now
				.valueMaxLen= PRACT_CFG_MASK_RESOURCE_LEN
			},
			{.attr= {PERIODIC_INTERVAL_PractResource, READWRITE_ResourceOperation, INTEGER_ResourceType}, .value.integer= 86400},
			{.attr= {RUN_PERIOD_PractResource, READWRITE_ResourceOperation, INTEGER_ResourceType}, .value.integer= 1},
			{.attr= {RECORD_PractResource, READ_ResourceOperation, OPAQUE_ResourceType}},
			{.attr= {RECORD_HEAD_PractResource, READ_ResourceOperation, INTEGER_ResourceType}},
			{.attr= {RECORD_TAIL_PractResource, READ_ResourceOperation, INTEGER_ResourceType}},
			{.attr= {RECORD_READ_PractResource, READWRITE_ResourceOperation, INTEGER_ResourceType}},
			{
				.attr= {RECORD_DISPATCH_START_MASK_PractResource, READWRITE_ResourceOperation, STRING_ResourceType}, 
				.observe= false, 
				.value.string= NULL, // Set pointer to NULL for now
				.notifyState= IDLE_NotifyState, 
				.valueMaxLen= PRACT_CFG_MASK_RESOURCE_LEN
			},
			{.attr= {RECORD_DISPATCH_PERIODIC_INTERVAL_PractResource, READWRITE_ResourceOperation, INTEGER_ResourceType}, .value.integer= 86400},
		};
		// Inside your CFG_ApplyDefaults(Config_t *_cfg) function

		// 1. Copy the default values from the constant array
		memcpy(_cfg->nbiot.lwm2m.pract.resource[QUERY_PractInstance], 
			PRACT_QUERY_DEFAULTS, 
			sizeof(PRACT_QUERY_DEFAULTS));

		// 2. Fix the pointers to point to the correct runtime locations within _cfg
		_cfg->nbiot.lwm2m.pract.resource[QUERY_PractInstance][SETTINGS_PractResource].value.opaque = 
		_cfg->nbiot.lwm2m.pract.practSetting2;
		
		_cfg->nbiot.lwm2m.pract.resource[QUERY_PractInstance][START_MASK_PractResource].value.string = 
		_cfg->nbiot.lwm2m.pract.startMask[QUERY_PractInstance];
		
		_cfg->nbiot.lwm2m.pract.resource[QUERY_PractInstance][RECORD_DISPATCH_START_MASK_PractResource].value.string = 
		_cfg->nbiot.lwm2m.pract.recordDispatchStartMask[QUERY_PractInstance];


		_cfg->nbiot.lwm2m.pract.rte[QUERY_PractInstance].runState= STOPPED_PractRunState;
		_cfg->nbiot.lwm2m.pract.rte[QUERY_PractInstance].log.partition= M95M01_PARTITION_PRACT_2;
		_cfg->nbiot.lwm2m.pract.rte[QUERY_PractInstance].log.head= 0;
		_cfg->nbiot.lwm2m.pract.rte[QUERY_PractInstance].log.elementCount= 0;
		_cfg->nbiot.lwm2m.pract.rte[QUERY_PractInstance].log.elementMax= PRACT_CFG_2_RECORD_RESOURCE_MAX_COUNT;
		_cfg->nbiot.lwm2m.pract.rte[QUERY_PractInstance].log.elementSize= sizeof(PRACT_QueryRecord_t);
		// _cfg->nbiot.lwm2m.pract.startMask[QUERY_PractInstance]= "------000000";
		// _cfg->nbiot.lwm2m.pract.recordDispatchStartMask[QUERY_PractInstance]= "------000000";
		memcpy(_cfg->nbiot.lwm2m.pract.startMask[QUERY_PractInstance], PRACTINSTANCE_TEMP2, sizeof(PRACTINSTANCE_TEMP2));
		memcpy(_cfg->nbiot.lwm2m.pract.recordDispatchStartMask[QUERY_PractInstance], PRACTINSTANCE_TEMP2, sizeof(PRACTINSTANCE_TEMP2));

		// _cfg->nbiot.lwm2m.dtmon.resource[ALARMS_DtmonInstance]= {
		// 		{.attr= {NAME_DtmonResource, READ_ResourceOperation, STRING_ResourceType}},
		// 		{.attr= {DESC_DtmonResource, READ_ResourceOperation, STRING_ResourceType}},
		// 		{.attr= {SETTINGS_DtmonResource, READWRITE_ResourceOperation, OPAQUE_ResourceType}, .value.opaque= config_cfg->nbiot.lwm2m.dtmon.settings[ALARMS_DtmonInstance], .valueMaxLen= DTMON_CFG_SETTINGS_RESOURCE_MAX_LEN},
		// 		{.attr= {DATA_DtmonResource, READ_ResourceOperation, OPAQUE_ResourceType}, .value.opaque= config_cfg->nbiot.lwm2m.dtmon.data[ALARMS_DtmonInstance], .valueMaxLen= DTMON_CFG_RESULTS_VALUE_LEN},
		// 		{.attr= {SAMPLING_START_MASK_DtmonResource, READWRITE_ResourceOperation, STRING_ResourceType}, .value.string= config_cfg->nbiot.lwm2m.dtmon.samplingStartMask[ALARMS_DtmonInstance], .valueMaxLen= DTMON_CFG_MASK_RESOURCE_LEN},
		// 		{.attr= {SAMPLING_INTERVAL_DtmonResource, READWRITE_ResourceOperation, INTEGER_ResourceType}, .value.integer= 30},
		// 		{.attr= {SAMPLING_RUN_PERIOD_DtmonResource, READWRITE_ResourceOperation, INTEGER_ResourceType}, .value.integer= 86400},
		// 		{.attr= {REFERENCE_A_DtmonResource, READWRITE_ResourceOperation, OPAQUE_ResourceType}, .value.opaque= config_cfg->nbiot.lwm2m.dtmon.referenceA[ALARMS_DtmonInstance], .valueMaxLen= DTMON_CFG_REF_RESOURCE_LEN},
		// 		{.attr= {COMPARISON_A_DtmonResource, READWRITE_ResourceOperation, INTEGER_ResourceType}},
		// 		{.attr= {REFERENCE_B_DtmonResource, READWRITE_ResourceOperation, OPAQUE_ResourceType}, .value.opaque= config_cfg->nbiot.lwm2m.dtmon.referenceB[ALARMS_DtmonInstance], .valueMaxLen= DTMON_CFG_REF_RESOURCE_LEN},
		// 		{.attr= {COMPARISON_B_DtmonResource, READWRITE_ResourceOperation, INTEGER_ResourceType}},
		// 		{.attr= {REFERENCE_C_DtmonResource, READWRITE_ResourceOperation, OPAQUE_ResourceType}, .value.opaque= config_cfg->nbiot.lwm2m.dtmon.referenceC[ALARMS_DtmonInstance], .valueMaxLen= DTMON_CFG_REF_RESOURCE_LEN},
		// 		{.attr= {COMPARISON_C_DtmonResource, READWRITE_ResourceOperation, INTEGER_ResourceType}},
		// 		{.attr= {RESULTS_DtmonResource, READWRITE_ResourceOperation, OPAQUE_ResourceType}},
		// };
		// At the top of your cfg.c file

		static const LWOBJ_Resource_t DTMON_ALARMS_DEFAULTS[MAX_DtmonResource] = {
			{.attr= {NAME_DtmonResource, READ_ResourceOperation, STRING_ResourceType}},
			{.attr= {DESC_DtmonResource, READ_ResourceOperation, STRING_ResourceType}},
			{
				.attr= {SETTINGS_DtmonResource, READWRITE_ResourceOperation, OPAQUE_ResourceType}, 
				.value.opaque= NULL, // FIX: Initialize pointer to NULL
				.valueMaxLen= DTMON_CFG_SETTINGS_RESOURCE_MAX_LEN
			},
			{
				.attr= {DATA_DtmonResource, READ_ResourceOperation, OPAQUE_ResourceType}, 
				.value.opaque= NULL, // FIX: Initialize pointer to NULL
				.valueMaxLen= DTMON_CFG_RESULTS_VALUE_LEN
			},
			{
				.attr= {SAMPLING_START_MASK_DtmonResource, READWRITE_ResourceOperation, STRING_ResourceType}, 
				.value.string= NULL, // FIX: Initialize pointer to NULL
				.valueMaxLen= DTMON_CFG_MASK_RESOURCE_LEN
			},
			{.attr= {SAMPLING_INTERVAL_DtmonResource, READWRITE_ResourceOperation, INTEGER_ResourceType}, .value.integer= 30},
			{.attr= {SAMPLING_RUN_PERIOD_DtmonResource, READWRITE_ResourceOperation, INTEGER_ResourceType}, .value.integer= 86400},
			{
				.attr= {REFERENCE_A_DtmonResource, READWRITE_ResourceOperation, OPAQUE_ResourceType}, 
				.value.opaque= NULL, // FIX: Initialize pointer to NULL
				.valueMaxLen= DTMON_CFG_REF_RESOURCE_LEN
			},
			{.attr= {COMPARISON_A_DtmonResource, READWRITE_ResourceOperation, INTEGER_ResourceType}},
			{
				.attr= {REFERENCE_B_DtmonResource, READWRITE_ResourceOperation, OPAQUE_ResourceType}, 
				.value.opaque= NULL, // FIX: Initialize pointer to NULL
				.valueMaxLen= DTMON_CFG_REF_RESOURCE_LEN
			},
			{.attr= {COMPARISON_B_DtmonResource, READWRITE_ResourceOperation, INTEGER_ResourceType}},
			{
				.attr= {REFERENCE_C_DtmonResource, READWRITE_ResourceOperation, OPAQUE_ResourceType}, 
				.value.opaque= NULL, // FIX: Initialize pointer to NULL
				.valueMaxLen= DTMON_CFG_REF_RESOURCE_LEN
			},
			{.attr= {COMPARISON_C_DtmonResource, READWRITE_ResourceOperation, INTEGER_ResourceType}},
			{.attr= {RESULTS_DtmonResource, READWRITE_ResourceOperation, OPAQUE_ResourceType}},
		};

		// Inside your CFG_ApplyDefaults(Config_t *_cfg) function

		// 1. Copy the default values from the constant array
		memcpy(_cfg->nbiot.lwm2m.dtmon.resource[ALARMS_DtmonInstance], 
			DTMON_ALARMS_DEFAULTS, 
			sizeof(DTMON_ALARMS_DEFAULTS));

		// 2. Fix all the pointers to point to the correct runtime locations within _cfg
		_cfg->nbiot.lwm2m.dtmon.resource[ALARMS_DtmonInstance][SETTINGS_DtmonResource].value.opaque = 
		_cfg->nbiot.lwm2m.dtmon.settings[ALARMS_DtmonInstance];
		
		_cfg->nbiot.lwm2m.dtmon.resource[ALARMS_DtmonInstance][DATA_DtmonResource].value.opaque = 
		_cfg->nbiot.lwm2m.dtmon.data[ALARMS_DtmonInstance];
		
		_cfg->nbiot.lwm2m.dtmon.resource[ALARMS_DtmonInstance][SAMPLING_START_MASK_DtmonResource].value.string = 
		_cfg->nbiot.lwm2m.dtmon.samplingStartMask[ALARMS_DtmonInstance];
		
		_cfg->nbiot.lwm2m.dtmon.resource[ALARMS_DtmonInstance][REFERENCE_A_DtmonResource].value.opaque = 
		_cfg->nbiot.lwm2m.dtmon.referenceA[ALARMS_DtmonInstance];
		
		_cfg->nbiot.lwm2m.dtmon.resource[ALARMS_DtmonInstance][REFERENCE_B_DtmonResource].value.opaque = 
		_cfg->nbiot.lwm2m.dtmon.referenceB[ALARMS_DtmonInstance];
		
		_cfg->nbiot.lwm2m.dtmon.resource[ALARMS_DtmonInstance][REFERENCE_C_DtmonResource].value.opaque = 
		_cfg->nbiot.lwm2m.dtmon.referenceC[ALARMS_DtmonInstance];

		_cfg->nbiot.lwm2m.dtmon.rte[ALARMS_DtmonInstance].log.partition= M95M01_PARTITION_DTMON_0;
		_cfg->nbiot.lwm2m.dtmon.rte[ALARMS_DtmonInstance].log.head= 0;
		_cfg->nbiot.lwm2m.dtmon.rte[ALARMS_DtmonInstance].log.elementCount= 0;
		_cfg->nbiot.lwm2m.dtmon.rte[ALARMS_DtmonInstance].log.elementMax= DTMON_CFG_RESULTS_RESOURCE_MAX_COUNT;
		_cfg->nbiot.lwm2m.dtmon.rte[ALARMS_DtmonInstance].log.elementSize= sizeof(DTMON_Result_t);
		_cfg->nbiot.lwm2m.dtmon.rte[ALARMS_DtmonInstance].resultIndex= 0;
		_cfg->nbiot.lwm2m.dtmon.rte[ALARMS_DtmonInstance].prevResultValue= 0;
		_cfg->nbiot.lwm2m.dtmon.rte[ALARMS_DtmonInstance].settings.alarms.rteClearTimestamp= 0;
		_cfg->nbiot.lwm2m.dtmon.rte[ALARMS_DtmonInstance].settings.alarms.activePeriod_s= 86400;
		_cfg->nbiot.lwm2m.dtmon.rte[ALARMS_DtmonInstance].settings.alarms.lowBatteryLevel_percent= 8;
		_cfg->nbiot.lwm2m.dtmon.rte[ALARMS_DtmonInstance].settings.alarms.nrtBackflowSamplingPeriod_s= 86400;
		// _cfg->nbiot.lwm2m.dtmon.samplingStartMask[ALARMS_DtmonInstance]= "------------";
		static const char PRACTINSTANCE_TEMP3[] = "------------";;
		memcpy(_cfg->nbiot.lwm2m.dtmon.samplingStartMask[ALARMS_DtmonInstance], PRACTINSTANCE_TEMP2, sizeof(PRACTINSTANCE_TEMP2));

		// _cfg->nbiot.lwm2m.dtmon.referenceA[ALARMS_DtmonInstance]= DTMON_CFG_DATA_REF_RESOURCE_DEFAULT;
		// _cfg->nbiot.lwm2m.dtmon.referenceB[ALARMS_DtmonInstance]= DTMON_CFG_DATA_REF_RESOURCE_DEFAULT;
		// _cfg->nbiot.lwm2m.dtmon.referenceC[ALARMS_DtmonInstance]= DTMON_CFG_DATA_REF_RESOURCE_DEFAULT;
		static const uint8_t DTMON_REF_DEFAULT[] = DTMON_CFG_DATA_REF_RESOURCE_DEFAULT;
		memcpy(_cfg->nbiot.lwm2m.dtmon.referenceA[ALARMS_DtmonInstance], DTMON_REF_DEFAULT, sizeof(DTMON_REF_DEFAULT));
		memcpy(_cfg->nbiot.lwm2m.dtmon.referenceB[ALARMS_DtmonInstance], DTMON_REF_DEFAULT, sizeof(DTMON_REF_DEFAULT));
		memcpy(_cfg->nbiot.lwm2m.dtmon.referenceC[ALARMS_DtmonInstance], DTMON_REF_DEFAULT, sizeof(DTMON_REF_DEFAULT));

		_cfg->nbiot.lwm2m.diversifyPractDispatchMask= false;/*no point diversify before sno injection*/
		_cfg->nbiot.lwm2m.practDispatchMaskInterval= 3;
		_cfg->nbiot.lwm2m.practDispatchMaskPeriod= 7199;
		_cfg->nbiot.lwm2m.practDispatchMaskOffset= 3600;/*pub: 1am - 4am transmit but we chose 1am - 3am to account for transmission buffer*/
		_cfg->nbiot.lwm2m.notifyRetryBackoffMin_s= 300;
		_cfg->nbiot.lwm2m.notifyRetryBackoffMax_s= 900;
		_cfg->nbiot.lwm2m.useCellTemperature= true;
}

uint32_t CFG_GetChecksum(uint8_t *_ptr, uint32_t _length)
{
	uint32_t _crc32;

    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_CRC);

	LL_CRC_SetPolynomialCoef(CRC, LL_CRC_DEFAULT_CRC32_POLY);
	LL_CRC_SetInitialData(CRC, LL_CRC_DEFAULT_CRC_INITVALUE);
	LL_CRC_SetInputDataReverseMode(CRC, LL_CRC_INDATA_REVERSE_NONE);
	LL_CRC_SetOutputDataReverseMode(CRC, LL_CRC_OUTDATA_REVERSE_NONE);
	LL_CRC_SetPolynomialSize(CRC, LL_CRC_POLYLENGTH_32B);

	LL_CRC_ResetCRCCalculationUnit(CRC);
	for(int i= 0; i<_length; i++)
    {
		LL_CRC_FeedData8(CRC, _ptr[i]);
    }
	_crc32= LL_CRC_ReadData32(CRC);

    LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_CRC);

	return _crc32;
}

/**
 * This checksum can be verified by the following site:
 * website: https://crccalc.com/
 * data: 0050002089980008
 * input type: Hex
 * type: CRC-16/ARC
 * poly: 0x8005
 * init: 0x0000
 * result: 0xB77B
 */
#define POLY 0xa001
uint32_t CFG_GetFWUChecksum(uint16_t _crc, uint8_t *_buf, uint32_t _len) /*TODO: change this to hardware checksum(faster)*/
{
    while (_len--)
    {
    	_crc ^= *_buf++;
    	_crc = _crc & 1 ? (_crc >> 1) ^ POLY : _crc >> 1;
    	_crc = _crc & 1 ? (_crc >> 1) ^ POLY : _crc >> 1;
    	_crc = _crc & 1 ? (_crc >> 1) ^ POLY : _crc >> 1;
    	_crc = _crc & 1 ? (_crc >> 1) ^ POLY : _crc >> 1;
    	_crc = _crc & 1 ? (_crc >> 1) ^ POLY : _crc >> 1;
    	_crc = _crc & 1 ? (_crc >> 1) ^ POLY : _crc >> 1;
    	_crc = _crc & 1 ? (_crc >> 1) ^ POLY : _crc >> 1;
    	_crc = _crc & 1 ? (_crc >> 1) ^ POLY : _crc >> 1;
    }
    return _crc;
}

#ifdef COMMENT
static uint16_t CFG_GetConfigVersion(uint32_t _eepromAddress)
{
	uint16_t _configVersion= *((__IO uint16_t*)(_eepromAddress+ 4));/*2 bytes len, 5th bytes from top*/

	return _configVersion;
}
#endif

ErrorStatus CFG_ReadSingleStruct(uint8_t *_struct, uint16_t _length, uint32_t _address)
{
	if(true== FAILSAFE_CFG_IsCorrupted())
	{
 		//DBG_Print("Config Corrupted.\r\n");
		return ERROR;
	}

	ErrorStatus _status= SUCCESS;
	uint32_t _checksumRead= ((uint32_t*)_address)[0];
	uint32_t _checksumCalculated= CFG_GetChecksum((uint8_t*)(_address+ 4), _length- 4);

	if(0xFFFF== _checksumRead)
	{
		//DBG_PrintLine("Checksum virgin(%d bytes).", _length);
		_status= ERROR;
	}
	else if(_checksumRead== _checksumCalculated)/*check sum is correct*/
	{
		//DBG_PrintLine("Checksum OK(%d bytes).", _length);
//		for(int i= 0; i< _length; i++)
//		{
//			_struct[i]= *((__IO uint8_t*)(_address+ (i)));
//		};
		for(int i= 0; i< _length; i+= 8)
		{
			memcpy(_struct+ i, (uint64_t*)(_address+ i), 8);
		}
	}
	else
	{
		DBG_PrintLine("Checksum FAILED(%d bytes).", _length);
		_status= ERROR;
	}
	return _status;
}

static void CFG_ReadStruct(Config_t *_cfg, uint32_t _eepromAddress)
{
	/*CFG_ReadStruct won't overwrite buffer if checksum failed, thus if failed, default valuye is loaded.*/
	DBG_Print("Test13 Flash_t: ");		CFG_ReadSingleStruct((uint8_t *)&(_cfg->flash), 	sizeof(Flash_t), 		CFG_CONFIG_ADDRESS_FLASH);
	DBG_Print("System_t: ");	CFG_ReadSingleStruct((uint8_t *)&(_cfg->system), 	sizeof(System_t), 		CFG_CONFIG_ADDRESS_SYSTEM);
	DBG_Print("FAILSAFE_t: ");	CFG_ReadSingleStruct((uint8_t *)&(_cfg->failsafe), 	sizeof(FAILSAFE_t), 	CFG_CONFIG_ADDRESS_FAILSAFE);
	DBG_Print("DIAG_t: ");		CFG_ReadSingleStruct((uint8_t *)&(_cfg->diagnostic),sizeof(DIAG_t), 		CFG_CONFIG_ADDRESS_DIAG);
	DBG_Print("PULSER_t: ");	CFG_ReadSingleStruct((uint8_t *)&(_cfg->pulser), 	sizeof(PULSER_t), 		CFG_CONFIG_ADDRESS_PULSER);
	DBG_Print("LOG_t: ");		CFG_ReadSingleStruct((uint8_t *)&(_cfg->log),		sizeof(LOG_t), 			CFG_CONFIG_ADDRESS_LOG);
	DBG_Print("SENSOR_t: ");	CFG_ReadSingleStruct((uint8_t *)&(_cfg->sensors), 	sizeof(SENSOR_t), 		CFG_CONFIG_ADDRESS_SENSOR);
	DBG_Print("ALARM_t: ");		CFG_ReadSingleStruct((uint8_t *)&(_cfg->alarm), 	sizeof(ALARM_t), 		CFG_CONFIG_ADDRESS_ALARM);
	DBG_Print("NBIOT_t: ");		CFG_ReadSingleStruct((uint8_t *)&(_cfg->nbiot), 	sizeof(NBIOT_t), 		CFG_CONFIG_ADDRESS_NBIOT);
	DBG_Print("Used: %d out of %d bytes. Remaining: %d bytes.\r\n\r\n", CFG_CONFIG_ADDRESS_END- CFG_PARTITION_CONFIG_ADDR, CFG_PARTITION_CONFIG_SIZE, CFG_PARTITION_CONFIG_SIZE- (CFG_CONFIG_ADDRESS_END- CFG_PARTITION_CONFIG_ADDR));
}

static ErrorStatus CFG_WriteStruct(uint8_t *_struct, uint16_t _length, uint32_t _address)
{
	uint32_t _checksumRam= CFG_GetChecksum((uint8_t*)(_struct+ 4), _length- 4);/*calculated cheksum*/
	HAL_StatusTypeDef _status;

	HAL_FLASH_Unlock();
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);

	((uint32_t *)_struct)[0]= _checksumRam;

	_length/= 8;/*we can only program in double-word aligned, so struct are expected to be double-word aligned*/
	for(int i= 0; i< _length; i++)
	{
		__IO uint64_t _data;
		memcpy(&_data, _struct, sizeof(uint64_t));
		_status= HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, _address, _data);
		_address+= 8;
		_struct+= 8;

		if(HAL_OK!= _status)
		{
			//DBG_Print("CFG_WriteStruct: error, _address: %d\r\n", _address);
			HAL_FLASH_Lock();
			return ERROR;
		}
	}

	HAL_FLASH_Lock();
	return SUCCESS;
}

ErrorStatus CFG_WriteProgram(uint32_t _address, uint8_t *_writeBuffer, uint16_t _length)
{
	HAL_StatusTypeDef _status= HAL_OK;

	HAL_FLASH_Unlock();
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);

	/*we can only program in double-word aligned, so struct are expected to be double-word aligned*/
	if(0!= (_length% 8))
	{
		//DBG_Print("CFG_WriteProgram: length error\r\n");
		return ERROR;
	}
	_length/= 8;

	for(int i= 0; i< _length; i++)
	{
		__IO uint64_t _data;
		__IO uint64_t _flashData= ((uint64_t *)_address)[0];
		memcpy(&_data, _writeBuffer, sizeof(uint64_t));
		if(_flashData!= _data)/*rewrite would failed, need to erase again.*/
		{
			_status= HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, _address, _data);
			if(HAL_OK!= _status)
			{
				_status= HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, _address, _data);
				if(HAL_OK!= _status)
				{
					_status= HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, _address, _data);
				}
			}
		}
		_address+= 8;
		_writeBuffer+= 8;
		if(HAL_OK!= _status)
		{
			//DBG_Print("HAL_FLASH_Program: error, _address: %d\r\n", _address);
			break;
		}
	}

	HAL_FLASH_Lock();
	if(HAL_OK!= _status)
	{
		return ERROR;
	}
	return SUCCESS;
}

ErrorStatus CFG_ErasePartition(CFG_FlashPartition_t _partition)
{
	FLASH_EraseInitTypeDef _eraseInitStruct;
	uint8_t _retry= 3;
	uint32_t _pageError= 0;

cfg_erase_partition_retry:
	_eraseInitStruct.TypeErase= FLASH_TYPEERASE_PAGES;
	_eraseInitStruct.Banks= FLASH_BANK_1;/*only have one bank for stm32l431/433*/

	switch(_partition)
	{
		case PARTITION_1_FlashPartition:
			_eraseInitStruct.Page= (CFG_PARTITION_1_ADDR- FLASH_BASE)/ FLASH_PAGE_SIZE;
			_eraseInitStruct.NbPages= CFG_PARTITION_SIZE/ FLASH_PAGE_SIZE;
			break;
		case PARTITION_2_FlashPartition:
			_eraseInitStruct.Page= (CFG_PARTITION_2_ADDR- FLASH_BASE)/ FLASH_PAGE_SIZE;
			_eraseInitStruct.NbPages= CFG_PARTITION_SIZE/ FLASH_PAGE_SIZE;
			break;
		case PARTITION_CONFIG_FlashPartition:
			_eraseInitStruct.Page= (CFG_PARTITION_CONFIG_ADDR- FLASH_BASE)/ FLASH_PAGE_SIZE;
			_eraseInitStruct.NbPages= CFG_PARTITION_CONFIG_SIZE/ FLASH_PAGE_SIZE;
			break;
		default:
			return ERROR;
	}

//	DBG_Print("partition: %d.\r\n", _partition);
//	DBG_Print("_eraseInitStruct.TypeErase: %d.\r\n", _eraseInitStruct.TypeErase);
//	DBG_Print("_eraseInitStruct.Banks: %d.\r\n", _eraseInitStruct.Banks);
//	DBG_Print("_eraseInitStruct.Page: %d.\r\n", _eraseInitStruct.Page);
//	DBG_Print("_eraseInitStruct.NbPages: %d.\r\n", _eraseInitStruct.NbPages);

	HAL_FLASH_Unlock();
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);
	if(HAL_OK!= HAL_FLASHEx_Erase(&_eraseInitStruct, &_pageError))
	{
		DBG_Print("HAL_FLASHEx_Erase Error.\r\n");
		HAL_FLASH_Lock();
		if(0!= --_retry)
		{
			goto cfg_erase_partition_retry;
		}
		return ERROR;
	}

	HAL_FLASH_Lock();
	//DBG_Print("HAL_FLASHEx_Erase Success.\r\n");
	return SUCCESS;
}

bool CFG_IsInPartition(CFG_FlashPartition_t _partition, uint32_t _startAddress, uint16_t _length)
{
	uint32_t _endAddress= _startAddress+ _length;
	uint32_t _partitionStart;
	uint32_t _partitionEnd;

	switch(_partition)
	{
		case PARTITION_1_FlashPartition:
			_partitionStart= CFG_PARTITION_1_ADDR;
			_partitionEnd= CFG_PARTITION_1_ADDR+ CFG_PARTITION_SIZE;
			break;
		case PARTITION_2_FlashPartition:
			_partitionStart= CFG_PARTITION_2_ADDR;
			_partitionEnd= CFG_PARTITION_2_ADDR+ CFG_PARTITION_SIZE;
			break;
		case PARTITION_CONFIG_FlashPartition:
			_partitionStart= CFG_PARTITION_CONFIG_ADDR;
			_partitionEnd= CFG_PARTITION_CONFIG_ADDR+ CFG_PARTITION_CONFIG_SIZE;
			break;
		default:
			return false;
	}

	if(((_startAddress>= _partitionStart)&& (_startAddress< _partitionEnd))
		|| 	((_endAddress>= _partitionStart)&& (_endAddress< _partitionEnd)))
	{
		return true;
	}

	return false;
}

ErrorStatus CFG_Store(Config_t *_cfg)
{
	ErrorStatus _status;
	uint32_t _eepromAddress= CFG_PARTITION_CONFIG_ADDR;

	//IOCTRL_MainPower_Enable(true);/*IO_PINS_Power_Enable(true)*/;/*when writing to flash it's crucial to have the power on all the time*/
	LL_mDelay(5);/*untested delay supposedly for power stabilization. this delay is minor compared to writing to eeprom itself*/

	_status= CFG_ErasePartition(PARTITION_CONFIG_FlashPartition);
	_status|= CFG_WriteStruct((uint8_t *)&(_cfg->flash), 		sizeof(Flash_t), 		_eepromAddress); _eepromAddress+= sizeof(Flash_t);
	_status|= CFG_WriteStruct((uint8_t *)&(_cfg->system), 		sizeof(System_t), 		_eepromAddress); _eepromAddress+= sizeof(System_t);
	_status|= CFG_WriteStruct((uint8_t *)&(_cfg->failsafe), 	sizeof(FAILSAFE_t), 	_eepromAddress); _eepromAddress+= sizeof(FAILSAFE_t);
	_status|= CFG_WriteStruct((uint8_t *)&(_cfg->diagnostic), 	sizeof(DIAG_t), 		_eepromAddress); _eepromAddress+= sizeof(DIAG_t);
	_status|= CFG_WriteStruct((uint8_t *)&(_cfg->pulser), 		sizeof(PULSER_t), 		_eepromAddress); _eepromAddress+= sizeof(PULSER_t);
	_status|= CFG_WriteStruct((uint8_t *)&(_cfg->log), 			sizeof(LOG_t), 			_eepromAddress); _eepromAddress+= sizeof(LOG_t);
	_status|= CFG_WriteStruct((uint8_t *)&(_cfg->sensors), 		sizeof(SENSOR_t), 		_eepromAddress); _eepromAddress+= sizeof(SENSOR_t);
	_status|= CFG_WriteStruct((uint8_t *)&(_cfg->alarm), 		sizeof(ALARM_t), 		_eepromAddress); _eepromAddress+= sizeof(ALARM_t);
	_status|= CFG_WriteStruct((uint8_t *)&(_cfg->nbiot), 		sizeof(NBIOT_t),		_eepromAddress); _eepromAddress+= sizeof(NBIOT_t);

	//IOCTRL_MainPower_Enable(config.misc.powerEnabled);/*IO_PINS_Power_Enable(config.misc.powerEnabled)*/;/*revert back to previous*/
	//LL_mDelay(2000);

	return _status;
}

ErrorStatus CFG_Load(Config_t *_cfg)
{
	uint8_t _status= SUCCESS;
	uint32_t _address= CFG_PARTITION_CONFIG_ADDR;
	Flash_t _flash;

	CFG_ApplyDefaults(_cfg);

	bool _useDefaultConfig= (0b1== (LL_RTC_BAK_GetRegister(RTC, OPERATIONAL_FLAGS_BKPReg)& 0b1))? true: false;;

	if(SUCCESS!= CFG_ReadSingleStruct((uint8_t *)&(_flash), sizeof(Flash_t), CFG_PARTITION_CONFIG_ADDR))
	{
		_flash.configVersion= 0;
	}

	if(true== _useDefaultConfig)
	{
		uint32_t _operationalFlags= LL_RTC_BAK_GetRegister(RTC, OPERATIONAL_FLAGS_BKPReg)& (~0b1);
		LL_RTC_BAK_SetRegister(RTC, OPERATIONAL_FLAGS_BKPReg, _operationalFlags);
	}
	else if((_flash.configVersion< CFG_MIN_CONFIG_VERSION)|| (_flash.configVersion> CFG_CONFIG_VERSION))
	{
		/*limit of supported config backward/forward*/
	}
	else
	{
		NBIOT_t _nbiot= _cfg->nbiot;

		CFG_ReadStruct(_cfg, _address);

		if(CFG_CONFIG_VERSION!= _flash.configVersion)
		{
			switch(_flash.configVersion)
			{
				case 0x0012://v1.5.N.62032a0 config
				case 0x0013:
				case 0x0014:
				case 0x0015:
				case 0x0016:
				case 0x0017:
				case 0x0018:
//					_cfg->nbiot.restartDelay_s= HOUR_TO_SECONDS(3);
//					/*All of the meters previously deployed are using gkcoap*/
//					_cfg->nbiot.gkcoap.enabled= true;
//					_cfg->nbiot.gkcoap.reportStartMask= 0xFFFFFFFF00000000;/*0xYYMMDDhhmmss0000*/
//					_cfg->nbiot.gkcoap.reportInterval_s= HOUR_TO_SECONDS(1);//HOUR_TO_MILISECONDS(1);
//					_cfg->nbiot.gkcoap.packetType= 0x32;
//					_cfg->nbiot.gkcoap.logTickType= HOUR_TickType;
//					_cfg->nbiot.gkcoap.logTickSize= 1;
//					_cfg->nbiot.gkcoap.logTickStartMask= 0xFFFF0000;
//					/*Need to get previous pulser info*/
//					void *_pulserCfgAddr= (void *)CFG_CONFIG_ADDRESS_PULSER;
//					_cfg->pulser.mode= *((uint8_t *)(_pulserCfgAddr+ 5));
//					/*Also need to set their meter model*/
//					switch(_cfg->pulser.mode)
//					{
//						case TRACSENS_Mode:
//						case TRACSENSi_Mode:
//							/*pulser config is not forward compatible*/
//							_cfg->pulser.weight_liter= *((float *)(_pulserCfgAddr+ 8));/*after padding*/
//							if(0.25== _cfg->pulser.weight_liter)
//							{
//								_cfg->system.meterModel= 250;
//							}
//							else if(2.5== _cfg->pulser.weight_liter)
//							{
//								_cfg->system.meterModel= 251;
//							}
//							break;
//						case ELSTER_Mode:
//							/*pulser config and meter model already forward compatible*/
//							break;
//						default:
//							break;
//					}
//					_cfg->sensors.battery.designCapacity_Ah= 14.0;/*Total 19.0. 3% discharge yearly. 12.0(15yrs) 14.0(10yrs)in Ah*/
//					_cfg->sensors.battery.lowThreshold= 10.0;

				case 0x0019:
//					/*at 0x0020 we start to migrate QH monitoring variables to RTC registers for better reliability*/
//					/*force reset the counter since previous flash variables were highly unreliable*/
//					_cfg->sensors.battery.rteQHMultiplier= 0;
//					LL_RTC_BAK_SetRegister(RTC, SNSR_QH_MULT_BKPReg, _cfg->sensors.battery.rteQHMultiplier);

				case 0x0020:
				case 0x0021:
					_cfg->pulser.tracsens.enableErrorPatternCheck= true;

//					_cfg->nbiot.gkcoap.enabled= true;
//					_cfg->nbiot.gkcoap.alarmActivePeriod_s= 1440;//120;/*active for 2 mins by default for ptp*/
//
//					_cfg->nbiot.lwm2m.enabled= true;
//					_cfg->nbiot.lwm2m.enableBootstrap= true;
//					_cfg->nbiot.lwm2m.registerBackoff_s= 28800;
//					UTILI_Array_CopyString(_cfg->nbiot.lwm2m.defaultConnection.serverIP, "18.142.171.195");
//					_cfg->nbiot.lwm2m.defaultConnection.serverPort= 5784;
//					_cfg->nbiot.lwm2m.defaultConnection.endpointName[0]= '\0';/*to auto set endpoint name as PUB's Decada format.*/
//					_cfg->nbiot.lwm2m.defaultConnection.securityMode= PSK_Bc66LinkLwm2mSecurityMode;
//					UTILI_Array_CopyString(_cfg->nbiot.lwm2m.defaultConnection.pskId, "GK");
//					UTILI_Array_CopyString(_cfg->nbiot.lwm2m.defaultConnection.psk, "6B552929767446753678");
//
//					LWOBJ_Obj_t *_readings= &(_cfg->nbiot.lwm2m.lwObj[PRACT_GET_READING_ObjName]);
//					_readings->resource[PERIODIC_INTERVAL_PractResource].value.integer= 3600;
//					_readings->resource[RECORD_DISPATCH_PERIODIC_INTERVAL_PractResource].value.integer= 86400;
//					UTILI_Array_CopyString(_readings->resource[START_MASK_PractResource].value.string, "--------0000");
//					UTILI_Array_CopyString(_readings->resource[RECORD_DISPATCH_START_MASK_PractResource].value.string,"------000000");
//
//					LWOBJ_Obj_t *_status= &(_cfg->nbiot.lwm2m.lwObj[PRACT_GET_STATUS_ObjName]);
//					_status->resource[PERIODIC_INTERVAL_PractResource].value.integer= 86400/ 2;
//					_status->resource[RECORD_DISPATCH_PERIODIC_INTERVAL_PractResource].value.integer= 86400*21;/*every 3 weeks*/
//					UTILI_Array_CopyString(_status->resource[START_MASK_PractResource].value.string, "--------0000");
//					UTILI_Array_CopyString(_status->resource[RECORD_DISPATCH_START_MASK_PractResource].value.string,"------000000");
//
//					_cfg->nbiot.lwm2m.dtmon.rte[ALARMS_DtmonInstance].settings.alarms.activePeriod_s= 86400;
//					_cfg->nbiot.lwm2m.dtmon.rte[ALARMS_DtmonInstance].settings.alarms.nrtBackflowSamplingPeriod_s= 86400;
//
//					_cfg->nbiot.lwm2m.diversifyPractDispatchMask= true;/*special: diversification relies on sno*/
//					_cfg->nbiot.lwm2m.practDispatchMaskInterval= 3;
//					_cfg->nbiot.lwm2m.practDispatchMaskPeriod= 7199;
//					_cfg->nbiot.lwm2m.practDispatchMaskOffset= 3600;/*pub: 1am - 4am transmit but we chose 1am - 3am to account for transmission buffer*/

				case 0x0022:
					_cfg->failsafe.saveConfigInterval_s= HOUR_TO_SECONDS(24);/*stm32l4 guaranteed write cycle max 10K*/

				case 0x0023:
					/*for PUB battery test unit settings*/
//					_cfg->nbiot.restartDelay_s= MINUTE_TO_SECONDS(30);
//
//					UTILI_Array_CopyString(_cfg->nbiot.lwm2m.defaultConnection.serverIP, "118.189.126.182");
//					_cfg->nbiot.lwm2m.defaultConnection.serverPort= 5682;
//					_cfg->nbiot.lwm2m.defaultConnection.endpointName[0]= '\0';/*to auto set endpoint name as PUB's Decada format.*/
//					_cfg->nbiot.lwm2m.defaultConnection.securityMode= PSK_Bc66LinkLwm2mSecurityMode;
//					UTILI_Array_CopyString(_cfg->nbiot.lwm2m.defaultConnection.pskId, "GKTest");
//					UTILI_Array_CopyString(_cfg->nbiot.lwm2m.defaultConnection.psk, "6B387A414C4344314D74");
//
//					LWOBJ_Obj_t *_readings= &(_cfg->nbiot.lwm2m.lwObj[PRACT_GET_READING_ObjName]);
//					_readings->resource[PERIODIC_INTERVAL_PractResource].value.integer= MINUTE_TO_SECONDS(1);
//					_readings->resource[RECORD_DISPATCH_PERIODIC_INTERVAL_PractResource].value.integer= MINUTE_TO_SECONDS(30);
//					UTILI_Array_CopyString(_readings->resource[START_MASK_PractResource].value.string, "----------00");
//					UTILI_Array_CopyString(_readings->resource[RECORD_DISPATCH_START_MASK_PractResource].value.string,"--------0000");
//
//					LWOBJ_Obj_t *_status= &(_cfg->nbiot.lwm2m.lwObj[PRACT_GET_STATUS_ObjName]);
//					_status->resource[PERIODIC_INTERVAL_PractResource].value.integer= 86400/ 2;
//					_status->resource[RECORD_DISPATCH_PERIODIC_INTERVAL_PractResource].value.integer= 86400*21;/*every 3 weeks*/
//					UTILI_Array_CopyString(_status->resource[START_MASK_PractResource].value.string, "--------0000");
//					UTILI_Array_CopyString(_status->resource[RECORD_DISPATCH_START_MASK_PractResource].value.string,"------000000");

				case 0x0024:
					_cfg->nbiot= _nbiot;

					/*RTC weren't initialized properly thus these error pattern count messed up*/
					_cfg->pulser.tracsens.rteErrorPatternCount= 0;
					_cfg->pulser.tracsens.rteErrorPatternCompensationStarted= false;
					_cfg->pulser.tracsens.rteErrorPatternState= 0;
					_cfg->pulser.tracsens.rteErrorPatternPreviousPulse= 0;
					_cfg->pulser.tracsens.rteErrorPatternJustStarted= false;

				case 0x0025:
				case 0x0026:
					_cfg->failsafe.lwm2mIgnoreCycle= 12;
					_cfg->failsafe.lwm2mAdditionMargin= 2;
					_cfg->failsafe.lwm2mMultiplierMargin= 3;
				case 0x0027:
					_cfg->nbiot.enableAttachOnMagnetTamper= true;
					_cfg->nbiot.enableAttachOnPulserThreshold= true;
					_cfg->nbiot.pulserThresholdValueForAttach_liter= 100; /*must be more than production flow test value*/
					_cfg->nbiot.enableAttachOnTiltTamper= false;
					_cfg->nbiot.tiltTamperAttachBackoff_s= HOUR_TO_SECONDS(3);/*roughly handling during installation*/
					_cfg->nbiot.lwm2m.notifyRetryBackoffMin_s= 300;
					_cfg->nbiot.lwm2m.notifyRetryBackoffMax_s= 900;
				case 0x0028:
				case 0x0029:
					_cfg->nbiot.bypassBuckBoost= false;/*bypoass does not directly solve BOR*/
					_cfg->nbiot.fastModemReboot= true;/*at this point, we are past production*/
					_cfg->nbiot.reattachWait_s= 30;
					_cfg->failsafe.PVDLevel= PWR_PVDLEVEL_1;/*lower PVD level to accomodate PTP BOR units*/
					//_cfg->failsafe.BORLevel= OB_BOR_LEVEL_0;/*BOR threshold cannot be set during firmware upgrade as this will trigger mcu POR, unlatch power*/
				case 0x0031:
					_cfg->nbiot.lwm2m.useCellTemperature= true;
					_cfg->diagnostic.rteFailsafeRebootCount= 0;
					_cfg->diagnostic.rtePVDRebootCount= 0;
					_cfg->diagnostic.rteBORCount= 0;
					_cfg->nbiot.stats.failsafeRebootCount= 0;
					_cfg->nbiot.stats.PVDRebootCount= 0;
					_cfg->nbiot.stats.BORRebootCount= 0;
					/*to add more:
				case 0xXXXX://previous config
					{
						//initialize newly added variable
					}
					*/

					/*
					For backward compatibility make sure to adjust the reserve array to match the following size:
					Checksum OK(48 bytes).
					Flash_t: Checksum OK(48 bytes).
					System_t: Checksum OK(1296 bytes).
					FAILSAFE_t: Checksum OK(384 bytes).
					DIAG_t: Checksum OK(176 bytes).
					PULSER_t: Checksum OK(432 bytes).
					LOG_t: Checksum OK(176 bytes).
					SENSOR_t: Checksum OK(296 bytes).
					ALARM_t: Checksum OK(624 bytes).
					NBIOT_t: Checksum OK(5640 bytes).
					Used: 9072 out of 12288 bytes. Remaining: 3216 bytes.
					*/
				break;/*break only on last case.*/

				default:
					_status= ERROR;
					break;
			}
		}
	}

	/*below are parameters that to be loaded not from EEPROM*/
	_cfg->flash.configVersion= CFG_CONFIG_VERSION;/*jic config misc same*/
	UTILI_Array_CopyString(_cfg->system.fwVersion, CFG_DEVICE_FIRMWARE_VERSION);
	UTILI_Array_CopyString(_cfg->system.hwVersion, CFG_DEVICE_HARDWARE_VERSION);

#if PARTITION== 2
	_cfg->flash.partition= PARTITION_2_FlashPartition;
	_cfg->flash.partitionStartAddress= CFG_PARTITION_2_ADDR;
#else
	_cfg->flash.partition= PARTITION_1_FlashPartition;
	_cfg->flash.partitionStartAddress= CFG_PARTITION_1_ADDR;
#endif
	_cfg->system.rteDisableSleep= false;


	//_cfg->nbiot.gkcoap.enabled= true;
	//_cfg->nbiot.gkcoap.reportStartMask= 0xFFFFFF0000000000;/*0xYYMMDDhhmmss0000*/
	//_cfg->nbiot.gkcoap.reportInterval_s= MINUTE_TO_SECONDS(1);//HOUR_TO_MILISECONDS(1);
	//_cfg->nbiot.gkcoap.packetType= 0x32;
	//_cfg->nbiot.gkcoap.logTickType= MINUTE_TickType;
	//_cfg->nbiot.gkcoap.logTickSize= 1;
	//_cfg->nbiot.gkcoap.logTickStartMask= 0xFFFFFF00;

//	_cfg->nbiot.lwm2m.enabled= true;
//	_cfg->nbiot.lwm2m.enableBootstrap= true;
//	_cfg->nbiot.lwm2m.serverPort= 5784;/*5683 5684(psk) 5783(bootstrap) 5784(bootstrap-PSK)*/
//	_cfg->nbiot.lwm2m.lifetime= 60;
//	_cfg->nbiot.lwm2m.securityMode= BC66LINK_LWM2M_SECURITY_MODE_PSK; /*BC66LINK_LWM2M_SECURITY_MODE_NONE BC66LINK_LWM2M_SECURITY_MODE_PSK*/

	/*maxis ip:port
	 * https://thingsboard.io/
	 *
	 * 52.163.228.72:568
	 * 20.24.153.230:56855
	 *
	 */
//#define ENDPOINT "mux_urn:imei:SMK_SAMPLE_2___\0"
//#define IP "lwm2m.thingsboard.cloud\0"
//#define PORT 5685
//#define ENDPOINT "gk-water-meter-01\0"
//#define IP "20.24.153.230\0"
//#define PORT 5685
	//memcpy(_cfg->nbiot.lwm2m.serverIP, IP, strlen(IP)+ 1);
	//_cfg->nbiot.lwm2m.serverPort= PORT;
	//memcpy(_cfg->nbiot.lwm2m.endpointName, ENDPOINT, strlen(ENDPOINT)+ 1);
	//_cfg->nbiot.lwm2m.enabled= true;
	//_cfg->nbiot.lwm2m.enableBootstrap= false;
	//_cfg->nbiot.lwm2m.lifetime_s= 3600;//3600;/*just set by app*/
	//_cfg->nbiot.lwm2m.securityMode= BC66LINK_LWM2M_SECURITY_MODE_NONE; /*BC66LINK_LWM2M_SECURITY_MODE_NONE BC66LINK_LWM2M_SECURITY_MODE_PSK*/
	//memcpy(_cfg->nbiot.lwm2m.serverIP, "223.25.247.73\0", strlen("223.25.247.73")+ 1);_cfg->nbiot.lwm2m.serverPort= 5683;//36002;/*36002 36001 5683 5684(psk) 5783(bootstrap) 5784(bootstrap-PSK)*/
	//memcpy(_cfg->nbiot.lwm2m.serverIP, "113.210.22.117\0", strlen("113.210.22.117")+ 1);
	//memcpy(_cfg->nbiot.lwm2m.serverIP, "leshan.eclipseprojects.io\0", strlen("leshan.eclipseprojects.io")+ 1);


	/*config to help stranger on quectel forum*/
//	 memcpy(_cfg->nbiot.lwm2m.endpointName, "A00001867997034736777\0",strlen("A00001867997034736777")+1);
//	 memcpy(_cfg->nbiot.lwm2m.pskId, "A00001867997034736777\0",strlen("A00001867997034736777")+1);
//	 memcpy(_cfg->nbiot.lwm2m.psk, "10203040\0",strlen("10203040")+1);

	//_cfg->alarm.backOffPeriod_ms= 300000;

	//_cfg->nbiot.gkcoap.retryBackoffMin_s= 6;
	//_cfg->nbiot.gkcoap.retryBackoffMax_s= 36;
	//coaps://demo.gkmetering.com:5684/data/v1.C.N.0.bin
	//coap://demo.gkmetering.com:5683/data/v1.C.N.0.bin

	//_cfg->nbiot.gkcoap.alarmActivePeriod_s= 120;/*active for 10 mins for ptp*/


	//DBG_Print("battery.lowThreshold offset:%d\r\n", (void*)&config.sensors.battery.lowThreshold-(void*)&config);
	//DBG_Print("nbiot.reattachWait_s offset:%d\r\n", (void*)&config.nbiot.reattachWait_s-(void*)&config);
	//DBG_Print("nbiot.enableRRCDrop offset:%d\r\n", (void*)&config.nbiot.enableRRCDrop-(void*)&config);
	//DBG_Print("nbiot.RRCDropPeriod_s offset:%d\r\n", (void*)&config.nbiot.RRCDropPeriod_s-(void*)&config);
	//DBG_Print("nbiot.downlinkWaitPeriod_s offset:%d\r\n", (void*)&config.nbiot.downlinkWaitPeriod_s-(void*)&config);
	//DBG_Print("tracsens.rteErrorPatternCount offset:%d, value:%d\r\n", (void*)&config.pulser.tracsens.rteErrorPatternCount-(void*)&config, config.pulser.tracsens.rteErrorPatternCount);
	//DBG_Print("tracsens.rteErrorPatternCompensationStarted offset:%d, value:%d\r\n", (void*)&config.pulser.tracsens.rteErrorPatternCompensationStarted-(void*)&config, config.pulser.tracsens.rteErrorPatternCompensationStarted);
	//DBG_Print("tracsens.rteErrorPatternState offset:%d, value:%d\r\n", (void*)&config.pulser.tracsens.rteErrorPatternState-(void*)&config, config.pulser.tracsens.rteErrorPatternState);
	//DBG_Print("tracsens.rteErrorPatternPreviousPulse offset:%d, value:%d\r\n", (void*)&config.pulser.tracsens.rteErrorPatternPreviousPulse-(void*)&config, config.pulser.tracsens.rteErrorPatternPreviousPulse);
	//DBG_Print("tracsens.rteErrorPatternJustStarted offset:%d, value:%d\r\n", (void*)&config.pulser.tracsens.rteErrorPatternJustStarted-(void*)&config, config.pulser.tracsens.rteErrorPatternJustStarted);
	//DBG_Print("_cfg->system.fwVersion offset:%d\r\n", (void*)&config.system.fwVersion-(void*)&config);

	//DBG_Print("rte[GET_READING_PractInstance].dispatchState:%d, value:%d\r\n", (void*)&config.nbiot.lwm2m.pract.rte[GET_READING_PractInstance].dispatchState-(void*)&config, config.nbiot.lwm2m.pract.rte[GET_READING_PractInstance].dispatchState);
	//DBG_Print("rte[GET_STATUS_PractInstance].dispatchState:%d, value:%d\r\n", (void*)&config.nbiot.lwm2m.pract.rte[GET_STATUS_PractInstance].dispatchState-(void*)&config, config.nbiot.lwm2m.pract.rte[GET_STATUS_PractInstance].dispatchState);
	//DBG_Print("rte[QUERY_PractInstance].dispatchState:%d, value:%d\r\n", (void*)&config.nbiot.lwm2m.pract.rte[QUERY_PractInstance].dispatchState-(void*)&config, config.nbiot.lwm2m.pract.rte[QUERY_PractInstance].dispatchState);

	//DBG_Print("rtePrevQH:%d, value:%d\r\n", (void*)&config.sensors.battery.rtePrevQH-(void*)&config, config.sensors.battery.rtePrevQH);
	//DBG_Print("rteQHMultiplier:%d, value:%d\r\n", (void*)&config.sensors.battery.rteQHMultiplier-(void*)&config, config.sensors.battery.rteQHMultiplier);

	//DBG_Print("_cfg->nbiot.enableLwm2mOnAttach:%d\r\n", (void*)&config.nbiot.enableLwm2mOnAttach-(void*)&config);
	//DBG_Print("_cfg->nbiot.bypassBuckBoost offset:%d, value:%d\r\n", (void*)&config.nbiot.bypassBuckBoost-(void*)&config, config.nbiot.bypassBuckBoost);
	//DBG_Print("_cfg->nbiot.skipProductionBreakpoint offset:%d, value:%d\r\n", (void*)&config.nbiot.skipProductionBreakpoint-(void*)&config, config.nbiot.skipProductionBreakpoint);
	//DBG_Print("_cfg->nbiot.iccid offset:%d, value:%s\r\n", (void*)&config.nbiot.iccid-(void*)&config, config.nbiot.iccid);
	//DBG_Print("_cfg->nbiot.lwm2m.enabled:%d\r\n", (void*)&config.nbiot.lwm2m.enabled-(void*)&config);

	//DBG_Print("nbiot.restartDelay_s:%d\r\n", (void*)&config.nbiot.restartDelay_s-(void*)&config);
	//DBG_Print("nbiot.gkcoap.logTickSize:%d\r\n", (void*)&config.nbiot.gkcoap.logTickSize-(void*)&config);
	//DBG_Print("nbiot.gkcoap.reportStartMask:%d\r\n", (void*)&config.nbiot.gkcoap.reportStartMask-(void*)&config);
	//DBG_Print("nbiot.gkcoap.reportInterval_s:%d\r\n", (void*)&config.nbiot.gkcoap.reportInterval_s-(void*)&config);
	//DBG_Print("nbiot.enableLwm2mOnAttach:%d\r\n", (void*)&config.nbiot.enableLwm2mOnAttach-(void*)&config);
	//DBG_Print("nbiot.lwm2m.enabled:%d\r\n", (void*)&config.nbiot.lwm2m.enabled-(void*)&config);
	//DBG_Print("nbiot.lwm2m.defaultConnection.serverIP:%d\r\n", (void*)&config.nbiot.lwm2m.defaultConnection.serverIP-(void*)&config);
	//DBG_Print("nbiot.lwm2m.defaultConnection.serverPort:%d\r\n", (void*)&config.nbiot.lwm2m.defaultConnection.serverPort-(void*)&config);
	//DBG_Print("nbiot.lwm2m.defaultConnection.endpointName:%d\r\n", (void*)&config.nbiot.lwm2m.defaultConnection.endpointName-(void*)&config);
	//DBG_Print("system.meterSerialNo:%d\r\n", (void*)&config.system.meterSerialNo-(void*)&config);
	//DBG_Print("system.serialNo:%d\r\n", (void*)&config.system.serialNo-(void*)&config);

	return _status;
}
