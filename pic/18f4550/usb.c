/*   usb.c - The main functions for usb handle.
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

#include <pic18fregs.h>
#include <string.h>
#include <stdio.h>
#include "usb.h"


/**
 * Device and configuration descriptors.  These are used as the
 * host enumerates the device and discovers what class of device
 * it is and what interfaces it supports.
**/

/**
 * Size in bytes of descriptors
 **/
#define DEVICE_DESCRIPTOR_SIZE  0x12
#define CONFIG_HEADER_SIZE      0x09
#define CONFIG_DESCRIPTOR_SIZE  0x25
/**
 * The total size of the configuration descriptor
 * 0x09	 +  0x09  +  0x07  +  0x07  =  0x20 
 **/

#define CFSZ CONFIG_HEADER_SIZE+CONFIG_DESCRIPTOR_SIZE

/** 
 * Struct of Configuration Descriptor
 * @configHeader:
 * @Descriptor:
 * 
 **/
typedef struct _configStruct {
    byte configHeader[CONFIG_HEADER_SIZE];
    byte Descriptor[CONFIG_DESCRIPTOR_SIZE]; 
} ConfigStruct;

/**
 * Global Variables
 **/

/**
 * Visible States (USB 2.0 Spec, chap 9.1.1)
 **/
byte deviceState;  
byte remoteWakeup;
byte deviceAddress;
byte selfPowered;
byte currentConfiguration;

/* Control Transfer Stages - see USB spec chapter 5                          */
/* Start of a control transfer (followed by 0 or more data stages)           */
#define SETUP_STAGE    0 
#define DATA_OUT_STAGE 1 /* Data from host to device                         */
#define DATA_IN_STAGE  2 /* Data from device to host                         */
#define STATUS_STAGE   3 /* Unused - if data I/O went ok, then back to setup */

byte ctrlTransferStage; /* Holds the current stage in a control transfer     */
byte requestHandled;    /* Set to 1 if request was understood and processed. */

byte *outPtr;           /* Data to send to the host                          */
byte *inPtr;            /* Data from the host                                */
word wCount;            /* Number of bytes of data                           */
byte RxLen;             /* # de bytes colocados dentro del buffer            */

/**
 * Device Descriptor
 **/
code byte deviceDescriptor[] = {
    DEVICE_DESCRIPTOR_SIZE, 0x01, /* bLength, bDescriptorType                */
    0x00, 0x02,                   /* bcdUSB (LB), bcdUSB (HB)                */
    0x00, 0x00,                   /* bDeviceClass, bDeviceSubClass           */
    0x00, E0SZ,                   /* bDeviceProtocl, bMaxPacketSize          */
    0xD8, 0x04,                   /* idVendor (LB), idVendor (HB)            */
    0x31, 0x75,                   /* idProduct (LB), idProduct (HB)          */
    0x01, 0x00,                   /* bcdDevice (LB), bcdDevice (HB)          */
    0x01, 0x02,                   /* iManufacturer, iProduct                 */
    0x00, 0x01                    /* iSerialNumber (none), bNumConfigurations*/
};

#define ISZ 7                /* wMaxPacketSize (low) of endopoint1IN         */
#define OSZ 7                /* wMaxPacketSize (low) of endopoint1OUT        */
#define OSZ2 1               /* wMaxPacketSize (low) of endopoint2OUT        */

/**
 * Configuration Descriptor
 **/
