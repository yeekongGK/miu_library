/******************************************************************************
 * File:        max17260.h
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file defines the public interface for the MAX17260 fuel gauge
 *   driver. It includes an enumeration of all accessible registers
 *   (`MAX17260_Reg_t`) with descriptions of their functions. It also provides
 *   function prototypes for initializing the device and retrieving key battery
 *   data, such as the raw coulomb counter value.
 *
 * Notes:
 *   - This header is intended for use by higher-level application modules
 *     that need to interact with the battery monitor.
 *
 * To Do:
 *   - -
 *
 ******************************************************************************/
#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    /* Main Information Registers */
    STATUS        = 0x00, // Contains various status flags, such as Power-On Reset (P   OR) and alert indicators.
                          // This bit must be cleared by system software to detect the next POR event. POR is set to 1 at power-up.
    V_ALRT_TH     = 0x01, // Sets voltage alert thresholds for maximum (MSB) and minimum (LSB) cell voltage.
    T_ALRT_TH     = 0x02, // Sets temperature alert thresholds for maximum (MSB) and minimum (LSB) temperature.
    S_ALRT_TH     = 0x03, // Sets State-of-Charge (SOC) alert thresholds for maximum (MSB) and minimum (LSB) percentage.
    AT_RATE       = 0x04, // A signed value of a hypothetical current used for Time-to-Empty/Full estimations.
    REP_CAP       = 0x05, // Reported remaining capacity of the battery in mAh or mWh.
    REP_SOC       = 0x06, // Reported State-of-Charge percentage of the battery.
    AGE           = 0x07, // Estimated battery age as a percentage of its expected life (100% = new).
    TEMP          = 0x08, // Measures the battery temperature from the thermistor.
    V_CELL        = 0x09, // Instantaneous measured cell voltage.
    CURRENT       = 0x0A, // Instantaneous measured current flowing into or out of the battery.
    AVG_CURRENT   = 0x0B, // Average measured current over a defined period.
    Q_RESIDUAL    = 0x0C, // Actually it is charge inside the cell but unavailable due to conditions (load/temp)
    MIX_SOC       = 0x0D, // The ModelGauge m5 algorithm's best estimate of the State-of-Charge.
    AV_SOC        = 0x0E, // The algorithm's available State-of-Charge, which can be less than MixSOC under heavy loads.
    MIX_CAP       = 0x0F, // The algorithm's best estimate of the remaining capacity.
    FULL_CAP_REP  = 0x10, // Reported full capacity of the battery, adjusted for current load conditions.
    TTE           = 0x11, // Estimated Time-to-Empty under the present discharge rate.
    QR_TABLE_00   = 0x12, // Part of the battery characterization data table (Q-R Table, block 00).
    FULL_SOC_THR  = 0x13, // Configures thresholds in percentage for detecting a fully charged state.
    R_CELL        = 0x14, // Stores the calculated internal resistance of the battery cell.
    AVG_TA        = 0x16, // Average ambient temperature over a defined period.
    CYCLES        = 0x17, // Number of charge/discharge cycles the battery has undergone.
    DESIGN_CAP    = 0x18, // The design capacity of the battery pack.
    AVG_V_CELL    = 0x19, // Average cell voltage over a defined period.
    MAX_MIN_TEMP  = 0x1A, // Logs the maximum and minimum temperatures measured since the last reset.
    MAX_MIN_VOLT  = 0x1B, // Logs the maximum and minimum voltages measured since the last reset.
    MAX_MIN_CURR  = 0x1C, // Logs the maximum and minimum currents measured since the last reset.
    CONFIG        = 0x1D, // Main configuration for alerts, temperature source, and other basic settings.
    I_CHG_TERM    = 0x1E, // Sets the charge termination current threshold.
    AV_CAP        = 0x1F, // Reports the available capacity, which can be less than remaining capacity under heavy loads.
    TTF           = 0x20, // Estimated Time-to-Full under the present charge rate.
    DEV_NAME      = 0x21, // Contains the device ID. Should read as 0x0010.
    QR_TABLE_10   = 0x22, // Part of the battery characterization data table (Q-R Table, block 10).
    FULL_CAP_NOM  = 0x23, // It’s nominal full discharge capacity of the cell, used for long-term learning, but still updated continuously (not truly “fixed”) .
    AIN           = 0x27, // it’s a ratiometric measurement of TH pin vs BATT, used with TGain/TOff/Curve for temperature.
    LEARN_CFG     = 0x28, // Configures the ModelGauge m5 learning algorithm behavior.
    FILTER_CFG    = 0x29, // Configures averaging periods for voltage, current, and temperature measurements.
    RELAX_CFG     = 0x2A, // Configures voltage relaxation detection for Open-Circuit Voltage (OCV) measurements.
    MISC_CFG      = 0x2B, // Miscellaneous configuration for features like enabling nonvolatile memory (NVM).
    T_GAIN        = 0x2C, // Temperature gain for thermistor calibration.
    T_OFF         = 0x2D, // Temperature offset for thermistor calibration.
    C_GAIN        = 0x2E, // Current gain for sense resistor calibration.
    C_OFF         = 0x2F, // Current offset for sense resistor calibration.
    QR_TABLE_20   = 0x32, // Part of the battery characterization data table (Q-R Table, block 20).
    DIE_TEMP      = 0x34, // Internal die temperature of the IC.
    FULL_CAP      = 0x35, // It’s full discharge capacity compensated for present conditions (temperature and load).
    R_COMP0       = 0x38, // A model parameter for cell resistance temperature compensation.
    TEMP_CO       = 0x39, // Temperature coefficient for resistance compensation.
    V_EMPTY       = 0x3A, // Defines the empty voltage point for the battery model.
    F_STAT        = 0x3D, // Contains flags related to the fuel gauge algorithm, such as "Data Not Ready".
    TIMER         = 0x3E, // A 16-bit general-purpose timer (LSB).
    SHDN_TIMER    = 0x3F, // Sets the delay before the device enters shutdown mode.
    USER_MEM1     = 0x40, // General purpose non-volatile user memory block 1.
    QR_TABLE_30   = 0x42, // Part of the battery characterization data table (Q-R Table, block 30).
    R_GAIN        = 0x43, // A model parameter for resistance scaling.
    DQ_ACC        = 0x45, // Change in capacity (mAh) during a charge/discharge cycle.
    DP_ACC        = 0x46, // Change in percentage SOC during a charge/discharge cycle.
    CONVG_CFG     = 0x49, // Configures the convergence rate of the fuel gauge algorithm.
    VF_REM_CAP    = 0x4A, // Remaining capacity estimation from the voltage fuel gauge part of the algorithm.
    QH            = 0x4D, // Raw coulomb counter value, representing the charge accumulated since the last reset.
    SOFT_WAKEUP   = 0x60, // Used to wake the device from hibernate mode.
    STATUS_2      = 0xB0, // Contains additional status flags, including hibernate status
                          // AtRateReady, DPReady, Serial Number Ready, FullDet flags
    POWER         = 0xB1, // Instantaneous calculated power (positive for charging, negative for discharging).
    ID_USER_MEM2  = 0xB2, // General purpose non-volatile user memory block 2.
    AVG_POWER     = 0xB3, // Average calculated power over a defined period.
    I_ALRT_TH     = 0xB4, // Sets current alert thresholds for charge (MSB) and discharge (LSB).
    TTF_CFG       = 0xB5, // Configuration for Time-to-Full calculations.
    CV_MIX_CAP    = 0xB6, // Constant Voltage phase capacity estimation.
    CV_HALF_TIME  = 0xB7, // Constant Voltage phase half-time estimation.
    CG_TEMP_CO    = 0xB8, // Current Gain Temperature Coefficient.
    CURVE         = 0xB9, // A model parameter describing battery discharge curve shape.
    HIB_CFG       = 0xBA, // Configures automatic entry and exit conditions for hibernate mode.
    CONFIG2       = 0xBB, // Secondary configuration register, often related to power modes.
    V_RIPPLE      = 0xBC, // Reports the magnitude of the voltage ripple.
    RIPPLE_CFG    = 0xBD, // Configures the ripple detection algorithm.
    TIMER_H       = 0xBE, // A 16-bit general-purpose timer (MSB). // Corrected from 0xCE
    R_SENSE_USER_MEM3 = 0xD0, // General purpose non-volatile user memory block 3, often for Rsense.
    SC_OCV_LIM    = 0xD1, // Step-charging OCV voltage limit.
    V_GAIN        = 0xD2, // Voltage gain calibration.
    SOC_HOLD      = 0xD3, // Freezes the SOC during specific conditions to prevent large jumps.
    MAX_PEAK_POWER = 0xD4, // Reports the maximum peak power the battery can deliver.
    SUS_PEAK_POWER = 0xD5, // Reports the maximum sustained peak power the battery can deliver.
    PACK_RESISTANCE = 0xD6,// Reports the total pack resistance including cell and contact resistance.
    SYS_RESISTANCE = 0xD7, // Stores the system/board resistance for more accurate IR compensation.
    MIN_SYS_VOLTAGE = 0xD8,// Minimum allowable system voltage for power calculations.
    MPP_CURRENT   = 0xD9, // Current associated with the MaxPeakPower register.
    SPP_CURRENT   = 0xDA, // Current associated with the SusPeakPower register.
    MODEL_CFG     = 0xDB, // Main configuration for the battery model; should be 0x8000 for the EZ model.
    AT_Q_RESIDUAL = 0xDC, // AtRate estimation of QResidual.
    AT_TTE        = 0xDD, // AtRate Time-to-Empty estimation.
    AT_AV_SOC     = 0xDE, // AtRate estimation of AvSOC.
    AT_AV_CAP     = 0xDF, // AtRate estimation of AvCap.
} MAX17260_Reg_t;



/**
 * @brief Initializes the battery monitor IC.
 *
 * This function handles the I2C initialization and configures the MAX17260
 * with the necessary values if a Power-On-Reset (POR) is detected.
 * It must be called once before any other functions in this module.
 */
void BatteryMonitor_Init(void);

/**
 * @brief Gets the raw Coulomb Counter (QH) value.
 *
 * The QH register represents the accumulated charge/discharge of the battery.
 * It is a 16-bit value that rolls over.
 *
 * @return The 16-bit value from the QH register.
 */
uint16_t BatteryMonitor_GetQH(void);

void MAX17260_Register_printout(void);

#endif // BATTERY_MONITOR_H
