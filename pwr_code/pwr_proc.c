#include <stdlib.h>
#include <stdio.h>
#include <inavr.h> 
#ifndef ENABLE_BIT_DEFINITIONS
#define ENABLE_BIT_DEFINITIONS
// Enable the bit definitions in the iom32.h file
#endif
#include <iom324p.h>  

#ifndef DATA_TYPES_DEF
  typedef unsigned char byte;
  #define DATA_TYPES_DEF
#endif

byte PWR_MSK;
unsigned int PW_CTRL;

#pragma vector=PCINT1_vect
__interrupt void INTERRUPT(void)
{  
  PWR_MSK = 0x0F&PINB;
  PW_CTRL ^= (1<<PWR_MSK);
  PORTD = (0xFF00&PW_CTRL)>>8;
  PORTA = 0x00FF&PW_CTRL;
}

void main( void )
{
  DDRA = 0xFF;
  DDRD = 0xFF;

  PW_CTRL = 0x00;
  PORTA = 0x00;
  PORTD = 0x00;  
  
  //enable interrupt mask for changes in PWR_STROBE
  PCMSK1 |= (1<<PCINT12);
  //enable pin change interrupt 1 (PCI1) overview mask
  PCICR |= (1<<PCIE1);
  
  __enable_interrupt();
  while(1);
}
