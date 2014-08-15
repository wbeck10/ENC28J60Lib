/*********************************************
 * vim:sw=8:ts=8:si:et
 * To use the above modeline in vim you must have "set modeline" in your .vimrc
 * Author: Guido Socher 
 * Copyright: GPL V2
 *
 * IP/ARP/UDP/TCP functions
 *
 *********************************************/
 /*********************************************
 * Modified: Wolfgang Beck for AVR32
 *********************************************/
//@{
#ifndef IP_ARP_UDP_TCP_H
#define IP_ARP_UDP_TCP_H
#include <stdint.h>

// you must call this function once before you use any of the other functions:
extern void TCPIP_Init(uint8_t *mymac,uint8_t *myip,uint8_t wwwp);
//
extern uint8_t TCPIP_IsARP(uint8_t *buf,uint16_t len);
extern uint8_t TCPIP_IsIP(uint8_t *buf,uint16_t len);
extern void TCP_SendARP(uint8_t *buf);
extern void TCPIP_SendPacket(uint8_t *buf,uint16_t len);
extern void UDP_SendPacket(uint8_t *buf,char *data,uint8_t datalen,uint16_t port);


extern void TCP_SendSynchronisationAcknowledge(uint8_t *buf);
extern void TCPIP_ReadLengthInformation(uint8_t *buf);
extern uint16_t TCP_GetDataPointer(void);
extern uint16_t TCP_SetData(uint8_t *buf,uint16_t pos, const char *s);
extern void TCPIP_SendAcknowledge(uint8_t *buf);
extern void TCPIP_SendAcknowledgeWithData(uint8_t *buf,uint16_t dlen);
extern void TCP_SendARPRequest(uint8_t *buf, uint8_t *server_ip);
extern void TCPIP_SendPackage(uint8_t *buf,uint16_t dest_port, uint16_t src_port, uint8_t flags, uint8_t max_segment_size, 
	uint8_t clear_seqck, uint16_t next_ack_num, uint16_t dlength, uint8_t *dest_mac, uint8_t *dest_ip);
extern uint16_t TCPIP_GetDataLength ( uint8_t *buf );


#endif /* IP_ARP_UDP_TCP_H */
//@}
