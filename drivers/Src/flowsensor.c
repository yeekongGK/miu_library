/******************************************************************************
 * File:        flowsensor.c
 * Author:      CYK
 * Created:     05-10-2025
 * Last Update: 05-10-2025
 *
 * Description:
 *   This file implements the logic for flow sensing and analysis. It
 *   processes data from the pulser driver to calculate flow rates and detect
 *   various conditions such as backflow, pipe bursts, no-flow, and leakage.
 *   It operates as a periodic task to monitor flow status and raise flags when
 *   predefined thresholds are met.
 *
 * Notes:
 *   - Relies on the Pulser and System modules for input data and timing.
 *
 * To Do:
 *   - -
 *
 ******************************************************************************/

#include "common.h"
#include "flowsensor.h"
#include "pulser.h"

static void FLOWSENSOR_SetTimeout_s(uint32_t _s);
static bool FLOWSENSOR_IsTimeout(void);

static FLOWSENSOR_t *pConfig;

static void FLOWSENSOR_SetTimeout_s(uint32_t _s)
{
	SYS_SetTimeout(FLOWSENSOR_TaskId, _s* 1000);
}

static bool FLOWSENSOR_IsTimeout(void)
{
	return SYS_IsTimeout(FLOWSENSOR_TaskId);
}

static void FLOWSENSOR_BackFlow_Detect(void)
{
	int32_t _currValue= PULSER_Value_Get();
	PULSER_CounterDirection_t _currDir=  PULSER_Direction();
	static uint32_t _prevBackflowValue= 0;
	static uint32_t _remainingBackflowValue= 0;

	if(UNKNOWN_CounterDirection== pConfig->rte.backflow.prevDirection)
	{
		if(FORWARD_CounterDirection== _currDir)
		{
			pConfig->rte.backflow.prevDirection= BACKWARD_CounterDirection;
			pConfig->rte.backflow.prevValue= pConfig->rte.backflow.prevMarker= _currValue- 1;
		}
		else if(BACKWARD_CounterDirection== _currDir)
		{
			pConfig->rte.backflow.prevDirection= FORWARD_CounterDirection;
			pConfig->rte.backflow.prevValue= pConfig->rte.backflow.prevMarker= _currValue+ 1;
		}
	}

	if(BACKWARD_CounterDirection== _currDir)
	{
		if(FORWARD_CounterDirection== pConfig->rte.backflow.prevDirection)/*backward transition*/
		{
			pConfig->rte.backflow.prevDirection= _currDir;
			if((TRACSENS_Mode== PULSER_Mode_Get())|| (TRACSENSi_Mode== PULSER_Mode_Get()))
			{
				/*due to error pattern mechanism, the first pulse detected for backward flow is not reported as backward direction.
				 * Only the second pulse and above. Thus we need to compensate this by adding one pulse to get true backflow pulse.*/
				pConfig->rte.backflow.prevMarker+= 1;
			}
		}

		uint32_t _thisBackflowValue= (pConfig->rte.backflow.prevMarker- _currValue);

		pConfig->rte.backflow.currValue= _thisBackflowValue+ _remainingBackflowValue;

		if(_prevBackflowValue< _thisBackflowValue)
		{
			pConfig->rte.backflow.totalPulse+= (_thisBackflowValue- _prevBackflowValue);
			_prevBackflowValue= _thisBackflowValue;
		}

		if((pConfig->rte.backflow.currValue- pConfig->rte.backflow.valueMarker)>= pConfig->backflowThreshold)
		{
			pConfig->status.backflowFlag= true;
			pConfig->status.backflowDetected= true;
		}
	}
	else if(FORWARD_CounterDirection== _currDir)
	{
		if(BACKWARD_CounterDirection== pConfig->rte.backflow.prevDirection)/*forward transition*/
		{
			_prevBackflowValue= 0;
			pConfig->rte.backflow.prevDirection= _currDir;
			pConfig->status.backflowDetected= false;
		}

		if(_currValue> pConfig->rte.backflow.prevValue)
		{
			int32_t _currBackflowValue= pConfig->rte.backflow.currValue- (_currValue- pConfig->rte.backflow.prevValue);
			_remainingBackflowValue= pConfig->rte.backflow.currValue= (_currBackflowValue> 0)? _currBackflowValue: 0;
			pConfig->rte.backflow.valueMarker= pConfig->rte.backflow.currValue;
		}

		pConfig->rte.backflow.prevMarker= _currValue;
	}

	pConfig->rte.backflow.prevValue= _currValue;
}

bool FLOWSENSOR_BackFlow_GetFlag(void)
{
	return pConfig->status.backflowFlag;
}

uint32_t FLOWSENSOR_BackFlow_GetCurrValue(void)
{
	return pConfig->rte.backflow.currValue;
}

void FLOWSENSOR_BackFlow_SetFlag(bool _flag)
{
	if(false== _flag)
	{
		pConfig->rte.backflow.valueMarker= pConfig->rte.backflow.currValue;
	}
	pConfig->status.backflowFlag= _flag;
}

