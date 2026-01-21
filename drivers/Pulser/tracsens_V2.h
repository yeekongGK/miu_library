/******************************************************************************
 * File:        tracsens.h
 * Author:      Firmware Team
 * Created:     05-10-2025
 * Last Update: -
 *
 * Description:
 *      This file implements the application-level logic for a tracking sensor.
 *      It extends a 16-bit hardware counter to a 32-bit software counter by
 *      tracking overflows and underflows. It provides functions to initialize
 *      the sensor, start counting, and display cumulative forward and backward
 *      counts. It relies on the `tracsens_io` module for hardware abstraction.
 *
 ******************************************************************************/

#ifndef TRACSENS_H
#define TRACSENS_H
#if DRIVERS_MODULE_ENABLED == ENABLE_MODULE

#include "main.h"

/**
 * @brief Initializes the Tracking Sensor module and its underlying hardware.
 * @retval 0 on success, non-zero on failure.
 */
int TRACSENS_Init(LPTIM_HandleTypeDef *hlptim);

/**
 * @brief Starts the sensor counting process.
 */
void TRACSENS_StartCounting(void);

/**
 * @brief Calculates and displays sensor statistics via UART.
 * @note  This function updates the cumulative forward/backward counts.
 */
void TRACSENS_DisplayInfo(void);

#endif // DRIVERS_MODULE_ENABLED
#endif // TRACSENS_H
