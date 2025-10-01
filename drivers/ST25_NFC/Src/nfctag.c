/*
 * nfc_tag.c
 *
 *  Created on: 23 May 2018
 *      Author: muhammad.ahmad@georgekent.net
 *
 *
 *      PIC: CYK
 *      TODO:
 *      * replace ioctrl
 *      * replace NFCTAG_GPOInit
 */

//#include "common.h"
#include "main.h"
#include "nfctag.h"
#include "nfctag_ndef.h"
#include "rtc.h"
//#include "ioctrl.h"
#include "msg.h"
#include "security.h"
#include "diag.h"
#include "sys.h"
#include "utili.h"

#define DBG_Print(...)

__IO ITStatus NFCTAG_Interrupt_GPO= RESET;
__IO uint8_t pucNfctagRxBuf[NFCTAG_RXTX_BUF_SIZE];
__IO uint8_t pucNfctagTxBuf[NFCTAG_RXTX_BUF_SIZE];

static bool bNFCAccessFlag= false;
static bool bNFCInvalidAccessFlag= false;

void st25_error(NFCTAG_Status_t status)
{
  if(status > NFCTAG_NACK)
  {
    /* select unknown status */
    status = (NFCTAG_Status_t)5;
  }

  //Menu_MsgStatus(titles[status], "RF may be busy.\nPlease move the reader...\nOr reconnect the board.", MSG_STATUS_ERROR);
  /* I2C peripheral may be in a bad shape... */
  if((status == NFCTAG_BUSY) || (status == NFCTAG_NACK))
  {
	  ST25DV_IO_DeInit();
	  ST25DV_IO_Init();
  }
  /* May have been NACKed because password presentation failed. */
  /* And if user has disconnected the board, password may be required as well. */
  if(status == NFCTAG_NACK)
  {
	  NFCTAG_OpenSecuritySession(((uint32_t *)UID_BASE)[1], ((uint32_t *)UID_BASE)[2]);
  }
}

