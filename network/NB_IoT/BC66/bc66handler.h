/*
 * bc66handler.h
 *
 *  Created on: 6 Feb 2021
 *      Author: muhgi_000
 */

#ifndef NBIOT_BC66HANDLER_H_
#define NBIOT_BC66HANDLER_H_

#include "main.h"
#include "bc66link.h"

void BC66HANDLER_URC(BC66LINK_t *_sBC66Link);
void BC66HANDLER_Reset(BC66LINK_t *_sBC66Link);
bool BC66HANDLER_GetUnintentionalResetFlag(void);
void BC66HANDLER_ClearUnintentionalResetFlag(void);

#endif /* NBIOT_BC66HANDLER_H_ */
