/*********************************************
 * vim:sw=8:ts=8:si:et
 * To use the above modeline in vim you must have "set modeline" in your .vimrc
 *
 * Author: Guido Socher 
 * Copyright: GPL V2
 * See http://www.gnu.org/licenses/gpl.html
 *
 * IP, Arp, UDP and TCP functions.
 *
 * The TCP implementation uses some size optimisations which are valid
 * only if all data can be sent in one single packet. This is however
 * not a big limitation for a microcontroller as you will anyhow use
 * small web-pages. The TCP stack is therefore a SDP-TCP stack (single data packet TCP).
 *
 *********************************************/
 /*********************************************
 * Modified: Wolfgang Beck for AVR32
 *********************************************/

#include <avr32/io.h>
#include "EtherShield/TransportLayer/net.h"
#include "EtherShield/ENC28J60/enc28j60.h"
#include "EtherShield/TransportLayer/transport_layer.h"

static uint8_t wwwport=80;
static uint8_t macaddr[6];
static uint8_t ipaddr[4];
static int16_t info_hdr_len=0;
static int16_t info_data_len=0;
static uint8_t seqnum=0xa; // my initial tcp sequence number
static uint16_t ip_identifier = 1;

// The Ip checksum is calculated over the ip header only starting
// with the header length field and a total length of 20 bytes
// unitl ip.dst
// You must set the IP checksum field to zero before you start
// the calculation.
// len for ip is 20.
//
// For UDP/TCP we do not make up the required pseudo header. Instead we 
// use the ip.src and ip.dst fields of the real packet:
// The udp checksum calculation starts with the ip.src field
// Ip.src=4bytes,Ip.dst=4 bytes,Udp header=8bytes + data length=16+len
// In other words the len here is 8 + length over which you actually
// want to calculate the checksum.
// You must set the checksum field to zero before you start
// the calculation.
// len for udp is: 8 + 8 + data length
// len for tcp is: 4+4 + 20 + option len + data length
//
// For more information on how this algorithm works see:
// http://www.netfor2.com/checksum.html
// http://www.msc.uky.edu/ken/cs471/notes/chap3.htm
// The RFC has also a C code example: http://www.faqs.org/rfcs/rfc1071.html

/************************************************************************/
/* Calculates the Checksum of a packet                                  */
/************************************************************************/
uint16_t CalculateChecksum(uint8_t *buf, uint16_t len,uint8_t type);
uint16_t CalculateChecksum(uint8_t *buf, uint16_t len,uint8_t type)
{
	// type 0=ip
	//      1=udp
	//      2=tcp
	uint32_t sum = 0;
  switch(type)
  {
    case 1:
		  sum+=IP_PROTO_UDP_V; // protocol udp
		  // the length here is the length of udp (data+header len)
		  // =length given to this function - (IP.scr+IP.dst length)
		  sum+=len-8; // = real tcp len
      break;
    case 2:
		  sum+=IP_PROTO_TCP_V;
		  // the length here is the length of tcp (data+header len)
		  // =length given to this function - (IP.scr+IP.dst length)
		  sum+=len-8; // = real tcp len
    break;
    default:
    break;
  }
	// build the sum of 16bit words
	while(len >1){
		sum += 0xFFFF & (*buf<<8|*(buf+1));
		buf+=2;
		len-=2;
	}
	// if there is a byte left then add it (padded with zero)
	if (len){
		sum += (0xFF & *buf)<<8;
	}
	// now calculate the sum over the bytes in the sum
	// until the result is only 16bit long
	while (sum>>16){
		sum = (sum & 0xFFFF)+(sum >> 16);
	}
	// build 1's complement:
	return( (uint16_t) sum ^ 0xFFFF);
}

/************************************************************************/
/* Initialize the IP, ARP, UDP and TCP library                          */
/* You must call this function once before you use any of the other     */
/* functions!                                                           */
/************************************************************************/
void TCPIP_Init(uint8_t *mymac,uint8_t *myip,uint8_t wwwp)
{
	uint8_t i=0;
	wwwport=wwwp;
	while(i<4){
		ipaddr[i]=myip[i];
		i++;
	}
	i=0;
	while(i<6){
		macaddr[i]=mymac[i];
		i++;
	}
}