ErrorStatus NFCTAG_Init(void)
{
	ErrorStatus _initStatus= SUCCESS;
	uint8_t _nfctagId= 0;
	uint8_t _initRetry= 0;

	NFCTAG_PowerPinInit();
	NFCTAG_PowerPinSet(1);
	NFCTAG_GPOInit();

NFCTAG_Init_Retry:
    if(NFCTAG_OK== St25Dv_i2c_Drv.Init())
    {
        St25Dv_i2c_Drv.ReadID(&_nfctagId);
    }

    if(I_AM_ST25DV04!= _nfctagId)
    {
    	_initStatus= ERROR;
    }
    else
    {
		/* change default password
		 * 1. present default factory password, if session is opened(authenticated) write a new password
		 * 2. verify the new password
		 * 3. if verified, present a wrong password to remove authentication and close the session
		 */
		ST25DV_PASSWD _password= {0x0000, 0x0000};//default factory password
		ST25DV_I2CSSO_STATUS _i2csso = ST25DV_SESSION_CLOSED;
		ST25DV_EH_MODE_STATUS _EHMode;

		St25Dv_i2c_ExtDrv.PresentI2CPassword(_password );
		St25Dv_i2c_ExtDrv.ReadI2CSecuritySession_Dyn(&_i2csso );
		if(ST25DV_SESSION_OPEN== _i2csso)
		{
			_password.LsbPasswd= ((uint32_t *)UID_BASE)[2];
			_password.MsbPasswd= ((uint32_t *)UID_BASE)[1];
			St25Dv_i2c_ExtDrv.WriteI2CPassword(_password );
		}

		_password.LsbPasswd= ((uint32_t *)UID_BASE)[2];
		_password.MsbPasswd= ((uint32_t *)UID_BASE)[1];
		St25Dv_i2c_ExtDrv.PresentI2CPassword(_password );
		St25Dv_i2c_ExtDrv.ReadI2CSecuritySession_Dyn(&_i2csso );
		if(ST25DV_SESSION_OPEN== _i2csso)
		{
			if(	NFCTAG_OK==	St25Dv_i2c_Drv.ConfigIT(ST25DV_GPO_ENABLE_MASK| ST25DV_GPO_FIELDCHANGE_MASK| ST25DV_GPO_RFINTERRUPT_MASK| ST25DV_GPO_RFPUTMSG_MASK| ST25DV_GPO_RFGETMSG_MASK)
				&&NFCTAG_OK== St25Dv_i2c_ExtDrv.WriteRFMngt(0)//rf disable=1, enable=0
				&&NFCTAG_OK== St25Dv_i2c_ExtDrv.WriteEHMode(ST25DV_EH_ACTIVE_AFTER_BOOT)
				/*TODO: who set these mail box init, pc or us?*/
				)
			{
				St25Dv_i2c_ExtDrv.ReadEHMode(&_EHMode);
				if(_EHMode== ST25DV_EH_ACTIVE_AFTER_BOOT)
				{
					_initStatus= SUCCESS;
				}
				else
				{
					_initStatus= ERROR;
				}
			}

			/*mailbox config:*/
			if( NFCTAG_OK== St25Dv_i2c_ExtDrv.WriteMBMode(ST25DV_ENABLE)//enable mailbox(TODO: clear content if already enabled)
//				&&NFCTAG_OK== St25Dv_i2c_ExtDrv.ResetEHENMode_Dyn()
//				&&NFCTAG_OK== St25Dv_i2c_ExtDrv.ResetMBEN_Dyn()
//				&&NFCTAG_OK== St25Dv_i2c_ExtDrv.SetMBEN_Dyn()
				&&NFCTAG_OK== St25Dv_i2c_ExtDrv.WriteMBWDG(ST25DV_DISABLE)//disable mailbox watchdog feature
				)
			{
				_initStatus= SUCCESS;
			}
		}
		else
		{
			_initStatus= ERROR;
		}
	}

    if(ERROR!= _initStatus)
    {
#if ENABLE_NFCTAG_NDEF == 1
    	if(NFCTAG_OK!= NFCTAG_NDEF_Init())
#else
    	if(NFCTAG_OK!= NFCTAG_NDEF_Clear())/*remove any ndef*/
#endif
    	{
			_initStatus= ERROR;
    	}
    }

    if(ERROR== _initStatus)
    {
		if(1> _initRetry)
		{
			_initRetry++;
			goto NFCTAG_Init_Retry;
		}

		DBG_Print("#stat:nfc_: INIT error>\r\n");
		DIAG_Code(INIT_FAILED_NFCDCode, 0);
    }

//	if(NCTAG_RFFieldIsPresent())
//	{
//		DBG_Print("#stat:nfc_: RF Field Present, Unlatch power>\r\n");
//		IOCTRL_MainPower_Enable(false);
//	}
//	else
//	{
//		DBG_Print("#stat:nfc_: RF Field NOT Present>\r\n");
//	}

	NFCTAG_PowerPinSet(0);
	return _initStatus;
}

void NFCTAG_GPOInit( void )
{
//	LL_EXTI_InitTypeDef EXTI_InitStruct;
//
//	SYS_EnablePortClock(NFC_BUSY_GPIO_Port);
//
//	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
//	LL_SYSCFG_SetEXTISource(NFC_BUSY_SYSCFG_EXTI_Port, NFC_BUSY_SYSCFG_EXTI_Line);
//
//	LL_GPIO_SetPinPull(NFC_BUSY_GPIO_Port, NFC_BUSY_Pin, LL_GPIO_PULL_NO);
//	LL_GPIO_SetPinMode(NFC_BUSY_GPIO_Port, NFC_BUSY_Pin, LL_GPIO_MODE_INPUT);
//
//	EXTI_InitStruct.Line_0_31= NFC_BUSY_EXTI_Line;
//	EXTI_InitStruct.LineCommand= ENABLE;
//	EXTI_InitStruct.Mode= LL_EXTI_MODE_IT;
//	EXTI_InitStruct.Trigger= LL_EXTI_TRIGGER_FALLING;
//	LL_EXTI_Init(&EXTI_InitStruct);
//
//	NVIC_SetPriority(NFC_BUSY_EXTI_IRQn, SYS_CFG_EXTI_PRIORITY);
//	NVIC_EnableIRQ(NFC_BUSY_EXTI_IRQn);
}

void NFCTAG_PowerPinInit(void)
{
//	IOCTRL_NFCPower_Init(false);/*reset by default*/
}

