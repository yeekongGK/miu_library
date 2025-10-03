/******************************************************************************
 * File:        security.h
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file defines the public interface for the high-level security module.
 *   It declares function prototypes for initializing the security system,
 *   managing cryptographic keys, and performing AES encryption and decryption
 *   in various modes (ECB, CBC, CTR).
 *
 * Notes:
 *   - -
 *
 * To Do:
 *   - -
 *
 ******************************************************************************/

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