/************************************************************************/
/* Returns 1 if packet is an ARP packet and the packet was addressed to */
/* us otherwise 0.                                                      */
/************************************************************************/
uint8_t TCPIP_IsARP(uint8_t *buf,uint16_t len)
{
	uint8_t i=0;

	if (len<41){
		return(0);
	}
	if(buf[ETH_TYPE_H_P] != ETHTYPE_ARP_H_V || buf[ETH_TYPE_L_P] != ETHTYPE_ARP_L_V){
		return(0);
	}
	while(i<4){
		if(buf[ETH_ARP_DST_IP_P+i] != ipaddr[i]){
			return(0);
		}
		i++;
	}
	return(1);
}

/************************************************************************/
/* Returns 1 if packet is an IP packet and the packet was addressed to  */
/* us otherwise 0.                                                      */
/************************************************************************/
uint8_t TCPIP_IsIP(uint8_t *buf,uint16_t len)
{
	uint8_t i=0;
	//eth+ip+udp header is 42
	if (len<42){
		return(0);
	}
	if(buf[ETH_TYPE_H_P]!=ETHTYPE_IP_H_V ||
	buf[ETH_TYPE_L_P]!=ETHTYPE_IP_L_V){
		return(0);
	}
	if (buf[IP_HEADER_LEN_VER_P]!=0x45){
		// must be IP V4 and 20 byte header
		return(0);
	}
	while(i<4){
		if(buf[IP_DST_P+i]!=ipaddr[i]){
			return(0);
		}
		i++;
	}
	return(1);
}

/************************************************************************/
/* Sets the destination MAC address with the previous received source   */
/* MAC address and replaces the source MAC address with our MAC address.*/
/************************************************************************/
void TCP_SwapMACAddresses(uint8_t *buf);
void TCP_SwapMACAddresses(uint8_t *buf)
{
	uint8_t i=0;
	//
	//copy the destination mac from the source and fill my mac into src
	while(i<6){
		buf[ETH_DST_MAC +i]=buf[ETH_SRC_MAC +i];
		buf[ETH_SRC_MAC +i]=macaddr[i];
		i++;
	}
}

/************************************************************************/
/* Sets the destination MAC address and sets the source MAC address     */
/* with our MAC address.                                                */
/************************************************************************/
void TCP_SetMACAddress(uint8_t *buf, uint8_t* dst_mac);
void TCP_SetMACAddress(uint8_t *buf, uint8_t* dst_mac)
{
  uint8_t i=0;
  //
  //copy the destination mac from the source and fill my mac into src
  while(i<6){
    buf[ETH_DST_MAC +i]=dst_mac[i];
    buf[ETH_SRC_MAC +i]=macaddr[i];
    i++;
  }
        
  buf[ ETH_TYPE_H_P ] = ETHTYPE_IP_H_V;
  buf[ ETH_TYPE_L_P ] = ETHTYPE_IP_L_V;
}

/************************************************************************/
/* Sets the Checksum in the TCP header                                  */
/************************************************************************/
void TCPIP_SetChecksum(uint8_t *buf);
void TCPIP_SetChecksum(uint8_t *buf)
{
  uint16_t ck;
  // clear the 2 byte checksum
  buf[IP_CHECKSUM_P]=0;
  buf[IP_CHECKSUM_P+1]=0;
  buf[IP_FLAGS_P]=0x40; // don't fragment
  buf[IP_FLAGS_P+1]=0;  // fragement offset
  buf[IP_TTL_P]=64; // ttl
  // calculate the checksum:
  ck=CalculateChecksum(&buf[IP_P], IP_HEADER_LEN,0);
  buf[IP_CHECKSUM_P]=ck>>8;
  buf[IP_CHECKSUM_P+1]=ck& 0xff;
}