void NFCTAG_PowerPinSet(uint8_t _state)
{
//	if(0== _state)
//	{
//		IOCTRL_NFCPower_Enable(false);
//	}
//	else
//	{
//		IOCTRL_NFCPower_Enable(true);
//		UTILI_usDelay(600);/*booting time*/
//		//HAL_Delay(10);/*booting time*/
//	}
}

uint8_t NFCTAG_PowerPinGet(void)
{
//	if(true== IOCTRL_NFCPower_IsEnable())
//	{
//		return 1;
//	}
//	else
//	{
//		return 0;
//	}
}

ErrorStatus NFCTAG_GetGPOStatus(NFC_GPOStatus_t* _gpoStatus)
{
	uint8_t _itStatus= 0;
	if(NFCTAG_OK!= St25Dv_i2c_ExtDrv.ReadITSTStatus_Dyn(&_itStatus ))
	{
		return ERROR;
	}

	_gpoStatus->rf_user= (_itStatus& ST25DV_ITSTS_RFUSERSTATE_MASK)>> ST25DV_ITSTS_RFUSERSTATE_SHIFT;
	_gpoStatus->rf_activity= (_itStatus& ST25DV_ITSTS_RFACTIVITY_MASK)>> ST25DV_ITSTS_RFACTIVITY_SHIFT;
	_gpoStatus->rf_interrupt= (_itStatus& ST25DV_ITSTS_RFINTERRUPT_MASK)>> ST25DV_ITSTS_RFINTERRUPT_SHIFT;
	_gpoStatus->field_falling= (_itStatus& ST25DV_ITSTS_FIELDFALLING_MASK)>> ST25DV_ITSTS_FIELDFALLING_SHIFT;
	_gpoStatus->field_rising= (_itStatus& ST25DV_ITSTS_FIELDRISING_MASK)>> ST25DV_ITSTS_FIELDRISING_SHIFT;
	_gpoStatus->rf_putMsg= (_itStatus& ST25DV_ITSTS_RFPUTMSG_MASK)>> ST25DV_ITSTS_RFPUTMSG_SHIFT;
	_gpoStatus->rf_getMsg= (_itStatus& ST25DV_ITSTS_RFGETMSG_MASK)>> ST25DV_ITSTS_RFGETMSG_SHIFT;
	_gpoStatus->rf_write= (_itStatus& ST25DV_ITSTS_RFWRITE_MASK)>> ST25DV_ITSTS_RFWRITE_SHIFT;

	return SUCCESS;
}

ErrorStatus NFCTAG_OpenSecuritySession(uint32_t _msb, uint32_t _lsb)
{
	ST25DV_PASSWD _password= {_msb, _lsb};
	ST25DV_I2CSSO_STATUS _i2csso = ST25DV_SESSION_CLOSED;

	if((NFCTAG_OK== St25Dv_i2c_ExtDrv.PresentI2CPassword(_password ))
		&&(NFCTAG_OK== St25Dv_i2c_ExtDrv.ReadI2CSecuritySession_Dyn(&_i2csso )))
	{
		if(ST25DV_SESSION_OPEN== _i2csso)
		{
			return SUCCESS;
		}
	}

	return ERROR;
}

NFCTAG_Status_t NFCTAG_Mailbox_Init(void )
{
  NFCTAG_Status_t _status= NFCTAG_OK;
  ST25DV_EN_STATUS _mbMode= ST25DV_DISABLE;

  St25Dv_i2c_ExtDrv.ReadMBMode(&_mbMode);
  if(ST25DV_DISABLE== _mbMode)
  {
	  _status= St25Dv_i2c_ExtDrv.WriteMBMode(ST25DV_ENABLE );
  }
  else
  {
    /* if already activated Clear MB content and flag */
	  _status= St25Dv_i2c_ExtDrv.ResetMBEN_Dyn();
	  _status|= St25Dv_i2c_ExtDrv.SetMBEN_Dyn();
  }

  /* Disable MB watchdog feature */
  _status|= St25Dv_i2c_ExtDrv.WriteMBWDG(0);

  return _status;
}

NFCTAG_Status_t NFCTAG_Mailbox_DeInit( void )
{
  return St25Dv_i2c_ExtDrv.ResetMBEN_Dyn( );
}

