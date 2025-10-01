/*
 * msg.c
 *
 *  Created on: 24 Jan 2021
 *      Author: muhammad.ahmad@georgekent.net
 */
#if 0

#include "common.h"
#include "msg.h"

static ErrorStatus MSG_FLASH_ProcessTlvs(uint8_t *_TLVsBuffer, uint16_t _TLVsBufferLen, uint8_t *_respTLVsBuffer, uint16_t *_respTLVsBufferLen);
static ErrorStatus MSG_INFO_READ_ProcessTlvs(uint8_t *_TLVsBuffer, uint16_t _TLVsBufferLen, uint8_t *_respTLVsBuffer, uint16_t *_respTLVsBufferLen);
static ErrorStatus MSG_INFO_WRITE_ProcessTlvs(uint8_t *_TLVsBuffer, uint16_t _TLVsBufferLen, uint8_t *_respTLVsBuffer, uint16_t *_respTLVsBufferLen);
static ErrorStatus MSG_ProcessTlvs(uint8_t *_TLVsBuffer, uint16_t _TLVsBufferLen, uint8_t *_respTLVsBuffer, uint16_t *_respTLVsBufferLen);

__IO static MSG_t	eMsgQueue[MSG_MAX_MSG_ARRAY];
__IO static uint16_t 	ubMsgIndex= 0;
__IO static uint8_t 	ubMsgArrayHead= 0;
__IO static uint8_t 	ubMsgArrayTail= 0;

__IO MSG_t		eCurrMsg;

uint8_t MSG_Msg_Depth(void)
{
	if(ubMsgArrayTail< ubMsgArrayHead)
	{
		return ubMsgArrayHead- ubMsgArrayTail;
	}
	else if(ubMsgArrayTail> ubMsgArrayHead)
	{
		return (MSG_MAX_MSG_ARRAY+ ubMsgArrayHead)- ubMsgArrayTail;
	}
	else
	{
		return 0;
	}
}

ErrorStatus MSG_Msg_Enqueue(MSG_t _msg)
{
	if(MSG_MAX_MSG_ARRAY> MSG_Msg_Depth())
	{
		eMsgQueue[ubMsgArrayHead]= _msg;
		ubMsgArrayHead= (ubMsgArrayHead+ 1)% MSG_MAX_MSG_ARRAY;
		//DBG_Print("MSG> ubMsgArrayHead: %d ubMsgArrayTail: %d \r\n", ubMsgArrayHead, ubMsgArrayTail);
		return SUCCESS;
	}
	return ERROR;
}

MSG_t MSG_Msg_Dequeue(void)
{
	MSG_t _msg= {0};
	if(ubMsgArrayTail!= ubMsgArrayHead)
	{
		_msg= eMsgQueue[ubMsgArrayTail];
		ubMsgArrayTail= (ubMsgArrayTail+ 1)% MSG_MAX_MSG_ARRAY;
	}
	return _msg;
}

MSG_t MSG_Msg_Peek(void)
{
	MSG_t _msg= {0};
	if(ubMsgArrayTail!= ubMsgArrayHead)
	{
		_msg= eMsgQueue[ubMsgArrayTail];
	}

	return _msg;
}

SYS_TaskId_t MSG_Msg_TaskId_Peek(void)
{
	return eMsgQueue[ubMsgArrayTail].taskId;
}

void MSG_Init(void)
{

}

