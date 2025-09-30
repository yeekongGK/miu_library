/*
 * nbiotlwm2m.h
 *
 *  Created on: 14 Feb 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef NBIOT_NBIOTLWM2M_H_
#define NBIOT_NBIOTLWM2M_H_

#include "main.h"
#include "bc66link.h"
#include "lwobject.h"

#define LWM2M_CFG_MAX_OBJECT_COUNT		15/*bc66 limitation.*/
#define LWM2M_CFG_MAX_INSTANCE_COUNT	4 /*bc66 limitation.*/
#define LWM2M_CFG_MAX_RESOURCE_COUNT	14/*bc66 limitation.*/
#define LWM2M_CFG_MAX_REQUEST_VALUE_LEN	BC66LINK_CFG_LWM2M_RESPONSE_MAX_LEN
#define LWM2M_CFG_MAX_REQUEST			10//5/*we may get simultaneous observe*/

typedef struct
{
	BC66LINK_Lwm2m_t lwm2mType;
	uint32_t messageId;
	uint32_t objectId;
	uint32_t instanceId;
	uint8_t resourceIndex;
	uint8_t resourceCount;
	uint32_t resourceIds[LWM2M_CFG_MAX_RESOURCE_COUNT];
	uint32_t valueType;
	uint32_t len;
	uint8_t value[LWM2M_CFG_MAX_REQUEST_VALUE_LEN];
	uint32_t index;
	uint8_t flag;
}LWM2M_LWM2M_Request_t;

typedef struct
{
	char serverIP[150];/*max 150 bytes*/
	uint16_t serverPort;
	char endpointName[150];/*max 150 bytes*/
	uint32_t lifetime_s;
	BC66LINK_Lwm2m_Security_Mode_t securityMode;
	char pskId[150];/*max 150 bytes*/
	char psk[256];/*max 256 bytes, must be even and hex string*/
}LWM2M_LWM2M_Connection_t;

typedef struct
{
	bool enabled;
	bool enableBootstrap;
	LWM2M_LWM2M_Connection_t defaultConnection;
	LWM2M_LWM2M_Connection_t currentConnection;
	uint32_t registerBackoff_s;
	uint8_t notifyACK;
	uint8_t notifyRAI;
	bool reregisterAfterPSM;
	LWOBJ_Obj_t lwObj[MAX_ObjName];
	SWMGT_ResourceHolder_t swmgt;
	PRACT_ResourceHolder_t pract;
	DTMON_ResourceHolder_t dtmon;
	bool diversifyPractDispatchMask;
	uint32_t practDispatchMaskInterval;
	uint32_t practDispatchMaskPeriod;
	uint32_t practDispatchMaskOffset;
	uint16_t notifyRetryBackoffMin_s;
	uint16_t notifyRetryBackoffMax_s;
	bool useCellTemperature;

	uint8_t reserve[18];/*to add more param reduce this, thus no need to do backward compatible thing. remember to set the default value after config*/
}LWM2M_t;

bool LWM2M_IsRegistered(void);
void LWM2M_RequestRegister(void);
LWOBJ_Resource_t *LWM2M_GetResource(LWOBJ_Obj_t *_lwObj, uint16_t _resourceNo);
void LWM2M_Init(LWM2M_t *_config);
void LWM2M_Task(void);
uint8_t LWM2M_TaskState(void);

#endif /* NBIOT_NBIOTLWM2M_H_ */
