/*
 * bc66handler.c
 *
 *  Created on: 6 Feb 2021
 *      Author: muhammad.ahamad@georgekent.net
 */

#include "common.h"
#include "bc66handler.h"
#include "bc66link.h"
#include "bc66phy.h"

#define DBG_Print

__IO static bool bUnintentionalResetDetected= false;

void BC66HANDLER_URC(BC66LINK_t *_sBC66Link)
{
	static bool bNewNetworkRegistration= false;/*for diag spacing*/
	static uint32_t ulPrevMessageId= 0;/*to remove duplication*/

	if(0!= BC66PHY_URCMsg_Depth())
	{
		uint8_t _depth= BC66PHY_URCMsg_Depth();
		char* _URCMsg= NULL;

		while(_depth--)
		{
			_URCMsg= BC66PHY_URCMsg_Peek();

			if(NULL!= strstr(_URCMsg, "+CSQ:"))//+CSQ: 28,99
			{
				char *_token= strtok(_URCMsg+ strlen("+CSQ:"), ",");
				_sBC66Link->status.rssi = atoi(_token);
			}
			else if(NULL!= strstr(_URCMsg, "+CGSN:"))
			{
				char *_token= _URCMsg+ strlen("+CGSN:");
				uint8_t _len= strlen(_token   );

				memcpy(config.nbiot.imei, _token, (_len> NBIOT_CFG_IMEI_LEN)? NBIOT_CFG_IMEI_LEN: _len);

				if('\0'== config.nbiot.lwm2m.defaultConnection.endpointName[0])
				{
					sprintf(config.nbiot.lwm2m.defaultConnection.endpointName, "urn:imei:%s", config.nbiot.imei);
				}
			}
			else if(NULL!= strstr(_URCMsg, "+QCCID:"))
			{
				char *_token= _URCMsg+ strlen("+QCCID:");
				uint8_t _len= strlen(_token   );

				memcpy(config.nbiot.iccid, _token, (_len> NBIOT_CFG_ICCID_LEN)? NBIOT_CFG_ICCID_LEN: _len);
			}
//			else if(NULL!= strstr(_URCMsg, "+CGPADDR:"))//else if(NULL!= strstr(_URCMsg, "+CGPADDR: "))
//			{
//				(void)strtok(_URCMsg+ strlen("+CGPADDR:"), ",");/*cid, not used*/
//				char *_token= strtok(NULL, ",");
//				if(NULL!= _token)
//				{
//					uint8_t _len= strlen(_token   );
//					memcpy(config.info.pdpAddress, _token, (_len> CONFIG_BC66_PDP_ADDR_LEN)? CONFIG_BC66_PDP_ADDR_LEN: _len);
//				}
//			}
			else if(NULL!= strstr(_URCMsg, "+CGATT:"))
			{
				if(0== strcmp(_URCMsg+ strlen("+CGATT:"), "0"))
				{
					_sBC66Link->status.isAttached= false;
				}
				else if(0== strcmp(_URCMsg+ strlen("+CGATT:"), "1"))
				{
					_sBC66Link->status.isAttached= true;
				}
				else
				{
					_sBC66Link->status.isAttached= false;
				}
			}
			else if(NULL!= strstr(_URCMsg, "+CPSMS:"))
			{
				char *_token= strtok(_URCMsg+ strlen("+CPSMS:"), ",");
				if(1!= atoi(_token))
				{
					_sBC66Link->status.cpsmsIsEnabled= false;
				}
				else
				{
					atoi(strtok(NULL, ","));/*RAU value, not used*/
					atoi(strtok(NULL, ","));/*GPRS ready timer value, not used*/
					_sBC66Link->status.cpsmsTauValue= atoi(strtok(NULL, ","));/*TAU*/
					_sBC66Link->status.cpsmsActiveTimeValue= atoi(strtok(NULL, ","));/*Active Time*/

					_sBC66Link->status.cpsmsIsEnabled= true;
				}
			}
			else if(NULL!= strstr(_URCMsg, "+CEDRXP:"))
			{
				char* _paramString = _URCMsg+ strlen("+CEDRXP:");
				char* _marker;
				_sBC66Link->status.eDrx.actType = strtol(_paramString, &_marker, 10);
				if(5== _sBC66Link->status.eDrx.actType )
				{
					_sBC66Link->status.eDrx.requested= BC66UTIL_EDrxToMiliseconds(strtol(_marker+ 1, &_marker, 2));
					_sBC66Link->status.eDrx.provided= BC66UTIL_EDrxToMiliseconds(strtol(_marker+ 1, &_marker, 2));
					_sBC66Link->status.eDrx.pagingTime= BC66UTIL_PagingTimeToMiliseconds(strtol(_marker+ 1, &_marker, 2));/*in miliseconds*/

					config.nbiot.rteEdrx= _sBC66Link->status.eDrx.provided;
					config.nbiot.rtePagingTime= _sBC66Link->status.eDrx.pagingTime;
				}
			}
			else if(NULL!= strstr(_URCMsg, "+CEREG:"))
			{
				char* _paramString = _URCMsg+ strlen("+CEREG:");
				char* _marker;

				_sBC66Link->status.cereg.regStatus= (BC66LINK_Network_t)strtol(_paramString, &_marker, 10);
				_sBC66Link->status.cereg.trackingAreaCode= strtol(_marker+ 1, &_marker, 16);
				_sBC66Link->status.cereg.cellId= strtol(_marker+ 1, &_marker, 16);
				_sBC66Link->status.cereg.accessTechnology= strtol(_marker+ 1, &_marker, 10);
				_sBC66Link->status.cereg.rejectType= strtol(_marker+ 1, &_marker, 10);
				_sBC66Link->status.cereg.rejectCause= strtol(_marker+ 1, &_marker, 10);
				_sBC66Link->status.cereg.activeTime_raw= strtol(_marker+ 1, &_marker, 2);
				_sBC66Link->status.cereg.periodicTau_raw= strtol(_marker+ 1, &_marker, 2);
				_sBC66Link->status.cereg.activeTime= BC66UTIL_GPRSTimer2ToSeconds(_sBC66Link->status.cereg.activeTime_raw);
				_sBC66Link->status.cereg.periodicTau= BC66UTIL_GPRSTimer3ToSeconds(_sBC66Link->status.cereg.periodicTau_raw);

				config.nbiot.rteActiveTime= _sBC66Link->status.cereg.activeTime;
				config.nbiot.rtePeriodicTau= _sBC66Link->status.cereg.periodicTau;

				if(REGISTRATION_DENIED_BC66LinkNetwork== _sBC66Link->status.cereg.regStatus)
				{
					sBC66Link.status.regDeniedCount++;
				}

				DIAG_Code(CEREG_NbiotDCode, _sBC66Link->status.cereg.regStatus);
				bNewNetworkRegistration= (1== _sBC66Link->status.cereg.regStatus)?true: false;
			}
			else if(NULL!= strstr(_URCMsg, "+IP:"))
			{
				char *_token= strtok(_URCMsg+ strlen("+IP:"), ",");
				strcpy(_sBC66Link->status.ip, _token);
			}
			else if(NULL!= strstr(_URCMsg, "+CSQ:"))//+CSQ: 28,99
			{
				char *_token= strtok(_URCMsg+ strlen("+CSQ:"), ",");
				_sBC66Link->status.rssi = atoi(_token);
			}
			else if(NULL!= strstr(_URCMsg, "+QCCLK:"))/*+QCCLK: 22/02/14,03:26:03+32*/
			{
				bool _isPositiveGMT= ('+'== _URCMsg[24])? true: false;
				char *_token= strtok(_URCMsg+ strlen("+QCCLK:"), "/");
				_sBC66Link->status.time.year= atoi(_token);
				_sBC66Link->status.time.month= atoi(strtok(NULL, "/"));
				_sBC66Link->status.time.day= atoi(strtok(NULL, ","));
				_sBC66Link->status.time.hour= atoi(strtok(NULL, ":"));
				_sBC66Link->status.time.minute= atoi(strtok(NULL, ":"));
				if(true== _isPositiveGMT)
				{
					_sBC66Link->status.time.second= atoi(strtok(NULL, "+"));
					_sBC66Link->status.time.gmt= atoi(strtok(NULL, ","));
				}
				else
				{
					_sBC66Link->status.time.second= atoi(strtok(NULL, "-"));
					_sBC66Link->status.time.gmt= (-1* atoi(strtok(NULL, ",")));
				}

				if(23<= _sBC66Link->status.time.year) /*floor check (2023)*/
				{
					struct tm _timeinfo;
					time_t _epochTime;
					_timeinfo.tm_mon= _sBC66Link->status.time.month- 1 ;   // check assumption here Jan = 0 in tm
					_timeinfo.tm_mday= _sBC66Link->status.time.day ;
					_timeinfo.tm_year= _sBC66Link->status.time.year+ 100;  // check assumption here years start from 1900 in tm
					_timeinfo.tm_hour= _sBC66Link->status.time.hour ;
					_timeinfo.tm_min= _sBC66Link->status.time.minute;
					_timeinfo.tm_sec= _sBC66Link->status.time.second;
					_epochTime= mktime(&_timeinfo);
					_epochTime+= _sBC66Link->status.time.gmt* 15* 60; /*15 mins to sec*/
					_timeinfo= *(localtime(&_epochTime));

					SYS_DateTime_Update(0, /*weekday*/
										UTILI_BinToBCD(_timeinfo.tm_mday), UTILI_BinToBCD(_timeinfo.tm_mon+ 1), UTILI_BinToBCD(_timeinfo.tm_year- 100),/*ddMMyy*/
										UTILI_BinToBCD(_timeinfo.tm_hour), UTILI_BinToBCD(_timeinfo.tm_min), UTILI_BinToBCD(_timeinfo.tm_sec),/*hhmmss*/
										_sBC66Link->status.time.gmt);/*UTC*/
				}

			}
			else if(NULL!= strstr(_URCMsg, "+QPING:"))
			{
				char *_token= strtok(_URCMsg+ strlen("+QPING:"), ",");
				uint8_t _result= atoi(_token);
				if(0== _result)/*successful*/
				{
					char *_next= strtok(NULL, ",");
					//09:15:37: +QPING: 0,"118.189.126.182",32,665,51
					//09:15:38: +QPING: 0,4,4,0,298,665,505
					if(NULL== strchr(_next, '.'))/*we get 2 types of QPING, we want the one that doesn't return an IP*/
					{
						uint16_t _sent= atoi(_next);
						uint16_t _rcvd= atoi(strtok(NULL, ","));
						uint16_t _lost= atoi(strtok(NULL, ","));
						config.nbiot.stats.pingLatency_ms= atoi(strtok(NULL, ","));
						config.nbiot.stats.pingLatency_ms= atoi(strtok(NULL, ","));
						config.nbiot.stats.pingLatency_ms=	atoi(strtok(NULL, ","));
					}
				}
				else
				{
					config.nbiot.stats.pingLatency_ms= 0xFFFF;
				}
			}
			else if(NULL!= strstr(_URCMsg, "+QENG:"))
			{
				char *_token= strtok(_URCMsg+ strlen("+QENG:"), ",");
				uint8_t _mode= atoi(_token);
				switch(_mode)
				{
					case 0:
						_sBC66Link->urc.QENG.earfcn= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.earfcnOffset= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.pci= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.cellID= UTILI_HexStringToUint32(strtok(NULL, ","));
						_sBC66Link->urc.QENG.rsrp= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.rsrq= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.rssi= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.sinr= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.band= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.tac= UTILI_HexStringToUint32(strtok(NULL, ","));
						_sBC66Link->urc.QENG.ecl= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.txPwr= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.oprMode= atoi(strtok(NULL, ","));

						config.nbiot.stats.rsrp= _sBC66Link->urc.QENG.rsrp;
						config.nbiot.stats.rssi= _sBC66Link->urc.QENG.rssi;
						config.nbiot.stats.sinr= _sBC66Link->urc.QENG.sinr;
						config.nbiot.stats.rsrq= _sBC66Link->urc.QENG.rsrq;
						config.nbiot.stats.txPower= _sBC66Link->urc.QENG.txPwr;
						config.nbiot.stats.ceMode= _sBC66Link->urc.QENG.oprMode;
						config.nbiot.stats.ecl= _sBC66Link->urc.QENG.ecl;
						config.nbiot.stats.battVoltage_mV= (uint16_t)SENSOR_GetValue(VCELL_Sensor);
						config.nbiot.rteSignalSampleCount++;
						config.nbiot.rteTotalRsrp+= config.nbiot.stats.rsrp;
						config.nbiot.rteTotalRssi+= config.nbiot.stats.rssi;
						config.nbiot.rteTotalSinr+= config.nbiot.stats.sinr;
						config.nbiot.rteTotalRsrq+= config.nbiot.stats.rsrq;
						config.nbiot.rteTotalTxPower+= config.nbiot.stats.txPower;

						config.nbiot.stats.aveRsrp= config.nbiot.rteTotalRsrp/ config.nbiot.rteSignalSampleCount;
						config.nbiot.stats.minRsrp= (config.nbiot.stats.rsrp< config.nbiot.stats.minRsrp)? config.nbiot.stats.rsrp: config.nbiot.stats.minRsrp;
						config.nbiot.stats.maxRsrp= (config.nbiot.stats.rsrp> config.nbiot.stats.maxRsrp)? config.nbiot.stats.rsrp: config.nbiot.stats.maxRsrp;

						config.nbiot.stats.aveRssi= config.nbiot.rteTotalRssi/ config.nbiot.rteSignalSampleCount;
						config.nbiot.stats.minRssi= (config.nbiot.stats.rssi< config.nbiot.stats.minRssi)? config.nbiot.stats.rssi: config.nbiot.stats.minRssi;
						config.nbiot.stats.maxRssi= (config.nbiot.stats.rssi> config.nbiot.stats.maxRssi)? config.nbiot.stats.rssi: config.nbiot.stats.maxRssi;

						config.nbiot.stats.aveSinr= config.nbiot.rteTotalSinr/ config.nbiot.rteSignalSampleCount;
						config.nbiot.stats.minSinr= (config.nbiot.stats.sinr< config.nbiot.stats.minSinr)? config.nbiot.stats.sinr: config.nbiot.stats.minSinr;
						config.nbiot.stats.maxSinr= (config.nbiot.stats.sinr> config.nbiot.stats.maxSinr)? config.nbiot.stats.sinr: config.nbiot.stats.maxSinr;

						config.nbiot.stats.aveRsrq= config.nbiot.rteTotalRsrq/ config.nbiot.rteSignalSampleCount;
						config.nbiot.stats.minRsrq= (config.nbiot.stats.rsrq< config.nbiot.stats.minRsrq)? config.nbiot.stats.rsrq: config.nbiot.stats.minRsrq;
						config.nbiot.stats.maxRsrq= (config.nbiot.stats.rsrq> config.nbiot.stats.maxRsrq)? config.nbiot.stats.rsrq: config.nbiot.stats.maxRsrq;

						config.nbiot.stats.aveTxPower= config.nbiot.rteTotalTxPower/ config.nbiot.rteSignalSampleCount;
						config.nbiot.stats.minTxPower= (config.nbiot.stats.txPower< config.nbiot.stats.minTxPower)? config.nbiot.stats.txPower: config.nbiot.stats.minTxPower;
						config.nbiot.stats.maxTxPower= (config.nbiot.stats.txPower> config.nbiot.stats.maxTxPower)? config.nbiot.stats.txPower: config.nbiot.stats.maxTxPower;

						if(true== bNewNetworkRegistration)
						{
							DIAG_Code(EARFCN_NbiotDCode, _sBC66Link->urc.QENG.earfcn);
							DIAG_Code(EARFCNOFFSET_PCI_NbiotDCode, (_sBC66Link->urc.QENG.earfcnOffset<< 16)| _sBC66Link->urc.QENG.pci);
							DIAG_Code(CELL_ID_NbiotDCode, _sBC66Link->urc.QENG.cellID);
							DIAG_Code(RSRP_RSRQ_NbiotDCode, (_sBC66Link->urc.QENG.rsrp<< 16)| (0xFFFF& _sBC66Link->urc.QENG.rsrq));
							DIAG_Code(RSSI_SINR_NbiotDCode, (_sBC66Link->urc.QENG.rssi<< 16)| (0xFFFF& _sBC66Link->urc.QENG.sinr));
							DIAG_Code(BAND_TAC_NbiotDCode, (_sBC66Link->urc.QENG.band<< 16)| (0xFFFF& _sBC66Link->urc.QENG.tac));
							DIAG_Code(ECL_TXPOWER_OPRMODE_NbiotDCode, (_sBC66Link->urc.QENG.ecl<< 24)| (_sBC66Link->urc.QENG.txPwr<< 8)| _sBC66Link->urc.QENG.oprMode);
						}
						break;
					case 1:
						_sBC66Link->urc.QENG.neighborEarfcn= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.neighborEarfcnOffset= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.neighborPci= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.neighborRsrp= atoi(strtok(NULL, ","));
						break;
					case 2:
						_sBC66Link->urc.QENG.rlcUlBler= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.rlcDlBler= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.macUlBler= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.macDlBler= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.macUlTotalBytes= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.macDlTotalBytes= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.macUlTotalHarqTx= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.macDlTotalHarqTx= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.macUlTotalHarqReTx= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.macDlTotalHarqReTx= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.rlcUlTput= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.rlcDlTput= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.macUlTput= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.macDlTput= atoi(strtok(NULL, ","));
						break;
					case 3:
						_sBC66Link->urc.QENG.sleepDuration= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.rxTime= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.txTime= atoi(strtok(NULL, ","));
						if(true== bNewNetworkRegistration)
						{
							DIAG_Code(SLEEP_DURATION_NbiotDCode, _sBC66Link->urc.QENG.sleepDuration);
							DIAG_Code(RX_TIME_NbiotDCode, _sBC66Link->urc.QENG.rxTime);
							DIAG_Code(TX_TIME_NbiotDCode, _sBC66Link->urc.QENG.txTime);
						}
						break;
					case 4:
						_sBC66Link->urc.QENG.emmState= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.emmMode= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.plmnState= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.plmnType= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.selectePlmn= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QENG.err= atoi(strtok(NULL, ","));
						bNewNetworkRegistration = false;
						break;
				}
			}
//			else if(NULL!= strstr(_URCMsg, "+NPSMR:"))
//			{
//				if(0== strcmp(_URCMsg+ strlen("+NPSMR:"), "0"))
//				{
//					_sBC66Link->status.inPowerSavingMode= false;
//				}
//				else if(0== strcmp(_URCMsg+ strlen("+NPSMR:"), "1"))
//				{
//					_sBC66Link->status.inPowerSavingMode= true;
//				}
//			}
			else if(NULL!= strstr(_URCMsg, "+CMEERROR"))
			{
				_sBC66Link->status.cmeErrorReceived= true;/*513 module not registered*/
				_sBC66Link->status.cmeErrorType= atoi(_URCMsg+ strlen("+CMEERROR:"));
				switch(_sBC66Link->status.cmeErrorType)
				{
					case 10:
					case 13:
						DIAG_Code(SIM_ERROR_NbiotDCode, ++config.nbiot.stats.noOfSimError);
						break;
					case 847:/*idle state*/
						BC66LINK_ResetModem();
						break;
				}

				DIAG_Code(CME_ERROR_NbiotDCode, (_sBC66Link->state<< 24)| (_sBC66Link->subState<< 16)| _sBC66Link->status.cmeErrorType);
			}
			else if(NULL!= strstr(_URCMsg, "+CMEEERROR"))/*babi quectel inconsistent spelling*/
			{
				_sBC66Link->status.cmeErrorReceived= true;/*513 module not registered*/
				_sBC66Link->status.cmeErrorType= atoi(_URCMsg+ strlen("+CMEEERROR:"));
				switch(_sBC66Link->status.cmeErrorType)
				{
					case 10:
					case 13:
						DIAG_Code(SIM_ERROR_NbiotDCode, ++config.nbiot.stats.noOfSimError);
						break;
					case 847:/*idle state*/
						BC66LINK_ResetModem();
						break;
					case 1:/*lw other error*//*this error occurs specifically when we send notify after cscon=0. due to unknown behavior, we should reset the modem(for now)*/
					case 4:/*lw not register error*//*this error occurs specifically when we send update after cscon=0. due to unknown behavior, we should reset the modem(for now)*/
					case 7:/*lw disable error*/
					case 32:/*keep connecting error*/
						BC66LINK_ResetModem();
						break;
				}

				DIAG_Code(CME_ERROR_NbiotDCode, (_sBC66Link->state<< 24)| (_sBC66Link->subState<< 16)| _sBC66Link->status.cmeErrorType);
			}
			else if(NULL!= strstr(_URCMsg, "+CSCON:"))
			{
				_sBC66Link->status.isConnected= (atoi(_URCMsg+ strlen("+CSCON:"))== 0)? false: true;
//				if((false== _sBC66Link->status.isConnected)&& (true== config.nbiot.lwm2m.lwObj[SOFTWARE_MANAGEMENT_ObjName].resource[UPDATE_STATE_SwMgtResource].observe))
//				{
//					config.nbiot.lwm2m.lwObj[SOFTWARE_MANAGEMENT_ObjName].resource[UPDATE_STATE_SwMgtResource].notify= true;
//				}
			}
			else if(NULL!= strstr(_URCMsg, "+QNBIOTEVENT:"))
			{
				char *_token= strtok(_URCMsg+ strlen("+QNBIOTEVENT:"), ",");

				if(0== strcmp(_token, "ENTERPSM"))
				{
					_sBC66Link->status.inPSM= true;
					_sBC66Link->status.sendLwUpdate= true;
					_sBC66Link->status.PSMExitTriggered= false;
					BC66PHY_Sleep();
				}

				if(0== strcmp(_token, "ENTERDEEPSLEEP"))
				{
					_sBC66Link->status.inPSM= true;
					_sBC66Link->status.sendLwUpdate= true;
					_sBC66Link->status.PSMExitTriggered= false;

					_sBC66Link->status.inDeepSleep= true;
					_sBC66Link->urc.QLWURC.recovered= 0xFF;/*need to check this flag before transmit notify*/
					BC66PHY_Sleep();
				}

				if(0== strcmp(_token, "EXITPSM"))
				{
					_sBC66Link->status.inPSM= false;
					_sBC66Link->status.inDeepSleep= false;/*use this flag cos it doesnt has its own exit flag*/
					BC66PHY_Wakeup();
				}
			}
			else if(NULL!= strstr(_URCMsg, "+QCOAPSEND:"))
			{
				_sBC66Link->urc.QCOAPSEND= (uint8_t)atoi(_URCMsg+ strlen("+QCOAPSEND:"));
			}
			else if(NULL!= strstr(_URCMsg, "+QCOAPURC:"))/*special handling for non urc message*//*p/s: " and spaced remove*/
			{
				char *_token= strtok(_URCMsg+ strlen("+QCOAPURC:"), ",");
				if(0== strcmp(_token, "rsp"))
				{
					_sBC66Link->urc.QCOAPURC.rsp.type= atoi(strtok(NULL, ","));
					char *_stringRspCode= strtok(NULL, ",");
					_sBC66Link->urc.QCOAPURC.rsp.messageId= atoi(strtok(NULL, ","));
					if(ulPrevMessageId!= _sBC66Link->urc.QCOAPURC.rsp.messageId)/*to not send duplication*/
					{
						ulPrevMessageId= _sBC66Link->urc.QCOAPURC.rsp.messageId;

						_sBC66Link->urc.QCOAPURC.rsp.len= atoi(strtok(NULL, ","));
						_sBC66Link->urc.QCOAPURC.rsp.data= strtok(NULL, ",");
						/*convert string to integer respond code, because strtok(NULL.. needs to be in sequence*/
						_sBC66Link->urc.QCOAPURC.rsp.rspCode= (atoi(strtok(_stringRspCode, "."))* 100)+ atoi(strtok(NULL, ","));

						BC66LINK_Coap_Response_t _response;

						_response.type= _sBC66Link->urc.QCOAPURC.rsp.type;
						_response.rspCode= _sBC66Link->urc.QCOAPURC.rsp.rspCode;
						_response.messageId= _sBC66Link->urc.QCOAPURC.rsp.messageId;
						_response.len= _sBC66Link->urc.QCOAPURC.rsp.len;
						_response.data= _sBC66Link->urc.QCOAPURC.rsp.data;

						if((1== sBC66Link.coapContext)&& (NULL!= _sBC66Link->coapCallback.Response))
						{
							_sBC66Link->coapCallback.Response((void *) (&_response));
						}

						if((2== sBC66Link.coapContext)&& (NULL!= _sBC66Link->coapCallback2.Response))
						{
							_sBC66Link->coapCallback2.Response((void *) (&_response));
						}
					}
				}
				else if(0== strcmp(_token, "req"))
				{
					BC66LINK_Coap_Request_t _request;

					_request.type= atoi(strtok(NULL, ","));
					_request.method= atoi(strtok(NULL, ","));
					_request.messageId= atoi(strtok(NULL, ","));
					_request.mode= strtok(NULL, ",");
					_request.tokenLen= atoi(strtok(NULL, ","));
					_request.token= strtok(NULL, ",");
					_request.optionName= atoi(strtok(NULL, ","));
					_request.optionValue= atoi(strtok(NULL, ","));
					_request.len= atoi(strtok(NULL, ","));
					_request.data= strtok(NULL, ",");

					if((1== sBC66Link.coapContext)&& (NULL!= _sBC66Link->coapCallback.Request))
					{
						_sBC66Link->coapCallback.Request((void *) (&_request));
					}

					if((2== sBC66Link.coapContext)&& (NULL!= _sBC66Link->coapCallback2.Request))
					{
						_sBC66Link->coapCallback2.Request((void *) (&_request));
					}
				}
				else if(0== strcmp(_token, "recovered"))
				{
					BC66LINK_Coap_Recovery_t _recovery;

					_recovery.state= atoi(strtok(NULL, ","));
					DIAG_Code(COAP_RECOVERED_NbiotDCode, _recovery.state);

					if(0!= _recovery.state)
					{
						sBC66Link.coapContext= 0;
					}

					if(NULL!= _sBC66Link->coapCallback.Recovery)
					{
						_sBC66Link->coapCallback.Recovery((void *) (&_recovery));
					}

					if(NULL!= _sBC66Link->coapCallback2.Recovery)
					{
						_sBC66Link->coapCallback2.Recovery((void *) (&_recovery));
					}
				}
			}
			else if(NULL!= strstr(_URCMsg, "+QLWREG:"))
			{
				_sBC66Link->urc.QLWREG= (uint8_t)atoi(_URCMsg+ strlen("+QLWREG:"));

				BC66LINK_Lwm2m_Register_t _register;

				_register.state= _sBC66Link->urc.QLWREG;
				_sBC66Link->status.qlwregReceived= true;

				if(NULL!= _sBC66Link->lwm2mCallback.Register)
				{
					_sBC66Link->lwm2mCallback.Register((void *) (&_register));
				}
			}
			else if(NULL!= strstr(_URCMsg, "+QLWDEREG:"))
			{
				_sBC66Link->urc.QLWDEREG= (uint8_t)atoi(_URCMsg+ strlen("+QLWDEREG:"));
			}
			else if(NULL!= strstr(_URCMsg, "+QLWADDOBJ:"))
			{
				char *_token= strtok(_URCMsg+ strlen("+QLWADDOBJ:"), ",");
				_sBC66Link->urc.QLWADDOBJ[_sBC66Link->urc.QLWADDOBJCount++]= atoi(_token);
			}
			else if(NULL!= strstr(_URCMsg, "+QLWWRRSP:"))
			{
				_sBC66Link->urc.QLWWRRSP= (uint8_t)atoi(_URCMsg+ strlen("+QLWWRRSP:"));
			}
			else if(NULL!= strstr(_URCMsg, "+QLWRDRSP:"))
			{
				_sBC66Link->urc.QLWRDRSP= (uint8_t)atoi(_URCMsg+ strlen("+QLWRDRSP:"));
			}
			else if(NULL!= strstr(_URCMsg, "+QLWOBSRSP:"))
			{
				_sBC66Link->urc.QLWOBSRSP= (uint8_t)atoi(_URCMsg+ strlen("+QLWOBSRSP:"));
			}
			else if(NULL!= strstr(_URCMsg, "+QLWEXERSP:"))
			{
				_sBC66Link->urc.QLWEXERSP= (uint8_t)atoi(_URCMsg+ strlen("+QLWEXERSP:"));
			}
			else if(NULL!= strstr(_URCMsg, "+QLWNOTIFY:"))
			{
				_sBC66Link->urc.QLWNOTIFY= (uint8_t)atoi(_URCMsg+ strlen("+QLWNOTIFY:"));
			}
			else if(NULL!= strstr(_URCMsg, "+QLWUPDATE:"))
			{
				/*qlupdate will return following:
				 * 	04:16:36: +QLWUPDATE: 23391
					04:16:36: OK
					04:16:37: +QLWUPDATE: 0,23391 <-- this is confirmation server has received.we want this.
				 */
				char *_token= strtok(_URCMsg+ strlen("+QLWUPDATE:"), ",");
				_sBC66Link->urc.QLWUPDATE= (uint8_t)atoi(_token);
			}
			else if(NULL!= strstr(_URCMsg, "+QLWCONFIG:"))
			{
				char *_token= strtok(_URCMsg+ strlen("+QLWCONFIG:"), ",");
				uint8_t _bootstrapEnabled= atoi(_token);
				if(0== _bootstrapEnabled)/*if this is 0, we have the latest one*/
				{
					_token= strtok(NULL, ",");
					UTILI_Array_CopyString(config.nbiot.lwm2m.currentConnection.serverIP, _token);
					config.nbiot.lwm2m.currentConnection.serverPort= atoi(strtok(NULL, ","));
					_token= strtok(NULL, ",");
					UTILI_Array_CopyString(config.nbiot.lwm2m.currentConnection.endpointName, _token);
					config.nbiot.lwm2m.currentConnection.lifetime_s= atoi(strtok(NULL, ","));
					config.nbiot.lwm2m.currentConnection.securityMode= (BC66LINK_Lwm2m_Security_Mode_t)atoi(strtok(NULL, ","));
					_token= strtok(NULL, ",");
					UTILI_Array_CopyString(config.nbiot.lwm2m.currentConnection.pskId, _token);
					_token= strtok(NULL, ",");
					UTILI_Array_CopyString(config.nbiot.lwm2m.currentConnection.psk, _token);
				}
			}
			else if(NULL!= strstr(_URCMsg, "+QLWURC:"))
			{
				char *_token= strtok(_URCMsg+ strlen("+QLWURC:"), ",");
				if(0== strcmp(_token, "ping"))
				{
					_token= strtok(NULL, ",");
					_sBC66Link->urc.QLWURC.ping= atoi(_token);
				}
				else if(0== strcmp(_token, "buffer"))
				{
					_token= strtok(NULL, ",");
				}
				else if(0== strcmp(_token, "lifetime_changed"))
				{
					_token= strtok(NULL, ",");
					config.nbiot.lwm2m.currentConnection.lifetime_s= atoi(_token);
				}
				else if(0== strcmp(_token, "write"))
				{
					BC66LINK_Lwm2m_Request_t _request;
					char *_hexString;

					_request.messageId= atoi(strtok(NULL, ","));
					if(ulPrevMessageId!= _request.messageId)/*to not acked on previous duplicated message*/
					{
						ulPrevMessageId= _request.messageId;
						_sBC66Link->urc.QLWURC.write= true;
						_request.objectId= atoi(strtok(NULL, ","));
						_request.instanceId= atoi(strtok(NULL, ","));
						_request.resourceId= atoi(strtok(NULL, ","));
						_request.valueType= atoi(strtok(NULL, ","));
						_request.len= atoi(strtok(NULL, ","));
						_request.value= strtok(NULL, ",");
						if(OPAQUE_Bc66LinkLwm2mValueType== _request.valueType)/*if value type is opaque, convert hexstring to bytes*/
						{
							UTILI_HexStringToBytes(_request.value, _request.value, _request.len);/*can use back same buffer because the final size is reduced by have*/
						}

						if(NULL!= _sBC66Link->lwm2mCallback.WriteRequest)
						{
							_sBC66Link->lwm2mCallback.WriteRequest((void *) (&_request));
						}
					}
				}
				else if(0== strcmp(_token, "read"))
				{
					BC66LINK_Lwm2m_Request_t _request;

					_request.messageId= atoi(strtok(NULL, ","));
					if(ulPrevMessageId!= _request.messageId)/*to not acked on previous duplicated message*/
					{
						ulPrevMessageId= _request.messageId;
						_sBC66Link->urc.QLWURC.read= true;
						_request.objectId= atoi(strtok(NULL, ","));
						_request.instanceId= atoi(strtok(NULL, ","));
						_request.resourceId= atoi(strtok(NULL, ","));

						if(NULL!= _sBC66Link->lwm2mCallback.ReadRequest)
						{
							_sBC66Link->lwm2mCallback.ReadRequest((void *) (&_request));
						}
					}
				}
				else if(0== strcmp(_token, "execute"))
				{
					BC66LINK_Lwm2m_Request_t _request;

					_request.messageId= atoi(strtok(NULL, ","));
					if(ulPrevMessageId!= _request.messageId)/*to not acked on previous duplicated message*/
					{
						ulPrevMessageId= _request.messageId;
						_sBC66Link->urc.QLWURC.execute= true;
						_request.objectId= atoi(strtok(NULL, ","));
						_request.instanceId= atoi(strtok(NULL, ","));
						_request.resourceId= atoi(strtok(NULL, ","));

						if(NULL!= _sBC66Link->lwm2mCallback.ExecuteRequest)
						{
							_sBC66Link->lwm2mCallback.ExecuteRequest((void *) (&_request));
						}
					}
				}
				else if(0== strcmp(_token, "observe"))
				{
					BC66LINK_Lwm2m_Request_t _request;

					_request.messageId= atoi(strtok(NULL, ","));
					if(ulPrevMessageId!= _request.messageId)/*to not acked on previous duplicated message*/
					{
						ulPrevMessageId= _request.messageId;

						_sBC66Link->urc.QLWURC.observe= true;
						_request.flag= atoi(strtok(NULL, ","));
						_request.objectId= atoi(strtok(NULL, ","));
						_request.instanceId= atoi(strtok(NULL, ","));
						_request.resourceId= atoi(strtok(NULL, ","));

						if(NULL!= _sBC66Link->lwm2mCallback.ObserveRequest)
						{
							_sBC66Link->lwm2mCallback.ObserveRequest((void *) (&_request));
						}
					}
				}
				else if(0== strcmp(_token, "recovered"))
				{
					BC66LINK_Lwm2m_Recovery_t _recovery;

					_recovery.state= atoi(strtok(NULL, ","));
					DIAG_Code(LWM2M_RECOVERED_NbiotDCode, _recovery.state);

					_sBC66Link->urc.QLWURC.recovered= _recovery.state;

					if(NULL!= _sBC66Link->lwm2mCallback.Recovery)
					{
						_sBC66Link->lwm2mCallback.Recovery((void *) (&_recovery));
					}
				}
				else if(0== strcmp(_token, "report_ack"))
				{
					_sBC66Link->urc.QLWURC.report_ack_status= atoi(strtok(NULL, ","));
					_sBC66Link->urc.QLWURC.report_ack_messageId= atoi(strtok(NULL, ","));

					if(ulPrevMessageId!= _sBC66Link->urc.QLWURC.report_ack_messageId)/*to not acked on previous duplicated message*/
					{
						ulPrevMessageId= _sBC66Link->urc.QLWURC.report_ack_messageId;
						_sBC66Link->urc.QLWURC.report_ack= true;
					}
				}
//				else if(0== strcmp(_token, "report"))
//				{
//					_sBC66Link->urc.QLWURC.report= true;
//					_sBC66Link->urc.QLWURC.report_messageId= atoi(strtok(NULL, ","));
//				}
			}

			BC66PHY_URCMsg_Dequeue();
		}
	}
}

