/******************************************************************************
 * File:        lptim1.c
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file implements the callback handler registration for the LPTIM1
 *   peripheral. It allows other modules to register custom callback functions
 *   for various LPTIM1 events, such as compare match, auto-reload match, and
 *   counter direction changes. This provides a flexible way to handle
 *   LPTIM1 interrupts without modifying the main interrupt vector.
 *
 * Notes:
 *   - The actual LPTIM1 interrupt handler must call these function pointers.
 *
 * To Do:
 *   - -
 *
 ******************************************************************************/
#include "common.h"
#include "lptim1.h"

void LPTIM1_NoCallback(void);

void (*LPTIM1_CompareCallback)(void)= LPTIM1_NoCallback;
void (*LPTIM1_AutoReloadMatchCallback)(void)= LPTIM1_NoCallback;
void (*LPTIM1_CounterChangedToUpCallback)(void)= LPTIM1_NoCallback;
void (*LPTIM1_CounterChangedToDownCallback)(void)= LPTIM1_NoCallback;

void LPTIM1_NoCallback(void)
{

}

void LPTIM1_SetCompareCallback(void *_callback)
{
	LPTIM1_CompareCallback= _callback;
}

void LPTIM1_SetAutoReloadMatchCallback(void *_callback)
{
	LPTIM1_AutoReloadMatchCallback= _callback;
}

void LPTIM1_SetCounterChangedToUpCallback(void *_callback)
{
	LPTIM1_CounterChangedToUpCallback= _callback;
}

void LPTIM1_SetCounterChangedToDownCallback(void *_callback)
{
	LPTIM1_CounterChangedToDownCallback= _callback;
}