void MSG_Task(void)
{
	if(false== ((0!= MSG_Msg_Depth())&& (MSG_TaskId== MSG_Msg_TaskId_Peek())))
	{
		return;
	}
	eCurrMsg= MSG_Msg_Dequeue();

	uint8_t *_reqPtr= eCurrMsg.buffer;
	uint16_t _lenProcessed= 0;
	uint8_t _replyBuffer[256];
	uint8_t *_replyPtr= _replyBuffer;
	uint16_t _replyLen= 0;

	while(eCurrMsg.bufferLen> _lenProcessed)
	{
		uint8_t 	respBuffer[256];
		uint16_t 	respBufferLen= 0;
		ErrorStatus _status;

		switch(_reqPtr[MSG_TAG])
		{
			case FLASH_TAG:
				_status= MSG_FLASH_ProcessTlvs(&_reqPtr[MSG_VALUE], _reqPtr[MSG_LEN], respBuffer, &respBufferLen);
				break;

			case INFO_READ_TAG:
				_status= MSG_INFO_READ_ProcessTlvs(&_reqPtr[MSG_VALUE], _reqPtr[MSG_LEN], respBuffer, &respBufferLen);
				break;

			case INFO_WRITE_TAG:
				_status= MSG_INFO_WRITE_ProcessTlvs(&_reqPtr[MSG_VALUE], _reqPtr[MSG_LEN], respBuffer, &respBufferLen);
				break;

			case CMD_TAG:
				//_status= MSG_CMD_ProcessTlvs(&_reqPtr[MSG_VALUE], _reqPtr[MSG_LEN], respBuffer, &respBufferLen);
				break;

			case AURA_TAG:
				_status= MSG_ProcessTlvs(&_reqPtr[MSG_VALUE], _reqPtr[MSG_LEN], respBuffer, &respBufferLen);
				break;

			default:
				_status= ERROR;
				break;
		}

		if(true== eCurrMsg.sendResponse)
		{
			uint16_t _respLen= 0;
			_replyPtr[MSG_TAG]= _reqPtr[MSG_TAG];
			_replyPtr[MSG_LEN]= respBufferLen+ 1;/*minus initial adddress*/;
			_respLen= 1+ 1+ _replyPtr[MSG_LEN];
			if(254< _respLen)/*reserve 2 bytes for crc*//*todo: clean this, this suppose to be agnostic*/
			{
				_replyPtr[MSG_LEN]= 1;
				_respLen= 1+ 1+ 1;
				_replyPtr[MSG_VALUE]= MSG_UNDEFINED_ERROR;
			}
			else
			{
				_replyPtr[MSG_VALUE]= (_status== SUCCESS)? MSG_NO_ERROR: MSG_UNDEFINED_ERROR;
				memcpy(&_replyPtr[MSG_VALUE+ 1], respBuffer, respBufferLen);
			}

			_replyLen+= _respLen;
			_replyPtr+= _replyLen;
		}

		_lenProcessed+= (1+ 1+ _reqPtr[MSG_LEN]);
		_reqPtr+= _lenProcessed;
	}

	if(true== eCurrMsg.sendResponse)
	{
		memcpy(eCurrMsg.buffer, _replyBuffer, _replyLen);
		eCurrMsg.bufferLen= _replyLen;
		eCurrMsg.taskId= eCurrMsg.responseTaskId;
		MSG_Msg_Enqueue(eCurrMsg);
	}
}