void BC66HANDLER_Reset(BC66LINK_t *_sBC66Link)
{
	if(0!= BC66PHY_ReplyMsg_Depth())
	{
		if(NULL!= strstr(BC66PHY_ReplyMsg_Peek(), "RDY"))/*The only bc66 Reset message that we recognize at baudrate 9600*/
		{
			if(false== _sBC66Link->status.bResetExpected)
			{
				DBG_Print("BC66HANDLER_Reset triggered.\r\n");
				if(true== BC66PHY_IsHardReset())
				{
					DIAG_Code(MODEM_ERROR_NbiotDCode, BC66PHY_DiagCode(HARD_RESET_Bc66PhyError));
				}
				else
				{
					DIAG_Code(MODEM_ERROR_NbiotDCode, BC66PHY_DiagCode(UINTENTIONAL_RESET_Bc66PhyError));
					bUnintentionalResetDetected=  true;
				}

				/*Unintended reset occur*/
				BC66LINK_ResetJob();
			}
			BC66PHY_ClearHardResetFlag();/*reagrdless, this flag must be cleard*/
		}
	}
}

bool BC66HANDLER_GetUnintentionalResetFlag(void)
{
	return bUnintentionalResetDetected;
}

void BC66HANDLER_ClearUnintentionalResetFlag(void)
{
	bUnintentionalResetDetected= false;
}
