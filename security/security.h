/*
 * security.h
 *
 *  Created on: 11 Dec 2018
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef SECURE_SECURITY_H_
#define SECURE_SECURITY_H_

#include "main.h"

void SECURE_Init(void);
void SECURE_KeyChange(uint8_t _keyNo, uint8_t* _newKey, uint16_t _newKeySize);
void SECURE_ECB_Encrypt(uint8_t* _buffer, uint16_t _bufferLen);
void SECURE_ECB_Decrypt(uint8_t *_buffer, uint32_t _bufferLen);
void SECURE_CBC_Encrypt(uint8_t *_iv, uint8_t *_buffer, uint32_t _bufferLen);
void SECURE_CBC_Decrypt(uint8_t *_iv, uint8_t *_buffer, uint32_t _bufferLen);
void SECURE_CTR_Transcrypt(uint8_t _keyNo, uint8_t *_iv, uint8_t *_buffer, uint32_t _bufferLen);

#endif /* SECURE_SECURITY_H_ */
