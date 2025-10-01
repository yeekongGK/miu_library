/*
 * diagnostic.h
 *
 *  Created on: 27 Mar 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef DIAG_H_
#define DIAG_H_

#include "main.h"
#include "sensor.h"
#include "queue.h"

#define DIAG_CFG_VERSION					0x01
#define DIAG_CFG_ENTRY_SIZE					(10)
#define DIAG_CFG_MAX_ENTRY					(1200)
#define DIAG_CFG_BUFFER_SIZE				(DIAG_CFG_ENTRY_SIZE* DIAG_CFG_MAX_ENTRY)/*SRAM2, 16K. We use one entry as overflow buffer*/

/*DIAG Message packet format
 * 1) 2 bit  value type 0b00:Uint32_t 0b01:Float 0b10:RFU 0b11:RFU
 * 2) 14 bit diag code <6bit: task id><8bit: code>
 * 3) 4 byte time()
 * 4) 4 bytes value (for packet type 0b01)
 * */
typedef enum
{
	/*[Description("NOTASK:Invalid DCode")]*/
	START_NoTaskDCode= (0b00111111& NO_TASK_TaskId)<< 8,
	START_SysDCode= (0b00111111& SYS_TaskId)<< 8,
	START_M95M01DCode= (0b00111111& M95M01_TaskId)<< 8,
	/*[Description("M95M01:Read Error")]*/READ_ERROR_M95M01DCode,
	/*[Description("M95M01:Write Error")]*/WRITE_ERROR_M95M01DCode,
	/*[Description("M95M01:Validate Error")]*/VALIDATE_ERROR_M95M01DCode,
	START_BC66PhyDCode= (0b00111111& BC66_PHY_TaskId)<< 8,
	START_BC66LinkDCode= (0b00111111& BC66_LINK_TaskId)<< 8,
	START_PulserDCode= (0b00111111& PULSER_TaskId)<< 8,
	START_ExtiDCode= (0b00111111& EXTI_TaskId)<< 8,
	START_MeterLogDCode= (0b00111111& METER_Log_TaskId)<< 8,
	/*[Description("LOGG:Meter Log")]*/METER_LOG_MeterLogDCode,
	START_NbiotDCode= (0b00111111& NBIOT_TaskId)<< 8,
	/*[Description("NBIT:[8]Modem Reset,[16]Volt(mV),[i8]Temp(C)")]*/MODEM_RESET_NbiotDCode,
	/*[Description("NBIT:[bcs8]BC66 State,[8]BC66 SubState,[16]CME Error")]*/CME_ERROR_NbiotDCode,
	/*[Description("NBIT:SIM Error")]*/SIM_ERROR_NbiotDCode,
	/*[Description("NBIT:Transmission")]*/TXRX_ATTEMPT_NbiotDCode,
	/*[Description("NBIT:Transmission Failed")]*/TXRX_FAILED_NbiotDCode,
	/*[Description("NBIT:[mps8]Modem Power State,[16]Volt(mV),[i8]Temp(C)")]*/MODEM_POWER_STATE_NbiotDCode,
	/*[Description("NBIT:Modem State")]*/MODEM_STATE_NbiotDCode,
	/*[Description("NBIT:Earfcn")]*/EARFCN_NbiotDCode,
	/*[Description("NBIT:[8]EarfcnOffset,[16]Pci")]*/EARFCNOFFSET_PCI_NbiotDCode,
	/*[Description("NBIT:[x32]Cell ID")]*/CELL_ID_NbiotDCode,
	/*[Description("NBIT:[i16]RSRP,[i16]RSRQ")]*/RSRP_RSRQ_NbiotDCode,
	/*[Description("NBIT:[i16]RSSI,[i16]SINR")]*/RSSI_SINR_NbiotDCode,
	/*[Description("NBIT:[16]Band,[16]TAC")]*/BAND_TAC_NbiotDCode,
	/*[Description("NBIT:[8]ECL,[i16]Tx Power,[8]Opr Mode")]*/ECL_TXPOWER_OPRMODE_NbiotDCode,
	/*[Description("NBIT:Sleep Duration")]*/SLEEP_DURATION_NbiotDCode,
	/*[Description("NBIT:Rx Time(s)")]*/RX_TIME_NbiotDCode,
	/*[Description("NBIT:Tx Time(s)")]*/TX_TIME_NbiotDCode,
	/*[Description("NBIT:[ts32]Modem Disabled. Restart Time")]*/MODEM_DISABLED_NbiotDCode,
	/*[Description("NBIT:[me8]Modem Error,[16]Volt(mV),[i8]Temp(C)")]*/MODEM_ERROR_NbiotDCode,
	/*[Description("NBIT:Cereg")]*/CEREG_NbiotDCode,
	/*[Description("NBIT:Notify")]*/NOTIFY_NbiotDCode,
	/*[Description("NBIT:Coap Recovered")]*/COAP_RECOVERED_NbiotDCode,
	/*[Description("NBIT:Lwm2m Recovered")]*/LWM2M_RECOVERED_NbiotDCode,
	START_NbiotGkCoapDCode= (0b00111111& NBIOT_GKCOAP_TaskId)<< 8,
	/*[Description("GKCOAP:[ts32]Report Start Time")]*/REPORT_START_TIME_GKCoapDCode,
	/*[Description("GKCOAP:[ts32]Report Next Time")]*/REPORT_NEXT_TIME_GKCoapDCode,
	/*[Description("GKCOAP:[8]Status Code Changed")]*/STATUS_CODE_CHANGED_GKCoapDCode,
	START_NbiotGkCoapReportDCode= (0b00111111& NBIOT_GKCOAP_REPORT_TaskId)<< 8,
	START_NbiotGkCoapPktDCode= (0b00111111& NBIOT_GKCOAP_PKT_TaskId)<< 8,
	START_NbiotLwm2mDCode= (0b00111111& NBIOT_LWOBJ_TaskId)<< 8,
	/*[Description("LWM2M:[ts32]Reading Next Run Time")]*/READING_NEXT_RUN_TIME_NbiotLwm2mDCode,
	/*[Description("LWM2M:[ts32]Reading Dispatch Dispatch Time")]*/READING_NEXT_DISPATCH_TIME_NbiotLwm2mDCode,
	/*[Description("LWM2M:[ts32]Status Next Run Time")]*/STATUS_NEXT_RUN_TIME_NbiotLwm2mDCode,
	/*[Description("LWM2M:[ts32]Status Next Dispatch Time")]*/STATUS_NEXT_DISPATCH_TIME_NbiotLwm2mDCode,
	/*[Description("LWM2M:[ts32]Query Next Run Time")]*/QUERY_NEXT_RUN_TIME_NbiotLwm2mDCode,
	/*[Description("LWM2M:[ts32]Query Next Dispatch Time")]*/QUERY_NEXT_DISPATCH_TIME_NbiotLwm2mDCode,
	/*[Description("LWM2M:[ts32]Alarm Next Sample Time")]*/ALARM_NEXT_SAMPLE_TIME_NbiotLwm2mDCode,
	/*[Description("LWM2M:[ts32]Alarm Stop Sample Time")]*/ALARM_STOP_SAMPLE_TIME_NbiotLwm2mDCode,
	/*[Description("LWM2M:[us8]Update State, [ur8]Update Result, [as8]Activation State, [ds8]Download State")]*/SWGT_STATE_NbiotLwm2mDCode,
	/*[Description("LWM2M:[ts32]Swmgt Backoff, Next Get Time")]*/SWGT_BACKOFF_NbiotLwm2mDCode,
	/*[Description("LWM2M:[16]Swmgt Current Block")]*/SWGT_CURR_BLOCK_NbiotLwm2mDCode,
	/*[Description("LWM2M:[x32]Swmgt Store Error")]*/SWGT_STORE_ERROR_NbiotLwm2mDCode,
	/*[Description("LWM2M:[x16]Swmgt Calculated, [x16]Expected CRC")]*/SWGT_CRC_ERROR_NbiotLwm2mDCode,
	/*[Description("LWM2M:[4]Ack,[4]Status,[4],[4],[16]Notify")]*/NOTIFY_NbiotLwm2mDCode,
	/*[Description("LWM2M:[4]Index,[4]Status,[4],[4],[16]Read")]*/READ_RESPONSE_NbiotLwm2mDCode,
	/*[Description("LWM2M:[4]Index,[4]Status,[4],[4],[16]Observe")]*/OBSERVE_RESPONSE_NbiotLwm2mDCode,
	/*[Description("LWM2M:[4]Result,[4]Status,[4],[4],[16]Write")]*/WRITE_RESPONSE_NbiotLwm2mDCode,
	/*[Description("LWM2M:[4]Result,[4]Status,[4],[4],[16]Execute")]*/EXECUTE_RESPONSE_NbiotLwm2mDCode,
	/*[Description("LWM2M:[4]Status,[4]Type,[4],[4],[16]Request")]*/REQUEST_NbiotLwm2mDCode,
	START_NFCDCode= (0b00111111& NFC_TaskId)<< 8,
	/*[Description("NFC:Init Failed")]*/INIT_FAILED_NFCDCode,
	START_RTCDCode= (0b00111111& RTC_TaskId)<< 8,
	START_MSGDCode= (0b00111111& MSG_TaskId)<< 8,
	START_WMBUSDCode= (0b00111111& WMBUS_TaskId)<< 8,
	START_DbgDCode= (0b00111111& DBG_TaskId)<< 8,
	START_SensorDCode= (0b00111111& SENSOR_TaskId)<< 8,
	/*[Description("SNSR:[f32]Average Current (uA)")]*/AVECURRENT_UA_SensorDCode,
	/*[Description("SNSR:[16]Volt(mV),[i16]Temp(C)")]*/VOLT_TEMP_MV_C_SensorDCode,
	/*[Description("SNSR:[x16]SOFT_WAKEUP,[x16]HIB_CFG")]*/ SOFT_WAKEUP_HIB_CFG_SensorDCode,
	/*[Description("SNSR:[x16]DESIGN_CAP,[x16]I_CHG_TERM")]*/ DESIGN_CAP_I_CHG_TERM_SensorDCode,
	/*[Description("SNSR:[x16]V_EMPTY,[x16]LEARN_CFG")]*/ V_EMPTY_LEARN_CFG_SensorDCode,
	/*[Description("SNSR:[x16]FILTER_CFG,[x16]C_OFF")]*/ FILTER_CFG_C_OFF_SensorDCode,
	/*[Description("SNSR:[x16]C_GAIN,[x16]FULL_CAP")]*/ C_GAIN_FULL_CAP_SensorDCode,
	/*[Description("SNSR:[x16]R_CELL")]*/ R_CELL_SensorDCode,
	/*[Description("SNSR:[x16]TIMER,[x16]TIMER_H")]*/ TIMER_TIMER_H_SensorDCode,
	/*[Description("SNSR:[x16]CONFIG,[x16]CONFIG2")]*/ CONFIG_CONFIG2_SensorDCode,
	/*[Description("SNSR:[x16]MODEL_CFG,[x16]QH")]*/ MODEL_CFG_QH_SensorDCode,
	/*[Description("SNSR:[x16]To, [x16]QH Overflowed From")]*/ QH_OVERFLOWED_SensorDCode,
	/*[Description("SNSR:[32]QH Multiplier")]*/ QH_MULTIPLIER_SensorDCode,
	START_FlowSensorDCode= (0b00111111& FLOWSENSOR_TaskId)<< 8,
	START_McuAdcDCode= (0b00111111& MCUADC_TaskId)<< 8,
	/*[Description("MCUADC:Init Error> Disable Timeout")]*/DISABLE_TIMEOUT_MCUADCDCode,
	/*[Description("MCUADC:Init Error> Calibration Timeout")]*/CALIBRATION_TIMEOUT_MCUADCDCode,
	/*[Description("MCUADC:Init Error> AD Ready Timeout")]*/AD_READY_TIMEOUT_MCUADCDCode,
	START_DiagDCode= (0b00111111& DIAG_TaskId)<< 8,
	/*[Description("DIAG:RTC TIME")]*/RTC_TIMESTAMP_MS_DiagDCode,
	START_AlarmDCode= (0b00111111& ALARM_TaskId)<< 8,
	START_FailsafeDCode= (0b00111111& FAILSAFE_TaskId)<< 8,
	/*[Description("FLSV:[x32]MCU Reset> NMI")]*/NMI_RESET_FailsafeDCode,
	/*[Description("FLSV:[x32]MCU Reset> Hard Fault")]*/HARDFAULT_RESET_FailsafeDCode,
	/*[Description("FLSV:[x32]MCU Reset> MEMANAGE")]*/MEMANAGE_RESET_FailsafeDCode,
	/*[Description("FLSV:[x32]MCU Reset> Bus Fault")]*/BUSFAULT_RESET_FailsafeDCode,
	/*[Description("FLSV:[x32]MCU Reset> Usage Fault")]*/USAGEFAULT_RESET_FailsafeDCode,
	/*[Description("FLSV:[x32]MCU Reset> SVC_HANDLER")]*/SVC_HANDLER_RESET_FailsafeDCode,
	/*[Description("FLSV:[x32]MCU Reset> DEBUG_MON")]*/DEBUG_MON_RESET_FailsafeDCode,
	/*[Description("FLSV:[x32]MCU Reset> PENDSV")]*/PENDSV_RESET_FailsafeDCode,
	/*[Description("FLSV:[x32]MCU Reset> Software Watchdog")]*/SWDG_RESET_FailsafeDCode,
	/*[Description("FLSV:[x32]MCU Reset> User")]*/USER_RESET_FailsafeDCode,
	/*[Description("FLSV:[x32]MCU Reset> User Shutdown")]*/SHUTDOWN_RESET_FailsafeDCode,
	/*[Description("FLSV:[x32]MCU Reset> Firmware Update")]*/FWU_RESET_FailsafeDCode,
	/*[Description("FLSV:[x32]MCU Reset> Config Corrupted")]*/FS_CORR_CFG_RESET_FailsafeDCode,
	/*[Description("FLSV:[x32]MCU Reset> PVD/PVM")]*/PVD_PVM_RESET_FailsafeDCode,
	/*[Description("FLSV:[x32]MCU Reset> Firewall")]*/FIREWALL_RESET_FailsafeDCode,
	/*[Description("FLSV:[x32]MCU Reset> Option Byte Load")]*/OB_LOAD_RESET_FailsafeDCode,
	/*[Description("FLSV:[x32]MCU Reset> Reset Pin")]*/NRST_PIN_RESET_FailsafeDCode,
	/*[Description("FLSV:[x32]MCU Reset> Brown Out")]*/BOR_RESET_FailsafeDCode,
	/*[Description("FLSV:[x32]MCU Reset> Internal Watchdog")]*/IWDG_RESET_FailsafeDCode,
	/*[Description("FLSV:[x32]MCU Reset> Window Watchdog")]*/WWDG_RESET_FailsafeDCode,
	/*[Description("FLSV:[x32]MCU Reset> Low Power")]*/LOW_POWER_RESET_FailsafeDCode,
	/*[Description("FLSV:RTC Drift(s)")]*/RTC_DRIFT_S_FailsafeDCode,
	/*[Description("FLSV:BC66 State Unchanged")]*/BC66_STATE_UNCHANGED_FailsafeDCode,
	/*[Description("FLSV:GKCOAP Hang")]*/GKCOAP_HANG_FailsafeDCode,
	/*[Description("FLSV:LWM2M Readings Not Dispatched")]*/LWM2M_READINGS_NOT_DISPATCHED_FailsafeDCode,
	/*[Description("FLSV:[tm32]Task Sleep Status Bitmap")]*/TASK_SLEEP_STATUS_FailsafeDCode,
}DIAG_DCode_t;

