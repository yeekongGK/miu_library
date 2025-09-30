/*
 * lptim1.c
 *
 *  Created on: 25 Jan 2021
 *      Author: muhammad.ahmad@georgekent.net
 */
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
