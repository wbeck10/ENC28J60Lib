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

#ifndef ENC28J60_H
#define ENC28J60_H
#include <inttypes.h>
#include <spi_master.h>

// Wrap the method names to be module independent
#define Module_Init           ENC28J60_Init
#define Module_ClkOut         ENC28J60_Clkout
#define Module_PacketReceived ENC28J60_PacketReceived

/************************************************************************/
/* ENC28J60 CONTROL REGISTER MAP                                        */
/************************************************************************/
/* 
Bank 0           Bank 1           Bank 2           Bank 3
Address Name     Address Name     Address Name     Address Name
00h     ERDPTL   00h     EHT0     00h     MACON1   00h MAADR1
01h     ERDPTH   01h     EHT1     01h     MACON2   01h MAADR0
02h     EWRPTL   02h     EHT2     02h     MACON3   02h MAADR3
03h     EWRPTH   03h     EHT3     03h     MACON4   03h MAADR2
04h     ETXSTL   04h     EHT4     04h     MABBIPG  04h MAADR5
05h     ETXSTH   05h     EHT5     05h        —     05h MAADR4
06h     ETXNDL   06h     EHT6     06h     MAIPGL   06h EBSTSD
07h     ETXNDH   07h     EHT7     07h     MAIPGH   07h EBSTCON
08h     ERXSTL   08h     EPMM0    08h     MACLCON1 08h EBSTCSL
09h     ERXSTH   09h     EPMM1    09h     MACLCON2 09h EBSTCSH
0Ah     ERXNDL   0Ah     EPMM2    0Ah     MAMXFLL  0Ah MISTAT
0Bh     ERXNDH   0Bh     EPMM3    0Bh     MAMXFLH  0Bh    —
0Ch     ERXRDPTL 0Ch     EPMM4    0Ch     Reserved 0Ch    —
0Dh     ERXRDPTH 0Dh     EPMM5    0Dh     MAPHSUP  0Dh    —
0Eh     ERXWRPTL 0Eh     EPMM6    0Eh     Reserved 0Eh    —
0Fh     ERXWRPTH 0Fh     EPMM7    0Fh        —     0Fh    —
10h     EDMASTL  10h     EPMCSL   10h     Reserved 10h    —
11h     EDMASTH  11h     EPMCSH   11h     MICON    11h    —
12h     EDMANDL  12h        —     12h     MICMD    12h EREVID
13h     EDMANDH  13h        —     13h        —     13h    —
14h     EDMADSTL 14h     EPMOL    14h     MIREGADR 14h    —
15h     EDMADSTH 15h     EPMOH    15h     Reserved 15h ECOCON
16h     EDMACSL  16h     EWOLIE   16h     MIWRL    16h Reserved
17h     EDMACSH  17h     EWOLIR   17h     MIWRH    17h EFLOCON
18h        —     18h     ERXFCON  18h     MIRDL    18h EPAUSL
19h        —     19h     EPKTCNT  19h     MIRDH    19h EPAUSH
1Ah     Reserved 1Ah     Reserved 1Ah     Reserved 1Ah Reserved
1Bh     EIE      1Bh     EIE      1Bh     EIE      1Bh EIE
1Ch     EIR      1Ch     EIR      1Ch     EIR      1Ch EIR
1Dh     ESTAT    1Dh     ESTAT    1Dh     ESTAT    1Dh ESTAT
1Eh     ECON2    1Eh     ECON2    1Eh     ECON2    1Eh ECON2
1Fh     ECON1    1Fh     ECON1    1Fh     ECON1    1Fh ECON1
*/

#define ADD_BANK_CODE(regAddress, bankNo) ((bankNo << 5) | regAddress)
#define GET_BANK_CODE(address)            (address >> 5)
#define GET_REGISTERADDRESS(address)      (address & 0x1F)

