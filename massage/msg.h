/*
 * msg.h
 *
 *  Created on: 24 Jan 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef MSG_MSG_H_
#define MSG_MSG_H_

#include "main.h"
#include "sys.h"

#define MSG_CFG_MAX_BUFFER_SIZE		 	249/*due to limitation of nfc mailbox size and packet format*/
#define MSG_CFG_MAX_MSG_ARRAY			5

#define MSG_NO_ERROR	0
#define MSG_LEN_ERROR	1
#define MSG_UNDEFINED_ERROR	0x0f

#define MSG_TAG		0
#define MSG_LEN		1
#define MSG_VALUE		2

#define FLASH_TAG						0x00
#define INFO_WRITE_TAG					0x10/*TLV1|TLV2|...*/
#define INFO_READ_TAG					0x11/*TLV1|TLV2|...*/
#define CMD_TAG							0x20
#define AURA_TAG						0x30

#define FLASH_INIT_TAG					0x00
#define FLASH_WRITE_TAG					0x01/*|uint32_t ADDR|128 uint8_t Data|*/
#define FLASH_READ_TAG					0x02/*|uint32_t ADDR|*/
#define FLASH_SWITCH_TAG				0x03
#define FLASH_EEPROM_SAVE_TAG			0x04

#define INFO_FW_VERSION_TAG				0x00
#define INFO_HW_VERSION_TAG				0x01
#define INFO_MFC_DATE_TAG				0x02
#define INFO_SERIAL_NO_TAG				0x03
#define INFO_NAME_TAG					0x04
#define INFO_ADDRESS_TAG				0x05
#define INFO_NDEF_URL_TAG				0x06
#define INFO_NOTE_TAG					0x07
#define INFO_METER_SERIAL_NO_TAG		0x08
#define INFO_NBIOT_IMEI_TAG				0x10
#define INFO_NBIOT_IMSI_TAG				0x11
#define INFO_NBIOT_ICCID_TAG			0x12
#define INFO_NBIOT_PDP_ADDR_TAG			0x13
#define INFO_NBIOT_CDP_IP_TAG			0x14
#define INFO_NBIOT_COAP_IP_TAG			0x15
#define INFO_NBIOT_COAP_PORT_TAG		0x16
#define INFO_NBIOT_COAP_DEVICE_URI_TAG	0x17
#define INFO_NBIOT_COAP_REPORT_URI_TAG	0x18
#define INFO_NBIOT_COAP_FOTA_URI_TAG	0x19
#define INFO_NBIOT_APN_TAG				0x1A
#define INFO_NBIOT_COAP_IP_2_TAG		0x1B
#define INFO_NBIOT_COAP_PORT_2_TAG		0x1C
#define INFO_RF_UID_TAG					0x20
#define INFO_LOG_PARAM_TAG				0x30
#define INFO_LOG_PARAM2_TAG				0x31
#define INFO_LOG_PARAM3_TAG				0x32
#define INFO_LOG_STATUS_TAG				0x33
#define INFO_PULSE_CNTR_PARAM_TAG		0x40
#define INFO_PULSE_CNTR_LC_PARAM_TAG	0x41
#define INFO_PULSE_CNTR_VALUE_TAG		0x42
#define INFO_PULSE_CNTR_RATE_TAG		0x43
#define INFO_PULSE_CNTR_PIPE_TAG		0x44
#define INFO_PULSE_CNTR_READING_TAG		0x45
#define INFO_FLASH_BANK_NO_TAG			0x50
#define INFO_NBIOT_PARAM_TAG			0x60
#define INFO_NBIOT_NETWORK_PARAM_TAG	0x61
#define INFO_NBIOT_NETWORK_STATUS_TAG	0x62
#define	INFO_NBIOT_CONTROL_PARAM_TAG	0x63
#define INFO_WMBUS_PARAM_TAG			0x70
#define INFO_WMBUS_TIME_TAG				0x71
#define INFO_WMBUS_SPIRIT_PARAM_TAG		0x73
#define INFO_WMBUS_TRANSMIT_PARAM_TAG	0x74
#define INFO_WMBUS_CHANNEL_PARAM_TAG	0x75
#define INFO_WMBUS_AMFIELD_PARAM_TAG	0x76
#define INFO_RTC_PARAM_TAG				0x80
#define INFO_STATUS_CODE_TAG			0x90
#define INFO_STATUS_CODE_MASK_TAG		0x91
#define INFO_STATUS_AUTO_CLEAR_TAG		0x92
#define INFO_VOLTAGE_TAG				0xA0
#define INFO_CONFIG_VERSION_TAG			0xA1
#define INFO_LOCATION_TAG				0xA2
#define INFO_TEMPERATURE_TAG			0xA3
#define INFO_DIAGNOSTIC_TAG				0xB0
#define INFO_DIAGNOSTIC_STATUS_TAG		0xB1