static ErrorStatus MSG_FLASH_ProcessTlvs(uint8_t *_TLVsBuffer, uint16_t _TLVsBufferLen, uint8_t *_respTLVsBuffer, uint16_t *_respTLVsBufferLen)
{
	TLV_Typedef _tlv;
	TLV_Typedef _respTlv;
	uint16_t 	 _tlvsIndex= 0;
	uint16_t     _respTlvsIndex= 0;
	ErrorStatus  _status= SUCCESS;

	static 	bool 							_ucFlashInitialized= false;
	static 	bool 							_ucFlashErased= false;
	__IO 	uint8_t 						_pulFlashBuffer[128];

#if PARTITION== 2
	uint8_t _alternateSector= 0;
	uint32_t _alternateSectorStartAddress= CFG_PROGRAM_SECTOR_1_ADDR;
#else
	uint8_t _alternateSector= 1;
	uint32_t _alternateSectorStartAddress= CFG_PROGRAM_SECTOR_2_ADDR;
#endif

	while(_tlvsIndex< _TLVsBufferLen)
	{
		_tlv.tag= 		(_TLVsBuffer+ _tlvsIndex)[0];
		_tlv.len= 		(_TLVsBuffer+ _tlvsIndex)[1];
		_tlv.value=		(_TLVsBuffer+ _tlvsIndex)+ 2;
		_respTlv.tag= 	 (_respTLVsBuffer+ _respTlvsIndex)[0]= _tlv.tag;
		_respTlv.len= 	 (_respTLVsBuffer+ _respTlvsIndex)[1]= 0;
		_respTlv.value= (_respTLVsBuffer+ _respTlvsIndex)+ 2;

		switch(_tlv.tag)
		{
			case FLASH_INIT_TAG:
				{
					DBG_Print("FLASH_INIT_TAG\r\n");
					if(SUCCESS== CFG_EraseProgramSector(_alternateSectorStartAddress))
					{
						_ucFlashInitialized= true;
						_ucFlashErased= true;
						memcpy(_respTlv.value, &_alternateSectorStartAddress, 4);
						_respTlv.value[4]= _alternateSector;
						_respTlv.len= 5;
					}
					else
					{
						DBG_Print("FLASH_INIT_TAG: ERROR\r\n");
						_status= ERROR;
					}
				}
				break;
			case FLASH_WRITE_TAG:
				if(false== _ucFlashInitialized)
				{
					DBG_Print("FLASH_WRTIE_TAG: NOT INITLIAZED\r\n");
					_status= ERROR;
				}
				else if(((4+ 128+ 2)!= _tlv.len)&& ((4+ 256+ 2)!= _tlv.len))
				{
					DBG_Print("FLASH_WRTIE_TAG: LENGTH ERROR\r\n");
					_status= ERROR;
				}
				else
				{
					uint32_t _noOfBytesToProgram= _tlv.len- (4+ 2);
					uint32_t _result;
					uint32_t _location;
					memcpy(&_location, _tlv.value, 4);
					if((_location+ _noOfBytesToProgram)> CFG_PROGRAM_SECTOR_SIZE)/*check for address overflow*/
					{
						DBG_Print("FLASH_WRTIE_TAG: ADDRESS OVERFLOW\r\n");
						_status= ERROR;
					}
					else
					{
						_location+= _alternateSectorStartAddress;

						//DBG_Print("FLASH_WRTIE_TAG: _location: %d, _noOfBytesToProgram: %d\r\n",_location, _noOfBytesToProgram);
						memcpy((uint8_t *)_pulFlashBuffer, _tlv.value+  4, _noOfBytesToProgram);
						_result= CFG_WriteProgram(_location, _pulFlashBuffer, _noOfBytesToProgram);
						if(SUCCESS!= _result)
						{
							_status= ERROR;
						}
						else
						{
							_respTlv.value[0]= _location>> 24;
							_respTlv.value[1]= _location>> 16;
							_respTlv.value[2]= _location>> 8;
							_respTlv.value[3]= _location>> 0;
							_respTlv.len= 4;
						}
					}
				}
				break;
			case FLASH_READ_TAG:
				break;
			case FLASH_SWITCH_TAG:
				if(6!= _tlv.len)/*4 bytes length, 4 bytes checksum*/
				{
					_status= ERROR;
				}
				else
				{
					uint32_t _fwLength;
					uint16_t _fwChecksum;
					uint32_t _calculatedChecksum;

					memcpy(&_fwLength, _tlv.value, 4);
					memcpy(&_fwChecksum, _tlv.value+ 4, 2);
					_calculatedChecksum=  CFG_GetFWUChecksum(0x0000, (uint8_t *)_alternateSectorStartAddress, _fwLength);

					DBG_Print("FLASH_SWITCH_TAG _fwLength: %d _fwChecksum: %d _calculatedChecksum: %d\r\n",
							_fwLength, _fwChecksum, _calculatedChecksum);

					if((0== _fwLength)
						|| (0!= (_fwChecksum^ _calculatedChecksum))
							)
					{
						_status= ERROR;
					}
					else
					{
						config.system.programStartAddress= _alternateSectorStartAddress;
						config.system.programLength= _fwLength;
						config.system.programChecksum= _calculatedChecksum;
						CFG_Store(&config);
						SYS_Request(SOFT_REBOOT_SysRequest);

						_respTlv.value[0]= 0;
						_respTlv.len= 1;
					}
				}
				break;
			case FLASH_EEPROM_SAVE_TAG:
				if(SUCCESS!= CFG_Store(&config))
				{
					_status= ERROR;
				}
				break;
			default:
				_status= ERROR;
				break;
		}

		if(ERROR!= _status)
		{
			_tlvsIndex+= (1+ 1+ _tlv.len);/*increment TLV buffer index*/
			if(0!= _respTlv.len)/*we have response*/
			{
				(_respTLVsBuffer+ _respTlvsIndex)[1]= _respTlv.len;/*copy the len into our response TLV buffer*/
				_respTlvsIndex+= (1+ 1+ _respTlv.len);/*increment resp TLV buffer index*/
			}
		}
		else
		{
			break;/*error detected, soko madei neh - keluar loop */
		}
	}

	*_respTLVsBufferLen= _respTlvsIndex;
	return _status;
}

