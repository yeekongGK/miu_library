/******************************************************************************
 * File:        sysclk.h
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file defines the public interface for the system clock management
 *   module. It includes an enumeration for different clock sources and
 *   declares the function prototypes for initializing the system clock and
 *   retrieving timestamps.
 *
 * Notes:
 *   - -
 *
 * To Do:
 *   - -
 *
 ******************************************************************************/

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
