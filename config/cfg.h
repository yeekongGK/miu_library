/******************************************************************************
 * File:        cfg.h
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file defines the main configuration structures and interfaces for the
 *   entire application. It specifies the layout of configuration data,
 *   including system settings, NBIoT parameters, sensor configurations, and
 *   more. It also declares the functions for managing the configuration
 *   lifecycle, such as loading from and saving to Flash memory.
 *
 * Notes:
 *   - The `Config_t` struct is the primary container for all configuration data.
 *   - Configuration versioning is managed via `CFG_CONFIG_VERSION`.
 *
 * To Do:
 *   - -
 *
 ******************************************************************************/

#ifndef INC_CONFIGURATION_H_
#define INC_CONFIGURATION_H_

#include "main.h"
#include "cfg_addr.h"
#include "sys.h"
#include "pulser.h"
#include "logger.h"
#include "nbiot.h"
#include "bc66link.h"
#include "sensor.h"
#include "alarm_cfg.h"
#include "failsafe.h"
#include "diag.h"
#include <time.h>

#define CFG_CONFIG_VERSION 				0x0032/*Please increment this when modifying config*/
#define CFG_MIN_CONFIG_VERSION			0x0012 /*minimum config version that can be forward-compatible*/
#define CFG_FW_VERSION_LEN				35
#define CFG_HW_VERSION_LEN				35
#define CFG_BOARD_ID_LEN				35 /*yyyyddmm*/
#define CFG_MFC_DATE_LEN				8 /*yyyyddmm*/
#define CFG_SERIAL_NO_LEN				35
#define CFG_UID_LEN						35/*uid can be imei for nbiot or rf uid for wmbus*/
#define CFG_NAME_LEN					35
#define CFG_ADDRESS_LEN					140
#define CFG_NDEF_URL_LEN				140
#define CFG_NOTE_LEN					140

#define UNINIT_STRING 					""
#define EMPTY_STRING 					""
#define NAME_STRING 					"GKM AURA NBIOT MIU"

#define CFG_FLASH_WRITE_BUFFER_LEN		256

typedef enum
{
	NBIOT_Transmission,
	WMBUS_Transmission
}CFG_Transmission_t;

typedef enum
{
	BTLDR_APP_CFG_FlashType= 0,/*typically btldr, app & Config. App will not run during firmware update through bootloader.*/
	APP1_APP2_CFG_FlashType,/*typically app_A,  app_B & Config. Different logical address, with embedded bootloader in each app.*/
	APP1CFG1_APP2CFG2_FlashType,/*typically app_A & app_B. Same logical address.*/
}CFG_FlashType_t;

typedef enum
{
	NONE_FlashPartition= 0,
	PARTITION_1_FlashPartition= CFG_PARTITION_1,
	PARTITION_2_FlashPartition= CFG_PARTITION_2,
	PARTITION_CONFIG_FlashPartition= CFG_PARTITION_CONFIG,
}CFG_FlashPartition_t;

typedef enum
{
	NONE_FlashBank= 0,
	BANK_1_FlashBank,
	BANK_2_FlashBank,
}CFG_FlashBank_t;

typedef struct __attribute__((aligned(8)))
{
	uint32_t checksum;
	uint16_t configVersion;
	bool useDefaultConfig;

	/*variable to manage sectors for firmware update*/
	CFG_FlashType_t type;
	CFG_FlashPartition_t  partition;
	uint32_t partitionStartAddress;/*indicate which sector should we run the mcu program from*/
	uint32_t partitionLength;
	uint32_t partitionChecksum;
	uint16_t failsafeWordA;
	uint8_t reserve[16];

	//uint8_t reserve[256];/*to add more param reduce this, thus no need to do backward compatible thing. remember to set the default value after config*/
}Flash_t;

typedef struct __attribute__((aligned(8)))
{
	uint32_t checksum;

	/*operational parameters*/
	uint32_t mcuFrequency;
	CFG_Transmission_t transmissionType;

	/*default date and time*/
	uint8_t hours;/*bcd*/
	uint8_t minutes;/*bcd*/
	uint8_t seconds;/*bcd*/
	uint8_t weekday;/*Mon is 1, Sun is 7*/
	uint8_t day;/*bcd*/
	uint8_t month;/*bcd*/
	uint8_t year;/*bcd*/
	uint8_t utc;/*bin, multiplier of 15 minutes*/

	/*encryption keys*/
	struct
	{
		uint8_t master[16];/*master key: this key can be used to auth and overwrite other key*/
		struct
		{
			uint8_t prod[16];/*production key*/
			uint8_t dev[16]; /*developer key*/
			uint8_t user[16];/*user key*/
		}cfg;/*configuration key: for configuration, mainly via nfc/ir/bluetooth*/
		struct
		{
			uint8_t a[256];/*key a: e.g. encrypt/ transcrypt key*/
			uint8_t b[256];/*key b: e.g. decrypt key*/
		}opr;/*operation key: for application usage*/
	}key;

	char fwVersion		[CFG_FW_VERSION_LEN+ 1];
	char hwVersion		[CFG_HW_VERSION_LEN+ 1];
	char boardId		[CFG_BOARD_ID_LEN+ 1];
	char mfcDate		[CFG_MFC_DATE_LEN+ 1];
	char uid			[CFG_UID_LEN+ 1];/*uid can be imei for nbiot or rf uid for wmbus*/
	char serialNo		[CFG_SERIAL_NO_LEN+ 1];
	char name			[CFG_NAME_LEN+ 1];
	char address		[CFG_ADDRESS_LEN+ 1];
	char note			[CFG_NOTE_LEN+ 1];
	char meterSerialNo	[CFG_SERIAL_NO_LEN+ 1];
	uint32_t meterModel;
	float latitude;
	float longitude;

	/*operational variable/ flags*/
	bool rteDisableSleep;
	uint64_t rteSysTick;
	SYS_SysRequest_t rteSysRequest;
	bool rteIsTurnedOn;
	time_t rteSysRequestTimeDelay;

	uint8_t reserve[128];/*to add more param reduce this, thus no need to do backward compatible thing. remember to set the default value after config*/
}System_t;

typedef struct
{
	/*TODO: rearrange this form least frequently changed/added. i.e nbiot|wmbus last*/
	Flash_t flash;
	System_t system;
	FAILSAFE_t failsafe;
	DIAG_t diagnostic;
	PULSER_t pulser;
	LOG_t log;
	SENSOR_t sensors;
	ALARM_t alarm;
	NBIOT_t nbiot;
}Config_t;

extern Config_t config;

uint32_t CFG_GetChecksum(uint8_t *_ptr, uint32_t _length);
uint32_t CFG_GetFWUChecksum(uint16_t _crc, uint8_t *_buf, uint32_t _len); /*TODO: change this to hardware checksum(faster)*/
ErrorStatus CFG_ReadSingleStruct(uint8_t *_struct, uint16_t _length, uint32_t _address);
ErrorStatus CFG_WriteProgram(uint32_t _address, uint8_t *_writeBuffer, uint16_t _length);
ErrorStatus CFG_ErasePartition(CFG_FlashPartition_t _partition);
bool CFG_IsInPartition(CFG_FlashPartition_t _partition, uint32_t _startAddress, uint16_t _length);
ErrorStatus CFG_Store(Config_t *_cfg);
ErrorStatus CFG_Load(Config_t *_cfg);
ErrorStatus CFG_LoadSystem(Config_t *_cfg);
void CFG_TLVRequest(TLV_t *_tlv);
void CFG_ApplyDefaults(Config_t *_cfg);

#endif /* INC_CONFIGURATION_H_ */