// Register accessible on all Banks
#define EIE              0x1B
#define EIR              0x1C
#define ESTAT            0x1D
#define ECON2            0x1E
#define ECON1            0x1F
// Registers for Bank 0
#define ERDPTL           ADD_BANK_CODE(0x00, 0x00)
#define ERDPTH           ADD_BANK_CODE(0x01, 0x00)
#define EWRPTL           ADD_BANK_CODE(0x02, 0x00)
#define EWRPTH           ADD_BANK_CODE(0x03, 0x00)
#define ETXSTL           ADD_BANK_CODE(0x04, 0x00)
#define ETXSTH           ADD_BANK_CODE(0x05, 0x00)
#define ETXNDL           ADD_BANK_CODE(0x06, 0x00)
#define ETXNDH           ADD_BANK_CODE(0x07, 0x00)
#define ERXSTL           ADD_BANK_CODE(0x08, 0x00)
#define ERXSTH           ADD_BANK_CODE(0x09, 0x00)
#define ERXNDL           ADD_BANK_CODE(0x0A, 0x00)
#define ERXNDH           ADD_BANK_CODE(0x0B, 0x00)
#define ERXRDPTL         ADD_BANK_CODE(0x0C, 0x00)
#define ERXRDPTH         ADD_BANK_CODE(0x0D, 0x00)
#define ERXWRPTL         ADD_BANK_CODE(0x0E, 0x00)
#define ERXWRPTH         ADD_BANK_CODE(0x0F, 0x00)
#define EDMASTL          ADD_BANK_CODE(0x10, 0x00)
#define EDMASTH          ADD_BANK_CODE(0x11, 0x00)
#define EDMANDL          ADD_BANK_CODE(0x12, 0x00)
#define EDMANDH          ADD_BANK_CODE(0x13, 0x00)
#define EDMADSTL         ADD_BANK_CODE(0x14, 0x00)
#define EDMADSTH         ADD_BANK_CODE(0x15, 0x00)
#define EDMACSL          ADD_BANK_CODE(0x16, 0x00)
#define EDMACSH          ADD_BANK_CODE(0x17, 0x00)
// Bank 1 registers
#define EHT0             ADD_BANK_CODE(0x00, 0x01)
#define EHT1             ADD_BANK_CODE(0x01, 0x01)
#define EHT2             ADD_BANK_CODE(0x02, 0x01)
#define EHT3             ADD_BANK_CODE(0x03, 0x01)
#define EHT4             ADD_BANK_CODE(0x04, 0x01)
#define EHT5             ADD_BANK_CODE(0x05, 0x01)
#define EHT6             ADD_BANK_CODE(0x06, 0x01)
#define EHT7             ADD_BANK_CODE(0x07, 0x01)
#define EPMM0            ADD_BANK_CODE(0x08, 0x01)
#define EPMM1            ADD_BANK_CODE(0x09, 0x01)
#define EPMM2            ADD_BANK_CODE(0x0A, 0x01)
#define EPMM3            ADD_BANK_CODE(0x0B, 0x01)
#define EPMM4            ADD_BANK_CODE(0x0C, 0x01)
#define EPMM5            ADD_BANK_CODE(0x0D, 0x01)
#define EPMM6            ADD_BANK_CODE(0x0E, 0x01)
#define EPMM7            ADD_BANK_CODE(0x0F, 0x01)
#define EPMCSL           ADD_BANK_CODE(0x10, 0x01)
#define EPMCSH           ADD_BANK_CODE(0x11, 0x01)
#define EPMOL            ADD_BANK_CODE(0x14, 0x01)
#define EPMOH            ADD_BANK_CODE(0x15, 0x01)
#define EWOLIE           ADD_BANK_CODE(0x16, 0x01)
#define EWOLIR           ADD_BANK_CODE(0x17, 0x01)
#define ERXFCON          ADD_BANK_CODE(0x18, 0x01)
#define EPKTCNT          ADD_BANK_CODE(0x19, 0x01)
// Bank 2 registers
#define MACON1           ADD_BANK_CODE(0x00, 0x02)
#define MACON2           ADD_BANK_CODE(0x01, 0x02)
#define MACON3           ADD_BANK_CODE(0x02, 0x02)
#define MACON4           ADD_BANK_CODE(0x03, 0x02)
#define MABBIPG          ADD_BANK_CODE(0x04, 0x02)
#define MAIPGL           ADD_BANK_CODE(0x06, 0x02)
#define MAIPGH           ADD_BANK_CODE(0x07, 0x02)
#define MACLCON1         ADD_BANK_CODE(0x08, 0x02)
#define MACLCON2         ADD_BANK_CODE(0x09, 0x02)
#define MAMXFLL          ADD_BANK_CODE(0x0A, 0x02)
#define MAMXFLH          ADD_BANK_CODE(0x0B, 0x02)
#define MAPHSUP          ADD_BANK_CODE(0x0D, 0x02)
#define MICON            ADD_BANK_CODE(0x11, 0x02)
#define MICMD            ADD_BANK_CODE(0x12, 0x02)
#define MIREGADR         ADD_BANK_CODE(0x14, 0x02)
#define MIWRL            ADD_BANK_CODE(0x16, 0x02)
#define MIWRH            ADD_BANK_CODE(0x17, 0x02)
#define MIRDL            ADD_BANK_CODE(0x18, 0x02)
#define MIRDH            ADD_BANK_CODE(0x19, 0x02)
// Bank 3 registers
#define MAADR1           ADD_BANK_CODE(0x00, 0x03)
#define MAADR0           ADD_BANK_CODE(0x01, 0x03)
#define MAADR3           ADD_BANK_CODE(0x02, 0x03)
#define MAADR2           ADD_BANK_CODE(0x03, 0x03)
#define MAADR5           ADD_BANK_CODE(0x04, 0x03)
#define MAADR4           ADD_BANK_CODE(0x05, 0x03)
#define EBSTSD           ADD_BANK_CODE(0x06, 0x03)
#define EBSTCON          ADD_BANK_CODE(0x07, 0x03)
#define EBSTCSL          ADD_BANK_CODE(0x08, 0x03)
#define EBSTCSH          ADD_BANK_CODE(0x09, 0x03)
#define MISTAT           ADD_BANK_CODE(0x0A, 0x03)
#define EREVID           ADD_BANK_CODE(0x12, 0x03)
#define ECOCON           ADD_BANK_CODE(0x15, 0x03)
#define EFLOCON          ADD_BANK_CODE(0x17, 0x03)
#define EPAUSL           ADD_BANK_CODE(0x18, 0x03)
#define EPAUSH           ADD_BANK_CODE(0x19, 0x03)

