/******************************************************************************
 * File:        max17260.c
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file provides the low-level driver for the MAX17260 fuel gauge IC.
 *   It implements the hardware abstraction layer (HAL) for I2C communication
 *   and provides functions for reading from and writing to the device's
 *   registers. The driver also includes initialization routines to configure
 *   the fuel gauge after a power-on reset, along with debugging functions to
 *   print register values.
 *
 * Notes:
 *   - The I2C read/write functions are implemented based on the project's
 *     I2C abstraction and must be adapted if the underlying hardware changes.
 *   - This driver is used by higher-level modules like `batterysensor.c`.
 *
 * To Do:
 *   - -
 *
 ******************************************************************************/
#include "max17260.h"
#include "main.h"
#include "i2c.h"

// I2C Address for the MAX17260
#define MAX17260_I2C_ADDRESS 0x36

// A subset of MAX17260 registers needed for init and getQH


//=============================================================================
// HARDWARE ABSTRACTION LAYER (HAL) - TO BE IMPLEMENTED BY USER
//=============================================================================
//
// You must provide the implementation for the following three static
// functions based on your specific microcontroller's I2C peripheral driver.
//
//=============================================================================

/**
 * @brief Initializes the I2C peripheral.
 * @note  This is a placeholder. Implement with your hardware's I2C init code.
 */
static void i2c_init(void) {
    // Example: i2c_master_init(I2C_NUM_0, ...);
}

/**
 * @brief Writes data to an I2C device.
 * @param dev_addr The 7-bit I2C device address.
 * @param reg_addr The register address to write to.
 * @param data A pointer to the data buffer to write.
 * @param len The number of bytes to write.
 * @return true on success, false on failure.
 */
static bool i2c_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t* data, uint16_t len) {
    // This is a placeholder.
    // Implement this function using your platform's I2C write function.
    // It should handle the START, address, register, data, and STOP conditions.
     return I2C1_MemWrite(dev_addr << 1, reg_addr, I2C_MEMADD_SIZE_8BIT, data, len, 100) == HAL_OK;
    //return true; // Assume success for now
}

/**
 * @brief Reads data from an I2C device.
 * @param dev_addr The 7-bit I2C device address.
 * @param reg_addr The register address to read from.
 * @param data A pointer to the buffer to store the read data.
 * @param len The number of bytes to read.
 * @return true on success, false on failure.
 */
static bool i2c_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t* data, uint16_t len) {
    // This is a placeholder.
    // Implement this function using your platform's I2C read function.
    return I2C1_MemRead(dev_addr << 1, reg_addr, I2C_MEMADD_SIZE_8BIT, data, len, 100) == HAL_OK;
    //return true; // Assume success for now
}

/**
 * @brief Delays execution for a specified number of milliseconds.
 * @note This is a placeholder. Implement with your hardware's delay function.
 * @param ms Milliseconds to delay.
 */
static void delay_ms(uint32_t ms) {
    // Example: vTaskDelay(pdMS_TO_TICKS(ms)); or HAL_Delay(ms);
    volatile uint32_t i, j;
    for (i = 0; i < ms; i++) {
        for (j = 0; j < 1000; j++) {
            __asm__("nop");
        }
    }
}


//=============================================================================
// MAX17260 DRIVER IMPLEMENTATION (INTERNAL)
//=============================================================================

/**
 * @brief Writes a single 16-bit value to a MAX17260 register.
 */
static bool MAX17260_Register_WriteSingle(uint8_t reg, uint16_t value) {
    uint8_t buffer[2];
    // Little-endian format
    buffer[0] = value & 0xFF;
    buffer[1] = (value >> 8) & 0xFF;
    return i2c_write(MAX17260_I2C_ADDRESS, reg, buffer, 2);
}

/**
 * @brief Reads a single 16-bit value from a MAX17260 register.
 */
static bool MAX17260_Register_ReadSingle(uint8_t reg, uint16_t* value) {
    uint8_t buffer[2];
    if (i2c_read(MAX17260_I2C_ADDRESS, reg, buffer, 2)) {
        // Little-endian format
        *value = (uint16_t)buffer[0] | ((uint16_t)buffer[1] << 8);
        return true;
    }
    *value = 0;
    return false;
}

/**
 * @brief Checks if the MAX17260 has experienced a Power-On-Reset.
 */
static bool IsPOR(void) {
    uint16_t status_val = 0;
    MAX17260_Register_ReadSingle(STATUS, &status_val);
    // POR bit is bit 1 of the STATUS register
    return (status_val & (1 << 1)) != 0;
}