/************************************************************************/
/* Makes and IP reply header from a received Packet with a given        */
/* destination IP                                                       */
/************************************************************************/
void IP_SetHeader(uint8_t *buf, uint16_t len,uint8_t *dst_ip);
void IP_SetHeader(uint8_t *buf, uint16_t len,uint8_t *dst_ip)
{
  uint8_t i=0;
		
	// set ipv4 and header length
	buf[ IP_P ] = IP_V4_V | IP_HEADER_LENGTH_V;

	// set TOS to default 0x00
	buf[ IP_TOS_P ] = 0x00;

	// set total length
	buf[ IP_TOTLEN_H_P ] = (len >>8)& 0xff;
	buf[ IP_TOTLEN_L_P ] = len & 0xff;
	
	// set packet identification
	buf[ IP_ID_H_P ] = (ip_identifier >>8) & 0xff;
	buf[ IP_ID_L_P ] = ip_identifier & 0xff;
	ip_identifier++;
	
	// set fragment flags	
	buf[ IP_FLAGS_H_P ] = 0x00;
	buf[ IP_FLAGS_L_P ] = 0x00;
	
	// set Time To Live
	buf[ IP_TTL_P ] = 128;
	
	// set ip packettype to tcp/udp/icmp...
	buf[ IP_PROTO_P ] = IP_PROTO_TCP_V;
	
	// set source and destination ip address
  while(i<4){
    buf[IP_DST_P+i]=dst_ip[i];
    buf[IP_SRC_P+i]=ipaddr[i];
    i++;
  }
  TCPIP_SetChecksum(buf);
}

/************************************************************************/
/* Sets our IP address to the source IP in the IP header and uses the   */
/* previously received IP address as the destination IP address         */
/************************************************************************/
void IP_SwapIP(uint8_t *buf);
void IP_SwapIP(uint8_t *buf)
{
  uint8_t i=0;
  while(i<4){
    buf[IP_DST_P+i]=buf[IP_SRC_P+i];
    buf[IP_SRC_P+i]=ipaddr[i];
    i++;
  }
  TCPIP_SetChecksum(buf);
}

/************************************************************************/
/* Makes a return TCP header from a received packet. rel_ack_num is how */
/* much we must step the seq number received from the other side.       */
/* We do not send more than 255 bytes of text (=data) in the TCP packet.*/
/* After calling this function you can fill in the first data byte at   */
/* TCP_OPTIONS_P+4. If cp_seq=0 then an initial sequence number is used */
/* (should be use in synack) otherwise it is copied from the packet we  */
/* received.                                                            */
/************************************************************************/
void TCP_SetHeader(uint8_t *buf,uint16_t rel_ack_num,uint8_t mss,uint8_t cp_seq);
void TCP_SetHeader(uint8_t *buf,uint16_t rel_ack_num,uint8_t mss,uint8_t cp_seq)
{
  uint8_t i=0;
  uint8_t tseq;
  while(i<2){
    buf[TCP_DST_PORT_H_P+i]=buf[TCP_SRC_PORT_H_P+i];
    buf[TCP_SRC_PORT_H_P+i]=0; // clear source port
    i++;
  }
  // set source port  (http):
  buf[TCP_SRC_PORT_L_P]=wwwport;
  i=4;
  // sequence numbers:
  // add the rel ack num to SEQACK
  while(i>0){
    rel_ack_num=buf[TCP_SEQ_H_P+i-1]+rel_ack_num;
    tseq=buf[TCP_SEQACK_H_P+i-1];
    buf[TCP_SEQACK_H_P+i-1]=0xff&rel_ack_num;
    if (cp_seq){
            // copy the acknum sent to us into the sequence number
            buf[TCP_SEQ_H_P+i-1]=tseq;
    }else{
            buf[TCP_SEQ_H_P+i-1]= 0; // some preset vallue
    }
    rel_ack_num=rel_ack_num>>8;
    i--;
  }
  if (cp_seq==0){
    // put inital seq number
    buf[TCP_SEQ_H_P+0]= 0;
    buf[TCP_SEQ_H_P+1]= 0;
    // we step only the second byte, this allows us to send packts 
    // with 255 bytes or 512 (if we step the initial seqnum by 2)
    buf[TCP_SEQ_H_P+2]= seqnum; 
    buf[TCP_SEQ_H_P+3]= 0;
    // step the inititial seq num by something we will not use
    // during this tcp session:
    seqnum+=2;
  }
  // zero the checksum
  buf[TCP_CHECKSUM_H_P]=0;
  buf[TCP_CHECKSUM_L_P]=0;

  // The tcp header length is only a 4 bit field (the upper 4 bits).
  // It is calculated in units of 4 bytes. 
  // E.g 24 bytes: 24/4=6 => 0x60=header len field
  //buf[TCP_HEADER_LEN_P]=(((TCP_HEADER_LEN_PLAIN+4)/4)) <<4; // 0x60
  if (mss) {
    // the only option we set is MSS to 1408:
    // 1408 in hex is 0x580
    buf[TCP_OPTIONS_P]=2;
    buf[TCP_OPTIONS_P+1]=4;
    buf[TCP_OPTIONS_P+2]=0x05; 
    buf[TCP_OPTIONS_P+3]=0x80;
    // 24 bytes:
    buf[TCP_HEADER_LEN_P]=0x60;
  }
  else{
    // no options:
    // 20 bytes:
    buf[TCP_HEADER_LEN_P]=0x50;
  }
}