uint8_t FLOWSENSOR_BackFlow_GetDetected(void)
{
	return pConfig->status.backflowDetected;
}

static void FLOWSENSOR_Burst_Detect(void)
{
	if(pConfig->rte.burst.samplingCount> (pConfig->burstSamplingPeriod_s/ pConfig->samplingPeriod_s))
	{
		/*raise flag*/
		pConfig->status.burstFlag= true;
		pConfig->status.burstDetected= true;
		pConfig->rte.burst.samplingCount= 0;/*reset*/
	}
	else if(pConfig->burstThreshold> (pConfig->status.flowrate* 3600))
	{
		pConfig->status.burstDetected= false;
		pConfig->rte.burst.samplingCount= 0;/*reset*/
	}
	else
	{
		pConfig->rte.burst.samplingCount++;
	}
}

bool FLOWSENSOR_Burst_GetFlag(void)
{
	return pConfig->status.burstFlag;
}

void FLOWSENSOR_Burst_SetFlag(bool _flag)
{
	pConfig->status.burstFlag= _flag;
}

uint8_t FLOWSENSOR_Burst_GetDetected(void)
{
	return pConfig->status.burstDetected;
}

static void FLOWSENSOR_Noflow_Detect(void)
{
	if(pConfig->rte.noflow.samplingCount> (pConfig->noflowSamplingPeriod_s/ pConfig->samplingPeriod_s))
	{
		/*raise flag*/
		pConfig->status.noflowFlag= true;
		pConfig->status.noflowDetected= true;
		pConfig->rte.noflow.samplingCount= 0;/*reset*/
	}
	else if(0!= pConfig->status.flowrate)
	{
		pConfig->status.noflowDetected= false;
		pConfig->rte.noflow.samplingCount= 0;/*reset*/
	}
	else
	{
		pConfig->rte.noflow.samplingCount++;
	}
}

bool FLOWSENSOR_Noflow_GetFlag(void)
{
	return pConfig->status.noflowFlag;
}

void FLOWSENSOR_Noflow_SetFlag(bool _flag)
{
	pConfig->status.noflowFlag= _flag;
}

uint8_t FLOWSENSOR_Noflow_GetDetected(void)
{
	return pConfig->status.noflowDetected;
}

static void FLOWSENSOR_Leakage_Detect(void)
{
	if(pConfig->rte.leakage.samplingCount> (pConfig->leakageSamplingPeriod_s/ pConfig->samplingPeriod_s))
	{
		/*raise flag*/
		pConfig->status.leakageFlag= true;
		pConfig->status.leakageDetected= true;
		pConfig->rte.leakage.samplingCount= 0;/*reset*/
	}
	else if(
			(pConfig->leakageThreshold< (pConfig->status.flowrateHourly* 3600))
			|| (0>= pConfig->status.flowrateHourly)
			)
	{
		pConfig->status.leakageDetected= false;
		pConfig->rte.leakage.samplingCount= 0;/*reset*/
	}
	else
	{
		pConfig->rte.leakage.samplingCount++;
	}
}

bool FLOWSENSOR_Leakage_GetFlag(void)
{
	return pConfig->status.leakageFlag;
}

void FLOWSENSOR_Leakage_SetFlag(bool _flag)
{
	pConfig->status.leakageFlag= _flag;
}

uint8_t FLOWSENSOR_Leakage_GetDetected(void)
{
	return pConfig->status.leakageDetected;
}

float FLOWSENSOR_Get(FLOWSENSOR_FlowStats_t _type)
{
	float _value= -1;

	switch(_type)
	{
		case FLOWRATE_FlowStats:
			_value= pConfig->status.flowrate;
			break;

		case FLOWRATEMIN_FlowStats:
			_value= pConfig->status.flowrateMin;
			break;

		case FLOWRATEMAX_FlowStats:
			_value= pConfig->status.flowrateMax;
			break;

		case FLOWRATEHOURLY_FlowStats:
			_value= pConfig->status.flowrateHourly;
			break;

		case BACKFLOW_FlowStats:
			_value= pConfig->rte.backflow.currValue;
			break;

		case BACKFLOWMAX_FlowStats:
			_value= pConfig->status.backflowMax;
			break;

		case BACKFLOWFLAG_FlowStats:
			_value= pConfig->status.backflowFlag;
			break;

		case BURSTFLAG_FlowStats:
			_value= pConfig->status.burstFlag;
			break;

		case NOFLOWFLAG_FlowStats:
			_value= pConfig->status.noflowFlag;
			break;

		case LEAKAGEFLAG_FlowStats:
			_value= pConfig->status.leakageFlag;
			break;

		default:
			break;
	}

	return _value;
}

