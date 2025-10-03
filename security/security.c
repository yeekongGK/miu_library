/******************************************************************************
 * File:        security.c
 * Author:      Firmware Team
 * Created:     03-10-2025
 * Last Update: -
 *
 * Description:
 *   This file provides a high-level security wrapper for cryptographic
 *   operations. It abstracts underlying AES and ECC (Elliptic Curve
 *   Cryptography) libraries to offer simplified functions for encryption,
 *   decryption, and key management. It supports AES in ECB, CBC, and CTR modes,
 *   and includes functionality for generating ECDH key pairs.
 *
 * Notes:
 *   - The module manages multiple cryptographic keys, which can be switched at
 *     runtime.
 *   - It relies on the `aes.c` and `uECC.c` libraries for the core
 *     cryptographic algorithms.
 *   - AES-GCM and ECDH key derivation functionalities are commented out.
 *
 * To Do:
 *   - Review and potentially enable or remove the commented-out AES-GCM and
 *     ECDH code.
 *
 ******************************************************************************/

//#include "common.h"
#include "cfg.h"
#include "security.h"
#include "aes.h"
#include "uECC.h"

__IO static uint8_t pucV[4][16];

struct AES_ctx ctx;
static uint8_t bSecureElemInitialized= 0;
static uint8_t ucCurrKeyNo= 0;

void SECURE_Init(void)
{
	SECURE_KeyChange(0, config.system.key.master, 16);
	SECURE_KeyChange(1, config.system.key.cfg.user, 16);
	SECURE_KeyChange(2, config.system.key.opr.a, 16);
	SECURE_KeyChange(3, config.system.key.opr.b, 16);
}

void SECURE_KeyChange(uint8_t _keyNo, uint8_t* _newKey, uint16_t _newKeySize)
{
	memcpy(&(pucV[_keyNo]), _newKey, _newKeySize);
	//uint8_t _hexstring[256];
	//DBG_Print("SECURE_KeyChange _keyNo:%d _newKey:%s.\r\n", _keyNo, UTILI_BytesToHexString(_newKey, _newKeySize, _hexstring));
}

void SECURE_KeyInit(uint8_t _keyNo)
{
	if((ucCurrKeyNo!= _keyNo)|| (0== bSecureElemInitialized))
	{
		ucCurrKeyNo= _keyNo;
	    AES_init_ctx(&ctx, pucV[_keyNo]);
	    bSecureElemInitialized= 1;

		//DBG_Print("SECURE_KeyInit _keyNo:%d _key:%s.\r\n", _keyNo, UTILI_BytesToHexString(pucV[_keyNo], 16, NULL));
	}
}

void SECURE_ECB_Encrypt(uint8_t* _buffer, uint16_t _bufferLen)
{
	SECURE_KeyInit(1);

    for(int i= 0; i< (_bufferLen/ 16); i++)
    {
    	AES_ECB_encrypt(&ctx, _buffer + (i * 16));
    }
}

void SECURE_ECB_Decrypt(uint8_t *_buffer, uint32_t _bufferLen)
{
	SECURE_KeyInit(1);

    for(int i= 0; i< (_bufferLen/ 16); i++)
    {
    	AES_ECB_decrypt(&ctx, _buffer + (i * 16));
    }
}

void SECURE_CBC_Encrypt(uint8_t *_iv, uint8_t *_buffer, uint32_t _bufferLen)
{
	SECURE_KeyInit(1);

	AES_ctx_set_iv(&ctx, _iv);
	AES_CBC_encrypt_buffer(&ctx, _buffer, _bufferLen);
}

void SECURE_CBC_Decrypt(uint8_t *_iv, uint8_t *_buffer, uint32_t _bufferLen)
{
	SECURE_KeyInit(1);

	AES_ctx_set_iv(&ctx, _iv);
	AES_CBC_decrypt_buffer(&ctx, _buffer, _bufferLen);
}

void SECURE_CTR_Transcrypt(uint8_t _keyNo, uint8_t *_iv, uint8_t *_buffer, uint32_t _bufferLen)
{
	SECURE_KeyInit(_keyNo);

	AES_ctx_set_iv(&ctx, _iv);
	AES_CTR_xcrypt_buffer(&ctx, _buffer, _bufferLen);
}

int SECURE_SetRandom(uint8_t *_dest, unsigned _size)
{
	for(int i= 0; i< _size; i++)
	{
		_dest[i]= (uint8_t)UTILI_GetRandom(0, 0xFFFFFFFF);
	}
	return 1;
}

const struct uECC_Curve_t * eECDHCurve;
uint8_t eECDHPrivateKey[32]= {0};
ErrorStatus SECURE_ECDH_GenerateKeyPair(uint8_t *_publicKey64B)
{
	uECC_set_rng(SECURE_SetRandom);
	eECDHCurve= uECC_secp256r1();
	return (1== uECC_make_key(_publicKey64B, eECDHPrivateKey, eECDHCurve))? SUCCESS: ERROR;
}

