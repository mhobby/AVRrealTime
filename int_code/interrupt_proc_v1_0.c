#include <stdlib.h>
#include <stdio.h>
#include <inavr.h> 
#ifndef ENABLE_BIT_DEFINITIONS
#define ENABLE_BIT_DEFINITIONS
// Enable the bit definitions in the iom32.h file
#endif
#include <iom324p.h> 

typedef unsigned char BYTE;

#define MAX_QUEUE_SIZE 16
#define QUEUE_RESET 8
#define DEFAULT_QUEUE_VAL 0x00;

#define MAX_INTERRUPT_CHAN 11


//flag for showing that we are logging data and interrupts should be live
__no_init BYTE LIVE @0x100;
__no_init unsigned int prev_interrupts_in @ 0x101;
__no_init BYTE busy_interrupt @0x103;


// Interrupt Service Queue pointer; points to end of ISQ
__no_init BYTE ISQ_PTR @ 0x104; 
// Interrupt Service Queue (ISQ); queue of interrupts waiting to be flagged to
//    the CLS.   Front of queue is indexed by 0;
__no_init BYTE ISQ[MAX_QUEUE_SIZE] @ 0x105;

#pragma vector=PCINT1_vect
//interrupt occurs when pin 5 changes (PCI12)
__interrupt void MSK_STROBE(void)
{//remove interrupt from INT QUEUE 
  //raise interrupt
  PORTC |= 0x01;  //interrupt_out = 1
  //reset MSK
  PORTB |= (0x0F);
  //reshuffle interrrupt queue
  if(LIVE)
  {
    for(BYTE i=0; i<ISQ_PTR; i++)
    {
      ISQ[i] = ISQ[i+1];
    }
    ISQ_PTR--;
    ISQ[ISQ_PTR]=DEFAULT_QUEUE_VAL;
  }
  LIVE = 1;
  busy_interrupt = 0;
}

inline void EXTERNAL_INTERRUPT(void)
{//add interrupt onto INT QUEUE
  unsigned int edge, thisEdge;
  unsigned int interrupts_in;

  interrupts_in = PIND<<8;
  interrupts_in = interrupts_in + (int)PINA;
  //find negative edge changes
  edge = (~interrupts_in)&(prev_interrupts_in);
  
  if(edge>0)
  {
    //go through each channel in turn to find if -ve edge occurred
    // fill up ISQ accordingly
    for(BYTE n=1; n<MAX_INTERRUPT_CHAN; n++)
    {
      thisEdge = 0x01&(edge>>n);   //thisEdge =1, if -ve edge occurred
      ISQ[ISQ_PTR]=n; //add the correct channel interrupt to the interrupt queue
      ISQ_PTR = ISQ_PTR + thisEdge; //increase interrupt queue pointer if -ve edge occurred
    }    
    ISQ[ISQ_PTR]=DEFAULT_QUEUE_VAL;
  }
  prev_interrupts_in = interrupts_in;
}


#pragma vector=PCINT0_vect 
__interrupt void pinChange0(void)
{
  EXTERNAL_INTERRUPT();
}

#pragma vector=PCINT3_vect 
__interrupt void pinChange3(void)
{
  EXTERNAL_INTERRUPT();
}


__monitor void SetInt()
{
  //set MSK
  PORTB &= 0x0F&(~ISQ[0]);  
  //drop interrupt
  PORTC &= (~0x01); //interrupt_out = 0   
  //set busy flag
  busy_interrupt = 1;
}

__monitor void ClearISQ()
{
  //clear queue
  ISQ_PTR = 0;
  for(BYTE i=0; i<MAX_QUEUE_SIZE; i++) ISQ[i] = DEFAULT_QUEUE_VAL;
  //raise interrupt_out
  PORTC |= 0x01;  
  //reset busy flag
  busy_interrupt = 0;
  prev_interrupts_in = (PIND<<8);
  prev_interrupts_in = prev_interrupts_in + (int)PINA;
}


void ArmSystem()
{
  //setup PORTA/PORTD as inputs for INT 
  DDRA = 0x0;
  DDRD = 0x0;

  //setup Interrupt_Out line and clear it
  PORTC |= 0x01;
  DDRC |= 0x01;
  //setup MSK lines to be outputs
  PORTB |= 0x0F;
  DDRB |= 0x0F;
  
  SMCR |= (1<<SM1); ///setup SLEEP to be POWER DOWN
  SMCR |= (1<<SE); //enable proc to sleep  
  
  //enable interrupt mask for changes in MSK_STROBE
  PCMSK1 |= (1<<PCINT12);
  //enable pin change interrupt 3, 1 and 0(PCI3, PCI1 and PCI0) overview mask
  PCICR |= (1<<PCIE3)|(1<<PCIE1)|(1<<PCIE0);
  __enable_interrupt();
}

void GoLive()
{
  //enable interrupt mask 
  PCMSK0 = 0xFA; //1111 1010  (i.e: all channels except CH0 & CH2)
  PCMSK3 = 0x07; //0000 0111 (i.e: channels 8, 9 & 10)
  LIVE = 1;
  busy_interrupt = 0;
}

void main( void )
{ 
  LIVE = 0;
  ClearISQ();
  ArmSystem();
  
  while(!LIVE);
  prev_interrupts_in = (PIND<<8);
  prev_interrupts_in = prev_interrupts_in + (int)PINA;
  GoLive();
  
  while(1)
  {
    if(ISQ_PTR>QUEUE_RESET)
    {
      ClearISQ();
    }  
     
    if((busy_interrupt==0)&&(ISQ_PTR>0)) SetInt();
    else if(ISQ_PTR==0) __sleep();

  };
 
}
