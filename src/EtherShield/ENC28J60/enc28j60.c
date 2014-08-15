/*****************************************************************************
* Title         : Microchip ENC28J60 Ethernet Interface Driver
* Author        : Pascal Stang (c)2005
* Modified by Wolfgang Beck to run on AVR32 MCU's
* Copyright: GPL V2
*
*This driver provides initialization and transmit/receive
*functions for the Microchip ENC28J60 10Mb Ethernet Controller and PHY.
*This chip is novel in that it is a full MAC+PHY interface all in a 28-pin
*chip, using an SPI interface to the host processor.
*****************************************************************************/

#include <avr32/io.h>
#include "EtherShield/ENC28J60/enc28j60.h"
#include "gpio.h"

static uint8_t encBankNumber = 0;
static uint16_t nextPacketPtr = 0;
static volatile avr32_spi_t *avr32SPI = 0;
static struct spi_device spiDevice;

uint8_t ENC28J60_ReadOp(uint8_t op, uint8_t address);
void ENC28J60_WriteOp(uint8_t op, uint8_t address, uint8_t data);
void ENC28J60_ReadBuffer(uint16_t len, uint8_t* data);
void ENC28J60_WriteBuffer(uint16_t len, uint8_t* data);
void ENC28J60_SetBank(uint8_t address);
uint8_t ENC28J60_Read(uint8_t address);
void ENC28J60_Write(uint8_t address, uint8_t data);
uint8_t ENC28J60_GetRevision(void);
void ENC28J60_PhyRegisterWrite(uint8_t address, uint16_t data);

uint8_t ENC28J60_ReadOp(uint8_t op, uint8_t address)
{
    uint8_t cmd;
    uint8_t data = 0;
    
    spi_select_device(avr32SPI, &spiDevice);
    cmd = op | GET_REGISTERADDRESS(address);
    spi_write_packet(avr32SPI, &cmd, 1);
    for(;;)
    {
      if(spi_is_tx_ready(avr32SPI))
      break;
    }
    spi_read_packet(avr32SPI, &data, 1);
    spi_deselect_device(avr32SPI, &spiDevice);
    return data;
}

void ENC28J60_WriteOp(uint8_t op, uint8_t address, uint8_t data)
{
    uint8_t cmd;
    
    spi_select_device(avr32SPI, &spiDevice);
    cmd = op | GET_REGISTERADDRESS(address);
    spi_write_single(avr32SPI, cmd);
    for(;;)
    {
      if(spi_is_tx_ready(avr32SPI))
        break;
    }
    spi_write_single(avr32SPI, data);
    for(;;)
    {
      if(spi_is_tx_ready(avr32SPI))
        break;
    }
    
    spi_deselect_device(avr32SPI, &spiDevice);
}

void ENC28J60_ReadBuffer(uint16_t len, uint8_t* data)
{
  status_code_t ret = STATUS_OK;
  spi_select_device(avr32SPI, &spiDevice);
  spi_write_single(avr32SPI, ENC28J60_READ_BUF_MEM);
  for(;;)
  {
    if(spi_is_tx_ready(avr32SPI))
    break;
  }
  ret = spi_read_packet(avr32SPI, data, len);
  spi_deselect_device(avr32SPI, &spiDevice);
  data[len] = '\0';
}

void ENC28J60_WriteBuffer(uint16_t len, uint8_t* data)
{
  status_code_t ret = STATUS_OK;
  spi_select_device(avr32SPI, &spiDevice);
  spi_write_single(avr32SPI, ENC28J60_WRITE_BUF_MEM);
  for(;;)
  {
    if(spi_is_tx_ready(avr32SPI))
    break;
  }
  ret = spi_write_packet(avr32SPI, data, len);
  spi_deselect_device(avr32SPI, &spiDevice);
}

void ENC28J60_SetBank(uint8_t address)
{
  // set the bank (if needed)
  if(GET_BANK_CODE(address) != encBankNumber)
  {
    // set the bank
    ENC28J60_WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, (ECON1_BSEL1|ECON1_BSEL0));
    ENC28J60_WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, GET_BANK_CODE(address));
    encBankNumber = GET_BANK_CODE(address);
  }
}

uint8_t ENC28J60_Read(uint8_t address)
{
        // set the bank
        ENC28J60_SetBank(address);
        // do the read
        return ENC28J60_ReadOp(ENC28J60_READ_CTRL_REG, address);
}

void ENC28J60_Write(uint8_t address, uint8_t data)
{
        // set the bank
        ENC28J60_SetBank(address);
        // do the write
        ENC28J60_WriteOp(ENC28J60_WRITE_CTRL_REG, address, data);
}

void ENC28J60_PhyRegisterWrite(uint8_t address, uint16_t data)
{
        // set the PHY register address
        ENC28J60_Write(MIREGADR, address);
        // write the PHY data
        ENC28J60_Write(MIWRL, data);
        ENC28J60_Write(MIWRH, data>>8);
        // wait until the PHY write completes
        while(ENC28J60_Read(MISTAT) & MISTAT_BUSY);
}

