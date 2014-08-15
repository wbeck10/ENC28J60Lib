/*****************************************************************************
* Title         : EtherShield library for AVR32 etherShield
* Author        : Wolfgang Beck
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
#define ENC28J60

#ifdef ENC28J60
# include "EtherShield/ENC28J60/enc28j60.h"
#endif
// or include for other future modules

#include "EtherShield/TransportLayer/transport_layer.h"
#include "EtherShield/etherShield.h"

uint16_t EtherShield_FillTCPData(uint8_t *buf,uint16_t pos, const char *s)
{
	return TCP_SetData(buf,pos, s);
}

/************************************************************************
Initializing the Ethernet shield
spi: Pointer to the AVR32 spi device
spiDeviceId: ID of the SPI device used in the NPCS (device selecton pin) 
spiFlags: SPI mode (see avr lib description)
spiBaudrate: Baudrate on which the SPI bus should run
macAddress: Address of the 6 mac. Pointer to a 6 uint8_t array
ipAddress: Address of the IP.
port: Port of the application (80)
************************************************************************/
void EtherShield_Init(volatile avr32_spi_t *spi, uint8_t spiDeviceId, spi_flags_t spiFlags, uint32_t spiBaudrate, uint8_t* macAddress, uint8_t *ipAddress, uint8_t port)
{
	Module_Init(spi, spiDeviceId, spiFlags, spiBaudrate, macAddress);
  TCPIP_Init(macAddress, ipAddress, port);
}

/************************************************************************
Set the clock rate.
Please refer to the ethernet module datasheet
************************************************************************/
void EtherShield_SetClock(uint8_t clock)
{
	Module_ClkOut(clock);
}

/************************************************************************
Return nonzero when a valid packet was received
************************************************************************/
uint16_t EtherShield_IsPacketReceived(uint16_t len, uint8_t* packet)
{
	return Module_PacketReceived(len, packet);
}

/************************************************************************
Return nonzero if the packet is an Address Resolution Protocol (ARP)
request.
************************************************************************/
uint8_t EtherShield_IsARP(uint8_t *buf,uint16_t len)
{
	return TCPIP_IsARP(buf,len);
}

/************************************************************************
Response to a previous Address Resolution Protocol (ARP) request.
************************************************************************/
void EtherShield_SendARP(uint8_t *buf)
{
	TCP_SendARP(buf);
}

/************************************************************************
Return nonzero if the packet is for the ethernet module.
************************************************************************/
uint8_t EtherShield_IsIP(uint8_t *buf,uint16_t len)
{
	return TCPIP_IsIP(buf, len);
}

/************************************************************************
Sends an Echo packet from a request
************************************************************************/
void EtherShield_SendPacket(uint8_t *buf,uint16_t len)
{
	TCPIP_SendPacket(buf,len);
}

/************************************************************************
Send an Acknowledge packet based from a synchronization request.
************************************************************************/
void EtherShield_SendSynchronisationAcknowledge(uint8_t *buf)
{
	TCP_SendSynchronisationAcknowledge(buf);
}	

/************************************************************************
Updated the length information from the package.
************************************************************************/
void EtherShield_ReadLengthInformation(uint8_t *buf)
{
	TCPIP_ReadLengthInformation(buf);
}

/************************************************************************
Returns a pointer of the TCP data
************************************************************************/
uint16_t EtherShield_GetDataPointer(void)
{
	return TCP_GetDataPointer();
}

/************************************************************************
Send an Acknowledge packet based from any packet with no tcp data inside.
This will modify the eth, ip and tcp header.
************************************************************************/
void EtherShield_SendAcknowledge(uint8_t *buf)
{
	TCPIP_SendAcknowledge(buf);
}

/************************************************************************
Send an Acknowledge packet containing the tcp data in buf.
Before using this method you should have call EtherShield_InitLengthInfo
before. Using this method after calling EtherShield_TCPAcknowledgeFromAny
is recommended. This method will not modify any header information 
except the length and checksum.
************************************************************************/
void EtherShield_SendAcknowledgeData(uint8_t *buf,uint16_t dlen)
{
	TCPIP_SendAcknowledgeWithData(buf,dlen);
}

/************************************************************************
Send an Address Resolution Protocol (ARP) request to the server ip.
************************************************************************/
void EtherShield_SendARPRequest(uint8_t *buf, uint8_t *server_ip)
{
	TCP_SendARPRequest(buf, server_ip);
}

/************************************************************************
Send a TCP packet to a client.
************************************************************************/
void EtherShield_SendNewPacket(uint8_t *buf,uint16_t dest_port, uint16_t src_port, uint8_t flags, uint8_t max_segment_size, 
                                     uint8_t clear_seqck, uint16_t next_ack_num, uint16_t dlength, uint8_t *dest_mac, uint8_t *dest_ip)
{
	TCPIP_SendPackage(buf, dest_port, src_port, flags, max_segment_size, clear_seqck, next_ack_num, dlength,dest_mac,dest_ip);
}

/************************************************************************
Returns the length of the data in the packet.
************************************************************************/
uint16_t EtherShield_GetDataLength( uint8_t *buf )
{
	return TCPIP_GetDataLength(buf);
}