static ErrorStatus MSG_INFO_READ_ProcessTlvs(uint8_t *_TLVsBuffer, uint16_t _TLVsBufferLen, uint8_t *_respTLVsBuffer, uint16_t *_respTLVsBufferLen)
{
	TLV_Typedef  _tlv;
	TLV_Typedef  _respTlv;
	uint16_t 	 _tlvsIndex= 0;
	uint16_t     _respTlvsIndex= 0;
	ErrorStatus  _status= SUCCESS;
	char 		 *_stringLoc= NULL;

	while(_tlvsIndex< _TLVsBufferLen)
	{
		_tlv.tag= 		(_TLVsBuffer+ _tlvsIndex)[0];
		_tlv.len= 		(_TLVsBuffer+ _tlvsIndex)[1];
		_tlv.value=		(_TLVsBuffer+ _tlvsIndex)+ 2;
		_respTlv.tag= 	 (_respTLVsBuffer+ _respTlvsIndex)[0]= _tlv.tag;
		_respTlv.len= 	 (_respTLVsBuffer+ _respTlvsIndex)[1]= 0;
		_respTlv.value= (_respTLVsBuffer+ _respTlvsIndex)+ 2;

		_stringLoc= NULL;
		switch(_tlv.tag)
		{
			case INFO_FW_VERSION_TAG:
				_stringLoc= config.info.fwVersion;
				break;
			case INFO_HW_VERSION_TAG:
				_stringLoc= config.info.hwVersion;
				break;
			case INFO_CONFIG_VERSION_TAG:
				{
					memcpy(_respTlv.value, (uint8_t *)&(config.system.configVersion), 2);
					_respTlv.len= 2;
				}
				break;
			case INFO_FLASH_BANK_NO_TAG:
				{
#if PARTITION== 2
					_respTlv.value[0]= 1;
#else
					_respTlv.value[0]= 0;
#endif
					_respTlv.len= 1;
				}
				break;
			case INFO_MFC_DATE_TAG:
				_stringLoc= config.info.mfcDate;
				break;
			case INFO_SERIAL_NO_TAG:
				_stringLoc= config.info.serialNo;
				break;
			case INFO_NAME_TAG:
				_stringLoc= config.info.name;
				break;
			case INFO_ADDRESS_TAG:
				_stringLoc= config.info.address;
				break;
			case INFO_NOTE_TAG:
				_stringLoc= config.info.note;
				break;
			case INFO_METER_SERIAL_NO_TAG:
				_stringLoc= config.info.meterSerialNo;
				break;
			case INFO_PULSE_CNTR_PARAM_TAG:
				_respTlv.value[0]= (uint8_t)3;
				memcpy(_respTlv.value+ 1, (uint8_t *)&(config.pulseCntr.rteOffsetPulse), 4);
				memcpy(_respTlv.value+ 5, (uint8_t *)&(config.pulseCntr.rteLastPulse), 4);
				memcpy(_respTlv.value+ 9, (uint8_t *)&(config.pulseCntr.literPerPulse), 4);
				memcpy(_respTlv.value+ 13, (uint8_t *)&(config.sensors.flow.samplingPeriod_s), 4);
				memcpy(_respTlv.value+ 17, (uint8_t *)&(config.sensors.flow.status.backwardPulse), 4);
				_respTlv.len= 21;
				break;
			case INFO_PULSE_CNTR_VALUE_TAG:
				{
					uint32_t _pulseValue= (PULSECNTR_Value());
					memcpy(_respTlv.value, (uint8_t *)&_pulseValue, 4);
					_respTlv.len= 4;
				}
				break;
			case INFO_PULSE_CNTR_RATE_TAG:
				memcpy(_respTlv.value, (uint8_t *)&config.sensors.flow.status.rate, 4);
				memcpy(_respTlv.value+ 4, (uint8_t *)&config.sensors.flow.status.slowRate, 4);
				memcpy(_respTlv.value+ 8, (uint8_t *)&config.sensors.flow.status.maxRate, 4);
				memcpy(_respTlv.value+ 12, (uint8_t *)&config.sensors.flow.status.minRate, 4);
				_respTlv.len= 16;
				break;
			case INFO_PULSE_CNTR_READING_TAG:
				{
					float _readingInLiter = PULSECNTR_ConvertToMeterReading(PULSECNTR_Value());
					memcpy(_respTlv.value, (uint8_t *)&_readingInLiter, 4);
					_respTlv.len= 4;
				}
				break;
			case INFO_RTC_PARAM_TAG:
				UTILI_WaitRTCSync();/*wait for sync*/
				_respTlv.value[0]= UTILI_BCDToBin(LL_RTC_DATE_GetWeekDay(RTC) );//_date.WeekDay;
				_respTlv.value[1]= UTILI_BCDToBin(LL_RTC_DATE_GetDay(RTC) );//_date.Date;
				_respTlv.value[2]= UTILI_BCDToBin(LL_RTC_DATE_GetMonth(RTC) );//_date.Month;
				_respTlv.value[3]= UTILI_BCDToBin(LL_RTC_DATE_GetYear(RTC) );//_date.Year;
				_respTlv.value[4]= UTILI_BCDToBin(LL_RTC_TIME_GetHour(RTC) );//_time.Hours;
				_respTlv.value[5]= UTILI_BCDToBin(LL_RTC_TIME_GetMinute(RTC) );//_time.Minutes;
				_respTlv.value[6]= UTILI_BCDToBin(LL_RTC_TIME_GetSecond(RTC) );//_time.Seconds;
				_respTlv.value[7]= config.system.utc;//_time.Seconds;
				_respTlv.len= 8;
				break;
			default:
				_status= ERROR;
				break;
		}

		if(ERROR!= _status)
		{
			_tlvsIndex+= (1+ 1+ _tlv.len);/*increment TLV buffer index*/
			if(NULL!= _stringLoc)/*there is a string response that need to be copied*/
			{
				_respTlv.len= (uint8_t)strlen(_stringLoc);
				memcpy(_respTlv.value, _stringLoc, _respTlv.len);
			}
			if(0!= _respTlv.len)/*we have response*/
			{
				(_respTLVsBuffer+ _respTlvsIndex)[1]= _respTlv.len;/*copy the len into our response TLV buffer*/
				_respTlvsIndex+= (1+ 1+ _respTlv.len);/*increment resp TLV buffer index*/
			}
		}
		else
		{
			break;/*error detected, soko madei neiiiii*/
		}
	}

	*_respTLVsBufferLen= _respTlvsIndex;
	return _status;
}

