/******************************************************************************
 * File:        tracsens.h
 * Author:      Firmware Team
 * Created:     05-10-2025
 * Last Update: -
 *
 * Description:
 *      This file provides the hardware abstraction layer for the TRACSENS,
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
 *      	and change `HAL_LPTIM_IRQHandler(&pLptimHandle);` to
 *      	`HAL_LPTIM_IRQHandler(TRACSENS_IO_GetHandle());`. This ensures the
 *      	interrupt calls the handler through our driver's abstraction layer.
 *
 * Notes:
 *   - Add any implementation details, assumptions, or dependencies if needed.
 *
 * To Do:
 *   - List pending improvements, refactors, or unimplemented parts if any.
 *
 ******************************************************************************/
#include "main.h"

#if DRIVERS_MODULE_ENABLED == ENABLE_MODULE

#if defined(STM32U031xx)

#include <tracsens_u0_io.h>
#include "main.h" // For STM32 HAL/LL drivers

// A static pointer to hold the injected LPTIM handle
static LPTIM_HandleTypeDef *pLptimHandle = NULL;
// LPTIM handle is now private to this hardware-specific file
//static LPTIM_HandleTypeDef pLptimHandle;

// Pointers to the application-level callbacks
static void (*App_ReloadCallback)(void) = NULL;
static void (*App_DirUpCallback)(void) = NULL;
static void (*App_DirDownCallback)(void) = NULL;

/* STM32-specific HAL Callbacks that will invoke the application callbacks */
static void LPTIM1_AutoReloadMatchCallback(LPTIM_HandleTypeDef *hlptim);
static void LPTIM1_CounterChangedToUpCallback(LPTIM_HandleTypeDef *hlptim);
static void LPTIM1_CounterChangedToDownCallback(LPTIM_HandleTypeDef *hlptim);

LPTIM_HandleTypeDef* TRACSENS_IO_GetHandle(void)
{
    return pLptimHandle;
}

/**
 * @brief Initializes the LPTIM1 peripheral in Encoder mode.
 */
int TRACSENS_IO_Init(LPTIM_HandleTypeDef *hlptim)
{
	if (hlptim == NULL)
	{
		return -1; // Invalid handle
	}

	pLptimHandle = hlptim; // Store the injected handle
  pLptimHandle->Instance = LPTIM1;
  pLptimHandle->Init.Clock.Source = LPTIM_CLOCKSOURCE_APBCLOCK_LPOSC;
  pLptimHandle->Init.Clock.Prescaler = LPTIM_PRESCALER_DIV1;
  pLptimHandle->Init.UltraLowPowerClock.Polarity = LPTIM_CLOCKPOLARITY_RISING;
  pLptimHandle->Init.UltraLowPowerClock.SampleTime = LPTIM_CLOCKSAMPLETIME_DIRECTTRANSITION;
  pLptimHandle->Init.Trigger.Source = LPTIM_TRIGSOURCE_SOFTWARE;
  pLptimHandle->Init.Period = 0xFFFF; // Autoreload value
  pLptimHandle->Init.UpdateMode = LPTIM_UPDATE_IMMEDIATE;
  pLptimHandle->Init.CounterSource = LPTIM_COUNTERSOURCE_EXTERNAL;
  pLptimHandle->Init.Input1Source = LPTIM_INPUT1SOURCE_GPIO;
  pLptimHandle->Init.Input2Source = LPTIM_INPUT2SOURCE_GPIO;
  pLptimHandle->Init.RepetitionCounter = 0;

  if (HAL_LPTIM_Init(pLptimHandle) != HAL_OK)
  {
    return -1; // Error
  }
  return 0; // Success
}

/**
 * @brief Starts the LPTIM1 counter.
 */
void TRACSENS_IO_Start(void)
{
    if (HAL_LPTIM_Counter_Start_IT(pLptimHandle) != HAL_OK)
    {
        // In a real application, you would handle this error
        while(1);
    }
    
	/* Enable the required interrupts */
	LL_LPTIM_EnableIT_ARRM(LPTIM1); 	
	LL_LPTIM_EnableIT_UP(LPTIM1);		
	LL_LPTIM_EnableIT_DOWN(LPTIM1);

	LL_LPTIM_SetEncoderMode(LPTIM1, LL_LPTIM_ENCODER_MODE_RISING_FALLING);
    LL_LPTIM_EnableEncoderMode(LPTIM1);
    LL_LPTIM_Enable(LPTIM1);
	LL_LPTIM_SetAutoReload(LPTIM1, 0xFFFF);
    LL_LPTIM_StartCounter(LPTIM1, LL_LPTIM_OPERATING_MODE_CONTINUOUS);
}

/**
 * @brief Reads the LPTIM1 counter register.
 */
uint16_t TRACSENS_IO_GetCounter(void)
{
    return LL_LPTIM_GetCounter(LPTIM1);
}

/**
 * @brief Registers application callbacks and assigns them to the HAL LPTIM callbacks.
 */
void TRACSENS_IO_RegisterCallbacks(void (*reload_cb)(void), void (*dir_up_cb)(void), void (*dir_down_cb)(void))
{
    // Store the application-level function pointers
    App_ReloadCallback = reload_cb;
    App_DirUpCallback = dir_up_cb;
    App_DirDownCallback = dir_down_cb;

    // Register the local, hardware-specific ISR handlers with the HAL driver
    HAL_LPTIM_RegisterCallback(pLptimHandle, HAL_LPTIM_AUTORELOAD_MATCH_CB_ID, LPTIM1_AutoReloadMatchCallback);
	HAL_LPTIM_RegisterCallback(pLptimHandle, HAL_LPTIM_DIRECTION_UP_CB_ID, LPTIM1_CounterChangedToUpCallback);
	HAL_LPTIM_RegisterCallback(pLptimHandle, HAL_LPTIM_DIRECTION_DOWN_CB_ID, LPTIM1_CounterChangedToDownCallback);
}


/* ----------------- STM32 HAL-Specific Callback Implementations ----------------- */

/**
 * @brief  LPTIM1 Auto-Reload Match callback.
 * @note   This function is called by the HAL driver on a counter wraparound.
 * It then calls the registered application-level callback.
 */
static void LPTIM1_AutoReloadMatchCallback(LPTIM_HandleTypeDef *hlptim)
{
    if (App_ReloadCallback != NULL)
    {
        App_ReloadCallback();
    }
}

/**
 * @brief  LPTIM1 Counter direction change to UP callback.
 */
static void LPTIM1_CounterChangedToUpCallback(LPTIM_HandleTypeDef *hlptim)
{
    if (App_DirUpCallback != NULL)
    {
        App_DirUpCallback();
    }
}

/**
 * @brief  LPTIM1 Counter direction change to DOWN callback.
 */
static void LPTIM1_CounterChangedToDownCallback(LPTIM_HandleTypeDef *hlptim)
{
    if (App_DirDownCallback != NULL)
    {
        App_DirDownCallback();
    }
}

#endif

#endif // DRIVERS_MODULE_ENABLED
