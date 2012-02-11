/********************************************************************

ANALOG TO DIGITAL CONVERTOR INTERFACING TEST PROGRAM

---------------------------------------------------------
Simple Program to connect with the internal ADC of PIC MCUs.
The program reads and display the analog input value at AN0.
Requires the PIC18 lcd library.

MCU: PIC18FXXXX Series from Microchip.
Compiler: HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)

Copyrights 2008-2010 Avinash Gupta
eXtreme Electronics, India

For More Info visit
http://www.eXtremeElectronics.co.in

Mail: me@avinashgupta.com

********************************************************************/

/**
 * Review this!
 * 
 * #include <htc.h>
 * 
 * 
 * //Chip Settings
 * __CONFIG(1,0x0200);
 * __CONFIG(2,0X1E1F);
 * __CONFIG(3,0X8100);
 * __CONFIG(4,0X00C1);
 * __CONFIG(5,0XC00F);
 **/

/**
 * AD Module
 * 
 * - The module has five registers:
 *     • A/D Result High Register (ADRESH)
 *     • A/D Result Low Register (ADRESL)
 *     • A/D Control Register 0 (ADCON0)
 *     • A/D Control Register 1 (ADCON1)
 *     • A/D Control Register 2 (ADCON2)
 **/

//Simple Delay Routine
void Wait(unsigned int delay)
{
  for(;delay;delay--)
    __delay_us(100);
}

//Function to Initialise the ADC Module
void ADCInit()
{

  /**
   * TRISE REGISTER (40/44-PIN DEVICES ONLY)
   *
   * |  R-0  |  R-0  | R/W-0 | R/W-0 |  U-0  | R/W-1 | R/W-1 | R/W-1 |
   * |  IBF  |  OBF  |  IBOV |PSPMODE|   —   | TRISE2| TRISE1| TRISE0|
   * | bit 7 | bit 6 | bit 5 | bit 4 | bit 3 | bit 2 | bit 1 | bit 0 |
   **/

  TRISE2=1;
  TRISE1=1;
  TRISE0=1;

  /**
   * ADCON1: A/D CONTROL REGISTER 1
   *
   * |  U-0  |  U-0  | R/W-0 | R/W-0 | R/W-0 | R/W-q | R/W-q | R/W-q |
   * |   —   |   —   | VCFG1 | VCFG0 | PCFG3 | PCFG2 | PCFG1 | PCFG0 |
   * | bit 7 | bit 6 | bit 5 | bit 4 | bit 3 | bit 2 | bit 1 | bit 0 |
   **/

  // That means +Vref = Vdd (5v) and -Vref = Vss
  // To use AN2 and AN3 set the bits
  //  VCFG1=0;
  //  VCFG0=0;

  //Port Configuration
  //We also use default value here too
  //All ANx channels are Analog
  // This configuration set all ports in Analog mode
  //  PCFG3=0;
  //  PCFG2=0;
  //  PCFG1=0;
  //  PCFG0=0;

  ADCON1=0b00000000;

  /**
   * ADCON2: A/D CONTROL REGISTER 2
   * | R/W-0 |  U-0  | R/W-0 | R/W-0 | R/W-0 | R/W-0 | R/W-0 | R/W-0 |
   * | ADFM  |   —   | ACQT2 | ACQT1 | ACQT0 | ADCS2 | ADCS1 | ADCS0 |
   * | bit 7 | bit 6 | bit 5 | bit 4 | bit 3 | bit 2 | bit 1 | bit 0 |
   **/

  // ADC Result Right Justified
  //  ADFM=1;

  // Acquisition Time = 2TAD
  //  ACQT2=0;
  //  ACQT1=0;
  //  ACQT0=1;

  // Conversion Clock = 32 Tosc
  //  ADCS2=0;
  //  ADCS1=1;
  //  ADCS0=0;

  ADCON2=0b10001010;

}

//Function to Read given ADC channel (0-13)
unsigned int ADCRead(unsigned char ch)
{
  if(ch>13) return 0;  //Invalid Channel

  ADCON0=0x00;

  ADCON0=(ch<<2);   //Select ADC Channel

  ADON=1;  //switch on the adc module

  GODONE=1;//Start conversion

  while(GODONE); //wait for the conversion to finish

  ADON=0;  //switch off adc

  return ADRES;
}

/**
 * void main()
 * {
 *   //Let the LCD Module start up
 *   Wait(100);
 * 
 *   //Initialize the ADC Module
 *   ADCInit();
 * 
 *   while(1)
 *     {
 *       unsigned int val; //ADC Value
 * 
 *       val=ADCRead(0);   //Read Channel 0
 *       Wait(1000);
 *     }
 * 
 * }
 **/