/************************************************************************/
/* ENC28J60 PACKET RECEPTION BITS                                       */
/************************************************************************/
// TABLE 7-4: SUMMARY OF REGISTERS USED FOR PACKET RECEPTION// 
// Register Name   Bit 7     Bit 6     Bit 5     Bit 4     Bit 3     Bit 2     Bit 1     Bit 0     Reset Values
// EIE             INTIE     PKTIE     DMAIE     LINKIE    TXIE      WOLIE     TXERIE    RXERIE    13
#define EIE_INTIE        0x80
#define EIE_PKTIE        0x40
#define EIE_DMAIE        0x20
#define EIE_LINKIE       0x10
#define EIE_TXIE         0x08
#define EIE_WOLIE        0x04
#define EIE_TXERIE       0x02
#define EIE_RXERIE       0x01
// EIR             —         PKTIF     DMAIF     LINKIF    TXIF      WOLIF     TXERIF    RXERIF    13
#define EIR_PKTIF        0x40
#define EIR_DMAIF        0x20
#define EIR_LINKIF       0x10
#define EIR_TXIF         0x08
#define EIR_WOLIF        0x04
#define EIR_TXERIF       0x02
#define EIR_RXERIF       0x01
// ESTAT           INT       r         r         LATECOL   —         RXBUSY    TXABRT    CLKRDY    13
#define ESTAT_INT        0x80
#define ESTAT_LATECOL    0x10
#define ESTAT_RXBUSY     0x04
#define ESTAT_TXABRT     0x02
#define ESTAT_CLKRDY     0x01
// ECON2           AUTOINC   PKTDEC    PWRSV     —         VRPS      —         —         —         13
#define ECON2_AUTOINC    0x80
#define ECON2_PKTDEC     0x40
#define ECON2_PWRSV      0x20
#define ECON2_VRPS       0x08
// ECON1           TXRST     RXRST     DMAST     CSUMEN    TXRTS     RXEN      BSEL1     BSEL0     13
#define ECON1_TXRST      0x80
#define ECON1_RXRST      0x40
#define ECON1_DMAST      0x20
#define ECON1_CSUMEN     0x10
#define ECON1_TXRTS      0x08
#define ECON1_RXEN       0x04
#define ECON1_BSEL1      0x02
#define ECON1_BSEL0      0x01
// ERXSTL          —         —         —         <- RX Start Low Byte (ERXST<7:0>) ---------->     13
// ERXSTH          —         —         —         <- RX Start High Byte (ERXST<12:8>) -------->     13
// ERXNDL          <---------------------- RX End Low Byte (ERXND<7:0>) --------------------->     13
// ERXNDH          —         —         —         <- RX End High Byte (ERXND<12:8>) ---------->     13
// ERXRDPTL        <---------------------- RX RD Pointer Low Byte (ERXRDPT<7:0>) ------------>     13
// ERXRDPTH        —         —         —         <- RX RD Pointer High Byte (ERXRDPT<12:8>) ->     13
// ERXFCON         UCEN      ANDOR     CRCEN     PMEN      MPEN      HTEN      MCEN      BCEN      14
#define ERXFCON_UCEN      0x80
#define ERXFCON_ANDOR     0x40
#define ERXFCON_CRCEN     0x20
#define ERXFCON_PMEN      0x10
#define ERXFCON_MPEN      0x08
#define ERXFCON_HTEN      0x04
#define ERXFCON_MCEN      0x02
#define ERXFCON_BCEN      0x01
// EPKTCNT         <---------------------- Ethernet Packet Count ---------------------------->     14
// MACON1          —         —         —         LOOPBK    TXPAUS    RXPAUS    PASSALL   MARXEN    14
#define MACON1_LOOPBK    0x10
#define MACON1_TXPAUS    0x08
#define MACON1_RXPAUS    0x04
#define MACON1_PASSALL   0x02
#define MACON1_MARXEN    0x01
// MACON2          MARST     RNDRST    —         —         MARXRST   RFUNRST   MATXRST   TFUNRST   14
#define MACON2_MARST     0x80
#define MACON2_RNDRST    0x40
#define MACON2_MARXRST   0x08
#define MACON2_RFUNRST   0x04
#define MACON2_MATXRST   0x02
#define MACON2_TFUNRST   0x01
// MACON3          PADCFG2   PADCFG1   PADCFG0   TXCRCEN   PHDRLEN   HFRMEN    FRMLNEN   FULDPX    14
#define MACON3_PADCFG2   0x80
#define MACON3_PADCFG1   0x40
#define MACON3_PADCFG0   0x20
#define MACON3_TXCRCEN   0x10
#define MACON3_PHDRLEN   0x08
#define MACON3_HFRMLEN   0x04
#define MACON3_FRMLNEN   0x02
#define MACON3_FULDPX    0x01
// MACON4          —         DEFER     BPEN      NOBKOFF   —         —         LONGPRE   PUREPRE   14
#define MACON4_DEFER     0x40
#define MACON4_BPEN      0x20
#define MACON4_NOBKOFF   0x10
#define MACON4_LONGPRE   0x02
#define MACON4_PUREPRE   0x01
// MAMXFLL         <---------------------- Maximum Frame Length Low Byte (MAMXFL<7:0>) ------>     14
// MAMXFLH         <---------------------- Maximum Frame Length High Byte (MAMXFL<15:8>) ---->     14
// MAPHSUP         r         —         —         r         RSTRMII   —         —         r         14
#define MAPHSUP_RSTRMII  0x08
// Legend: — = unimplemented, r = reserved bit. Shaded cells are not used.