/**
 * @brief Clears the Power-On-Reset flag in the STATUS register.
 */
static void ClearPOR(void) {
    uint16_t status_val = 0;
    MAX17260_Register_ReadSingle(STATUS, &status_val);
    // Write the value back, but with the POR bit cleared
    MAX17260_Register_WriteSingle(STATUS, status_val & ~(1 << 1));
}


//=============================================================================
// MAX17260 DRIVER DEBUG
//=============================================================================


void Print_MAX1726x_Config2(void)
{
    uint16_t config2_reg;

    // Read the current value from the config2 register
    // maxim_max1726x_read_reg(MAX1726X_CONFIG2_REG, &config2_reg);
    MAX17260_Register_ReadSingle(CONFIG2, &config2_reg);

    UART_Printf("\r\n--- MAX1726x Config2 Register [0x%04X] ---\r\n", config2_reg);
    UART_Printf("This register controls advanced model and IC features.\r\n");
    UART_Printf("----------------------------------------\r\n");

    // Bit 15: Enable Dynamic Power
    UART_Printf("[Bit 15] DP_EN    (Dynamic Power Enable) : %s\r\n",
                (config2_reg & (1 << 15)) ? "ENABLED" : "DISABLED");

    // Bit 14: Enable AtRate Function
    UART_Printf("[Bit 14] AtRateEn (AtRate Function En)  : %s\r\n",
                (config2_reg & (1 << 14)) ? "ENABLED" : "DISABLED");
                
    // Bit 12: Enable Constant-Power Load Model
    UART_Printf("[Bit 12] CP_EN    (Constant Power Model) : %s\r\n",
                (config2_reg & (1 << 12)) ? "ENABLED" : "DISABLED");
                
    // Bit 10: I2C Timeout Disable
    UART_Printf("[Bit 10] TMOUT_DIS(I2C Timeout Disable)  : %s\r\n",
                (config2_reg & (1 << 10)) ? "DISABLED" : "ENABLED");
                
    // Bit 9: Enable Thermistor Simulation
    UART_Printf("[Bit 9]  TS_EN    (Thermistor Sim En)    : %s\r\n",
                (config2_reg & (1 << 9)) ? "ENABLED" : "DISABLED");

    // Bit 8: Enable State of Health (SoH) Compensation
    UART_Printf("[Bit 8]  SOH_EN   (SoH Compensation En)  : %s\r\n",
                (config2_reg & (1 << 8)) ? "ENABLED" : "DISABLED");

    // Bit 7: Force EZ Config Load
    UART_Printf("[Bit 7]  LDMDL    (Load Model Command)   : %s - Writing '1' reloads EZ config.\r\n",
                (config2_reg & (1 << 7)) ? "ACTIVE" : "IDLE");

    // Bit 5: Enable Ripple and Particle Filter
    UART_Printf("[Bit 5]  RI_EN    (Ripple Filter En)     : %s\r\n",
                (config2_reg & (1 << 5)) ? "ENABLED" : "DISABLED");

    // Bit 4: Enable Current-Voltage Synchronization
    UART_Printf("[Bit 4]  CVSYNC_EN(CV Sync Enable)       : %s\r\n",
                (config2_reg & (1 << 4)) ? "ENABLED" : "DISABLED");
    
    // Bit 3: Disable Model Comp Learning
    UART_Printf("[Bit 3]  MOD_DIS  (Model Learning Dis)   : %s\r\n",
                (config2_reg & (1 << 3)) ? "LEARNING DISABLED" : "LEARNING ENABLED");
    
    // Bit 1: ADC Offset Correction Disable
    UART_Printf("[Bit 1]  ADC_DIS  (ADC Offset Corr Dis)  : %s\r\n",
                (config2_reg & (1 << 1)) ? "DISABLED" : "ENABLED");

    UART_Printf("----------------------------------------\r\n\r\n");
}