code ConfigStruct configDescriptor = {
    {
    /* Descriptor de configuracin, contiene el descriptor de la clase        */
    CONFIG_HEADER_SIZE, 0x02, /* bLength, bDescriptorType (Configuration)    */
    CFSZ, 0x00,               /* wTotalLength (low), wTotalLength (high)     */
    0x01, 0x01,               /* bNumInterfaces, bConfigurationValue         */
    0x00, 0xA0,               /* iConfiguration, bmAttributes ()             */
    0x32,                     /* bMaxPower (100 mA)                          */
    },
    {
    /* Interface Descriptor   */
    0x09, 0x04,               /* bLength, bDescriptorType (Interface)        */
    0x00, 0x00,               /* bInterfaceNumber, bAlternateSetting         */
    0x04, 0x07,               /* bNumEndpoints, bInterfaceClass (Printer)    */
    0x01, 0x00,               /* bInterfaceSubclass, bInterfaceProtocol      */
    0x00,                     /* iInterface                                  */
    /* EP1 IN */
    0x07, 0x05,               /* bLength, bDescriptorType (Endpoint)         */
    0x81, 0x02,               /* bEndpointAddress, bmAttributes (Bulk)       */
    ISZ, 0x00,                /* wMaxPacketSize (L), wMaxPacketSize (H)      */
    0x01,                     /* bInterval (1 millisecond)                   */
    /* EP1 OUT */
    0x07, 0x05,               /* bLength, bDescriptorType (Endpoint)         */
    0x01, 0x02,               /* bEndpointAddress, bmAttributes (Bulk)       */
    OSZ, 0x00,                /* wMaxPacketSize (L), wMaxPacketSize (H)      */
    0x01,                     /* bInterval (1 millisecond)                   */
    /* EP2 IN */
    0x07, 0x05,               /* bLength, bDescriptorType (Endpoint)         */
    0x82, 0x02,               /* bEndpointAddress, bmAttributes (Bulk)       */
    ISZ, 0x00,                /* wMaxPacketSize (L), wMaxPacketSize (H)      */
    0x01,                     /* bInterval (1 millisecond)                   */
    /* EP2 OUT */
    0x07, 0x05,               /* bLength, bDescriptorType (Endpoint)         */
    0x02, 0x02,               /* bEndpointAddress, bmAttributes (Bulk)       */
    OSZ2, 0x00,               /* wMaxPacketSize (L), wMaxPacketSize (H)      */
    0x01,                     /* bInterval (1 millisecond)                   */
    } 
};

/**
 * According to USB spec, the 0 index has the lenguage code 
 **/
code byte stringDescriptor0[] = {
    0x04, STRING_DESCRIPTOR,    /* bLength, bDscType                         */
    0x09, 0x04,		        /* Stablish wLANGID with 0x0409 (English,USA)*/
};

code byte stringDescriptor1[] = {
    0x1A, STRING_DESCRIPTOR,                    /* bLength, bDscType */
    'E', 0x00, 'm', 0x00, 'b', 0x00, 'o', 0x00,
    's', 0x00, 's', 0x00, 'e', 0x00, 'r', 0x00,
    ' ', 0x00, ' ', 0x00,
    ' ', 0x00, ' ', 0x00,
};

code byte stringDescriptor2[] = {
    0x20, STRING_DESCRIPTOR,                    /* bLength, bDscType */
    'U', 0x00, 'S', 0x00, 'B', 0x00, ' ', 0x00,
    'B', 0x00, 'r', 0x00, 'a', 0x00, 'i', 0x00,
    'l', 0x00, 'l', 0x00, 'e', 0x00, ' ', 0x00,
    '0', 0x00, '.', 0x00, '1', 0x00,
};

volatile BDT at 0x0400 ep0Bo; /* Endpoint #0 BD OUT     */
volatile BDT at 0x0404 ep0Bi; /* Endpoint #0 BD IN      */
volatile BDT at 0x0408 ep1Bo; /* Endpoint #1 BD OUT     */
volatile BDT at 0x040C ep1Bi; /* Endpoint #1 BD IN      */
volatile BDT at 0x0410 ep2Bo; /* Endpoint #2 BD OUT     */
volatile BDT at 0x0414 ep2Bi; /* Endpoint #2 BD IN      */

/*
 * Put endpoint 0 buffers into dual port RAM
 **/
#pragma udata usbram5 SetupPacket controlTransferBuffer
volatile setupPacketStruct SetupPacket;
volatile byte controlTransferBuffer[E0SZ];

/**
 * Put I/O buffersinto dual port USB RAM
 **/
#pragma udata usbram5 RxBuffer TxBuffer 
#pragma udata usbram6 RxBuffer2 TxBuffer2

/** 
 * Specific Buffers
 **/
volatile byte RxBuffer[OSZ];
volatile byte TxBuffer[ISZ];
volatile byte RxBuffer2;
volatile byte TxBuffer2[ISZ];

/** 
 * Enpoints Initialization
 **/
