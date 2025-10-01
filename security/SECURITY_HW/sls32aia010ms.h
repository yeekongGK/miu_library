/*
 * sls32aia010ms.h
 *
 *  Created on: 19 Jan 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef SECURITY_SLS32AIA010MS_H_
#define SECURITY_SLS32AIA010MS_H_

#include "main.h"

#define SLS32AIA_CFG_I2C_ADDRESS 		(0x30<< 1)/*for this chip we need to shift 1 bit to make a 7-bit address for stm32 register*/
#define SLS32AIA_CFG_I2C_TIMEOUT_MAX	1000
#define SLS32AIA_CFG_STARTUP_TIME_US	15000/*15ms*/
#define SLS32AIA_CFG_RST_LOW_TIME_US	150/*min 10us, max 2.5ms*/

void SLS32AIA_Init(void);

#endif /* SECURITY_SLS32AIA010MS_H_ */
