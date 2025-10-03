/******************************************************************************
 * File:        sls32aia010ms.h
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file defines the public interface and configuration constants for the
 *   SLS32AIA010MS hardware security element driver. It includes definitions
 *   for the I2C address, communication timeout, and timing parameters for
 *   reset and startup sequences. It also declares the initialization function
 *   for the driver.
 *
 * Notes:
 *   - -
 *
 * To Do:
 *   - -
 *
 ******************************************************************************/

#ifndef SECURITY_SLS32AIA010MS_H_
#define SECURITY_SLS32AIA010MS_H_

#include "main.h"

#define SLS32AIA_CFG_I2C_ADDRESS 		(0x30<< 1)/*for this chip we need to shift 1 bit to make a 7-bit address for stm32 register*/
#define SLS32AIA_CFG_I2C_TIMEOUT_MAX	1000
#define SLS32AIA_CFG_STARTUP_TIME_US	15000/*15ms*/
#define SLS32AIA_CFG_RST_LOW_TIME_US	150/*min 10us, max 2.5ms*/

void SLS32AIA_Init(void);

#endif /* SECURITY_SLS32AIA010MS_H_ */