void Print_MAX1726x_Config(void)
{
    uint16_t config_reg;
    const char* temp_src;

    // Read the current value from the config register
    // maxim_max1726x_read_reg(MAX1726X_CONFIG_REG, &config_reg);
    MAX17260_Register_ReadSingle(CONFIG, &config_reg);

    UART_Printf("\r\n--- MAX1726x Config Register [0x%04X] ---\r\n", config_reg);
    UART_Printf("This register controls alerts and operating modes.\r\n");
    UART_Printf("----------------------------------------\r\n");

    // Bit 14: SOC Change Alert Enable
    UART_Printf("[Bit 14] S_INT    (SOC Change Alert En)  : %s\r\n",
                (config_reg & (1 << 14)) ? "ENABLED" : "DISABLED");

    // Bit 13: Thermistor Enable
    UART_Printf("[Bit 13] T_EN     (Thermistor Enable)    : %s\r\n",
                (config_reg & (1 << 13)) ? "ENABLED (External)" : "DISABLED (Internal sensor)");

    // Bit 12: Peak Current Alert Enable
    UART_Printf("[Bit 12] PA_EN    (Peak Current Alert En): %s\r\n",
                (config_reg & (1 << 12)) ? "ENABLED" : "DISABLED");

    // Bit 11: Temperature Channel Assignment
    if (config_reg & (1 << 13)) { // T_EN is enabled (external)
        temp_src = (config_reg & (1 << 11)) ? "THRM2 pin" : "THRM1 pin";
    } else { // T_EN is disabled (internal)
        temp_src = (config_reg & (1 << 11)) ? "Die Temp" : "V_CELL Temp";
    }
    UART_Printf("[Bit 11] T_SEL    (Temp Source Select)   : %s\r\n", temp_src);

    // Bit 8: Enable Sticky Alerts
    UART_Printf("[Bit 8]  Sticky   (Sticky Alerts)        : %s - If enabled, alert flags are not auto-cleared.\r\n",
                (config_reg & (1 << 8)) ? "ENABLED" : "DISABLED");
                
    // Bit 3: ALRT Pin Enable
    UART_Printf("[Bit 3]  A_EN     (ALRT Pin Enable)      : %s - If enabled, the ALRT pin will assert on alerts.\r\n",
                (config_reg & (1 << 3)) ? "ENABLED" : "DISABLED");

    // Bit 2: State of Charge Alert Enable
    UART_Printf("[Bit 2]  S_EN     (SOC Alert En)         : %s - For main SOC threshold (SAlrtTh).\r\n",
                (config_reg & (1 << 2)) ? "ENABLED" : "DISABLED");
                
    // Bit 1: Temperature Alert Enable
    UART_Printf("[Bit 1]  TALRT_EN (Temperature Alert En) : %s\r\n",
                (config_reg & (1 << 1)) ? "ENABLED" : "DISABLED");

    // Bit 0: Voltage Alert Enable
    UART_Printf("[Bit 0]  VALRT_EN (Voltage Alert En)     : %s\r\n",
                (config_reg & (1 << 0)) ? "ENABLED" : "DISABLED");

    UART_Printf("----------------------------------------\r\n\r\n");
}

