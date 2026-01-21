/******************************************************************************
 * File:        tracsens.h
 * Author:      Firmware Team
 * Created:     05-10-2025
 * Last Update: -
 *
 * Description:
 *      This file provides the hardware abstraction layer for the tracking sensor,
 *      specifically for an STM32 microcontroller. It configures and manages the
 *      LPTIM1 peripheral in encoder mode to read signals from a quadrature encoder.
 *      It handles hardware initialization, starting the counter, reading the raw
 *      counter value, and registering callbacks for hardware events (counter
 *      wrap-around, direction change).
 *
 * 						##### How to use this driver #####
 * 		==============================================================================
 *      1. Under CubeMX enable LPTIM, set mode to "Encoder mode from IN1 IN2".
 *      2. Enable LPTIM global interrupt.
 *      3. Under "Clock configuration" tab, set the LPTIM Clock Mux
 *         (PCLK,LSI,LSI,HSI16) according with the project.
 *      4. Under "Project management" -> "Advanced Settings" -> "Register CallBack"
 *          -> Enable LPTIM.
 *      5. In the `stm32xxxx_it.c` file, find the `LPTIM1_IRQHandler` function
 *      	and change `HAL_LPTIM_IRQHandler(&hlptim1);` to
 *      	`HAL_LPTIM_IRQHandler(TRACSENS_IO_GetHandle());`. This ensures the
 *      	interrupt calls the handler through our driver's abstraction layer.
 *
 ******************************************************************************/
#if defined(STM32U031xx)

#ifndef TRACSENS_IO_H
#define TRACSENS_IO_H
#if DRIVERS_MODULE_ENABLED == ENABLE_MODULE


#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Enumeration for hardware counter direction.
 */
typedef enum
{
    TRACSENS_IO_DIR_UP,
    TRACSENS_IO_DIR_DOWN
} TRACSENS_IO_Direction_t;

/**
 * @brief Initializes the underlying hardware timer/counter for the tracking sensor.
 * @retval 0 on success, non-zero on failure.
 */
int TRACSENS_IO_Init(LPTIM_HandleTypeDef *hlptim);

/**
 * @brief Starts the hardware counter.
 */
void TRACSENS_IO_Start(void);

/**
 * @brief Gets the current raw value from the hardware counter.
 * @retval The 16-bit counter value.
 */
uint16_t TRACSENS_IO_GetCounter(void);

/**
 * @brief Registers the application-level callback functions with the hardware driver.
 * @param reload_cb   Function pointer for the auto-reload (overflow/underflow) event.
 * @param dir_up_cb   Function pointer for the direction change to 'up' event.
 * @param dir_down_cb Function pointer for the direction change to 'down' event.
 */
void TRACSENS_IO_RegisterCallbacks(void (*reload_cb)(void), void (*dir_up_cb)(void), void (*dir_down_cb)(void));

/**
 * @brief Checks if a hardware counter wraparound (overflow/underflow) is pending.
 * @retval true if the Auto-Reload Match flag is set, false otherwise.
 */
bool TRACSENS_IO_IsWrapAroundPending(void);

/**
 * @brief Gets the current counting direction from the hardware.
 * @retval The current direction (UP or DOWN).
 */
TRACSENS_IO_Direction_t TRACSENS_IO_GetDirection(void);


LPTIM_HandleTypeDef* TRACSENS_IO_GetHandle(void);

#endif // TRACSENS_IO_H

#endif // DRIVERS_MODULE_ENABLED
#endif