/************************************************************************/
/* This method send an ARP answere based on a previous received ARP     */
/* request.                                                             */
/************************************************************************/
void TCP_SendARP(uint8_t *buf)
{
  uint8_t i=0;
  //
  TCP_SwapMACAddresses(buf);
  buf[ETH_ARP_OPCODE_H_P]=ETH_ARP_OPCODE_REPLY_H_V;
  buf[ETH_ARP_OPCODE_L_P]=ETH_ARP_OPCODE_REPLY_L_V;
  // fill the mac addresses:
  while(i<6){
    buf[ETH_ARP_DST_MAC_P+i]=buf[ETH_ARP_SRC_MAC_P+i];
    buf[ETH_ARP_SRC_MAC_P+i]=macaddr[i];
    i++;
  }
  i=0;
  while(i<4){
    buf[ETH_ARP_DST_IP_P+i]=buf[ETH_ARP_SRC_IP_P+i];
    buf[ETH_ARP_SRC_IP_P+i]=ipaddr[i];
    i++;
  }
  // eth+arp is 42 bytes:
  ENC28J60_PacketSend(42,buf);
}

/************************************************************************/
/* Replies an echo to a previously received echo request.               */
/************************************************************************/
void TCPIP_SendPacket(uint8_t *buf,uint16_t len)
{
  TCP_SwapMACAddresses(buf);
  IP_SwapIP(buf);
  buf[ICMP_TYPE_P]=ICMP_TYPE_ECHOREPLY_V;
  // we changed only the icmp.type field from request(=8) to reply(=0).
  // we can therefore easily correct the checksum:
  if (buf[ICMP_CHECKSUM_P] > (0xff-0x08)){
    buf[ICMP_CHECKSUM_P+1]++;
  }
  buf[ICMP_CHECKSUM_P]+=0x08;
  ENC28J60_PacketSend(len,buf);
}

/************************************************************************/
/* This method sends an UDP package based to the destination from the   */
/* source of the previous received package. You can only send 220 bytes */
/* of data.                                                             */
/************************************************************************/
void UDP_SendPacket(uint8_t *buf,char *data,uint8_t datalen,uint16_t port)
{
  uint8_t i=0;
  uint16_t ck;
  TCP_SwapMACAddresses(buf);
  if (datalen>220){
    datalen=220;
  }
  // total length field in the IP header must be set:
  buf[IP_TOTLEN_H_P]=0;
  buf[IP_TOTLEN_L_P]=IP_HEADER_LEN+UDP_HEADER_LEN+datalen;
  IP_SwapIP(buf);
  buf[UDP_DST_PORT_H_P]=port>>8;
  buf[UDP_DST_PORT_L_P]=port & 0xff;
  // source port does not matter and is what the sender used.
  // calculte the udp length:
  buf[UDP_LEN_H_P]=0;
  buf[UDP_LEN_L_P]=UDP_HEADER_LEN+datalen;
  // zero the checksum
  buf[UDP_CHECKSUM_H_P]=0;
  buf[UDP_CHECKSUM_L_P]=0;
  // copy the data:
  while(i<datalen){
    buf[UDP_DATA_P+i]=data[i];
    i++;
  }
  ck=CalculateChecksum(&buf[IP_SRC_P], 16 + datalen,1);
  buf[UDP_CHECKSUM_H_P]=ck>>8;
  buf[UDP_CHECKSUM_L_P]=ck& 0xff;
  ENC28J60_PacketSend(UDP_HEADER_LEN+IP_HEADER_LEN+ETH_HEADER_LEN+datalen,buf);
}