#define MICMD_MIISCAN    0x02
#define MICMD_MIIRD      0x01
// ENC28J60 MISTAT Register Bit Definitions
#define MISTAT_NVALID    0x04
#define MISTAT_SCAN      0x02
#define MISTAT_BUSY      0x01

// ENC28J60 Packet Control Byte Bit Definitions
#define PKTCTRL_PHUGEEN  0x08
#define PKTCTRL_PPADEN   0x04
#define PKTCTRL_PCRCEN   0x02
#define PKTCTRL_POVERRIDE 0x01

/************************************************************************/
/* ENC28J60 PHY REGISTER                                                */
/************************************************************************/
#define PHCON1           0x00
#define PHSTAT1          0x01
#define PHHID1           0x02
#define PHHID2           0x03
#define PHCON2           0x10
#define PHSTAT2          0x11
#define PHIE             0x12
#define PHIR             0x13
#define PHLCON           0x14
// ENC28J60 PHY REGISTER SUMMARY
// Addr Name         Bit 15   Bit 14   Bit 13   Bit 12   Bit 11   Bit 10   Bit 9    Bit 8    Bit 7    Bit 6    Bit 5    Bit 4    Bit 3    Bit 2    Bit 1    Bit 0   Reset Values
// 0h   PHCON1       PRST     PLOOPBK  —        —        PPWRSV   r        —        PDPXMD   r        —        —        —        —        —        —        —       00-- 10-q 0--- ----
#define PHCON1_PRST      0x8000
#define PHCON1_PLOOPBK   0x4000
#define PHCON1_PPWRSV    0x0800
#define PHCON1_PDPXMD    0x0100
// 1h   PHSTAT1      —        —        —        PFDPX    PHDPX    —        —        —        —        —        —        —        —        LLSTAT   JBSTAT   —       ---1 1--- ---- -00-
#define PHSTAT1_PFDPX    0x1000
#define PHSTAT1_PHDPX    0x0800
#define PHSTAT1_LLSTAT   0x0004
#define PHSTAT1_JBSTAT   0x0002
// 2h   PHID1        <------------------------------------ PHY Identifier (PID18:PID3) = 0083h ------------------------------------------------------------->       0000 0000 1000 0011
// 3h   PHID2        <--------------------------- PHY Identifier (PID24:PID19) = 000101 PHY P/N (PPN5:PPN0) = 00h PHY Revision (PREV3:PREV0) = 00h --------->       0001 0100 0000 0000
// 0h   PHCON2       —        FRCLNK   TXDIS    r        r        JABBER   r        HDLDIS   r        r        r        r        r        r        r        r       -000 0000 0000 0000
#define PHCON2_FRCLINK   0x4000
#define PHCON2_TXDIS     0x2000
#define PHCON2_JABBER    0x0400
#define PHCON2_HDLDIS    0x0100
// 1h   PHSTAT2      —        —        TXSTAT   RXSTAT   COLSTAT  LSTAT    DPXSTAT  —        —        —        —        PLRITY   —        —        —        —       --00 00q- ---0 ----
#define PHSTAT2_TXSTAT   0x2000
#define PHSTAT2_RXSTAT   0x1000
#define PHSTAT2_COLSTAT  0x0800
#define PHSTAT2_LSTAT    0x0400
#define PHSTAT2_DPXSTAT  0x0200
#define PHSTAT2_PLRITY   0x0010
// 2h   PHIE         r        r        r        r        r        r        r        r        r        r        r        PLNKIE   r        r        PGEIE    r       0000 0000 0000 0000
#define PHIE_PLNKIE      0x0010
#define PHIE_PGEIE       0x0002
// 3h   PHIR         r        r        r        r        r        r        r        r        r        r        r        PLNKIF   r        PGIF     r        r       xxxx xxxx xx00 00x0
#define PHIR_PLNKIF      0x0010
#define PHIR_PGIF        0x0004
// 4h   PHLCON       r        r        r        r        <------- LACFG3:LACFG0 ---->        <------- LBCFG3:LBCFG0 ---->        < LFRQ1:LFRQ0 >   STRCH    r       0011 0100 0010 001x
// Legend: x= unknown, u= unchanged, — = unimplemented, q= value depends on condition, r = reserved, do not modify.

