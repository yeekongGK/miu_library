/*
 * gkcoap_packet.h
 *
 *  Created on: 14 Jun 2021
 *      Author: muhammad.ahmad@georgekent.net
 */

#ifndef NBIOT_GKCOAP_GKCOAPPACKET_H_
#define NBIOT_GKCOAP_GKCOAPPACKET_H_

#include "main.h"

#define GKCOAPPKT_CFG_MSG_BUFFER_SIZE		 		256

uint16_t GKCOAPPKT_PopulateRegisterPacketSample(uint8_t *_payloadBuf);
uint16_t GKCOAPPKT_PopulateReportingPacketSample(uint8_t *_payloadBuf);
uint16_t GKCOAPPKT_PopulateRegisterPacket(uint8_t *_payloadBuf);
uint16_t GKCOAPPKT_PopulateFotaPacket(uint8_t _type, uint32_t _currPacket, char * _version, uint8_t *_payloadBuf);
uint16_t GKCOAPPKT_PopulateMsgPacket(uint8_t *_payloadBuf, uint8_t *_msg, uint16_t _msgLen);
ErrorStatus GKCOAPPKT_ProcessDownlinkPacket(uint8_t *_rxBuffer, uint32_t _rxLen);

#endif /* NBIOT_GKCOAP_GKCOAPPACKET_H_ */