void Print_MAX1726x_Status(void)
{
    uint16_t status_reg;

    // Read the current value from the status register
    // maxim_max1726x_read_reg(MAX1726X_STATUS_REG, &status_reg);
    MAX17260_Register_ReadSingle(STATUS, &status_reg);

    UART_Printf("\r\n--- MAX1726x Status Register [0x%04X] ---\r\n", status_reg);

    // Check each bit and print its meaning and state (1=SET, 0=NOT SET)

    // Bit 15: Battery Insertion
    UART_Printf("[Bit 15] Bst      (Battery Status Alert) : %s - Set on battery insertion.\r\n",
                (status_reg & (1 << 15)) ? "SET" : "NOT SET");

    // Bit 14: Reserved
    // UART_Printf("[Bit 14] Sc       (State of Charge Alert): %s - 1%% SOC change or charging termination.\r\n",
    //             (status_reg & (1 << 14)) ? "SET" : "NOT SET");

    // Bit 13: Peak Current Alert
    UART_Printf("[Bit 13] PA       (Peak Current Alert)   : %s - Sustained peak current exceeded.\r\n",
                (status_reg & (1 << 13)) ? "SET" : "NOT SET");

    // Bit 12: Temperature Alert
    UART_Printf("[Bit 12] TA       (Temperature Alert)    : %s - Temperature threshold exceeded.\r\n",
                (status_reg & (1 << 12)) ? "SET" : "NOT SET");

    // Bit 11: Voltage Alert
    UART_Printf("[Bit 11] VA       (Voltage Alert)        : %s - Voltage threshold exceeded.\r\n",
                (status_reg & (1 << 11)) ? "SET" : "NOT SET");

    // Bit 10: Current Alert
    UART_Printf("[Bit 10] CA       (Current Alert)        : %s - Current threshold exceeded.\r\n",
                (status_reg & (1 << 10)) ? "SET" : "NOT SET");

    // Bit 9: Delta SOC Interrupt
    UART_Printf("[Bit 9]  dSOCi    (Delta SOC Interrupt)  : %s - RepSOC changed by 1%%.\r\n",
                (status_reg & (1 << 9)) ? "SET" : "NOT SET");

    // Bit 8: Min Current Alert
    UART_Printf("[Bit 8]  Imn      (Min Current Alert)    : %s - Current fell below min threshold.\r\n",
                (status_reg & (1 << 8)) ? "SET" : "NOT SET");

    // Bit 7: Max Current Alert
    UART_Printf("[Bit 7]  Imx      (Max Current Alert)    : %s - Current rose above max threshold.\r\n",
                (status_reg & (1 << 7)) ? "SET" : "NOT SET");

    // Bit 6: Min Temperature Alert
    UART_Printf("[Bit 6]  Tmn      (Min Temp Alert)       : %s - Temp fell below min threshold.\r\n",
                (status_reg & (1 << 6)) ? "SET" : "NOT SET");

    // Bit 5: Max Temperature Alert
    UART_Printf("[Bit 5]  Tmx      (Max Temp Alert)       : %s - Temp rose above max threshold.\r\n",
                (status_reg & (1 << 5)) ? "SET" : "NOT SET");

    // Bit 4: Min Voltage Alert
    UART_Printf("[Bit 4]  Vmn      (Min Voltage Alert)    : %s - Voltage fell below min threshold.\r\n",
                (status_reg & (1 << 4)) ? "SET" : "NOT SET");

    // Bit 3: Max Voltage Alert
    UART_Printf("[Bit 3]  Vmx      (Max Voltage Alert)    : %s - Voltage rose above max threshold.\r\n",
                (status_reg & (1 << 3)) ? "SET" : "NOT SET");

    // Bit 2: State of Charge Alert
    UART_Printf("[Bit 2]  dSOC     (SOC Alert)            : %s - RepSOC crossed main SOC threshold.\r\n",
                (status_reg & (1 << 2)) ? "SET" : "NOT SET");

    // Bit 1: Power On Reset
    UART_Printf("[Bit 1]  POR      (Power On Reset)       : %s - Chip has reset since last cleared.\r\n",
                (status_reg & (1 << 1)) ? "SET" : "NOT SET");

    // Bit 0: Battery Presence
    UART_Printf("[Bit 0]  BATT_PRES(Battery Presence)     : %s - Indicates if battery is present.\r\n",
                (status_reg & (1 << 0)) ? "PRESENT" : "ABSENT");

    UART_Printf("----------------------------------------\r\n\r\n");
}

void MAX17260_Register_printout(void)
{
    Print_MAX1726x_Status();
    Print_MAX1726x_Config();
    Print_MAX1726x_Config2();
}
/**
 * @brief Performs the initial configuration of the MAX17260.
 *
 * This sequence is extracted from the `BATTSENSOR_LWInit` function and is
 * critical for setting up the device's model and configuration.
 */
static void LWInit(void) {
    int16_t value;

    // Exit hibernate mode sequence
    MAX17260_Register_WriteSingle(SOFT_WAKEUP, 0x90);
    MAX17260_Register_WriteSingle(HIB_CFG, 0x00);
    MAX17260_Register_WriteSingle(SOFT_WAKEUP, 0x00);

    // Initial config values from original file
    MAX17260_Register_WriteSingle(DESIGN_CAP, 0x7FF8);
    MAX17260_Register_WriteSingle(I_CHG_TERM, 0x0000);
    MAX17260_Register_WriteSingle(V_EMPTY, 0x9661);

    // Write to ModelCfg and wait for it to clear
    MAX17260_Register_WriteSingle(MODEL_CFG, 0x8000);
    do {
        MAX17260_Register_ReadSingle(MODEL_CFG, (uint16_t*)&value);
    } while (value != 0x0000);

    uint16_t status_val;
    MAX17260_Register_ReadSingle(STATUS, &status_val);
    MAX17260_Register_WriteSingle(STATUS, status_val & ~(1 << 1)); // Clear POR bit

    MAX17260_Register_WriteSingle(HIB_CFG, 0x870C);
    
    // Write final configuration
    MAX17260_Register_WriteSingle(CONFIG2, 0x0658);
    MAX17260_Register_WriteSingle(MODEL_CFG, 0x8000);
}

//=============================================================================
// PUBLIC FUNCTION IMPLEMENTATIONS
//=============================================================================

void BatteryMonitor_Init(void) {
    i2c_init();

    // Small delay to ensure the sensor is ready after power-up.
    delay_ms(10);

    if (IsPOR()) {
        LWInit();
        ClearPOR();
    }
}

uint16_t BatteryMonitor_GetQH(void) {
    uint16_t qh_value = 0;
    MAX17260_Register_ReadSingle(QH, &qh_value);
    return qh_value;
}
