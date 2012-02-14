/*   main.c - The main program of the firmware.
 *
 *  Copyright (C) 2008  Rosales Victor (todoesverso@gmail.com)
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
#include <stdio.h>
#include "usb.h"

/**
 *
 * Constant declaration
 *
 **/
#define NUMLINES 7

/** 
 * Instruction codes from the host 
 **/
#define RESET           0x00
#define PRINT           0x01
#define MOV_SHORT       0x02
#define MOV_LONG        0x03
#define ROT_SHORT       0x04
#define ROT_LONG        0x05
#define PULL_PAPER      0x06
#define STATUS          0x07

/**
 * Types of movements
 **/
/* Horizontal */
#define SHORT_OUT 1
#define LONG_OUT  2

#define LEFT    1
#define RIGHT   0

/* Vertical */
#define SHORT_STEPS_OUT 4
#define LONG_STEPS_OUT  7

#define UP      1
#define DOWN    0

/**      
 * Physical devices (PORT D)
 **/
#define SENS_PAPER PORTDbits.RD3
#define SENS_CAR   PORTDbits.RD2
#define HAMMER     PORTDbits.RD1

#define CAR	1
#define	ROLLER	0

/** 
 * NOTE: 
 *     There are problems related with GET_FEATURE 
 *     when working under 48 MHZ
 *
 * Current:
 *     23 mA @ 16 MHz
 *     26 mA @ 24 MHz
 *     35 mA @ 48 MHz
 **/
/* Fuses configuration */
#if defined(pic18f4550)
code char at 0x300000 CONFIG1L = 0x20;	/* USB clk 96MHz PLL/2, PLL 4MHz */
code char at 0x300001 CONFIG1H = 0x0e;	/* HSPLL                         */
code char at 0x300002 CONFIG2L = 0x20;	/* USB regulator enable, PWRT On */
code char at 0x300003 CONFIG2H = 0x00;	/* Watchdog OFF                  */
code char at 0x300004 CONFIG3L = 0xff;	/*         ***UNUSED***          */
code char at 0x300005 CONFIG3H = 0x81;	/* MCLR enabled, PORTB digital   */
code char at 0x300006 CONFIG4L = 0x80;	/* all OFF except STVREN         */
code char at 0x300007 CONFIG4H = 0xff;	/*         ***UNUSED***          */
code char at 0x300008 CONFIG5L = 0xff;	/* No code read protection       */
code char at 0x300009 CONFIG5H = 0xff;	/* No data/boot read protection  */
code char at 0x30000A CONFIG6L = 0xff;	/* No code write protection      */
code char at 0x30000B CONFIG6H = 0xff;	/* No data/boot/table protection */
code char at 0x30000C CONFIG7L = 0xff;	/* No table read protection      */
code char at 0x30000D CONFIG7H = 0xff;	/* No boot table protection      */
#endif

/**
 * Buffers for Enpoint 1 (Data bus)
 **/
volatile byte txBuffer[INPUT_BYTES];
volatile byte rxBuffer[OUTPUT_BYTES];
volatile byte echoVector[INPUT_BYTES];
//unsigned int adval; //ADC Value
char adval; //ADC Value

/**
 * Buffers for Enpoint 2 (Instruction bus)
 **/
volatile byte instruction;

/**
 * UserInit(void) - Initialize the PIC registers
 *
 * Here should be initialized all registers of the PIC. 
 * This would be the state needed at boot time.
 **/
void UserInit(void)
{
  /* TODO CHANGE FOR A DEFAULT VALUE AND ADDIT INTO A ADCONVERSION FUNCTION
     BECAUSE THIS IS THE REGISTER TO SET THE CHANNEL FOR CONVERSION  */
  ADCON0 = 0xFF;             /* Set RA4 as output                    */

  /* TODO CHANGE ADCON1 TO SET ANALOG INPUT FOR PORTE/AN5,6,7 PINS   */
  ADCON1 = 0x00;             /* Set all I/O as digital               */

  TRISB = 0x00;              /* Set all pins of PORTB as output      */
  TRISD = 0xc;               /* Set pins 4,3 as in and 1,2 as out    */

  /* SET THE TRISE CONFIGURATION ACCORDING TO THE AD REQs            */
  TRISEbits.TRISE0 = 1;      /* Set pins of AD for ouput/analog      */
  TRISEbits.TRISE1 = 1;      /* Set pins of AD for ouput/analog      */
  TRISEbits.TRISE2 = 1;      /* Set pins of AD for ouput/analog      */

  /* Set the reference voltages as VDD and VSS                       */
  ADCON1bits.VCFG0=0;
  ADCON1bits.VCFG1=0;

  /* Configure Justification, Acquisition time and conversion clock  */
  ADCON2=0b10001010;

  INTCON = 0;                /* Tunr off all interrupts              */
  INTCON2 = 0;               /* Tunr off all interrupts              */
  PORTB = 0x00;              /* Start with motors turned off         */
  HAMMER = 0;                /* Start with hammer turned off         */
}

