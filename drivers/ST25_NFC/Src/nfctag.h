/*
 * nfctag.h
 *
 *  Created on: 23 May 2018
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef NFC_NFCTAG_H_
#define NFC_NFCTAG_H_

#include <stdint.h>
#include "st25dv_io.h"
#include "stm32l4xx_hal.h"
#include "main.h"
#include "sys.h"

#define NDEF_DEMO_CC3_COMPLIANT_VALUE   ((uint8_t)0x5)
#define NFCT5_MAGICNUMBER_E1_CCFILE		0xE1  /**<  Complete data area can be read by 1-byte block adrdess commands. */
#define NFCT5_MAGICNUMBER_E2_CCFILE		0xE2  /**<  Last part of the data area can be only read by 2-bytes block address commands. The first 256 blocks can be read by 1-byte block address commands. */
#define NFCT5_EXTENDED_CCFILE           0x00
#define NFCT5_NDEF_MSG_TLV              ((uint8_t) 0x03)
#define NFCT5_TERMINATOR_TLV            ((uint8_t) 0xFE)
#define URI_ID_0x03                 	0x03
#define NFCT5_VERSION_V1_0              0x40

#define NFCTAG_RXTX_BUF_SIZE 256

typedef struct
{
	uint8_t rf_user;
	uint8_t rf_activity;
	uint8_t rf_interrupt;
	uint8_t field_falling;
	uint8_t field_rising;
	uint8_t rf_putMsg;
	uint8_t rf_getMsg;
	uint8_t rf_write;
}NFC_GPOStatus_t;

/* Exported macro ------------------------------------------------------------*/
#define ST25_RETRY_NB     ((uint8_t) 15)
#define ST25_RETRY_DELAY  ((uint8_t) 40)

/**
  * @brief Iterate ST25DV command depending on the command return status.
  * @param cmd A ST25DV function returning a NFCTAG_Status_t status.
  */
#define ST25_RETRY(cmd) do {                                                  \
                          int st25_retry = ST25_RETRY_NB;                     \
                          NFCTAG_Status_t st25_status = NFCTAG_ERROR;    \
                          while(st25_status != NFCTAG_OK)                     \
                          {                                                   \
                            st25_status = cmd;                                \
                            if(st25_status != NFCTAG_OK)                      \
								LL_mDelay(ST25_RETRY_DELAY);                  \
                            if(st25_retry-- <= 0)                             \
                            {                                                 \
                              st25_error(st25_status);                        \
                              st25_retry = ST25_RETRY_NB;                     \
                            }                                                 \
                          }                                                   \
                      } while(0)

ErrorStatus NFCTAG_Init(void);
void NFCTAG_GPOInit( void );
void NFCTAG_PowerPinInit(void);
void NFCTAG_PowerPinSet(uint8_t _state);
uint8_t NFCTAG_PowerPinGet(void);
void NFCTAG_PowerPinSetTimeout(uint16_t _seconds);
bool NFCTAG_PowerPinIsTimeout(void);
void NFCTAG_UpdateGPOStatus(NFC_GPOStatus_t* _gpoStatus);
ErrorStatus NFCTAG_OpenSecuritySession(uint32_t _msb, uint32_t _lsb);
NFCTAG_Status_t NFCTAG_Mailbox_Init(void );
NFCTAG_Status_t NFCTAG_Mailbox_DeInit( void );
NFCTAG_Status_t NFCTAG_Mailbox_Write(const uint8_t * const _pData, const uint16_t _NbBytes );
NFCTAG_Status_t NFCTAG_Mailbox_Read( uint8_t * const _pData, uint16_t * const _pLength );
bool NCTAG_RFFieldIsPresent(void);
bool NFCTAG_GetNFCAccessFlag(void);
void NFCTAG_ClearNFCAccessFlag(void);
bool NFCTAG_GetNFCInvalidAccessFlag(void);
void NFCTAG_ClearNFCInvalidAccessFlag(void);
void NFCTAG_Task(void);
uint8_t NFCTAG_TaskState(void);

#endif /* NFC_NFCTAG_H_ */
