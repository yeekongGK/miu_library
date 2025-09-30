/*
 * sysclk.h
 *
 *  Created on: 5 Feb 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef SYSTEM_SYSCLK_H_
#define SYSTEM_SYSCLK_H_

#include "main.h"

typedef enum
{
	SYSCLK_MSI,
	SYSCLK_MSI_PLL,
	SYSCLK_HSI16,
	SYSCLKK_HSI16_PLL,
}SYSCLK_t;

extern void (*SYSCLK_InitWakeupClock)(void);

void SYSCLK_InitSystemClock(uint32_t _frequency);
uint64_t SYSCLK_GetTimestamp_ms(void);
uint32_t SYSCLK_GetTimestamp_s(void);
void SYSCLK_SyncTick(void);

#endif /* SYSTEM_SYSCLK_H_ */