/**
 * delay() -    Routine that generates a simple delay
 *
 * @ms:         Amount of cycles that will be executed
 *
 * This routine is not an accurate delay, it was implemented this way because
 * using the internal timer requiers more program space.
 **/
void delay(byte ms)
{
	byte i;
	while (ms--)
		for (i = 0; i < 200; i++);	/* Experimental value   */
}

/**
 * motors_off(void) - Truns off both motors
 *
 * This function just turn off both motors by setting PORTB to 0
 **/
void motors_off(void)
{
	PORTB = 0x00;
}

/**
 * move() -     Function to move both motors
 * @loops:      Number of loops of a complete sequence
 * @direction:  Direction to move (RIGHT/LEFT, UP/DOWN)
 * @motor:	Which motor to move (CAR/ROLLER)
 *
 * This function moves each motor in a defined direction and an specific
 * amount of turns depending on the parameters that are passed to it.
 **/
void move(byte loops, byte direction, byte motor)
{
	byte stepsI[8] =
	    { 0x77, 0x33, 0xbb, 0x99, 0xdd, 0xcc, 0xee, 0x66 };
	byte stepsD[8] =
	    { 0x66, 0xee, 0xcc, 0xdd, 0x99, 0xbb, 0x33, 0x77 };
	byte val, i, loops_aux;

	if (direction)
		for (loops_aux = 0; loops_aux < loops; loops_aux++) {
			for (i = 0; i < 8; i++) {
				val = stepsI[i];
				if (motor) {
					PORTB =
					    (PORTB & 0xf0) | (val & 0x0f);
					delay(50);
				} else {
					PORTB =
					    (PORTB & 0x0f) | (val & 0xf0);
					delay(50);
				}
			}
	} else
		for (loops_aux = 0; loops_aux < loops; loops_aux++) {
			for (i = 0; i < 8; i++) {
				val = stepsD[i];
				if (motor) {
					PORTB =
					    (PORTB & 0xf0) | (val & 0x0f);
					delay(50);
				} else {
					PORTB =
					    (PORTB & 0x0f) | (val & 0xf0);
					delay(50);
				}
			}
		}
}

/**
 * mov_paper() -        Moves the paper up
 * @steps:              Amount of steps to move
 *
 * Moves the paper up according to the input 'steps'
 **/
void mov_paper(byte steps)
{
	move(steps, UP, ROLLER);
}

/**
 * reset_car(void) -    Resets the car to the start position
 *
 * Resets the car to the beginning and inserts a left indentation
 **/
void reset_car(void)
{
	while (SENS_CAR)
		move(1, LEFT, CAR);
	move(8, RIGHT, CAR);	/* Indentation  */
}

/**
 * hit(void) -  Makes the hammer to hit the paper
 *
 * Generates an impact with delays in the middle.
 * This delays should be review when an actual hammer is in place.
 **/
void hit(void)
{
	delay(100);
	HAMMER = 1;
	delay(100);
	HAMMER = 0;
	delay(100);
}

/**
 * check_bit() -        Checks the status of a bit in a byte
 * @ byte_in:           The byte to check
 * @pos:                The bit to check
 *
 * Recives a byte an an index for it. 
 * Returns 1 if the bit is setted or 0 if is off.
 * (byte, [0-7])
 **/
byte check_bit(byte byte_in, byte pos)
{
	byte mascara;
	mascara = 0x01;
	mascara = mascara << pos;
	if (byte_in & mascara)
		return 1;
	return 0;
}

/**
 * set_bit() -  Set a particual bit of a byte
 * @byte_in:    The byte to modify
 * @pos:        The particular bit to set.
 *
 * Used mainly to reconstruct the recived byte, this function accepts a pointer
 * to a byte and the particular bit that want to be setted.
 **/
void set_bit(byte * byte_in, byte pos)
{
	byte mask = 0x01, byte_aux = *byte_in;
	mask = mask << pos;
	*byte_in = byte_aux ^ mask;
	/** 
         * This could be a simple | too, but ^ makes it better
	 * because it would destroy the byte if something goes wrong. 
	 * Let's say that byte_in = 0x07 (0000 0111), and for some 
	 * reason we try to set the bit 2 the XOR would destroy the byte 
	 * byte_in = 0000 0111 ^ 0000 0100 = 0000 0011 = 0x03. This should
	 * never happen because pos will increment, but you never know ;)
	 **/
}

/**
 * print_byte() -       Prints the byte revived
 * @p:                  Pointer to the byte that want to be printed
 *
 * The function recives a pointer to a byte and prints it. 
 * It returns a control byte recreated in order to send it back to the host
 * to make controls.
 **/
byte print_byte(byte * p)
{
	byte a, i, byte_in, byte_ctl = 0x00;
	byte_in = *p;
	for (i = 8; i > 0; i--) {
		if (check_bit(byte_in, i - 1)) {
			hit();
			set_bit(&byte_ctl, i - 1);	/*Recreate the byte */
		}
		a = (byte) i;
		/**
                 * Movment of the car according to odd-even position of the
                 * braille dot.
                 **/
		if (!(a & 1))	/* Checks parity (even = min, odd = max) */
			move(2, RIGHT, CAR);	/* Min separation */
		else
			move(4, RIGHT, CAR);	/* Max separation */
	}
	return byte_ctl;	/* Return control byte */
}

