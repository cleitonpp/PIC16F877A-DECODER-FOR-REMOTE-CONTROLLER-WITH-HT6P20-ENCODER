/*
CODE GRABBER FUNCTION FOR HT6P20 ENCODER USING PIC16F877A AS DECODER
CCS PIC C COMPILER
CREATED BY: CLEITON PETTER PAPPEN, DANILO SOARES AND JEAN SANTOS
EMAIL: ppcleiton (at) gmail (dot) com
LICENSE: GPL
*/

#include <16F877A.h>
#include "pulsin.c"            //Adapted from proton basic library, check manual for more information 
#FUSES NOWDT                    //No Watch Dog Timer
#FUSES HS                       //High speed Osc (> 4mhz FOR PCM/PCH) (>10mhz for PCD)
#FUSES NOPUT                    //No Power Up Timer
#FUSES NOPROTECT                //Code not protected from reading
#FUSES NODEBUG                  //No Debug mode FOR ICD
#FUSES NOBROWNOUT               //No brownout reset
#FUSES NOLVP                    //No low voltage prgming, B3(PIC16) or B5(PIC18) used FOR I/O
#FUSES NOCPD                    //No EE protection
#FUSES NOWRT                    //Program memory not write protected
#use delay(clock=20000000)      //20MHz crystal
#define LED PIN_D0

int lambda, counter;
unsigned long int pulse0, pulse1;
int16 addrfp, addrsp;         //first and second part of address
boolean startbit;

// definitions of used ports
int8 portb = 0x0;            //portB as output
set_tris_b(portb);
int8 portc = 0xFF;            //portC as input
set_tris_c(portc);

int verify_rc() 
{
   if (startbit == false)
   {
      pulse0 = pulsin (PIN_C7, 0);   //returns pulse width in microseconds on "low" state
      // {233.333*23 = 5366, 433.333*23 = 9966} - 23lambdas = pilot address
      if ((pulse0 > 5366) && (pulse0 < 9966) && startbit == false)
      {
         lambda = pulse0 / 23;   // lambda = pulse width of each cycle
         pulse0 = 0;
         addrfp = 0;
         addrsp = 0;
         counter = 0;
         startbit = true;
      }
   }

   while (startbit == true && counter < 28)
   {
      ++counter;
      pulse1 = pulsin (PIN_C7, 1);   //returns pulse width in microseconds on "high" state
      if ((pulse1 > (1.5 * lambda)) && (pulse1 < (2.5 * lambda)))   //pulse 0 = 2 of 3 cycles   
      {
         if (counter < 16)
         {
       //first part of 16-bit address
            addrfp = (addrfp << 1);   //bitshift method to store bit "0" on variable
         }

         ELSE
         {
       //second part of 16-bit address
            addrsp = (addrsp << 1);
         }
      }

      else if ((pulse1 > (0.5 * lambda)) && (pulse1 < (1.5 * lambda)))   //pulse 1 = 1 of 3 cycles
      {
         if (counter < 16)
         {
            addrfp = (addrfp << 1) + 1; //bitshift method to store bit "1" on variable
         }

         else
         {
            addrsp = (addrsp << 1) + 1;
         }
      }

      else
      {
         startbit = false;   //reset the loop
      }
   }

   if (counter == 28)
   {
      if ((addrsp & 15) == 5) // bitwise checksum, it will check if the endcode is "0101"
      {
         counter = 0;
         startbit = false;
         pulse1 = 0;
         return 0;
      }

      else
      {
         startbit = false;
         addrfp = 0;
         addrsp = 0;
         return 1;
      }
   }
   return 1;
}

void main() 
{
   while(1)
   {
      delay_ms (100) ;
      int mydata = 2;
      mydata = verify_rc();
      if (mydata == 0)
      {
         output_toggle(LED);
       mydata = 2;
      }
   }
}
