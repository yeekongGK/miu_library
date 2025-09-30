/*
 * syspwr.h
 *
 *  Created on: 5 Feb 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef SYS_SYSPWR_H_
#define SYS_SYSPWR_H_

#include "main.h"

void SYSPWR_InitGPIO(void);
void SYSPWR_InitPower(void);
void SYSPWR_EnableModem(bool _enable);

#endif /* SYS_SYSPWR_H_ */
