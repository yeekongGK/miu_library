/******************************************************************************
 * File:        syspwr.h
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file defines the public interface for the system power management
 *   module. It provides function prototypes for initializing GPIOs to a
 *   low-power state, managing the initial power-up sequence, and controlling
 *   the power supply to the modem.
 *
 * Notes:
 *   - -
 *
 * To Do:
 *   - -
 *
 ******************************************************************************/

#ifndef SYS_SYSPWR_H_
#define SYS_SYSPWR_H_

#include "main.h"

void SYSPWR_InitGPIO(void);
void SYSPWR_InitPower(void);
void SYSPWR_EnableModem(bool _enable);

#endif /* SYS_SYSPWR_H_ */