static ErrorStatus MSG_INFO_WRITE_ProcessTlvs(uint8_t *_TLVsBuffer, uint16_t _TLVsBufferLen, uint8_t *_respTLVsBuffer, uint16_t *_respTLVsBufferLen)
{
	TLV_Typedef  _tlv;
	TLV_Typedef  _respTlv;
	uint16_t 	 _tlvsIndex= 0;
	uint16_t     _respTlvsIndex= 0;
	ErrorStatus  _status= SUCCESS;
	uint8_t 	 _stringLen;
	char 		 *_stringLoc= NULL;

	while(_tlvsIndex< _TLVsBufferLen)
	{
		_tlv.tag= 		(_TLVsBuffer+ _tlvsIndex)[0];
		_tlv.len= 		(_TLVsBuffer+ _tlvsIndex)[1];
		_tlv.value=		(_TLVsBuffer+ _tlvsIndex)+ 2;
		_respTlv.tag= 	 (_respTLVsBuffer+ _respTlvsIndex)[0]= _tlv.tag;
		_respTlv.len= 	 (_respTLVsBuffer+ _respTlvsIndex)[1]= 0;
		_respTlv.value= (_respTLVsBuffer+ _respTlvsIndex)+ 2;

		_stringLen= 0;
		switch(_tlv.tag)
		{
			case INFO_FW_VERSION_TAG:
				_stringLoc= config.info.fwVersion;
				_stringLen= CFG_FW_VERSION_LEN;
				break;
			case INFO_HW_VERSION_TAG:
				_stringLoc= config.info.hwVersion;
				_stringLen= CFG_HW_VERSION_LEN;
				break;
			case INFO_MFC_DATE_TAG:
				_stringLoc= config.info.mfcDate;
				_stringLen= CFG_MFC_DATE_LEN;
				break;
			case INFO_SERIAL_NO_TAG:
				_stringLoc= config.info.serialNo;
				_stringLen= CFG_SERIAL_NO_LEN;
				break;
			case INFO_NAME_TAG:
				_stringLoc= config.info.name;
				_stringLen= CFG_NAME_LEN;
				break;
			case INFO_ADDRESS_TAG:
				_stringLoc= config.info.address;
				_stringLen= CFG_ADDRESS_LEN;
				break;
			case INFO_NOTE_TAG:
				_stringLoc= config.info.note;
				_stringLen= CFG_NOTE_LEN;
				break;
			case INFO_METER_SERIAL_NO_TAG:
				_stringLoc= config.info.meterSerialNo;
				_stringLen= CFG_SERIAL_NO_LEN;
				break;
			case INFO_PULSE_CNTR_PARAM_TAG:
				if((5== _tlv.len)|| (6== _tlv.len)|| (10== _tlv.len))
				{
					/*quick fix for IPM: we want to ignore setting pulse mode, thus we ignore first byte*/
					//config.pulseCntr.mode= (PULSE_CNTR_Mode)_tlv.value[0];
					memcpy(&config.pulseCntr.literPerPulse, &_tlv.value[1], 4);//config.pulseCntr.pulsePerLiter= MAKELONG(_tlv.value[12], _tlv.value[11], _tlv.value[10], _tlv.value[9]);

					//SENSORS_ReInitPipeSense();
				}
				else
				{
					_status= ERROR;
				}
				break;
			case INFO_PULSE_CNTR_VALUE_TAG:
				if(4== _tlv.len)
				{
					int32_t _pulseValueToSet;
					memcpy(&_pulseValueToSet, &_tlv.value[0], 4);
					_pulseValueToSet-= PULSECNTR_Value();
					config.pulseCntr.rteOffsetPulse+= _pulseValueToSet;
					//SENSORS_ReInitPipeSense();
				}
				else
				{
					_status= ERROR;
				}
				break;
			case INFO_PULSE_CNTR_RATE_TAG:
				if(12== _tlv.len)
				{
					memcpy(&config.sensors.flow.status.slowRate, &_tlv.value[0], 4);
					memcpy(&config.sensors.flow.status.maxRate, &_tlv.value[4], 4);
					memcpy(&config.sensors.flow.status.minRate, &_tlv.value[8], 4);
				}
				else
				{
					_status= ERROR;
				}
				break;
			case INFO_PULSE_CNTR_PIPE_TAG:
				if((20== _tlv.len)|| (24== _tlv.len))
				{
					memcpy(&config.sensors.flow.burstThreshold_m3, &_tlv.value[0], 4);
					memcpy(&config.sensors.flow.burstSamplingInterval_s, &_tlv.value[4], 4);
					memcpy(&config.sensors.flow.noConsumpSamplingInterval_s, &_tlv.value[8], 4);
					memcpy(&config.sensors.flow.leakageThreshold_m3, &_tlv.value[12], 4);
					memcpy(&config.sensors.flow.leakageSamplingInterval_s, &_tlv.value[16], 4);
					if(24== _tlv.len)
					{
						memcpy(&config.sensors.flow.backflowThreshold, &_tlv.value[20], 4);
					}
					//SENSORS_ReInitPipeSense();
				}
				else
				{
					_status= ERROR;
				}
				break;
			case INFO_PULSE_CNTR_READING_TAG:
				if(4== _tlv.len)
				{
					float _readingToSet;
					int32_t _pulseValueToSet;

					memcpy(&_readingToSet, &_tlv.value[0], 4);
					_pulseValueToSet= (int32_t)(_readingToSet/ config.pulseCntr.literPerPulse);
					_pulseValueToSet-= PULSECNTR_Value();
					config.pulseCntr.rteOffsetPulse+= _pulseValueToSet;
					//SENSORS_ReInitPipeSense();
				}
				else
				{
					_status= ERROR;
				}
				break;
			case INFO_RTC_PARAM_TAG:
				if(8== _tlv.len)
				{
					uint32_t _timestamp= SYS_GetTick();

					SYS_DateTime_Update(UTILI_BinToBCD(_tlv.value[0]), /*weekday*/
										UTILI_BinToBCD(_tlv.value[1]), UTILI_BinToBCD(_tlv.value[2]), UTILI_BinToBCD(_tlv.value[3]),/*ddMMyy*/
										UTILI_BinToBCD(_tlv.value[4]), UTILI_BinToBCD(_tlv.value[5]), UTILI_BinToBCD(_tlv.value[6]),/*hhmmss*/
										_tlv.value[7]);/*utc*/

					uint32_t _timestamp2= SYS_GetTick();
					uint32_t _timestamp3= _timestamp2-_timestamp;
					DBG_Print("Time to update RTC: %d ms (%u - %u).\r\n", _timestamp3, _timestamp2, _timestamp);
				}
				else
				{
					_status= ERROR;
				}
				break;
			case INFO_DIAGNOSTIC_TAG:
				if(12== _tlv.len)
				{
					memcpy(&config.diagnostic.rteHardfaultRebootCount, &_tlv.value[0], 2);
					memcpy(&config.diagnostic.rteFailsafeRebootCount, &_tlv.value[2], 2);
					memcpy(&config.diagnostic.rtePVDRebootCount, &_tlv.value[4], 2);
					memcpy(&config.diagnostic.rteBORCount, &_tlv.value[6], 2);
					memcpy(&config.diagnostic.rteShutdownCount, &_tlv.value[8], 2);
					memcpy(&config.diagnostic.rteRebootCount, &_tlv.value[10], 2);
					memcpy(&config.diagnostic.rteVRefDippedCount, &_tlv.value[8], 2);
					memcpy(&config.diagnostic.rteNbModemSelfResetCount, &_tlv.value[10], 2);
				}
				else
				{
					_status= ERROR;
				}
				break;
			case INFO_DIAGNOSTIC_STATUS_TAG:
				_status= ERROR;
				break;
			default:
				_status= ERROR;
				break;
		}

		if(ERROR!= _status)
		{
			_tlvsIndex+= (1+ 1+ _tlv.len);/*increment TLV buffer index*/
			if(0!= _stringLen)/*there is a string that need to be copied*/
			{
				_stringLen= ( _tlv.len> _stringLen)? _stringLen: _tlv.len;/*if len is shorter, use that len*/
				memcpy(_stringLoc, _tlv.value, _stringLen);
				_stringLoc[_stringLen]= '\0';
			}
			if(0!= _respTlv.len)/*we have response*/
			{
				(_respTLVsBuffer+ _respTlvsIndex)[1]= _respTlv.len;/*copy the len into our response TLV buffer*/
				_respTlvsIndex+= (1+ 1+ _respTlv.len);/*increment resp TLV buffer index*/
			}
		}
		else
		{
			break;/*error detected, soko madei neh - keluar loop */
		}
	}

	*_respTLVsBufferLen= _respTlvsIndex;
	return _status;
}