void ENC28J60_Clkout(uint8_t clk)
{
        //setup clkout: 2 is 12.5MHz:
	ENC28J60_Write(ECOCON, clk & 0x7);
}

void ENC28J60_Init(volatile avr32_spi_t *spi, uint8_t spiDeviceId, spi_flags_t spiFlags, uint32_t spiBaudrate, uint8_t* macaddr)
{
  spiDevice.id = spiDeviceId;
  avr32SPI = spi;
  spi_master_init(avr32SPI);
  spi_master_setup_device(avr32SPI, &spiDevice, spiFlags,	spiBaudrate, spiDevice.id);
  spi_enable(avr32SPI);

	ENC28J60_WriteOp(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);
  while(!(ENC28J60_Read(ESTAT) & ESTAT_CLKRDY));
  
	// do bank 0 stuff
	// initialize receive buffer
	// 16-bit transfers, must write low byte first
	// set receive buffer start address
	nextPacketPtr = RXSTARTBUFFER;
        // Rx start
	ENC28J60_Write(ERXSTL, RXSTARTBUFFER&0xFF);
	ENC28J60_Write(ERXSTH, RXSTARTBUFFER>>8);
	// set receive pointer address
	ENC28J60_Write(ERXRDPTL, RXSTARTBUFFER&0xFF);
	ENC28J60_Write(ERXRDPTH, RXSTARTBUFFER>>8);
	// RX end
	ENC28J60_Write(ERXNDL, RXSTOPBUFFER&0xFF);
	ENC28J60_Write(ERXNDH, RXSTOPBUFFER>>8);
	// TX start
	ENC28J60_Write(ETXSTL, TXSTART_INIT&0xFF);
	ENC28J60_Write(ETXSTH, TXSTART_INIT>>8);
	// TX end
	ENC28J60_Write(ETXNDL, TXSTOP_INIT&0xFF);
	ENC28J60_Write(ETXNDH, TXSTOP_INIT>>8);
	// do bank 1 stuff, packet filter:
        // For broadcast packets we allow only ARP packtets
        // All other packets should be unicast only for our mac (MAADR)
        //
        // The pattern to match on is therefore
        // Type     ETH.DST
        // ARP      BROADCAST
        // 06 08 -- ff ff ff ff ff ff -> ip checksum for theses bytes=f7f9
        // in binary these poitions are:11 0000 0011 1111
        // This is hex 303F->EPMM0=0x3f,EPMM1=0x30
	ENC28J60_Write(ERXFCON, ERXFCON_UCEN|ERXFCON_CRCEN|ERXFCON_PMEN);
	ENC28J60_Write(EPMM0, 0x3f);
	ENC28J60_Write(EPMM1, 0x30);
	ENC28J60_Write(EPMCSL, 0xf9);
	ENC28J60_Write(EPMCSH, 0xf7);
        //
        //
	// do bank 2 stuff
	// enable MAC receive
	ENC28J60_Write(MACON1, MACON1_MARXEN|MACON1_TXPAUS|MACON1_RXPAUS);
	// bring MAC out of reset
	ENC28J60_Write(MACON2, 0x00);
	// enable automatic padding to 60bytes and CRC operations
	ENC28J60_WriteOp(ENC28J60_BIT_FIELD_SET, MACON3, MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN);
	// set inter-frame gap (non-back-to-back)
	ENC28J60_Write(MAIPGL, 0x12);
	ENC28J60_Write(MAIPGH, 0x0C);
	// set inter-frame gap (back-to-back)
	ENC28J60_Write(MABBIPG, 0x12);
	// Set the maximum packet size which the controller will accept
        // Do not send packets longer than MAX_FRAMELEN:
	ENC28J60_Write(MAMXFLL, MAX_FRAMELEN&0xFF);	
	ENC28J60_Write(MAMXFLH, MAX_FRAMELEN>>8);
	// do bank 3 stuff
        // write MAC address
        // NOTE: MAC address in ENC28J60 is byte-backward
        ENC28J60_Write(MAADR5, macaddr[0]);
        ENC28J60_Write(MAADR4, macaddr[1]);
        ENC28J60_Write(MAADR3, macaddr[2]);
        ENC28J60_Write(MAADR2, macaddr[3]);
        ENC28J60_Write(MAADR1, macaddr[4]);
        ENC28J60_Write(MAADR0, macaddr[5]);
	// no loopback of transmitted frames
	ENC28J60_PhyRegisterWrite(PHCON2, PHCON2_HDLDIS);
	// switch to bank 0
	ENC28J60_SetBank(ECON1);
	// enable interrutps
	ENC28J60_WriteOp(ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE|EIE_PKTIE);
	// enable packet reception
	ENC28J60_WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);
  
  // MagJack LED config
  // LEDA=greed LEDB=yellow
  //
  // 0x880 is PHLCON LEDB=on, LEDA=on
  // enc28j60PhyWrite(PHLCON,0b0000 1000 1000 00 00);
  ENC28J60_PhyRegisterWrite(PHLCON,0x880);
  //
  // 0x990 is PHLCON LEDB=off, LEDA=off
  // enc28j60PhyWrite(PHLCON,0b0000 1001 1001 00 00);
  ENC28J60_PhyRegisterWrite(PHLCON,0x990);
  //
  // 0x880 is PHLCON LEDB=on, LEDA=on
  // enc28j60PhyWrite(PHLCON,0b0000 1000 1000 00 00);
  ENC28J60_PhyRegisterWrite(PHLCON,0x880);
  //
  // 0x990 is PHLCON LEDB=off, LEDA=off
  // enc28j60PhyWrite(PHLCON,0b0000 1001 1001 00 00);
  ENC28J60_PhyRegisterWrite(PHLCON,0x990);
  //
  // 0x476 is PHLCON LEDA=links status, LEDB=receive/transmit
  // enc28j60PhyWrite(PHLCON,0b0000 0100 0111 01 10);
  ENC28J60_PhyRegisterWrite(PHLCON,0x476);
}

