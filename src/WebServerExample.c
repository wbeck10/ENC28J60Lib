/*
 * WebServer2.c
 *
 * Created: 8/6/2014 2:31:31 PM
 *  Author: wbeck
 */ 
#include <string.h>
#include "compiler.h"
#include "preprocessor.h"
#include "board.h"
#include "gpio.h"
#include "sysclk.h"
#include "spi_master.h"
#include "EtherShield/etherShield.h"

#define SPI_ENC28J60             AT45DBX_SPI
#define SPI_DEVICE_EXAMPLE_ID    AT45DBX_SPI_NPCS
#define SPI_EXAMPLE_BAUDRATE     3000000

static uint8_t mymac[6] = {0x54,0x55,0x58,0x10,0x00,0x24};
static uint8_t myip[4] = {198,162,1,15};
static char baseurl[]="http://198.162.1.15/";
static uint16_t mywwwport =80; // listen port for tcp/www (max range 1-254)

#define BUFFER_SIZE 500
static uint8_t buf[BUFFER_SIZE+1];
uint16_t print_webpage(uint8_t *buf);

void setup(void);
void setup(void)
{
	sysclk_init();

	/* Initialize the board.
	 * The board-specific conf_board.h file contains the configuration of
	 * the board initialization.
	 */
	board_init();
   
  /*initialize enc28j60*/
  EtherShield_Init(SPI_ENC28J60, 0,  SPI_MODE_0,	SPI_EXAMPLE_BAUDRATE, mymac, myip, mywwwport);
  EtherShield_SetClock(2);
}

int main(void)
{
  uint16_t plen, dat_p;
  gpio_configure_pin(AVR32_PIN_PA13, GPIO_DIR_OUTPUT | GPIO_INIT_LOW);
  gpio_clr_gpio_pin(AVR32_PIN_PA13);
  setup();
  
  while(1)
  {
    gpio_set_gpio_pin(AVR32_PIN_PA13);
    plen = EtherShield_IsPacketReceived(BUFFER_SIZE, buf);

    /*plen will ne unequal to zero if there is a valid packet (without crc error) */
    if(plen!=0){
      
      // arp is broadcast if unknown but a host may also verify the mac address by sending it to a unicast address.
      if(EtherShield_IsARP(buf,plen)){
        EtherShield_SendARP(buf);
        continue;
      }

      // check if ip packets are for us:
      if(EtherShield_IsIP(buf,plen)==0){
        continue;
      }
      
      // check if we need to echo a package
      if(buf[IP_PROTO_P]==IP_PROTO_ICMP_V && buf[ICMP_TYPE_P]==ICMP_TYPE_ECHOREQUEST_V){
        EtherShield_SendPacket(buf,plen);
        continue;
      }
      
      // tcp port www start, compare only the lower byte
      if (buf[IP_PROTO_P]==IP_PROTO_TCP_V&&buf[TCP_DST_PORT_H_P]==0&&buf[TCP_DST_PORT_L_P]==mywwwport){
        if (buf[TCP_FLAGS_P] & TCP_FLAGS_SYN_V){
          EtherShield_SendSynchronisationAcknowledge(buf); // make_tcp_synack_from_syn does already send the syn,ack
          continue;
        }
        if (buf[TCP_FLAGS_P] & TCP_FLAGS_ACK_V){
          EtherShield_ReadLengthInformation(buf); // Update the length variables in the EtherShield lib
          dat_p=EtherShield_GetDataPointer();
          if (dat_p==0){ // we can possibly have no data, just ack:
            if (buf[TCP_FLAGS_P] & TCP_FLAGS_FIN_V){
              EtherShield_SendAcknowledge(buf);
            }
            continue;
          }
          if (strncmp("GET ",(char *)&(buf[dat_p]),4)!=0){
            // head, post and other methods for possible status codes see:
            // http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
            plen=EtherShield_FillTCPData(buf,0,"HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>200 OK</h1>");
          }
          else if (strncmp("/ ",(char *)&(buf[dat_p+4]),2)==0){
            plen=print_webpage(buf);
          }
          else {
            plen=print_webpage(buf);
          }
          EtherShield_SendAcknowledge(buf); // send ack for http get
          EtherShield_SendAcknowledgeData(buf,plen); // send data
        }
      }
    }
    gpio_clr_gpio_pin(AVR32_PIN_PA13);
  }
}

uint16_t print_webpage(uint8_t *buf)
{
  int i=0;
  uint16_t plen;
  
  plen=EtherShield_FillTCPData(buf,0,"HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n");
  plen=EtherShield_FillTCPData(buf,plen,"<center><p><h1>Welcome to AVR32 Ethernet Shield V1.0  </h1></p> ");
  
  plen=EtherShield_FillTCPData(buf,plen,"<hr><br><form METHOD=get action=\"");
  plen=EtherShield_FillTCPData(buf,plen,baseurl);
  plen=EtherShield_FillTCPData(buf,plen,"\">");
  
    
  while (temp_string[i]) {
    buf[TCP_CHECKSUM_L_P+3+plen]=temp_string[i++];
    plen++;
  }

  plen=EtherShield_FillTCPData(buf,plen,"  &#176C</font></h1><br> ");
  plen=EtherShield_FillTCPData(buf,plen,"<input type=hidden name=cmd value=1>");
  plen=EtherShield_FillTCPData(buf,plen,"<input type=submit value=\"Send Request\"></form>");
  
  return(plen);
}