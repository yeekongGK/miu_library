/*
 * lptim1.h
 *
 *  Created on: 25 Jan 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef PULSECNTR_LPTIM1_H_
#define PULSECNTR_LPTIM1_H_

#include "main.h"

void LPTIM1_SetCompareCallback(void *);
void LPTIM1_SetAutoReloadMatchCallback(void *);
void LPTIM1_SetCounterChangedToUpCallback(void *);
void LPTIM1_SetCounterChangedToDownCallback(void *);

#endif /* PULSECNTR_LPTIM1_H_ */
