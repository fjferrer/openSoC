
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
#include <htc.h>

//Chip Settings
__CONFIG(1,0x0200);
__CONFIG(2,0X1E1F);
__CONFIG(3,0X8100);
__CONFIG(4,0X00C1);
__CONFIG(5,0XC00F);


//Simple Delay Routine
void Wait(unsigned int delay)
{
  for(;delay;delay--)
    __delay_us(100);
}

//Function to Initialise the ADC Module
void ADCInit()
{
  //We use default value for +/- Vref

  //VCFG0=0,VCFG1=0
  //That means +Vref = Vdd (5v) and -Vref=GEN

  //Port Configuration
  //We also use default value here too
  //All ANx channels are Analog

  /*
    ADCON2

    *ADC Result Right Justified.
    *Acquisition Time = 2TAD
    *Conversion Clock = 32 Tosc
    */

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

void main()
{
  //Let the LCD Module start up
  Wait(100);

  //Initialize the ADC Module
  ADCInit();

  while(1)
    {
      unsigned int val; //ADC Value

      val=ADCRead(0);   //Read Channel 0
      Wait(1000);
    }

}