void InitEndpoint(void)
{
    	/* Turn on both IN and OUT for this endpoints (EP1 & EP2)       */
        UEP1 = 0x1E;  /* See PIC datasheet, page 169 (USB E1 Control)   */
        UEP2 = 0x1E;  /* Same as above for EP2                          */
	/** 
         * Load EP1's BDT
         **/
        ep1Bo.Cnt = sizeof(RxBuffer);
	ep1Bo.ADDR = PTR16(&RxBuffer);
	ep1Bo.Stat = UOWN | DTSEN;
	ep1Bi.ADDR = PTR16(&TxBuffer);
	ep1Bi.Stat = DTS;
	/** 
         * Load de EP2's BDT
         **/
        ep2Bo.Cnt = RxBuffer2;
	ep2Bo.ADDR = PTR16(&RxBuffer2);
	ep2Bo.Stat = UOWN | DTSEN;
	ep2Bi.ADDR = PTR16(&TxBuffer2);
	ep2Bi.Stat = DTS;
}

/**
 * BulkIn() - Makes an IN and returns the amount of bytes transfered
 * @ep_num:   Number of the endpoint to be used (only EP1 & EP2)
 * @buffer:   Buffer of the data to be transfered
 * @len:      Lenght of the bytes transfered
 *
 * The function checks if the BD is owned by the CPU retunrning 0 if so.
 * (PIC 18F4550 datasheet page 171 section 17.4.1.1 - Buffer Ownership)
 *
 * Send up to len bytes to the host.  The actual number of bytes sent is returned
 * to the caller.  If the send failed (usually because a send was attempted while
 * the SIE was busy processing the last request), then 0 is returned.
 **/ 
byte BulkIn(byte ep_num, byte *buffer, byte len)
{
	byte i;
        /**
         * If slelected EP1
         **/
        if (ep_num == 1) {        
        /** 
         * If SIE owns the BD do not try to send anything and return 0. 
         **/
	        if (ep1Bi.Stat & UOWN)
		        return 0;
	/**
         * Truncate requests that are too large 
         **/
	        if(len > ISZ)       
		        len = ISZ;
        /**
        * Copy data from user's buffer to dual-port ram buffer
        **/
	        for (i = 0; i < len; i++)
		        TxBuffer[i] = buffer[i];
        /**
         * Toggle the data bit and give control to the SIE
         **/
	        ep1Bi.Cnt = len;
	        if(ep1Bi.Stat & DTS)
		        ep1Bi.Stat = UOWN | DTSEN;
        	else
	        	ep1Bi.Stat = UOWN | DTS | DTSEN;

	        return len;
        }
        /**
        * If selected EP2 (same as above)
        **/
        else if (ep_num == 2) {
	        if (ep2Bi.Stat & UOWN)
		        return 0;
	
	        if(len > ISZ)
		        len = ISZ;
	
        	for (i = 0; i < len; i++)
	        	TxBuffer2[i] = buffer[i];
	
        	ep2Bi.Cnt = len;
	        if(ep2Bi.Stat & DTS)
		        ep2Bi.Stat = UOWN | DTSEN;
        	else
	        	ep2Bi.Stat = UOWN | DTS | DTSEN;

        	return len;
        }
        /**
         * In case of error (ep_num != 1|2) return 0
         **/
        return 0;
}

/**
 * BulkOut() - Reads 'len' bytes from the dual-port buffer
 * @ep_num:    Number of the endpoint to be read
 * @buffer:    Buffer to collect the data
 * @len:       Lenght of the data recived
 *
 * Actual number of bytes put into buffer is returned.  
 * If there are fewer than len bytes, then only the available
 * bytes will be returned.  Any bytes in the buffer beyond len 
 * will be discarded.
 **/
