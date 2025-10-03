/******************************************************************************
 * File:        lptim2.h
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file declares the public interface for registering LPTIM2 interrupt
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

#ifndef INC_LPTIM2_H_
#define INC_LPTIM2_H_

#include "main.h"

void LPTIM2_SetCompareCallback(void *);
void LPTIM2_SetAutoReloadMatchCallback(void *);
void LPTIM2_SetCounterChangedToUpCallback(void *);
void LPTIM2_SetCounterChangedToDownCallback(void *);

#endif /* INC_LPTIM2_H_ */