typedef enum
{
	NONE_DiagTLVTag= 0,
	CODE_DiagTLVTag,
}SYS_DiagTLVTag_t;

/*TO EXPORT OUT TO CFG*/
typedef struct __attribute__((aligned(8)))/*compulsory alignment*/
{
	uint32_t checksum;/*compulsory checksum*/
	uint8_t version;

	QUEUE_FIFO_t queue;

	uint16_t rteHardfaultRebootCount;
	uint16_t rteFailsafeRebootCount;
	uint16_t rtePVDRebootCount;
	uint16_t rteBORCount;
	uint16_t rteWatchdogRebootCount;
	uint16_t rteShutdownCount;
	uint16_t rteRebootCount;
	uint16_t rteVRefDippedCount;
	uint16_t rteNbModemSelfResetCount;

	uint8_t reserve[128];/*to add more param reduce this, thus no need to do backward compatible thing. remember to set the default value after config*/
}DIAG_t;

void DIAG_Code(uint16_t _dcode, uint32_t _value);
void DIAG_Code_f(uint16_t _dcode, float _value);
uint16_t DIAG_Code_Pop(uint8_t *_buffer, uint16_t _count);
uint16_t DIAG_Code_UnPop(uint16_t _backcount);
void DIAG_Reset(void);
void DIAG_TLVRequest(TLV_t *_tlv);
void DIAG_Init(DIAG_t *_config);
void DIAG_Task(void);
uint8_t DIAG_TaskState(void);

#endif /* DIAG_H_ */
