/*
 * st25dv_io.h
 *
 *  Created on: 20 May 2018
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef NFC_ST25DV_ST25DV_IO_H_
#define NFC_ST25DV_ST25DV_IO_H_

#include "stdbool.h"
#include "st25dv.h"
#include "main.h"
#include "utili.h"
#include "sys.h"


NFCTAG_Status_t ST25DV_IO_Init( void );
NFCTAG_Status_t ST25DV_IO_DeInit( void );
NFCTAG_Status_t ST25DV_IO_MemWrite( const uint8_t * const pData, const uint8_t DevAddr, const uint16_t TarAddr, const uint16_t Size );
NFCTAG_Status_t ST25DV_IO_MemRead( uint8_t * const pData, const uint8_t DevAddr, const uint16_t TarAddr, const uint16_t Size );
NFCTAG_Status_t ST25DV_IO_Read( uint8_t * const pData, const uint8_t DevAddr, const uint16_t Size );
uint8_t ST25DV_IO_IsNacked( void );
NFCTAG_Status_t ST25DV_IO_IsDeviceReady( const uint8_t DevAddress, const uint32_t Trials );

extern ITStatus ST25DV_IO_GPOInterrupt;

#endif /* NFC_ST25DV_ST25DV_IO_H_ */