byte BulkOut(byte ep_num, byte *buffer, byte len) 
{
        RxLen = 0;
        /**
        * If selected EP1
        **/
        if (ep_num == 1) {
        /**
        * If the SIE doesn't own the output buffer descriptor, 
        * then it is safe to pull data from it
        **/
	        if(!(ep1Bo.Stat & UOWN)) {
        /**
         * See if the host sent fewer bytes that we asked for.
         **/
		        if(len > ep1Bo.Cnt)
			        len = ep1Bo.Cnt;
        /**
         * Copy data from dual-ram buffer to user's buffer
         **/
		for (RxLen = 0; RxLen < len; RxLen++)
			buffer[RxLen] = RxBuffer[RxLen];
        /**
         * Resets the OUT buffer descriptor so the host can send more data
         **/
		ep1Bo.Cnt = sizeof(RxBuffer);
		if (ep1Bo.Stat & DTS)
			ep1Bo.Stat = UOWN | DTSEN;
		else
			ep1Bo.Stat = UOWN | DTS | DTSEN;
                } 
        }
        /**
        * The same that above but for EP2. In this case the 'len'
        * is hardcoded to '1' because only 1 byte is needed for
        * instructions for the printer.
        **/
        else if (ep_num == 2) {
	        if (!(ep2Bo.Stat & UOWN)) {
		        if (len > ep2Bo.Cnt)
			        len = ep2Bo.Cnt;

        		buffer[0] = RxBuffer2;
                        RxLen = 1;
        
	        	ep2Bo.Cnt = 1;
		        if (ep2Bo.Stat & DTS)
			        ep2Bo.Stat = UOWN | DTSEN;
	        	else
		        	ep2Bo.Stat = UOWN | DTS | DTSEN;
	        }       
        }       
        /**
        * Retunrs the lenght of the data recived
        **/
        return RxLen;
}

/**
 * Start of code to process standard requests (USB spec chapter 9)
 **/

/**
 * GetDescriptor(void) - Process Descriptors requests
 *
 * 
 *
 **/
static void GetDescriptor(void)
{
        /**
         * If direction is device --> host
         **/
        if (SetupPacket.bmRequestType == 0x80) {
                /**
                 * MSB has descriptor type
                 * LSB has descriptor Index
                 **/
                byte descriptorType  = SetupPacket.wValue1;
                byte descriptorIndex = SetupPacket.wValue0; 
                /**
                * If request for device
                **/
                if (descriptorType == DEVICE_DESCRIPTOR) {
                        requestHandled = 1;		
                        /**
                         * Points to device descriptor's address 
                         **/
                        outPtr = (byte *) &deviceDescriptor;
                        wCount = DEVICE_DESCRIPTOR_SIZE; 
                }
                /**
                 * If requested for descriptor's configuration
                 **/
	        else if (descriptorType == CONFIGURATION_DESCRIPTOR) {
                        requestHandled = 1;
                        outPtr = (byte *) &configDescriptor;
                        wCount = configDescriptor.configHeader[2]; 
                        /*** Note: SDCC may generate bad code with this ***/
                }
                /**
                 * If requested for descriptor's string
                 **/
	        else if (descriptorType == STRING_DESCRIPTOR) {
                        requestHandled = 1;
                        if (descriptorIndex == 0)
                                /* Language encoding */
                                outPtr = (byte *) &stringDescriptor0;
                        else if (descriptorIndex == 1)  
                                /* Author name */
                                outPtr = (byte *) &stringDescriptor1;
                        else
                                /* Device name */
                                outPtr = (byte *) &stringDescriptor2;
                        wCount = *outPtr;	
                } else {
                /**
                 * A blinking LED may be used if error occur
                 **/
                }
    }
}

// Solicitud GET_STATUS (los datos parecen estar contenidos en el paquete Setup) 
/**
 * GetStatus(void) - 
 *
 **/
