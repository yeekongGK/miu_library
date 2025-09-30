/*
 * lwresource.h
 *
 *  Created on: 17 Feb 2022
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef NBIOT_LWM2M_LWRESOURCE_H_
#define NBIOT_LWM2M_LWRESOURCE_H_


typedef enum
{
	READ_ResourceOperation,
	WRITE_ResourceOperation,
	READWRITE_ResourceOperation,
	EXECUTE_ResourceOperation,
}LWOBJ_ResourceOperation_t;

typedef enum
{
	STRING_ResourceType= 1,
	OPAQUE_ResourceType= 2,
	INTEGER_ResourceType= 3,
	FLOAT_ResourceType= 4,
	BOOLEAN_ResourceType= 5,
	//TIME_ResourceType= 6, /*Unix time, seconds since Unix Epoch, essentially an integer*/
	OBJLINK_ResourceType= 7,
	NONE_ResourceType= 8,
}LWOBJ_ResourceType_t;

typedef union
{
	char *string;
	uint8_t *opaque;
	uint32_t integer;
	double single;
	uint8_t boolean;
	char *objlink;/*object link*/
	void (*execute)(void *, ...);
}LWOBJ_ValueType_t;

typedef struct
{
	uint16_t id;
	LWOBJ_ResourceOperation_t operation;
	LWOBJ_ResourceType_t type;
}LWOBJ_ResourceAttr_t;

typedef enum
{
	IDLE_NotifyState= 0,
	DO_NOTIFY_NotifyState= 1,
	FIRST_RETRY_NotifyState= 2,
	SECOND_RETRY_NotifyState= 3,
	RETRY_DONE_NotifyState= 4,
}LWOBJ_NotifyState_t;

typedef struct
{
	LWOBJ_ResourceAttr_t attr;
	bool observe;
	LWOBJ_NotifyState_t notifyState;
	bool written;
	uint16_t valueMaxLen;
	uint16_t valueLen;
	LWOBJ_ValueType_t value;
}LWOBJ_Resource_t;


#endif /* NBIOT_LWM2M_LWRESOURCE_H_ */
