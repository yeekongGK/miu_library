/******************************************************************************
 * File:        lptim2.c
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file implements the callback handler registration for the LPTIM2
 *   peripheral. It allows other modules to register custom callback functions
 *   for various LPTIM2 events, such as compare match, auto-reload match, and
 *   counter direction changes. This provides a flexible way to handle
 *   LPTIM2 interrupts without modifying the main interrupt vector.
 *
 * Notes:
 *   - The actual LPTIM2 interrupt handler must call these function pointers.
 *
 * To Do:
 *   - -
 *
 ******************************************************************************/

#include "common.h"
#include "lptim2.h"

void LPTIM2_NoCallback(void);

void (*LPTIM2_CompareCallback)(void)= LPTIM2_NoCallback;
void (*LPTIM2_AutoReloadMatchCallback)(void)= LPTIM2_NoCallback;
void (*LPTIM2_CounterChangedToUpCallback)(void)= LPTIM2_NoCallback;
void (*LPTIM2_CounterChangedToDownCallback)(void)= LPTIM2_NoCallback;

void LPTIM2_NoCallback(void)
{

}

void LPTIM2_SetCompareCallback(void *_callback)
{
	LPTIM2_CompareCallback= _callback;
}

void LPTIM2_SetAutoReloadMatchCallback(void *_callback)
{
	LPTIM2_AutoReloadMatchCallback= _callback;
}

void LPTIM2_SetCounterChangedToUpCallback(void *_callback)
{
	LPTIM2_CounterChangedToUpCallback= _callback;
}

void LPTIM2_SetCounterChangedToDownCallback(void *_callback)
{
	LPTIM2_CounterChangedToDownCallback= _callback;
}