NFCTAG_Status_t NFCTAG_Mailbox_Write(const uint8_t * const _pData, const uint16_t _NbBytes )
{
  NFCTAG_Status_t _status = NFCTAG_OK;
  ST25DV_MB_CTRL_DYN_STATUS _data = {0};

  /* Check if Mailbox is available */
  _status= St25Dv_i2c_ExtDrv.ReadMBctrl_Dyn(&_data );
  if(NFCTAG_OK!= _status)
  {
	LL_mDelay(20);
    return _status;
  }

  /* If available, write data */
  if((0== _data.HostPutMsg)&& (0== _data.RfPutMsg))
  {
	  _status= St25Dv_i2c_ExtDrv.WriteMailboxData( _pData, _NbBytes );
  }
  else
  {
	LL_mDelay(20);
    return NFCTAG_BUSY;
  }

  return _status;
}

NFCTAG_Status_t NFCTAG_Mailbox_Read( uint8_t * const _pData, uint16_t * const _pLength )
{
  NFCTAG_Status_t _status = NFCTAG_OK;
  uint16_t _mbLength = 0;

  /* Read length of message */
  _status = St25Dv_i2c_ExtDrv.ReadMBLength_Dyn((uint8_t *)&_mbLength);
  if(NFCTAG_OK!= _status)
  {
    return _status;
  }
  *_pLength= _mbLength + 1;

  /* Read all data in Mailbox */
  return St25Dv_i2c_ExtDrv.ReadMailboxData(_pData, 0, *_pLength);
}

void NFCTAG_PowerPinSetTimeout(uint16_t _seconds)
{
	SYS_Sleep(NFC_TaskId, _seconds* 1000);
}

bool NFCTAG_PowerPinIsTimeout(void)
{
	return SYS_IsAwake(NFC_TaskId);
}

bool NCTAG_RFFieldIsPresent(void)
{
	ST25DV_FIELD_STATUS _status= {0};
	NFCTAG_PowerPinSet(1);
	if(NFCTAG_OK== St25Dv_i2c_ExtDrv.GetRFField_Dyn(&_status))
	{
		if(ST25DV_FIELD_ON== _status)
		{
			NFCTAG_PowerPinSet(0);
			return true;
		}
	}
	DBG_Print("#stat:nfc_:NFCTAG_ERROR>\r\n");
	NFCTAG_PowerPinSet(0);
	return false;
}

bool NFCTAG_GetNFCAccessFlag(void)
{
	return bNFCAccessFlag;
}

void NFCTAG_ClearNFCAccessFlag(void)
{
	bNFCAccessFlag= false;
}

bool NFCTAG_GetNFCInvalidAccessFlag(void)
{
	return bNFCInvalidAccessFlag;
}

void NFCTAG_ClearNFCInvalidAccessFlag(void)
{
	bNFCInvalidAccessFlag= false;
}