static void GetStatus(void)
{
	/** 
         * Mask Off the Recipient bits 
         *
         * From USB spec chapter 9.4.5
         * ---------------------------
         *  bmRequestType
         * 10000000 -> Device
         * 10000001 -> Interface
         * 10000010 -> Endpoint
         **/
        byte recipient = SetupPacket.bmRequestType & 0x1F;
        controlTransferBuffer[0] = 0;
        controlTransferBuffer[1] = 0;

        /**
         * Requested for Device
         **/
        if (recipient == 0x00) {
                requestHandled = 1;
	        if (selfPowered)
                        /* Set SelfPowered bit */
		        controlTransferBuffer[0] |= 0x01;
                if (remoteWakeup)
                        /* Set RemoteWakeUp bit */
		        controlTransferBuffer[0] |= 0x02; 
        }
        /**
         * Requested for Interface
         **/
        else if (recipient == 0x01) 
                requestHandled = 1;
        /**
         * Requested for Endpoint
         **/
        else if (recipient == 0x02) { 
                byte endpointNum = SetupPacket.wIndex0 & 0x0F;
                byte endpointDir = SetupPacket.wIndex0 & 0x80;
                requestHandled = 1;
        /**
         * Endpoint descriptors are 8 bytes long, with each in and out 
         * taking 4 bytes within the endpoint 
         * (See PIC18F4550 'Buffer Descriptors and the Buffer Descriptor 
         * Table' chapter 17.4)
         **/
                inPtr = (byte *)&ep0Bo + (endpointNum * 8); 

                if (endpointDir) 
                        inPtr += 4;

                if (*inPtr & BSTALL)
                        controlTransferBuffer[0] = 0x01;
        }
        /**
         * If a request was handled (reuestHandled) move OUT pointer to the
         * controlTransferBuffer adress.
         **/
	if (requestHandled) {
		outPtr = (byte *)&controlTransferBuffer;
		wCount = 2;
	}
}

/**
 * SetFeature(void) -
 *
 **/
static void SetFeature(void)
{
        byte recipient = SetupPacket.bmRequestType & 0x1F;
        byte feature = SetupPacket.wValue0;
        /**
         * Requested for Device
         **/
        if (recipient == 0x00) {
                if (feature == DEVICE_REMOTE_WAKEUP) {
                        requestHandled = 1;

                if (SetupPacket.bRequest == SET_FEATURE)
                        remoteWakeup = 1;
                else
                        remoteWakeup = 0;
                }
        }
        /**
         * Requested for Endpoint
         **/
        else if (recipient == 0x02) {
                byte endpointNum = SetupPacket.wIndex0 & 0x0F;
                byte endpointDir = SetupPacket.wIndex0 & 0x80;

                if ((feature == ENDPOINT_HALT) && (endpointNum != 0)) {
                        requestHandled = 1;
                        inPtr = (byte *) &ep0Bo + (endpointNum * 8); 

                        if (endpointDir)
                                inPtr += 4;

                        if (SetupPacket.bRequest == SET_FEATURE)
                                *inPtr = 0x84;
                        else {
                                if (endpointDir == 1)
                                        *inPtr = 0x00;
                                else
                                        *inPtr = 0x88;
                        }
                }
        }
}

/**
 * ProcessStandarRequest(void) -
 *
 **/
void ProcessStandardRequest(void)
{
        /**
         * See USB 2.0 spec chapter 9.3
         **/
        byte request = SetupPacket.bRequest; 

        /**
         * Only attend Standar requests D6..5 == 00b
         **/
        if ((SetupPacket.bmRequestType & 0x60) != 0x00)
	        return;

        if (request == SET_ADDRESS) {
        /**
         * Set the address of the device.  All future requests
         * will come to that address.  Can't actually set UADDR
         * to the new address yet because the rest of the SET_ADDRESS
         * transaction uses address 0.
         **/
                requestHandled = 1;
                deviceState = ADDRESS;
                deviceAddress = SetupPacket.wValue0;
        }

        else if (request == GET_DESCRIPTOR) {
                GetDescriptor();
        }

        else if (request == SET_CONFIGURATION) {
                requestHandled = 1;
                currentConfiguration = SetupPacket.wValue0;
            /**
             * TBD: ensure the new configuration value is one that  
             * exists in the descriptor.
             **/
                if (currentConfiguration == 0)
                 /**
                  * If configuration value is zero, device is put in
                  * address state (USB 2.0 spec - 9.4.7) 
                  **/
                        deviceState = ADDRESS;
                else {
                        deviceState = CONFIGURED;
   		        InitEndpoint();
                }
        }

        else if (request == GET_CONFIGURATION) {
                requestHandled = 1;
                outPtr = (byte*)&currentConfiguration;
                wCount = 1;
        }

        else if (request == GET_STATUS) {
                GetStatus();
        }

        else if ((request == CLEAR_FEATURE) || (request == SET_FEATURE)) {
                SetFeature();
        }

        else if (request == GET_INTERFACE) {
            /**
             * No support for alternate interfaces.  Send
             * zero back to the host.
             **/
                requestHandled = 1;
                controlTransferBuffer[0] = 0;
                outPtr = (byte *) &controlTransferBuffer;
                wCount = 1;
        }
        
        else if (request == SET_INTERFACE) {
            /**
             * No support for alternate interfaces - just ignore.
             **/
                requestHandled = 1;
        }
    /* else if (request == SET_DESCRIPTOR)      */
    /* else if (request == SYNCH_FRAME)         */
    /* else                                     */
}

