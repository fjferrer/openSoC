/*   usb.h - The header file for usb.h.
 *
 *  Copyright (C) 2009  Rosales Victor (todoesverso@gmail.com)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef USB_H
#define USB_H

/**
 * Convert pointers to fit PIC memory type 
 **/
#define PTR16(x) ((unsigned int)(((unsigned long)x) & 0xFFFF))

/**
 * Define two new types of variables
 **/
typedef unsigned char byte; 
typedef unsigned int  word;

/**
 * Separate words into 2 varialbes type byte
 **/
#define LSB(x) (x & 0xFF)
#define MSB(x) ((x & 0xFF00) >> 8)

/**
 * Standard Request Codes USB 2.0 Spec Ref Table 9-4
 **/
#define GET_STATUS         0
#define CLEAR_FEATURE      1
#define SET_FEATURE        3
#define SET_ADDRESS        5
#define GET_DESCRIPTOR     6
#define SET_DESCRIPTOR     7
#define GET_CONFIGURATION  8
#define SET_CONFIGURATION  9
#define GET_INTERFACE     10
#define SET_INTERFACE     11
#define SYNCH_FRAME       12

/**
 * Descriptors Types
 **/
#define DEVICE_DESCRIPTOR        0x01
#define CONFIGURATION_DESCRIPTOR 0x02
#define STRING_DESCRIPTOR        0x03
#define INTERFACE_DESCRIPTOR     0x04
#define ENDPOINT_DESCRIPTOR      0x05

/**
 * Standard Feature Selectors
 **/
#define DEVICE_REMOTE_WAKEUP    0x01
#define ENDPOINT_HALT           0x00

/**
 * Buffer Descriptor bit masks (from PIC datasheet)
 **/
#define UOWN   0x80 /* USB Own Bit                              */
#define DTS    0x40 /* Data Toggle Synchronization Bit          */
#define KEN    0x20 /* BD Keep Enable Bit                       */
#define INCDIS 0x10 /* Address Increment Disable Bit            */
#define DTSEN  0x08 /* Data Toggle Synchronization Enable Bit   */
#define BSTALL 0x04 /* Buffer Stall Enable Bit                  */
#define BC9    0x02 /* Byte count bit 9                         */
#define BC8    0x01 /* Byte count bit 8                         */

/**
 *  Device states (USB spec Chap 9.1.1)
 **/
#define DETACHED     0
#define ATTACHED     1
#define POWERED      2
#define DEFAULT      3
#define ADDRESS      4
#define CONFIGURED   5

/**
 * BDT  - Buffer Descriptor Table
 * @stat: 
 * @Cnt:
 * @ADDR:
 *
 **/
typedef struct _BDT
{
    byte Stat;
    byte Cnt;
    word ADDR;
} BDT; 


/**
 * Global Variables 
 **/
extern byte deviceState; /* Visible device states (from USB 2.0, chap 9.1.1) */ 
extern byte selfPowered;
extern byte remoteWakeup;
extern byte currentConfiguration;

extern volatile BDT at 0x0400 ep0Bo; /* Endpoint #0 BD Out      */
extern volatile BDT at 0x0404 ep0Bi; /* Endpoint #0 BD In       */      
extern volatile BDT at 0x0408 ep1Bo; /* Endpoint #1 BD Out      */
extern volatile BDT at 0x040C ep1Bi; /* Endpoint #1 BD In       */      
extern volatile BDT at 0x0410 ep2Bo; /* Endpoint #2 BD Out      */
extern volatile BDT at 0x0414 ep2Bi; /* Endpoint #2 BD In       */

/**
 * setupPacketStruct - 
 *
 * Every device request starts with an 8 byte setup packet (USB 2.0, chap 9.3)
 * with a standard layout.  The meaning of wValue and wIndex will
 * vary depending on the request type and specific request.
 **/
typedef struct _setupPacketStruct {
    byte bmRequestType; /* D7: Direction, D6..5: Type, D4..0: Recipient      */
    byte bRequest;      /* Specific request                                  */
    byte wValue0;       /* LSB of wValue                                     */
    byte wValue1;       /* MSB of wValue                                     */
    byte wIndex0;       /* LSB of wIndex                                     */
    byte wIndex1;       /* MSB of wIndex                                     */
    word wLength;       /* Number of bytes to transfer if a data stage       */
    byte extra[56];     /* Fill out to same size as Endpoint 0 max buffer    */
} setupPacketStruct;

/**
 * Variable for Setup Packets
 **/
extern volatile setupPacketStruct SetupPacket;

/**
 * Size of the buffer for endpoint 0
 **/
#define E0SZ 64

/**
 * Size of data for BulkIN and BulkOut
 **/
#define INPUT_BYTES     7 
#define OUTPUT_BYTES    7

/**
 * IN/OUT Buffers
 * RxBuffer2 is only 1 byte long.
 **/
extern volatile byte TxBuffer[OUTPUT_BYTES];
extern volatile byte RxBuffer[INPUT_BYTES];
extern volatile byte TxBuffer2[OUTPUT_BYTES];
extern volatile byte RxBuffer2;

/**
 * Pointers inPtr and outPtr are used to move data between buffers from user
 * memory to dual port buffers of the USB module
 **/
extern byte *outPtr;        
extern byte *inPtr;         
extern unsigned int wCount; /* Total number of bytes to move */

// Funciones para uso del USB
/**
 * User functions to process USB transactions
 **/
void EnableUSBModule(void);
void ProcessUSBTransactions(void);

/**
 * Functions to read and write bulk endpoints
 **/
byte BulkOut(byte ep_num, byte *buffer, byte len);
byte BulkIn(byte ep_num, byte *buffer, byte len);

#endif /* USB_H */
