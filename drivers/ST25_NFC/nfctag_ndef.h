/*
 * nfctag_ndef.h
 *
 *  Created on: 26 Mar 2019
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef NFC_ST25DV_NFCTAG_NDEF_H_
#define NFC_ST25DV_NFCTAG_NDEF_H_

#include <stdint.h>
#include "st25dv_io.h"
#include "stm32l4xx_hal.h"

NFCTAG_Status_t NFCTAG_NDEF_Init(void);
NFCTAG_Status_t NFCTAG_NDEF_Clear(void);
NFCTAG_Status_t NFCTAG_NDEF_writeURI(void);
void NFCTAG_NDEF_SetLock(uint16_t _seconds);
bool NFCTAG_NDEF_IsLocked(void);

#endif /* NFC_ST25DV_NFCTAG_NDEF_H_ */