/**
 * InDataStage(void) - Data stage for a Control Transfer.
 *
 * Data stage for a Control Transfer that sends data to the host.
 **/
void InDataStage(void)
{
        byte i;
        word bufferSize;
        /* Determine how many bytes are going to the host */
        if (wCount < E0SZ)
                bufferSize = wCount;
        else
                bufferSize = E0SZ;
        /**
        * Load the high two bits of the byte count into BC8:BC9
        **/
        ep0Bi.Stat &= ~(BC8 | BC9); /* Delete BC8 and BC9 */
        ep0Bi.Stat |= (byte) ((bufferSize & 0x0300) >> 8);
        ep0Bi.Cnt = (byte) (bufferSize & 0xFF);
        ep0Bi.ADDR = PTR16(&controlTransferBuffer);
        /**
        * Update the number of bytes that still need to be sent.  Getting
        * all the data back to the host can take multiple transactions, so
        * we need to track how far along we are.
        **/
        wCount = wCount - bufferSize;
        /**
        * Move data to the USB output buffer from wherever it sits now.
        **/
        inPtr = (byte *)&controlTransferBuffer;

        for (i=0;i<bufferSize;i++)
                *inPtr++ = *outPtr++; 
}

/**
 * OutDataStage(void) - Data stage for a Control Transfer 
 *
 * Data stage for a Control Transfer that reads data from the host
 **/
void OutDataStage(void)
{
        word i, bufferSize;

        bufferSize = ((0x03 & ep0Bo.Stat) << 8) | ep0Bo.Cnt;
        /**
        * Accumulate total number of bytes read
        **/
        wCount = wCount + bufferSize;

        outPtr = (byte*)&controlTransferBuffer;

        for (i=0;i<bufferSize;i++)
                *inPtr++ = *outPtr++; 
}

/**
 * SetupStage(void) - 
 *
 * Process the Setup stage of a control transfer.  This code initializes the
 * flags that let the firmware know what to do during subsequent stages of
 * the transfer.
 **/
void SetupStage(void)
{
        /**
        *  Note: Microchip says to turn off the UOWN bit on the IN direction as
        * soon as possible after detecting that a SETUP has been received.
        **/
        ep0Bi.Stat &= ~UOWN;
        ep0Bo.Stat &= ~UOWN;

        /* Initialize the transfer process */
        ctrlTransferStage = SETUP_STAGE;
        requestHandled = 0; /* Default is that request hasn't been handled */
        wCount = 0;         /* No bytes transferred */
        /**
        * See if this is a standard (as definded in USB chapter 9) request
        **/
        ProcessStandardRequest();

        if (!requestHandled) {
        /**
         * If this service wasn't handled then stall endpoint 0
         **/
                ep0Bo.Cnt = E0SZ;
                ep0Bo.ADDR = PTR16(&SetupPacket);
                ep0Bo.Stat = UOWN | BSTALL;
                ep0Bi.Stat = UOWN | BSTALL;
        }

        else if (SetupPacket.bmRequestType & 0x80) {
        /**
         * Direction: Device --> Host
         **/
                if(SetupPacket.wLength < wCount)
                        wCount = SetupPacket.wLength;

                InDataStage();
                ctrlTransferStage = DATA_IN_STAGE;
        /**
         * Reset the out buffer descriptor for endpoint 0
         **/
                ep0Bo.Cnt = E0SZ;
                ep0Bo.ADDR = PTR16(&SetupPacket);
                ep0Bo.Stat = UOWN;
        /**
         * Set the in buffer descriptor on endpoint 0 to send data
         **/
                ep0Bi.ADDR = PTR16(&controlTransferBuffer);
         /**
          * Give to SIE, DATA1 packet, enable data toggle checks
          **/
                ep0Bi.Stat = UOWN | DTS | DTSEN; 
        } else {
        /**
         * Direction: Host --> Device
         **/
                ctrlTransferStage = DATA_OUT_STAGE;
        /**
         * Clear the input buffer descriptor 
         **/
                ep0Bi.Cnt = 0;
                ep0Bi.Stat = UOWN | DTS | DTSEN;
        /**
         * Set the out buffer descriptor on endpoint 0 to receive data
         **/
                ep0Bo.Cnt = E0SZ;
                ep0Bo.ADDR = PTR16(&controlTransferBuffer);
        /**
         * Give to SIE, DATA1 packet, enable data toggle checks
         **/
                ep0Bo.Stat = UOWN | DTS | DTSEN;
        }
        /**
         * Enable SIE token and packet processing
         **/
        UCONbits.PKTDIS = 0;
}

