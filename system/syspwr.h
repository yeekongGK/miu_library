/******************************************************************************
 * File:        syspwr.h
 * Author:      Firmware Team
 * Created:     05-10-2025
 * Last Update: -
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
#if SYSTEM_MODULE_ENABLED == ENABLE_MODULE

#include "main.h"

void SYSPWR_InitGPIO(void);
void SYSPWR_InitPower(void);
void SYSPWR_EnableModem(bool _enable);

#endif // SYSTEM_MODULE_ENABLED
#endif /* SYS_SYSPWR_H_ */