#define CMD_LOG_START_LOG_TAG			0x30
#define CMD_LOG_READ_LOG_TAG			0x31
#define CMD_LOG_READ_RADIO_LOG_TAG		0x32
#define CMD_NBIOT_FORCE_SEND_TAG		0x60
#define CMD_NBIOT_SARA_TAG			    0x61
#define CMD_NBIOT_NUESTATS_QUERY_TAG	0x62
#define CMD_NBIOT_SARA_UTEST_TAG		0X63
#define CMD_NBIOT_APP_TAG				0x64
#define CMD_WMBUS_BER_TEST_TAG			0x70
//#define CMD_WMBUS_CHANNEL_PARAM_TAG		0x71/*removed*/
#define CMD_WMBUS_MUTE_TAG				0x72
#define CMD_SYS_INHIBIT_STOP_MODE_TAG	0x80
#define CMD_SYS_REBOOT_TAG				0x81
#define CMD_SYS_SHUTDOWN_TAG			0x82
#define CMD_SYS_TURN_ON_TAG				0x83
#define CMD_RTC_CALIB_TAG				0x84
#define CMD_RTC_BKP_REG_TAG				0x85
#define CMD_FLASH_ERASE_EEPROM_TAG		0x86
#define CMD_SENSORS_MEMS_TAG			0x90
#define CMD_DIAGNOSTIC_CONFIG_TAG		0xB0
#define CMD_KEY_TAG						0xC0

//new command starts here
#define AURA_TAG_SENSOR_VALUES			0x00
#define AURA_TAG_SYS					0x01
#define AURA_TAG_NBIOT_MODEM			0x02
#define AURA_TAG_NBIOT_EXIT_FWU			0x03
#define AURA_TAG_NBIOT_TESTMODE			0x04
#define AURA_TAG_CONFIG					0xFF

typedef enum
{
	MICA_FLASH_GroupTLVTag= 0x00,
	MICA_INFO_WRITE_GroupTLVTag= 0x01,
	MICA_INFO_READ_GroupTLVTag= 0x11,
	MICA_CMD_GroupTLVTag= 0x20,
	AURA_GroupTLVTag= 0x30,
}MSG_GroupTLVTag_t;

typedef enum
{
	SYS_TLVTag= 0,
	LOG_TLVTag,
	NBIOT_TLVTag,
	PULSECNTR_TLVTag,
	SENSOR_TLVTag,
	ALARM_TLVTag,
	DIAGNOSTIC_TLVTag,
	FAILSAFE_TLVTag,
	DEVELOPER_TLVTag,
	PRODUCTION_TLVTag,
}MSG_TLVTag_t;

#define WMBUS_REQ_LOG_TAG			0x72

typedef struct
{
	uint8_t *buffer;
	uint16_t bufferLen;
	SYS_TaskId_t taskId;
	bool sendResponse;
	SYS_TaskId_t responseTaskId;
}MSG_t;

typedef struct
{
	MSG_TLVTag_t tag;
	uint8_t len;
	uint8_t *value;
}TLV_Typedef, *pTLV_Typedef;/*if modified, check padding*/

uint8_t 			MSG_Msg_Depth(void);
ErrorStatus 		MSG_Msg_Enqueue(MSG_t _msg);
MSG_t 		MSG_Msg_Dequeue(void);
MSG_t 		MSG_Msg_Peek(void);
SYS_TaskId_t 	MSG_Msg_TaskId_Peek(void);
void MSG_SyncTask(MSG_t *_msg);
void MSG_Init(void);
void MSG_Task(void);
uint8_t MSG_TaskState(void);

#endif /* MSG_MSG_H_ */