/**
 * WaitForSetupStage(void) - Configures the buffer descriptor for EP0
 *
 * Configures the buffer descriptor for endpoint 0 so that it is waiting for
 * the status stage of a control transfer.
 **/
void WaitForSetupStage(void)
{
        ctrlTransferStage = SETUP_STAGE;
        ep0Bo.Cnt = E0SZ;
        ep0Bo.ADDR = PTR16(&SetupPacket);
        ep0Bo.Stat = UOWN | DTSEN; /* Give to SIE, enable data toggle checks */
        ep0Bi.Stat = 0x00;         /* Give control to CPU */
}

/**
 * ProcessControlTransfer(void) -
 *
 * This is the starting point for processing a Control Transfer.  The code directly
 * follows the sequence of transactions described in the USB spec chapter 5.  The
 * only Control Pipe in this firmware is the Default Control Pipe (endpoint 0).
 * Control messages that have a different destination will be discarded.
 **/
void ProcessControlTransfer(void)
{
        if (USTAT == 0) {
        /* Endpoint 0:OUT                       */
        /* Pull PID from middle of BD0STAT      */
                byte PID = (ep0Bo.Stat & 0x3C) >> 2;

	        if (PID == 0x0D)
        /**
         * SETUP PID - a transaction is starting
         **/
                        SetupStage();

                else if (ctrlTransferStage == DATA_OUT_STAGE) {
        /**
        * Complete the data stage so that all information has
        * passed from host to device before servicing it.
        **/
                        OutDataStage();
        /**
         * Turn control over to the SIE and toggle the data bit
         **/
                if(ep0Bo.Stat & DTS)
                        ep0Bo.Stat = UOWN | DTSEN;
                else
                        ep0Bo.Stat = UOWN | DTS | DTSEN;
                } else {
        /**
        * Prepare for the Setup stage of a control transfer
        **/
                        WaitForSetupStage();
                }
        } 

        else if (USTAT == 0x04) {
        /**
         * Endpoint 0:IN
         **/
                if ((UADDR == 0) && (deviceState == ADDRESS)) {
                        UADDR = SetupPacket.wValue0;

                        if(UADDR == 0)
        /**
         * If we get a reset after a SET_ADDRESS, then we need
         * to drop back to the Default state.
         **/
                                deviceState = DEFAULT;
                }

        if (ctrlTransferStage == DATA_IN_STAGE) {
        /**
        * Start (or continue) transmitting data
        **/
            InDataStage();
        /**
        * Turn control over to the SIE and toggle the data bit
        **/
            if(ep0Bi.Stat & DTS)
                ep0Bi.Stat = UOWN | DTSEN;
            else
                ep0Bi.Stat = UOWN | DTS | DTSEN;
        } else {
        /**
        * Prepare for the Setup stage of a control transfer
        **/     
            WaitForSetupStage();
        }
    }
        /* else */

}

/**
 * EnableUSBModule(void) -
 *
 **/