static ErrorStatus MSG_ProcessTlvs(uint8_t *_TLVsBuffer, uint16_t _TLVsBufferLen, uint8_t *_respTLVsBuffer, uint16_t *_respTLVsBufferLen)
{
	TLV_Typedef  _tlv;
	TLV_Typedef  _respTlv;
	uint16_t 	 _tlvsIndex= 0;
	uint16_t     _respTlvsIndex= 0;
	uint16_t	 _subTag;
	ErrorStatus  _status= SUCCESS;
	char 		 *_stringLoc= NULL;

	while(_tlvsIndex< _TLVsBufferLen)
	{
		_tlv.tag= 		(_TLVsBuffer+ _tlvsIndex)[0];
		_tlv.len= 		(_TLVsBuffer+ _tlvsIndex)[1];
		_tlv.value=		(_TLVsBuffer+ _tlvsIndex)+ 2;
		_respTlv.tag= 	(_respTLVsBuffer+ _respTlvsIndex)[0]= _tlv.tag;
		_respTlv.len= 	(_respTLVsBuffer+ _respTlvsIndex)[1]= 0;
		_respTlv.value= (_respTLVsBuffer+ _respTlvsIndex)+ 2;

		/*_subTag comprises of TLV length + subtag, thus we can check length and subtag simultaneously*/
		_subTag= MAKEWORD(_tlv.len, _tlv.value[0]);
		_stringLoc= NULL;

		switch(_tlv.tag)
		{
			case AURA_TAG_SENSOR_VALUES:
				for(int i= 0; i< MAX_Sensor; i++)
				{
					float _value= SENSOR_GetValue(i);
					memcpy(_respTlv.value+ (i* 4), (uint8_t *)&_value, 4);
				}
				_respTlv.len= (4* MAX_Sensor);

				/*temp for board testing*/
				_respTlv.value[_respTlv.len++]= GKCOAP_Register_Completed();//LWM2M_IsRegistered();
				_respTlv.value[_respTlv.len++]= sBC66Link.state;
				memcpy(_respTlv.value+ _respTlv.len, sBC66Link.BC66Version, strlen(sBC66Link.BC66Version));_respTlv.len+= strlen(sBC66Link.BC66Version);
				_respTlv.value[_respTlv.len++]= 0;
				memcpy(_respTlv.value+ _respTlv.len, config.nbiot.imei, strlen(config.nbiot.imei));_respTlv.len+= strlen(config.nbiot.imei);
				_respTlv.value[_respTlv.len++]= 0;
				memcpy(_respTlv.value+ _respTlv.len, config.nbiot.iccid, strlen(config.nbiot.iccid));_respTlv.len+= strlen(config.nbiot.iccid);
				DBG_Print("#stat:msg_: _respTlv.len:%d >\r\n", _respTlv.len);
				break;
			case AURA_TAG_SYS:
				if((0x01== _tlv.len)&& (MAX_SysRequest> _tlv.value[0]))
				{
					SYS_Request(0x00FF& _subTag);
				}
				else
				{
					_status= ERROR;
				}
				break;
 			case AURA_TAG_NBIOT_MODEM:
				config.nbiot.modemMode= FW_UPDATE_ModemMode;
				CFG_Store(&config);
				SYS_Request(SOFT_REBOOT_SysRequest);
				break;
			case AURA_TAG_NBIOT_EXIT_FWU:
				config.nbiot.modemMode= APPLICATION_ModemMode;
				CFG_Store(&config);
				SYS_Request(SOFT_REBOOT_SysRequest);
				break;
			case AURA_TAG_NBIOT_TESTMODE:
				if(7== _tlv.len)
				{
					if(0== _tlv.value[0])/*read*/
					{
						_status= ERROR;
					}
					else if(1== _tlv.value[0])/*write*/
					{
						uint32_t _channel;

						memcpy(&_channel, &_tlv.value[2], 4);
						BC66LINK_TestMode(_tlv.value[1], _channel, _tlv.value[6]);
					}
					else
					{
						_status= ERROR;
					}
				}
				else
				{
					_status= ERROR;
				}
				break;
			case AURA_TAG_CONFIG:
				if((1== _tlv.len)&& (0== _tlv.value[0]))/*read length command*/
				{
					uint16_t _length= sizeof(Config_t);

					memcpy(_respTlv.value+ 0, &_length, 2);
					_respTlv.len= 2;
				}
				else if((5== _tlv.len)&& (1== _tlv.value[0]))/*read command*/
				{
					uint16_t _offset;
					uint16_t _length;

					memcpy(&_offset, &_tlv.value[1], 2);
					memcpy(&_length, &_tlv.value[3], 2);

					memcpy(_respTlv.value+ 0, ((uint8_t *)&config)+ _offset, _length);
					_respTlv.len= _length;
				}
				else if((5< _tlv.len)&& (2== _tlv.value[0]))/*write command*/
				{
					uint16_t _offset;
					uint16_t _length;

					memcpy(&_offset, &_tlv.value[1], 2);
					memcpy(&_length, &_tlv.value[3], 2);

					if((_tlv.len- 5)!= _length)
					{
						_status= ERROR;
						break;
					}

					memcpy(((uint8_t *)&config)+ _offset, _tlv.value+ 5, _length);
					_respTlv.len= _length;
				}
				else
				{
					_status= ERROR;
				}
				break;
			default:
				_status= ERROR;
				break;
		}

		if(ERROR!= _status)
		{
			_tlvsIndex+= (1+ 1+ _tlv.len);/*increment TLV buffer index*/
			if(NULL!= _stringLoc)/*there is a string response that need to be copied*/
			{
				_respTlv.len= (uint8_t)strlen(_stringLoc);
				memcpy(_respTlv.value, _stringLoc, _respTlv.len);
			}
			if(0!= _respTlv.len)/*we have response*/
			{
				(_respTLVsBuffer+ _respTlvsIndex)[1]= _respTlv.len;/*copy the len into our response TLV buffer*/
				_respTlvsIndex+= (1+ 1+ _respTlv.len);/*increment resp TLV buffer index*/
			}
		}
		else
		{
			break;/*error detected, soko madei neiiiii*/
		}
	}

	*_respTLVsBufferLen= _respTlvsIndex;
	return _status;
}

uint8_t MSG_TaskState(void)
{
	if(0!= MSG_Msg_Depth())
	{
		return true;
	}
	return false;
}

#endif
