/*****************************************************************************
* Title         : EtherShield library for AVR32 etherShield
* Author        : Xing Yu
* Modified by Wolfgang Beck to run on AVR32 MCU's
* Copyright: GPL V2
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*****************************************************************************/

#ifndef ETHERSHIELD_H
#define ETHERSHIELD_H

#include <inttypes.h>
#include "EtherShield/TransportLayer/transport_layer.h"
#include "EtherShield/TransportLayer/net.h"


uint16_t EtherShield_FillTCPData(uint8_t *buf,uint16_t pos, const char *s);
void EtherShield_Init(volatile avr32_spi_t *spi, uint8_t spiDeviceId, spi_flags_t spiFlags, uint32_t spiBaudrate, uint8_t* macAddress, uint8_t *ipAddress, uint8_t port);
void EtherShield_SetClock(uint8_t clk);
uint16_t EtherShield_IsPacketReceived(uint16_t len, uint8_t* packet);
uint8_t EtherShield_IsARP(uint8_t *buf,uint16_t len);
void EtherShield_SendARP(uint8_t *buf);
uint8_t EtherShield_IsIP(uint8_t *buf,uint16_t len);
void EtherShield_SendPacket(uint8_t *buf,uint16_t len);
void EtherShield_SendSynchronisationAcknowledge(uint8_t *buf);
void EtherShield_ReadLengthInformation(uint8_t *buf);
uint16_t EtherShield_GetDataPointer(void);
void EtherShield_SendAcknowledge(uint8_t *buf);
void EtherShield_SendAcknowledgeData(uint8_t *buf,uint16_t dlen);
void EtherShield_SendARPRequest(uint8_t *buf, uint8_t *server_ip);
void EtherShield_SendNewPacket(uint8_t *buf,uint16_t dest_port, uint16_t src_port, uint8_t flags, uint8_t max_segment_size, uint8_t clear_seqck, uint16_t next_ack_num, uint16_t dlength, uint8_t *dest_mac, uint8_t *dest_ip);
uint16_t EtherShield_GetDataLength( uint8_t *buf );
		
#endif // ETHERSHIELD_H