void FLOWSENSOR_SetFlag(FLOWSENSOR_FlowStats_t _type, bool _flag)
{
	switch(_type)
	{
		case BACKFLOWFLAG_FlowStats:
			FLOWSENSOR_BackFlow_SetFlag(_flag);
			break;

		case BURSTFLAG_FlowStats:
			FLOWSENSOR_Burst_SetFlag(_flag);
			break;

		case NOFLOWFLAG_FlowStats:
			FLOWSENSOR_Noflow_SetFlag(_flag);
			break;

		case LEAKAGEFLAG_FlowStats:
			FLOWSENSOR_Leakage_SetFlag(_flag);
			break;

		default:
			break;
	}
}

bool FLOWSENSOR_GetFlag(FLOWSENSOR_FlowStats_t _type)
{
	switch(_type)
	{
		case BACKFLOWFLAG_FlowStats:
			return FLOWSENSOR_BackFlow_GetFlag();
		case BURSTFLAG_FlowStats:
			return FLOWSENSOR_Burst_GetFlag();
		case NOFLOWFLAG_FlowStats:
			return FLOWSENSOR_Noflow_GetFlag();
		case LEAKAGEFLAG_FlowStats:
			return FLOWSENSOR_Leakage_GetFlag();
		default:
			break;
	}

	return false;
}

void FLOWSENSOR_Init(FLOWSENSOR_t *_config)
{
	pConfig= _config;

//	pConfig->enabled= true;
//	pConfig->samplingPeriod_s= 5;//config.sensors.flow.samplingPeriod_s* 1000;
//	pConfig->status.flowrate= 0;
//	pConfig->status.flowrateMin= 0;
//	pConfig->status.flowrateMax= 0;
//	pConfig->status.flowrateHourly= 0;
//	pConfig->status.backflow= 0;
//	pConfig->status.backflowMax= 0;
//	pConfig->samplingTimeMultiplier= 1;/*default is 1*/
//	pConfig->rte.prevReading_l= PULSER_ValueInLiter_Get();
//	pConfig->rte.backflow.prevValue= PULSER_Value_Get();
//	pConfig->rte.backflow.prevMarker= PULSER_Value_Get();
//	pConfig->rte.backflow.prevDirection= PULSER_Direction();
//	pConfig->rte.backflow.valueMarker= 0;
//	pConfig->rte.backflow.currValue= 0;
//	pConfig->status.backflowFlag= false;
//	pConfig->status.backflowDetected= false;

	if((false== pConfig->enabled)|| (0== pConfig->samplingPeriod_s))
	{
		FLOWSENSOR_SetTimeout_s(1);/*immediate sampling*/
	}
}

void FLOWSENSOR_Task(void)
{
	if((false== pConfig->enabled)|| (0== pConfig->samplingPeriod_s))
	{
		return;
	}

	/*calculate flowrate*/
	if(true== FLOWSENSOR_IsTimeout())
	{
		FLOWSENSOR_SetTimeout_s(pConfig->samplingPeriod_s);

		__IO float _currentReading_l= PULSER_ValueInLiter_Get();

		pConfig->status.flowrate= (_currentReading_l- pConfig->rte.prevReading_l)/ (pConfig->samplingPeriod_s);

		if(pConfig->rte.prevReading_l!= _currentReading_l)
		{
			pConfig->status.flowrateHourly=
					(_currentReading_l- pConfig->rte.prevReading_l)
					/ ((pConfig->samplingPeriod_s* pConfig->samplingTimeMultiplier)/ 1.0);
			pConfig->samplingTimeMultiplier= 1;
		}
		else
		{
			if(3600 <(pConfig->samplingPeriod_s* pConfig->samplingTimeMultiplier))
			{
				/*if more than one hour no pulse we reset slow rate*/
				pConfig->status.flowrateHourly= 0;
				pConfig->samplingTimeMultiplier= 1;
			}
			else
			{
				/*no pulse diff since previous sample so we wait more*/
				pConfig->samplingTimeMultiplier++;
			}
		}
		pConfig->rte.prevReading_l= _currentReading_l;

		if(pConfig->status.flowrate> pConfig->status.flowrateMax)
		{
			pConfig->status.flowrateMax= pConfig->status.flowrate;
		}

		if(
				(0< pConfig->status.flowrate)&&/*we don't want zero and negative rate*/
				((pConfig->status.flowrate< pConfig->status.flowrateMin)|| (0== pConfig->status.flowrateMin))/*we also don't want minRate to be zero*/
		  )
		{
			pConfig->status.flowrateMin= pConfig->status.flowrate;
		}

		FLOWSENSOR_Burst_Detect();
		FLOWSENSOR_Noflow_Detect();
		FLOWSENSOR_Leakage_Detect();

//		DBG_Print(">>>curr:%f\r\n currentBf:%d\r\n flow:%f slowflow:%f\r\n",
//				_currentReading_l, pConfig->rte.backflow.currValue, pConfig->status.flowrate* 3600, pConfig->status.flowrateHourly* 3600);
	}

	FLOWSENSOR_BackFlow_Detect();/*backflow need to be detected in real time, not periodically*/

}

uint8_t FLOWSENSOR_TaskState(void)
{
	return SLEEP_TaskState;
}
