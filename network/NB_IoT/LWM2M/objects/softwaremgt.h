/*
 * swmgt.h
 *
 *  Created on: 9 Dec 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef NBIOT_LWM2M_OBJECTS_SOFTWAREMGT_H_
#define NBIOT_LWM2M_OBJECTS_SOFTWAREMGT_H_

#include "main.h"
#include "lwresource.h"

#define SWMGT_CFG_URI_MAX_LEN		255

typedef enum
{
	FOTA_SwMgtInstance= 0,
	MAX_SwMgtInstance,
}SWMGT_SwMgtInstance_t;

typedef enum
{
	PKG_NAME_SwMgtResource= 0,
	PKG_VERSION_SwMgtResource= 1,
	PACKAGE_SwMgtResource= 2,/*TODO: restructure so that we don't have to have fillers*/
	PACKAGE_URI_SwMgtResource= 3,
	INSTALL_SwMgtResource= 4,
	FILLER1_SwMgtResource= 5,
	UNINSTALL_SwMgtResource= 6,
	UPDATE_STATE_SwMgtResource= 7,
	FILLER2_SwMgtResource= 8,
	UPDATE_RESULT_SwMgtResource= 9,
	ACTIVATE_SwMgtResource= 10,
	DEACTIVATE_SwMgtResource= 11,
	ACTIVATION_STATE_SwMgtResource= 12,
	MAX_SwMgtResource= 13,/*total count*/
}SWMGT_SwMgtResource_t;

typedef enum
{
	INITIAL_SwMgtUpdateState= 0,
	DOWNLOAD_STARTED_SwMgtUpdateState,
	DOWNLOADED_SwMgtUpdateState,
	DELIVERED_SwMgtUpdateState,
	INSTALLED_SwMgtUpdateState,
	MAX_SwMgtUpdateState,
}SWMGT_SwMgtUpdateState_t;

typedef enum
{
	INITIAL_SwMgtUpdateResult= 0,
	DOWNLOADING_SwMgtUpdateResult= 1,
	SUCCESSSFULLY_INSTALLED_SwMgtUpdateResult= 2,
	SUCCESSSFULLY_DOWNLOADED_SwMgtUpdateResult= 3,
	NOT_ENOUGH_STORAGE_SwMgtUpdateResult= 50,
	OUT_OF_MEMORY_SwMgtUpdateResult= 51,
	CONNECTION_LOST_SwMgtUpdateResult= 52,
	INTEGRITY_FAILURE_SwMgtUpdateResult= 53,
	UNSUPPORTED_PACKAGE_SwMgtUpdateResult= 54,
	INVALID_URI_SwMgtUpdateResult= 56,
	DEVICE_DEFINED_UPDATE_ERROR_SwMgtUpdateResult= 57,
	INSTALLATION_FAILURE_SwMgtUpdateResult= 58,
	UNINSTALLATION_FAILURE_SwMgtUpdateResult= 59,
}SWMGT_SwMgtUpdateResult_t;

typedef enum
{
	DISABLED_SwMgtActivationState= 0,
	ENABLED_SwMgtActivationState,
}SWMGT_SwMgtActivationState_t;

typedef enum
{
	IDLE_SwDownloadState= 0,
	ERASE_PARTITION_SwDownloadState,
	REST_SwDownloadState,
	GET_PACKET_SwDownloadState,
	STORE_PACKET_SwDownloadState,
	SET_BACKOFF_SwDownloadState,
	BACKOFF_SwDownloadState,
	SUSPEND_SwDownloadState,
	DOWNLOADED_SwDownloadState,
	FAILED_SwDownloadState,
	MAX_SwDownloadState
}SWGT_SwDownloadState_t;

typedef struct
{
	SWGT_SwDownloadState_t state;
	SWGT_SwDownloadState_t prevState;
	bool isRequested;
	bool isCompleted;
	bool isOnGoing;
	uint8_t retryCount;

	//uint16_t currentPacket;
	//uint16_t totalPacket;
	//uint32_t packetSize;

	bool isFirstBlock;
	uint16_t currentBlock;
	uint16_t blockSize;

	uint32_t flashSize;
	uint32_t flashWriteAddress;
	uint16_t flashWriteTotal;
	uint16_t flashWriteCount;
	uint32_t flashWrittenBytes;
	uint16_t flashChecksum;
	uint8_t status;
	char version[16];
}SWGT_SwUpdate_t;

typedef struct
{
	bool enabled;
	SWGT_SwUpdate_t swUpdate;
}SWMGT_Rte_t;

typedef struct
{
	LWOBJ_Resource_t resource[MAX_SwMgtResource];
	SWMGT_Rte_t rte;
	char uri[SWMGT_CFG_URI_MAX_LEN];
	char packageversion[SWMGT_CFG_URI_MAX_LEN];
}SWMGT_ResourceHolder_t;

#endif /* NBIOT_LWM2M_OBJECTS_SOFTWAREMGT_H_ */