/*
3.3.1 READING PHY REGISTERS
When a PHY register is read, the entire 16 bits are obtained. To read from a PHY register:
1. Write the address of the PHY register to read from into the MIREGADR register.
2. Set the MICMD.MIIRD bit. The read operation begins and the MISTAT.BUSY bit is set.
3. Wait 10.24µs. Poll the MISTAT.BUSY bit to be certain that the operation is complete. While busy, the host controller should not start any MIISCAN operations or write to the MIWRH register.
   When the MAC has obtained the register contents, the BUSY bit will clear itself.
4. Clear the MICMD.MIIRD bit.
5. Read the desired data from the MIRDL and MIRDH registers. The order that these bytes are accessed is unimportant.

3.3.2 WRITING PHY REGISTERS
When a PHY register is written to, the entire 16 bits is written at once; selective bit writes are not implemented. If it is necessary to reprogram only select bits in the register, 
the controller must first read the PHY register, modify the resulting data and then write the data back to the PHY register. To write to a PHY register:
1. Write the address of the PHY register to write to into the MIREGADR register.
2. Write the lower 8 bits of data to write into the MIWRL register.
3. Write the upper 8 bits of data to write into the MIWRH register. Writing to this register auto-matically begins the MII transaction, so it must be written to after MIWRL. The MISTAT.BUSY bit becomes set.
   The PHY register will be written after the MII operation completes, which takes 10.24µs. When the write operation has completed, the BUSY bit will clear itself. The host controller should not start any MIISCAN or
   MIIRD operations while busy.
*/