/**
 * print_line() -       Prints a complete line
 * @p:                  Pointer of the line to print
 * @e:                  Pinter to a recreated line
 *
 * Prints a complete line while recreates it in another pointer 
 * to send back to the host.
 **/
void print_line(byte * p, byte * e)
{
	byte width_b;
	reset_car();
	width_b = NUMLINES - 1;
	while (width_b) {
	  //		*e = print_byte(p);
		*e = 'a';
		p++;
		e++;
		width_b--;
	}
}

/**
 * USB(void) -  Main function to process usb transactions      
 *
 * This is the main function that process all usb transactions, it decides what
 * to do according to the instructions recived by the host.
 **/

static void USB(void)
{
  byte rxCnt, tmpBuff;
	rxCnt = BulkOut(1, rxBuffer, INPUT_BYTES);
	if (rxCnt == 0)
	  return;

	/* funcionando con este bloque
	print_line(rxBuffer, echoVector);

	do {
	  rxCnt = BulkIn(1, echoVector, INPUT_BYTES);
	} while (rxCnt == 0);	*/

	tmpBuff = (byte) adval;
	do {
	  rxCnt = BulkIn(1, &tmpBuff, 1);
	} while (rxCnt == 0); 

	while (ep1Bi.Stat & UOWN)
	  ProcessUSBTransactions();
}


static void USBc(void)
{
	byte rxCnt;
	/** 
         * The code should present a clear distiction between
	 * data direction
	 **/
	/** 
	 * Find out if an output request has arrived to EP2
         * *** HOST --> DEVICE  ***
	 **/

	rxCnt = BulkOut(2, &instruction, 1);
	if (rxCnt == 0)
		return;

	/** 
         * Interpret instructions recived from host
         **/
	if (instruction == MOV_SHORT) {
		mov_paper(SHORT_STEPS_OUT);
	} else if (instruction == MOV_LONG) {
		mov_paper(LONG_STEPS_OUT);
	} else if (instruction == PRINT) {
		do {
			/* *** HOST --> DEVICE  *** */
			rxCnt = BulkOut(1, rxBuffer, INPUT_BYTES);
		} while (rxCnt == 0);
		print_line(rxBuffer, echoVector);

		while (ep1Bi.Stat & UOWN)
			ProcessUSBTransactions();
		/** 
                 * We recived the order to PRINT, so we did it. 
		 * Then we process usb transactions if any.
		 * Now we are ready to send the data treated back
		 * to the host so he can check if there where
		 * any errors.
		 **/
		/* *** DEVICE --> HOST *** */
		do {
			rxCnt = BulkIn(1, echoVector, INPUT_BYTES);
		} while (rxCnt == 0);

	} else if (instruction == RESET)
		reset_car();

	while (ep1Bi.Stat & UOWN)
		ProcessUSBTransactions();

	motors_off();
}

/**
 * ProcessIO(void) -    Process IO requests
 *
 * This function just checks if a IO has been requested and calls
 * USB() if so.
 **/
void ProcessIO(void)
{
	if ((deviceState < CONFIGURED) || (UCONbits.SUSPND == 1))
		return;
	USB();
}


//Function to Read given ADC channel (0-13)
unsigned int ADCRead(unsigned char ch)
{
  if(ch>13) return 0;  //Invalid Channel

  ADCON0=0x00;

  ADCON0=(ch<<2);   //Select ADC Channel

  ADCON0bits.ADON=1;  //switch on the adc module

  ADCON0bits.GO=1;//Start conversion

  while(ADCON0bits.GO); //wait for the conversion to finish

  ADCON0bits.ADON=0;  //switch off adc

  return ADRESH;
}

/**
 * main(void) - Main entry point of the firmware
 *
 * This is the main entrance of the firmware and it creates the mail loop.
 **/
void main(void)
{
	/**
         * Inits the PIC
         **/
	UserInit();
	/**
         * Inits the USB
         *
         * Full-speed mode and sets pull-up internal resistances of PORTB
         * Starts the USB DEATACHED, no wake ups, and no configured.
         * Configuring the USB is the job of the host.
         **/
	UCFG = 0x14;
	deviceState = DETACHED;
	remoteWakeup = 0x00;
	currentConfiguration = 0x00;

	//motors_off();

	//Initialize the ADC Module
	//ADCInit();

	adval = 'a';
 
	while (1) {
		/** 
                 * Make sure the USB is available 
                 **/
		EnableUSBModule();
		/**
                 * As soon as we get out of test mode (UTEYE)
                 * we process USB transactions
                 **/
		if (UCFGbits.UTEYE != 1)
			ProcessUSBTransactions();

       		/**
                 * Now we can make our work
                 **/
		ProcessIO();

		delay(2000);
		adval=ADCRead(7);   //Read Channel 7
		delay(1000);

	}
}