/************************************************************************/
/* THis method send a synchronisation acknowledge based on a previously */
/* received synchronisation request package.                            */
/************************************************************************/
void TCP_SendSynchronisationAcknowledge(uint8_t *buf)
{
  uint16_t ck;
  TCP_SwapMACAddresses(buf);
  // total length field in the IP header must be set:
  // 20 bytes IP + 24 bytes (20tcp+4tcp options)
  buf[IP_TOTLEN_H_P]=0;
  buf[IP_TOTLEN_L_P]=IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+4;
  IP_SwapIP(buf);
  buf[TCP_FLAG_P]=TCP_FLAGS_SYNACK_V;
  TCP_SetHeader(buf,1,1,0);
  // calculate the checksum, len=8 (start from ip.src) + TCP_HEADER_LEN_PLAIN + 4 (one option: mss)
  ck=CalculateChecksum(&buf[IP_SRC_P], 8+TCP_HEADER_LEN_PLAIN+4,2);
  buf[TCP_CHECKSUM_H_P]=ck>>8;
  buf[TCP_CHECKSUM_L_P]=ck& 0xff;
  // add 4 for option mss:
  ENC28J60_PacketSend(IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+4+ETH_HEADER_LEN,buf);
}

/************************************************************************/
/* Returns the beginning of the TCP data from a package. Returns 0 when */
/* no TCP data are in the package.                                      */
/* You must call TCPIP_ReadLengthInformation once before calling this   */ 
/* function.                                                            */
/************************************************************************/
uint16_t TCP_GetDataPointer(void)
{
  if (info_data_len)
    return((uint16_t)TCP_SRC_PORT_H_P+info_hdr_len);
  return(0);
}

/************************************************************************/
/* Updates the length variables with the current TCP header information.*/
/************************************************************************/
void TCPIP_ReadLengthInformation(uint8_t *buf)
{
  info_data_len=(buf[IP_TOTLEN_H_P]<<8)|(buf[IP_TOTLEN_L_P]&0xff);
  info_data_len-=IP_HEADER_LEN;
  info_hdr_len=(buf[TCP_HEADER_LEN_P]>>4)*4; // generate len in bytes;
  info_data_len-=info_hdr_len;
  if (info_data_len<=0){
    info_data_len=0;
  }
}

/************************************************************************/
/* Fills the datasection with data of a TCP package and returns the     */
/* end of the data position from the package. The end of the string (s*)*/
/* must be null terminated ('\0').                                      */
/************************************************************************/
uint16_t TCP_SetData(uint8_t *buf,uint16_t pos, const char *s)
{
  // fill in tcp data at position pos
  // with no options the data starts after the checksum + 2 more bytes (urgent ptr)
  while (*s) {
    buf[TCP_CHECKSUM_L_P+3+pos]=*s;
    pos++;
    s++;
  }
  return(pos);
}

/************************************************************************/
/* Send an Acknowledge package based from a previous received           */
/* acknowledge request.                                                 */
/************************************************************************/
void TCPIP_SendAcknowledge(uint8_t *buf)
{
  uint16_t j;
  TCP_SwapMACAddresses(buf);
  // fill the header:
  buf[TCP_FLAG_P]=TCP_FLAG_ACK_V;
  if (info_data_len==0){
    // if there is no data then we must still acknoledge one packet
    TCP_SetHeader(buf,1,0,1); // no options
    }else{
    TCP_SetHeader(buf,info_data_len,0,1); // no options
  }

  // total length field in the IP header must be set:
  // 20 bytes IP + 20 bytes tcp (when no options)
  j=IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN;
  buf[IP_TOTLEN_H_P]=j>>8;
  buf[IP_TOTLEN_L_P]=j& 0xff;
  IP_SwapIP(buf);
  // calculate the checksum, len=8 (start from ip.src) + TCP_HEADER_LEN_PLAIN + data len
  j=CalculateChecksum(&buf[IP_SRC_P], 8+TCP_HEADER_LEN_PLAIN,2);
  buf[TCP_CHECKSUM_H_P]=j>>8;
  buf[TCP_CHECKSUM_L_P]=j& 0xff;
  ENC28J60_PacketSend(IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+ETH_HEADER_LEN,buf);
}