void EnableUSBModule(void)
{
        if (UCONbits.USBEN == 0) {
                UCON = 0;
                UIE = 0;
                UCONbits.USBEN = 1;
                deviceState = ATTACHED;
        }
        /**
        * If we are attached and no single-ended zero is detected, then
        * we can move to the Powered state.
        **/
        if ((deviceState == ATTACHED) && !UCONbits.SE0) {
                UIR = 0;
                UIE = 0;
                UIEbits.URSTIE = 1;
                UIEbits.IDLEIE = 1;
                deviceState = POWERED;
        }
}

/**
 * UnSuspend(void) -
 *
 **/
void UnSuspend(void)
{
        UCONbits.SUSPND = 0;
        UIEbits.ACTVIE = 0;
        UIRbits.ACTVIF = 0;
}

/**
 * StartOfFrame(void) - 
 *
 * Full speed devices get a Start Of Frame (SOF) packet every 1 millisecond.
 * Nothing is currently done with this interrupt (it is simply masked out).
 **/
void StartOfFrame(void)
{
        /** 
        * TBD: Add a callback routine to do something
        **/
        UIRbits.SOFIF = 0;
}

/**
 * Stall(void) -
 *
 * This routine is called in response to the code stalling an endpoint.
 **/
void Stall(void)
{
        if (UEP0bits.EPSTALL == 1) {
        /**
         * Prepare for the Setup stage of a control transfer
         **/
                WaitForSetupStage();
                UEP0bits.EPSTALL = 0;
        }
        UIRbits.STALLIF = 0;
}


/**
 * BusReset(void) -
 *
 **/
void BusReset()
{
        UEIR  = 0x00;
        UIR   = 0x00;
        UEIE  = 0x9f;
        UIE   = 0x7b;
        UADDR = 0x00;
        /**
        * Set endpoint 0 as a control pipe
        **/
        UEP0 = 0x16;
       /**
        * Flush any pending transactions
        **/
        while (UIRbits.TRNIF == 1)
                UIRbits.TRNIF = 0;
        /**
         *  Enable packet processing
         **/
        UCONbits.PKTDIS = 0;
        /**
         * Prepare for the Setup stage of a control transfer
         **/
        WaitForSetupStage();
        remoteWakeup = 0;               /* Remote wakeup is off by default */ 
        selfPowered = 0;                /* Self powered is off by default  */
        currentConfiguration = 0;       /* Clear active configuration      */
        deviceState = DEFAULT;
}

/**
 * ProcessUSBTransactions(void) - 
 *
 * Main entry point for USB tasks.  
 * Checks interrupts, then checks for transactions.
 **/
void ProcessUSBTransactions(void)
{
        /**
         * See if the device is connected yet.
         **/
        if (deviceState == DETACHED)
                return;
        /**
         * If the USB became active then wake up from suspend
        **/
        if (UIRbits.ACTVIF && UIEbits.ACTVIE)
                UnSuspend();
        /**
         * If we are supposed to be suspended, then don't try performing any
         * processing.
         **/
        if (UCONbits.SUSPND == 1)
                return;
        /**
         * Process a bus reset
         **/
        if (UIRbits.URSTIF && UIEbits.URSTIE)
                BusReset();
        /**
         * Process a suspend
         **/
        if (UIRbits.IDLEIF && UIEbits.IDLEIE)
        /**
         * Process a SOF
         **/
        if (UIRbits.SOFIF && UIEbits.SOFIE)
                StartOfFrame();
        /**
         * Process a Stall
         **/
        if (UIRbits.STALLIF && UIEbits.STALLIE)
                Stall();
        /**
         * Process error - Clear errors
         * TBD: See where it came from.
         **/
        if (UIRbits.UERRIF && UIEbits.UERRIE)
                UIRbits.UERRIF = 0;
        /**
         *  Unless we have been reset by the host, no need to keep processing
         **/
        if (deviceState < DEFAULT)
                return;
        /**
         * A transaction has finished. Try default processing on endpoint 0.
         **/
        if (UIRbits.TRNIF && UIEbits.TRNIE) {
                ProcessControlTransfer();
        /**
         *  Turn off interrupt
         **/
                UIRbits.TRNIF = 0;
        }
}


#if 0
// Test - put something into EEPROM
code at 0xF00000 word dataEEPROM[] =
{
    0, 1, 2, 3, 4, 5, 6, 7,
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};
#endif