/************************************************************************/
/* SPI operation codes                                                  */
/************************************************************************/
#define ENC28J60_READ_CTRL_REG       0x00
#define ENC28J60_READ_BUF_MEM        0x3A
#define ENC28J60_WRITE_CTRL_REG      0x40
#define ENC28J60_WRITE_BUF_MEM       0x7A
#define ENC28J60_BIT_FIELD_SET       0x80
#define ENC28J60_BIT_FIELD_CLR       0xA0
#define ENC28J60_SOFT_RESET          0xFF

/************************************************************************/
/* Ethernet Buffer                                                      */
/************************************************************************/
/*
The Ethernet buffer contains transmit and receive memory used by the Ethernet controller. The entire buffer is 8 kBytes (0x0000 - 0x1FFF), divided into separate receive and transmit buffer spaces.
The sizes and locations of transmit and receive memory are fully programmable by the host controller using the SPI interface.
*/

// Change this if you need more RX buffer
#define RX_BUFFER_SIZE    0x0600
// Const not changeable
#define ENC28J80_TOTAL_BUFFER_SIZE       0x1FFF
#define RXSTARTBUFFER     0x0
#define RXSTOPBUFFER      (ENC28J80_TOTAL_BUFFER_SIZE - RX_BUFFER_SIZE - 1)
#define TXSTART_INIT      (ENC28J80_TOTAL_BUFFER_SIZE - RX_BUFFER_SIZE)
#define TXSTOP_INIT       ENC28J80_TOTAL_BUFFER_SIZE
//
// Max frame length
#define MAX_FRAMELEN      1518

// Public Methods
void ENC28J60_Init(volatile avr32_spi_t *spi, uint8_t spiDeviceId, spi_flags_t spiFlags, uint32_t spiBaudrate, uint8_t* macaddr);
void ENC28J60_Clkout(uint8_t clk);
void ENC28J60_PhyRegisterWrite(uint8_t address, uint16_t data);
uint16_t ENC28J60_PacketReceived(uint16_t maxlen, uint8_t* packet);
void ENC28J60_PacketSend(uint16_t len, uint8_t* packet);

#endif
