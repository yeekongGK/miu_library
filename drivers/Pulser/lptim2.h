/*
 * lptim.h
 *
 *  Created on: 16 Dec 2020
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef INC_LPTIM2_H_
#define INC_LPTIM2_H_

#include "main.h"

void LPTIM2_SetCompareCallback(void *);
void LPTIM2_SetAutoReloadMatchCallback(void *);
void LPTIM2_SetCounterChangedToUpCallback(void *);
void LPTIM2_SetCounterChangedToDownCallback(void *);

#endif /* INC_LPTIM2_H_ */