//ErrorStatus SECURE_ECDH_DeriveAESKey(uint8_t *_publicKey64B, uint8_t *_derivedKeyA, uint8_t *_derivedKeyB)
//{
//    uint8_t _sharedSecret[32]= {0};
//	if(1== uECC_shared_secret(_publicKey64B, eECDHPrivateKey, _sharedSecret, eECDHCurve))
//	{
//		/*do hash512*/
//		/*derive AES128 key*/
//		uint8_t _derivedKey[32];
//		HKDFinput_stt HKDFinput_st; /* Structure to store the input to HKDF */
//		uint8_t salt_A[] =
//		  {
//		    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c
//		  };
//		uint8_t info_A[] =
//		  {
//		    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9
//		  };
//
//		__CRC_CLK_ENABLE();
//		/* Setting the input structure */
//		HKDFinput_st.pmKey = _sharedSecret;
//		HKDFinput_st.mKeySize = 32;
//		HKDFinput_st.pmSalt = salt_A;
//		HKDFinput_st.mSaltSize = sizeof(salt_A);
//		HKDFinput_st.pmInfo = info_A;
//		HKDFinput_st.mInfoSize = sizeof(info_A);
//		/* We are ready to call the HKDF_SHA512 */
//		if(HASH_SUCCESS!= HKDF_SHA512(&HKDFinput_st, _derivedKey, 32))
//		{
//			return ERROR;
//		}
//		memcpy(_derivedKeyA, _derivedKey+ 0, 16);
//		memcpy(_derivedKeyB, _derivedKey+ 16, 16);
//
//		return SUCCESS;
//	}
//
//	return ERROR;
//}

//ErrorStatus SECURE_AESGCM_Encrypt(
//		const uint8_t *InputMessage,
//        uint32_t   InputMessageLength,
//        const uint8_t *AES_Key,
//        uint32_t   KeyLength,
//        const uint8_t *IV,
//        uint32_t   IVSize,
//        const uint8_t *Header,
//        uint32_t   HeaderSize,
//        uint8_t   *Tag,
//        uint32_t   TagSize,
//        uint8_t   *OutputMessage)
//{
//	  /* AES context, error status and output length */
//	  AESGCMctx_stt AESctx;
//	  uint32_t error_status = AES_SUCCESS;
//	  int32_t outputLength = 0;
//
//	  /* Set flag field to default value */
//	  AESctx.mFlags = E_SK_DEFAULT;
//	  /* Set the key size in AES status */
//	  AESctx.mKeySize = KeyLength;
//	  /* Set the IV size in AES status */
//	  AESctx.mIvSize = IVSize;
//	  /* Set the tag size in AES status */
//	  AESctx.mTagSize = TagSize;
//	  /* Initialize the operation, by passing key and IV */
//	  error_status = AES_GCM_Encrypt_Init(&AESctx, AES_Key, IV);
//
//	  /* check for initialization errors */
//	  if (error_status == AES_SUCCESS)
//	  {
//	    /* Process Header (data to be authenticated but NOT encrypted) */
//	    error_status = AES_GCM_Header_Append(&AESctx, Header, HeaderSize);
//
//	    if (error_status == AES_SUCCESS)
//	    {
//	      /* Encrypt Data */
//	      error_status = AES_GCM_Encrypt_Append(&AESctx, InputMessage,
//	                                            InputMessageLength, OutputMessage, &outputLength);
//
//	      /* check for encryption errors */
//	      if (error_status == AES_SUCCESS)
//	      {
//	        /* Finalize data and write associated tag */
//	        error_status = AES_GCM_Encrypt_Finish(&AESctx,
//	                                              Tag, &outputLength);
//	      }
//	    }
//	  }
//
//	  return error_status;
//}

//ErrorStatus SECURE_AESGCM_Decrypt(
//		const uint8_t *InputMessage,
//        uint32_t   InputMessageLength,
//        const uint8_t *AES_Key,
//        uint32_t   KeyLength,
//        const uint8_t *IV,
//        uint32_t   IVSize,
//        const uint8_t *Header,
//        uint32_t   HeaderSize,
//        uint8_t   *Tag,
//        uint32_t   TagSize,
//        uint8_t   *OutputMessage)
//{
//	  /* AES context, error status and output length */
//	  AESGCMctx_stt AESctx;
//	  uint32_t error_status = AES_SUCCESS;
//	  int32_t outputLength = 0;
//
//	  /* Set flag field to default value */
//	  AESctx.mFlags = E_SK_DEFAULT;
//	  /* Set the key size in AES status */
//	  AESctx.mKeySize = KeyLength;
//	  /* Set the IV size in AES status */
//	  AESctx.mIvSize = IVSize;
//	  /* Set the tag size in AES status */
//	  AESctx.mTagSize = TagSize;
//	  /* Set the tag in AES status */
//	  AESctx.pmTag = Tag;
//
//	  /* Initialize the operation, by passing the key and IV */
//	  error_status = AES_GCM_Decrypt_Init(&AESctx, AES_Key, IV);
//
//	  /* check for initialization errors */
//	  if (error_status == AES_SUCCESS)
//	  {
//	    /* Process header */
//	    error_status = AES_GCM_Header_Append(&AESctx, Header, HeaderSize);
//
//	    if (error_status == AES_SUCCESS)
//	    {
//	      /* Decrypt Data */
//	      error_status = AES_GCM_Decrypt_Append(&AESctx, InputMessage,
//	                                            InputMessageLength, OutputMessage, &outputLength);
//
//	      /* Authenticate */
//	      if (error_status == AES_SUCCESS)
//	      {
//	        /* Finalize data */
//	        error_status = AES_GCM_Decrypt_Finish(&AESctx, NULL, &outputLength);
//	      }
//	    }
//	  }
//
//	  return error_status;
//}