/************************************************************************/
/* Set the TCP data and send an Acknowledge package based from a        */
/* previous received acknowledge request.                               */
/* Before using this method you should have called                      */
/* TCPIP_ReadLengthInformation before to update the length variables.   */
/* calling this function.                                               */
/* After calling TCP_SendAcknowledge you can call this method           */
/* immediately because this method will NOT modify the TCP and IP       */
/* headers except the length and checksum.                              */
/************************************************************************/
void TCPIP_SendAcknowledgeWithData(uint8_t *buf,uint16_t dataLen)
{
  uint16_t j;
  // fill the header:
  // This code requires that we send one data packet only because we not keeping state information. 
  // Therefore we need to set the fin here:
  buf[TCP_FLAG_P]=TCP_FLAG_ACK_V|TCP_FLAG_PUSH_V|TCP_FLAG_FIN_V;

  // total length field in the IP header must be set:
  // 20 bytes IP + 20 bytes tcp (when no options) + len of data
  j=IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+dataLen;
  buf[IP_TOTLEN_H_P]=j>>8;
  buf[IP_TOTLEN_L_P]=j& 0xff;
  TCPIP_SetChecksum(buf);
  // zero the checksum
  buf[TCP_CHECKSUM_H_P]=0;
  buf[TCP_CHECKSUM_L_P]=0;
  // calculate the checksum, len=8 (start from ip.src) + TCP_HEADER_LEN_PLAIN + data len
  j=CalculateChecksum(&buf[IP_SRC_P], 8+TCP_HEADER_LEN_PLAIN+dataLen,2);
  buf[TCP_CHECKSUM_H_P]=j>>8;
  buf[TCP_CHECKSUM_L_P]=j& 0xff;
  ENC28J60_PacketSend(IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+dataLen+ETH_HEADER_LEN,buf);
}

/************************************************************************/
/* Send an ARP request to a given server IP                             */
/************************************************************************/
void TCP_SendARPRequest(uint8_t *buf, uint8_t *server_ip)
{
  uint8_t i=0;

  while(i<6)
  {
    buf[ETH_DST_MAC +i]=0xff;
    buf[ETH_SRC_MAC +i]=macaddr[i];
    i++;
  }
  
  buf[ ETH_TYPE_H_P ] = ETHTYPE_ARP_H_V;
  buf[ ETH_TYPE_L_P ] = ETHTYPE_ARP_L_V;

  // generate arp packet
  buf[ARP_OPCODE_H_P]=ARP_OPCODE_REQUEST_H_V;
  buf[ARP_OPCODE_L_P]=ARP_OPCODE_REQUEST_L_V;

  // fill in arp request packet
  // setup hardware type to ethernet 0x0001
  buf[ ARP_HARDWARE_TYPE_H_P ] = ARP_HARDWARE_TYPE_H_V;
  buf[ ARP_HARDWARE_TYPE_L_P ] = ARP_HARDWARE_TYPE_L_V;

  // setup protocol type to ip 0x0800
  buf[ ARP_PROTOCOL_H_P ] = ARP_PROTOCOL_H_V;
  buf[ ARP_PROTOCOL_L_P ] = ARP_PROTOCOL_L_V;

  // setup hardware length to 0x06
  buf[ ARP_HARDWARE_SIZE_P ] = ARP_HARDWARE_SIZE_V;

  // setup protocol length to 0x04
  buf[ ARP_PROTOCOL_SIZE_P ] = ARP_PROTOCOL_SIZE_V;

  // setup arp destination and source mac address
  for ( i=0; i<6; i++)
  {
    buf[ ARP_DST_MAC_P + i ] = 0x00;
    buf[ ARP_SRC_MAC_P + i ] = macaddr[i];
  }

  // setup arp destination and source ip address
  for ( i=0; i<4; i++)
  {
    buf[ ARP_DST_IP_P + i ] = server_ip[i];
    buf[ ARP_SRC_IP_P + i ] = ipaddr[i];
  }

  // eth+arp is 42 bytes:
  ENC28J60_PacketSend(42,buf);

}

