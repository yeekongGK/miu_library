/*
 * radiolog.h
 *
 *  Created on: 13 Jan 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef LOGGER_RADIOLOG_H_
#define LOGGER_RADIOLOG_H_

#include "main.h"

typedef struct
{
	/*6 bytes timestamp*/
	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;
	uint8_t date;
	uint8_t month;
	uint8_t year;

	/*1 byte npsmr*/
	uint8_t npsmr;

	/*6 bytes cereg info*/
	uint8_t regStat;
	uint8_t regTac[2];
	//uint8_t regRejectType;/*no need*/
	uint8_t regRejectCause;
	uint8_t regActiveTime;
	uint8_t regPeriodicTAU;

	/*27 bytes nuestats=radio info*/
	uint8_t radSignalPower[2];/*rsrp*///from -140 dBm to – 44
	uint8_t radTotalPower[2];/*rssi*/
	uint8_t radTxPower[2];
	uint8_t radTxTime[4];
	uint8_t radRxTime[4];
	uint8_t radCellID[4];
	uint8_t radEcl;/*last ecl*/
	uint8_t radSnr[2];/*last snr*/
	uint8_t radEarfcn[2];/*last earfcn*/
	uint8_t radPci[2];/*last pci*/
	uint8_t radRsrq[2];/*last rsrq*///RSRQ is defined from -3…-19.5dB

	/*16   bytes nuestats=bler info*/
//	uint8_t blerRlcUl; tak cukup one byte but this always retyrned 0
//	uint8_t blerRlcDlL;
//	uint8_t blerMacUL;
//	uint8_t blerMacDL;
	uint8_t blerTotalTxBytes[4];
	uint8_t blerTotalRxBytes[4];
	uint8_t blerTotalTxBlocks[2];
	uint8_t blerTotalRxBlocks[2];
	uint8_t blerTotalRtxBlocks[2];
	uint8_t blerTotalAckNack[2];

	/*8   bytes nuestats=thp info*/
	uint8_t thpRlcUl[2];
	uint8_t thpRlcDl[2];
	uint8_t thpMacUl[2];
	uint8_t thpMacDl[2];
} RADIOLOG_Log_t;

#endif /* LOGGER_RADIOLOG_H_ */