void NFCTAG_Task(void)
{
	static uint64_t _timestamp;
	static uint8_t _failedCounter= 0;

	if((0!= MSG_Msg_Depth())&& (NFC_TaskId== MSG_Msg_TaskId_Peek()))
	{
		NFCTAG_PowerPinSetTimeout(5);
		NFCTAG_PowerPinSet(1);
		//if(NFCTAG_OK== St25Dv_i2c_ExtDrv.SetMBEN_Dyn()
		//	)
		{
			/*Packet format:
			* |msgLen(1-byte)|msg(n-byte)|checksum(2-byte)|compulsoryRandom(4-byte)|paddedRandom(m-byte)|
			*
			* Packet is in multiplication of 16 bytes for AES encryption.
			* paddedRandom field is padded with random number to make multiplication of 16bytes packet.
			* msgLen field describes the length of real msg only(without padding/random/checksum)
			* checksum field correspond to checksum from msgLen to msg fields only
			* so max useful msg bytes is 249 bytes
			* */
			MSG_t _msg= MSG_Msg_Dequeue();
			if(MSG_CFG_MAX_BUFFER_SIZE>= _msg.bufferLen)
			{
				uint8_t _txLen= 0;
				pucNfctagTxBuf[_txLen++]= _msg.bufferLen;
				memcpy((uint8_t*)&(pucNfctagTxBuf[1]), _msg.buffer, _msg.bufferLen);
				_txLen+= _msg.bufferLen;
				uint16_t _checksum= UTILI_GetChecksum(0x0000, (uint8_t*)pucNfctagTxBuf, _txLen);
				pucNfctagTxBuf[_txLen++]= _checksum>> 8;
				pucNfctagTxBuf[_txLen++]= _checksum>> 0;
				pucNfctagTxBuf[_txLen++]= (uint8_t)rand();
				pucNfctagTxBuf[_txLen++]= (uint8_t)rand();
				pucNfctagTxBuf[_txLen++]= (uint8_t)rand();
				pucNfctagTxBuf[_txLen++]= (uint8_t)rand();
				while(0!= (_txLen% 16))
				{
					pucNfctagTxBuf[_txLen++]= (uint8_t)rand();
				}

				SECURE_ECB_Encrypt((uint8_t*)pucNfctagTxBuf, _txLen);
				NFCTAG_Mailbox_Write((uint8_t*)pucNfctagTxBuf, _txLen);
				bNFCAccessFlag= true;


				//DBG_Print("NFCTag TX (%d ms)\r\n", SYS_GetTick_ms()- _timestamp);
				_timestamp= SYS_GetTimestamp_ms();
			}

//			MSG_t _msg= MSG_Msg_Dequeue();
//			uint16_t _bufferLen= _msg.bufferLen;
//			uint16_t _checksum= UTILI_GetChecksum(0x0000, _msg.buffer, _bufferLen);
//			_msg.buffer[_bufferLen++]= _checksum>> 8;
//			_msg.buffer[_bufferLen++]= _checksum>> 0;
//
//			_bufferLen+= (16- (_bufferLen% 16));/*pad the buffer*/
//
//			SECURE_ECB_Encrypt((uint8_t *)_msg.buffer, _bufferLen);
//			NFCTAG_Mailbox_Write(_msg.buffer, _bufferLen);
		}

		bNFCInvalidAccessFlag= (true== bNFCAccessFlag)? false: true;
	}

	/*Power pin timeout need to be placed after NFC tx because
	 * message task might update the RTC and this prematurely timeout.*/
	if(true== NFCTAG_PowerPinIsTimeout())
	{
		if(1== NFCTAG_PowerPinGet())
		{
			NFCTAG_PowerPinSet(0);
			uint64_t _timestamp2= SYS_GetTimestamp_ms();
			uint64_t _timestamp3= _timestamp2-_timestamp;
			//DBG_Print("NFCTag power off after %d ms \r\n", (uint32_t)_timestamp3);
		}
	}

	if(RESET== NFCTAG_Interrupt_GPO)
	{
		return;
	}
	//DBG_Print("NFCTag NFCTAG_Interrupt_GPO \r\n");

	NFCTAG_Interrupt_GPO= RESET;
	NFCTAG_PowerPinSetTimeout(5);
	NFCTAG_PowerPinSet(1);

	NFC_GPOStatus_t _gpoStatus= {0};

	if(SUCCESS!= NFCTAG_GetGPOStatus(&_gpoStatus))
	{
		_failedCounter++;
		if(2== _failedCounter)
		{
			/*nfc i2c comm might be error, we do reinit and try again*/
//			St25Dv_i2c_Drv.Init();//NFCTAG_Init();
//			if(SUCCESS!= NFCTAG_GetGPOStatus(&_gpoStatus))
			{
					/*TODO: flag nfc error*/
					/*nfc i2c comm might be error, we do reinit and try again*/
					NFCTAG_PowerPinSet(0);
					UTILI_usDelay(600);/*shutdown time, remove if not required*/
					NFCTAG_Init();
					_failedCounter= 0;
					if(SUCCESS!= NFCTAG_GetGPOStatus(&_gpoStatus))
					{
						/*TODO: flag nfc error*/
					}
					return;
			}
		}
		else
		{
//			St25Dv_i2c_Drv.Init();
//			if(SUCCESS!= NFCTAG_GetGPOStatus(&_gpoStatus))
//			{
//				NFCTAG_PowerPinSet(0);
				return;
//			}
		}
	}
	else
	{
		_failedCounter= 0;
	}

	if(1== _gpoStatus.rf_interrupt)
	{
		//DBG_Print("NFCTag rf_interrupt \r\n");
		NFCTAG_PowerPinSetTimeout(5);
	}

	if(1== _gpoStatus.rf_putMsg) //msg arrived in mailbox || msg placed in mailbox has been read
	{
		//DBG_Print("NFCTag rf_putMsg \r\n");
		/*Packet format:
		* |msgLen(1-byte)|msg(n-byte)|checksum(2-byte)|compulsoryRandom(4-byte)|paddedRandom(m-byte)|
		*
		* Packet is in multiplication of 16 bytes for AES encryption.
		* paddedRandom field is padded with random number to make multiplication of 16bytes packet.
		* msgLen field describes the length of real msg only(without padding/random/checksum)
		* checksum field correspond to checksum from msgLen to msg fields only
		* so max useful msg bytes is 249 bytes
		* */
		uint16_t _mailLen;
		NFCTAG_Mailbox_Read((uint8_t *)pucNfctagRxBuf, &_mailLen);
		SECURE_ECB_Decrypt((uint8_t *)pucNfctagRxBuf, _mailLen);
		uint8_t _msgLen= pucNfctagRxBuf[0];

		if(MSG_CFG_MAX_BUFFER_SIZE>= _msgLen)
		{
			uint16_t _checksum= MAKEWORD(pucNfctagRxBuf[1+ _msgLen], pucNfctagRxBuf[1+ _msgLen+ 1]);
			if(0x00== (_checksum^ UTILI_GetChecksum(0x0000,  (uint8_t *)pucNfctagRxBuf, 1+ _msgLen)))
			{
				MSG_t _msg= {0};
				_msg.taskId= MSG_TaskId;
				_msg.buffer= (uint8_t *)&pucNfctagRxBuf[1];
				_msg.bufferLen= pucNfctagRxBuf[0];
				_msg.sendResponse= true;
				_msg.responseTaskId= NFC_TaskId;
				MSG_Msg_Enqueue(_msg);

				//DBG_Print("NFCTag RX (%d ms)\r\n", SYS_GetTick_ms()- _timestamp);
				_timestamp= SYS_GetTimestamp_ms();
			}
		}

//		uint16_t _bufferLen;
//
//		NFCTAG_Mailbox_Read((uint8_t *)pucNfctagRxTxBuf, &_bufferLen);
//		SECURE_ECB_Decrypt((uint8_t *)pucNfctagRxTxBuf, _bufferLen);
//		_bufferLen= 1+ 1+ pucNfctagRxTxBuf[1];/*resize according to TLV len*/
//
//		if(0x00== (MAKEWORD(pucNfctagRxTxBuf[_bufferLen], pucNfctagRxTxBuf[_bufferLen+ 1])^ UTILI_GetChecksum(0x0000,  (uint8_t *)pucNfctagRxTxBuf, _bufferLen)))
//		{
//			MSG_t _msg= {0};
//			_msg.taskId= MSG_TaskId;
//			_msg.buffer= (uint8_t *)pucNfctagRxTxBuf;
//			_msg.bufferLen= 1+ 1+ pucNfctagRxTxBuf[1];/*resize according to TLV len*/
//			_msg.sendResponse= true;
//			_msg.responseTaskId= NFC_TaskId;
//			MSG_Msg_Enqueue(_msg);
//		}
	}

	if(1== _gpoStatus.rf_getMsg)
	{
		//DBG_Print("NFCTag rf_getMsg \r\n");
	}

	if(1== _gpoStatus.field_rising)//temp
	{
		//DBG_Print("NFCTag field_rising \r\n");
#if ENABLE_NFCTAG_NDEF == 1
		if(false== NFCTAG_NDEF_IsLocked())
		{
			NFCTAG_NDEF_Init();
			NFCTAG_NDEF_writeURI();
			NFCTAG_NDEF_SetLock(5);/*seconds*/
		}
#endif
	}

	if(1== _gpoStatus.field_falling)
	{
		//DBG_Print("NFCTag field_falling \r\n");
	}
  }

uint8_t NFCTAG_TaskState(void)
{
	return false;
}
