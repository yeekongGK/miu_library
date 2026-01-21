/******************************************************************************
 * File:        lptim1.h
 * Author:      Firmware Team
 * Created:     05-10-2025
 * Last Update: -
 *
 * Description:
 *   This file declares the public interface for registering LPTIM1 interrupt
 *   callbacks. It provides function prototypes that allow other modules to
 *   set custom handlers for compare match, auto-reload match, and counter
 *   direction change events.
 *
 * Notes:
 *   - -
 *
 * To Do:
 *   - -
 *
 ******************************************************************************/

#ifndef PULSECNTR_LPTIM1_H_
#define PULSECNTR_LPTIM1_H_
#if DRIVERS_MODULE_ENABLED == ENABLE_MODULE

#include "main.h"

void LPTIM1_SetCompareCallback(void *);
void LPTIM1_SetAutoReloadMatchCallback(void *);
void LPTIM1_SetCounterChangedToUpCallback(void *);
void LPTIM1_SetCounterChangedToDownCallback(void *);

#endif // DRIVERS_MODULE_ENABLED
#endif /* PULSECNTR_LPTIM1_H_ */
