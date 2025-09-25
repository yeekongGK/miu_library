/*
 * st25dv_io.c
 *
 *  Created on: 20 May 2018
 *      Author: muhammad.ahmad@georgekent.net
 */
#include <i2c1.h>
#include "st25dv.h"
#include "st25dv_io.h"

NFCTAG_Status_t ST25DV_IO_ConvertHALStatus( const HAL_StatusTypeDef status )
{
  switch( status )
  {
    case HAL_OK:
      return NFCTAG_OK;
    case HAL_ERROR:
      return NFCTAG_ERROR;
    case HAL_BUSY:
      return NFCTAG_BUSY;
    case HAL_TIMEOUT:
      return NFCTAG_TIMEOUT;

    default:
      return NFCTAG_TIMEOUT;
  }
}

NFCTAG_Status_t ST25DV_IO_Init( void )
{
	static bool _isInitialized= false;

	if(true== _isInitialized)/*could be a re-init*/
	{
		I2C1_DeInit();
		_isInitialized= false;
	}

	if(HAL_OK!= I2C1_Init())
	{
		return NFCTAG_ERROR;
	}

	_isInitialized= true;
	return NFCTAG_OK;
}

NFCTAG_Status_t ST25DV_IO_DeInit( void )
{
	if(HAL_OK!= I2C1_DeInit())
	{
		return NFCTAG_ERROR;
	}

	return NFCTAG_OK;
}

NFCTAG_Status_t ST25DV_IO_MemWrite( const uint8_t * const pData, const uint8_t DevAddr, const uint16_t TarAddr, const uint16_t Size )
{
  NFCTAG_Status_t _pollstatus;
  NFCTAG_Status_t _ret= NFCTAG_OK;
  uint32_t _tickstart;

  if(HAL_OK== I2C1_MemWrite(DevAddr, TarAddr, I2C_MEMADD_SIZE_16BIT, (uint8_t *)pData, Size, ST25DV_I2C_TIMEOUT ))
  {
    /* Poll until EEPROM is available */
    _tickstart= HAL_GetTick();
    /* Wait until ST25DV is ready or timeout occurs */
    do
    {
      _pollstatus= ST25DV_IO_IsDeviceReady(DevAddr, 1 );
    } while(((HAL_GetTick()- _tickstart)< ST25DV_I2C_TIMEOUT) && (_pollstatus!= NFCTAG_OK) );

    if(NFCTAG_OK!= _pollstatus )
    {
      _ret= NFCTAG_TIMEOUT;
    }
  }
  else
  {
    /* Check if Write was NACK */
    if(true== I2C1_IsNacked())
    {
      _ret= NFCTAG_NACK;
    }
  }

  return _ret;
}

NFCTAG_Status_t ST25DV_IO_MemRead(uint8_t * const pData, const uint8_t DevAddr, const uint16_t TarAddr, const uint16_t Size )
{
	return ST25DV_IO_ConvertHALStatus(I2C1_MemRead(DevAddr, TarAddr, I2C_MEMADD_SIZE_16BIT, pData, Size, ST25DV_I2C_TIMEOUT ));
}

NFCTAG_Status_t ST25DV_IO_Read(uint8_t * const pData, const uint8_t DevAddr, const uint16_t Size )
{
  return ST25DV_IO_ConvertHALStatus(I2C1_Read(DevAddr, pData, Size, ST25DV_I2C_TIMEOUT));
}

NFCTAG_Status_t ST25DV_IO_IsDeviceReady( const uint8_t DevAddr, const uint32_t Trials )
{
  return  ST25DV_IO_ConvertHALStatus(I2C1_IsDeviceReady(DevAddr, Trials, ST25DV_I2C_TIMEOUT ));
}