/************************************************************************/
/* Send a TCP/IP packet to the client.                                  */
/************************************************************************/
void TCPIP_SendPackage(uint8_t *buf,uint16_t dest_port, uint16_t src_port, uint8_t flags, uint8_t max_segment_size,
uint8_t clear_seqack, uint16_t next_ack_num, uint16_t dlength, uint8_t *dest_mac, uint8_t *dest_ip)
{
  uint8_t i=0;
  uint8_t tseq;
  uint16_t ck;

  TCP_SetMACAddress(buf, dest_mac);

  buf[TCP_DST_PORT_H_P]= (uint8_t) ( (dest_port>>8) & 0xff);
  buf[TCP_DST_PORT_L_P]= (uint8_t) (dest_port & 0xff);

  buf[TCP_SRC_PORT_H_P]= (uint8_t) ( (src_port>>8) & 0xff);
  buf[TCP_SRC_PORT_L_P]= (uint8_t) (src_port & 0xff);

  // sequence numbers:
  // add the rel ack num to SEQACK

  if(next_ack_num)
  {
    for(i=4; i>0; i--)
    {
      next_ack_num=buf[TCP_SEQ_H_P+i-1]+next_ack_num;
      tseq=buf[TCP_SEQACK_H_P+i-1];
      buf[TCP_SEQACK_H_P+i-1]=0xff&next_ack_num;
      // copy the acknum sent to us into the sequence number
      buf[TCP_SEQ_P + i - 1 ] = tseq;
      next_ack_num>>=8;
    }
  }

  // initial tcp sequence number,require to setup for first transmit/receive
  if(max_segment_size)
  {
    // put inital seq number
    buf[TCP_SEQ_H_P+0]= 0;
    buf[TCP_SEQ_H_P+1]= 0;
    // we step only the second byte, this allows us to send packts
    // with 255 bytes or 512 (if we step the initial seqnum by 2)
    buf[TCP_SEQ_H_P+2]= seqnum;
    buf[TCP_SEQ_H_P+3]= 0;
    // step the inititial seq num by something we will not use
    // during this tcp session:
    seqnum+=2;

    // setup maximum segment size
    buf[TCP_OPTIONS_P]=2;
    buf[TCP_OPTIONS_P+1]=4;
    buf[TCP_OPTIONS_P+2]=0x05;
    buf[TCP_OPTIONS_P+3]=0x80;
    // 24 bytes:
    buf[TCP_HEADER_LEN_P]=0x60;

    dlength +=4;
  }
  else{
    // no options:
    // 20 bytes:
    buf[TCP_HEADER_LEN_P]=0x50;
  }
  
  IP_SetHeader(buf,IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+dlength, dest_ip);

  // clear sequence ack numer before send tcp SYN packet
  if(clear_seqack)
  {
    buf[TCP_SEQACK_P] = 0;
    buf[TCP_SEQACK_P+1] = 0;
    buf[TCP_SEQACK_P+2] = 0;
    buf[TCP_SEQACK_P+3] = 0;
  }
  // zero the checksum
  buf[TCP_CHECKSUM_H_P]=0;
  buf[TCP_CHECKSUM_L_P]=0;

  // set up flags
  buf[TCP_FLAG_P] = flags;
  // setup maximum windows size
  buf[ TCP_WINDOWSIZE_H_P ] = ((600 - IP_HEADER_LEN - ETH_HEADER_LEN)>>8) & 0xff;
  buf[ TCP_WINDOWSIZE_L_P ] = (600 - IP_HEADER_LEN - ETH_HEADER_LEN) & 0xff;

  // setup urgend pointer (not used -> 0)
  buf[ TCP_URGENT_PTR_H_P ] = 0;
  buf[ TCP_URGENT_PTR_L_P ] = 0;

  // check sum
  ck=CalculateChecksum(&buf[IP_SRC_P], 8+TCP_HEADER_LEN_PLAIN+dlength,2);
  buf[TCP_CHECKSUM_H_P]=ck>>8;
  buf[TCP_CHECKSUM_L_P]=ck& 0xff;
  // add 4 for option mss:
  ENC28J60_PacketSend(IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+dlength+ETH_HEADER_LEN,buf);
}

/************************************************************************/
/* Retruns the length of the data in the package.                       */
/************************************************************************/
uint16_t TCPIP_GetDataLength ( uint8_t *buf )
{
  int dlength, hlength;

  dlength = ( buf[ IP_TOTLEN_H_P ] <<8 ) | ( buf[ IP_TOTLEN_L_P ] );
  dlength -= IP_HEADER_LEN;
  hlength = (buf[ TCP_HEADER_LEN_P ]>>4) * 4; // generate len in bytes;
  dlength -= hlength;
  if ( dlength <= 0 )
  dlength=0;
  
  return ((uint16_t)dlength);
}

/* end of ip_arp_udp.c */
