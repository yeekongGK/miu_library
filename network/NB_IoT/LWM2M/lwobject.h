/*
 * lwobject.h
 *
 *  Created on: 9 Mar 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef NBIOT_LWM2M_LWOBJECT_H_
#define NBIOT_LWM2M_LWOBJECT_H_

#include "main.h"
#include "queue.h"
#include "lwm2m.h"
#include "lwresource.h"
#include "cbor.h"
#include "periodicactivity.h"
#include "datamonitoring.h"
#include "softwaremgt.h"
#include "eventdatadelivery.h"

#define LWOBJ_CFG_MAX_OBJECT_COUNT							15/*bc66 limitation.*/
#define LWOBJ_CFG_MAX_INSTANCE_COUNT						4/*bc66 limitation.*/
#define LWOBJ_CFG_MAX_RESOURCE_COUNT						14/*bc66 limitation.*/
#define LWOBJ_CFG_LWOBJ_CBOR_BUFFER_SIZE					(96* 10)
#define LWOBJ_CFG_LWOBJ_RECORD_COUNT						8
#define LWOBJ_CFG_MAX_SLEEP_TIME_S							86400
#define LWOBJ_CFG_MIN_SLEEP_TIME_S							1

typedef enum
{
	SOFTWARE_MANAGEMENT_ObjName,
	PRACT_GET_READING_ObjName,
	PRACT_GET_STATUS_ObjName,
	PRACT_QUERY_ObjName,
	DTMON_ALARMS_ObjName,
	//DTMON_BACKFLOW_ObjName,
	MAX_ObjName,
}LWOBJ_ObjName_t;

typedef enum
{
	SWMGT_ObjType,/*Software Management*/
	PRACT_ObjType,/*Periodic Activity*/
	DTMON_ObjType,/*Data Monitoring*/
	MAX_ObjType,
}LWOBJ_ObjType_t;

typedef struct
{
	LWOBJ_ObjType_t type;
	uint16_t id;
	float version;
	uint8_t instance;
	uint8_t resourceCount;
	LWOBJ_Resource_t *resource;
	bool written;/*this to flag when there is a resource written*/
	union
	{
		PRACT_Rte_t *prAct;
		DTMON_Rte_t *dtMon;
		SWMGT_Rte_t *swMgt;
		//EDD_Rte_t *edd;
	}rte;

}LWOBJ_Obj_t;

LWOBJ_ValueType_t LWOBJ_ReadResource(LWOBJ_ObjType_t _objType, uint16_t _resourceId);
void LWOBJ_SEW_EVENT_Task(LWOBJ_Obj_t *pLwObj);
LWOBJ_ValueType_t LWOBJ_ExecuteResource(LWOBJ_Obj_t *pLwObj, uint16_t _resourceId);

#endif /* NBIOT_LWM2M_LWOBJECT_H_ */
