/******************************************************************************
 * File:        cfg_device.h
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file contains device-specific definitions, including the firmware and
 *   hardware version strings. These macros are used throughout the application
 *   to identify the current build and hardware revision.
 *
 * Notes:
 *   - -
 *
 * To Do:
 *   - -
 *
 ******************************************************************************/

#ifndef INC_CFG_DEVICE_H_
#define INC_CFG_DEVICE_H_

//#define CFG_DEVICE_FIRMWARE_VERSION		    FIRMWARE_VERSION
#define CFG_DEVICE_FIRMWARE_VERSION		    "v2_testing"
#define CFG_DEVICE_HARDWARE_VERSION 		"NB-MAIN-01U1"
#define CFG_DEVICE_USE_RADIO_SW				0/*only for a couple of initial prototype boards that has radio switch configuration*/

#endif /* INC_CFG_DEVICE_H_ */
