/*
 * lptim.c
 *
 *  Created on: 16 Dec 2020
 *      Author: muhammad.ahamd@georgekent.net
 */

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
