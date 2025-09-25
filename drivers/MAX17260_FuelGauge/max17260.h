/*
 * MAX17260.h
 *
 *  Created on: 19 Jan 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef SENSOR_MAX17260_H_
#define SENSOR_MAX17260_H_

#include "main.h"

#define MAX17260_CFG_I2C_ADDRESS 		0x6C
#define MAX17260_CFG_I2C_TIMEOUT_MAX    0x2000

typedef enum {
        STATUS = 0x00,
        V_ALRT_TH,
        T_ALRT_TH,
        S_ALRT_TH,
        AT_RATE,
        REP_CAP,
        REP_SOC,
        AGE,
        TEMP,
        V_CELL,
        CURRENT,
        AVG_CURRENT,
        Q_RESIDUAL,
        MIX_SOC,
        AV_SOC,
        MIX_CAP,
        FULL_CAP_REP,
        TTE,
        QR_TABLE_00,
        FULL_SOC_THR,
        R_CELL,
        AVG_TA = 0x16,
        CYCLES,
        DESIGN_CAP,
        AVG_V_CELL,
        MAX_MIN_TEMP,
        MAX_MIN_VOLT,
        MAX_MIN_CURR,
        CONFIG,
        I_CHG_TERM,
        AV_CAP,
        TTF,
        DEV_NAME,
        QR_TABLE_10,
        FULL_CAP_NOM,
        AIN = 0x27,
        LEARN_CFG,
        FILTER_CFG,
        RELAX_CFG,
        MISC_CFG,
        T_GAIN,
        T_OFF,
        C_GAIN,
        C_OFF,
        QR_TABLE_20 = 0x32,
        DIE_TEMP = 0x34,
        FULL_CAP,
        R_COMP0 = 0x38,
        TEMP_CO,
        V_EMPTY,
        F_STAT = 0x3D,
        TIMER,
        SHDN_TIMER,
        USER_MEM1,
        QR_TABLE_30 = 0x42,
        R_GAIN,
        DQ_ACC = 0x45,
        DP_ACC,
        CONVG_CFG = 0x49,
        VF_REM_CAP,
        QH = 0x4D,
		SOFT_WAKEUP = 0x60,
        STATUS_2 = 0xB0,
        POWER,
        ID_USER_MEM2,
        AVG_POWER,
        I_ALRT_TH,
        TTF_CFG,
        CV_MIX_CAP,
        CV_HALF_TIME,
        CG_TEMP_CO,
        CURVE,
        HIB_CFG,
        CONFIG2,
        V_RIPPLE,
        RIPPLE_CFG,
        TIMER_H,
        R_SENSE_USER_MEM3 = 0xD0,
        SC_OCV_LIM,
        V_GAIN,
        SOC_HOLD,
        MAX_PEAK_POWER,
        SUS_PEAK_POWER,
        PACK_RESISTANCE,
        SYS_RESISTANCE,
        MIN_SYS_VOLTAGE,
        MPP_CURRENT,
        SPP_CURRENT,
        MODEL_CFG,
        AT_Q_RESIDUAL,
        AT_TTE,
        AT_AV_SOC,
        AT_AV_CAP
}MAX17260_Reg_t;

ErrorStatus MAX17260_Register_WriteSingle(uint8_t _reg, uint16_t _value);
ErrorStatus MAX17260_Register_ReadSingle(uint8_t _reg, uint16_t *_value);
uint16_t MAX17260_Register_ReadSingleFast(uint8_t _reg);
void MAX17260_Callback_Set(void* _callback);
void MAX17260_Init(void);
void MAX17260_Task(void);
uint8_t MAX17260_TaskState(void);

#endif /* SENSOR_MAX17260_H_ */