// read the revision of the chip:
uint8_t ENC28J60_GetRevision(void)
{
	return(ENC28J60_Read(EREVID));
}

void ENC28J60_PacketSend(uint16_t len, uint8_t* packet)
{
	// Set the write pointer to start of transmit buffer area
	ENC28J60_Write(EWRPTL, TXSTART_INIT&0xFF);
	ENC28J60_Write(EWRPTH, TXSTART_INIT>>8);
	// Set the TXND pointer to correspond to the packet size given
	ENC28J60_Write(ETXNDL, (TXSTART_INIT+len)&0xFF);
	ENC28J60_Write(ETXNDH, (TXSTART_INIT+len)>>8);
	// write per-packet control byte (0x00 means use macon3 settings)
	ENC28J60_WriteOp(ENC28J60_WRITE_BUF_MEM, 0, 0x00);
	// copy the packet into the transmit buffer
	ENC28J60_WriteBuffer(len, packet);
	// send the contents of the transmit buffer onto the network
	ENC28J60_WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);
        // Reset the transmit logic problem. See Rev. B4 Silicon Errata point 12.
	if( (ENC28J60_Read(EIR) & EIR_TXERIF) ){
                ENC28J60_WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRTS);
        }
}

// Gets a packet from the network receive buffer, if one is available.
// The packet will by headed by an ethernet header.
//      maxlen  The maximum acceptable length of a retrieved packet.
//      packet  Pointer where packet data should be stored.
// Returns: Packet length in bytes if a packet was retrieved, zero otherwise.
uint16_t ENC28J60_PacketReceived(uint16_t maxlen, uint8_t* packet)
{
	uint16_t rxstat;
	uint16_t len;
	// check if a packet has been received and buffered
	//if( !(enc28j60Read(EIR) & EIR_PKTIF) ){
        // The above does not work. See Rev. B4 Silicon Errata point 6.
	if( ENC28J60_Read(EPKTCNT) ==0 ){
		return(0);
        }

	// Set the read pointer to the start of the received packet
	ENC28J60_Write(ERDPTL, (nextPacketPtr));
	ENC28J60_Write(ERDPTH, (nextPacketPtr)>>8);
	// read the next packet pointer
	nextPacketPtr  = ENC28J60_ReadOp(ENC28J60_READ_BUF_MEM, 0);
	nextPacketPtr |= ENC28J60_ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
	// read the packet length (see datasheet page 43)
	len  = ENC28J60_ReadOp(ENC28J60_READ_BUF_MEM, 0);
	len |= ENC28J60_ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
        len-=4; //remove the CRC count
	// read the receive status (see datasheet page 43)
	rxstat  = ENC28J60_ReadOp(ENC28J60_READ_BUF_MEM, 0);
	rxstat |= ENC28J60_ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
	// limit retrieve length
        if (len>maxlen-1){
                len=maxlen-1;
        }
        // check CRC and symbol errors (see datasheet page 44, table 7-3):
        // The ERXFCON.CRCEN is set by default. Normally we should not
        // need to check this.
        if ((rxstat & 0x80)==0){
                // invalid
                len=0;
        }else{
                // copy the packet from the receive buffer
                ENC28J60_ReadBuffer(len, packet);
        }
	// Move the RX read pointer to the start of the next received packet
	// This frees the memory we just read out
	ENC28J60_Write(ERXRDPTL, (nextPacketPtr));
	ENC28J60_Write(ERXRDPTH, (nextPacketPtr)>>8);
	// decrement the packet counter indicate we are done with this packet
	ENC28J60_WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);
	return(len);
}

