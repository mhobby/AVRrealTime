#include "lib.h"

#pragma vector=TIMER1_OVF_vect 
//interrupt occurs every 100ms
__interrupt void EVENT_CLOCK(void)
{
  TCNT1 =0;
}

#pragma vector=WDT_vect
__interrupt void WDT(void)
{
  /* 19/05/10 : CODE ONLY DEALS WITH INTERRUPTS FROM SATCOMM FUNCTIONS. 
                NO OTHER FUCNTIONS SHOULD USE THE WATCHDOG AT THIS TIME */
  WATCHDOG_COUNTER++;
  //n.b Watchdog interrupt occurs every 1048576 cycles ~= 8seconds
  //   Iridium specifies max timeout of 50s, therefore wait at least 8 interrupts
  byte failComms=0;
  
  switch(SatComState)
  {
  case 1 : //DTE AUTH
    if(WATCHDOG_COUNTER>5)
    { 
      if(SatComStateAttempt<DTE_ATTEMPT)
      {
        AddTask(&DTEAuth);
        __watchdog_stop(); 
      }
      else failComms=1;
    }
    break;
  case 2: //Call Init
    if(WATCHDOG_COUNTER>5)
    { 
      if(SatComStateAttempt<CALL_ATTEMPT)
      {
        AddTask(&CallInit);
        __watchdog_stop(); 
      }
      else failComms=1;
    }    
    break;
  case 3: //Sys Page Tx
    if(WATCHDOG_COUNTER>ACK_WAIT)
    {
      SEMAPHORE|=0x10; 
      SatComState=5; 
      AddTask(&SatExit); 
      __watchdog_stop();
    }
    break;
  case 4: //Data Page Tx
    if(WATCHDOG_COUNTER>ACK_WAIT)
    {
      SEMAPHORE|=0x10; 
      SatComState=5;
      AddTask(&SatExit); 
      __watchdog_stop(); 
    }
    break;
  case 5: //Exit
    if(WATCHDOG_COUNTER>5)
    { 
      if(SatComStateAttempt<EXIT_ATTEMPT)
      {
        AddTask(&SatExit);
        __watchdog_stop(); 
      }
      else failComms=1;
    }         
    break;
  case 6: //Hang Up
    if(WATCHDOG_COUNTER>5)
    { 
      if(SatComStateAttempt<HANGUP_ATTEMPT)
      {
        AddTask(&HangUp);
        __watchdog_stop(); 
      }
      else failComms=1;
    }         
    break;    
  case 7: //Time Check
    break;
  case 8: //WAIT    
    SatComState=2;
    AddTask(&CallInit);
    __watchdog_stop(); 
    break;    
  }
  

  if(failComms)
  {
    PowerDn(13);
    SatComState=0; //IDLE
  }
}

#pragma vector=ADC_vect
__interrupt void ADC_CONVERSION_RDY(void)
{
  AddTask(&ADCParse);
}

#pragma vector=USART0_RX_vect
__interrupt void USART_RX(void)
{
  if(SystemState==LOOP)
  {
    AddTask(&PCParse);
    UCSR0B &= ~(1<<RXCIE0); //ignore all further incoming bytes to stop scheduler overrun
  }
  else if(SystemState==HALT) PCParse();
}


#pragma vector=INT2_vect 
__interrupt void INTERRUPT(void)
{
  byte MSK;
  MSK = ((PINC&0x01)<<3);
  MSK = MSK + ((PINC&0x02)<<1);
  MSK = MSK + ((PINC&0x40)>>5);
  MSK = MSK + ((PINC&0x80)>>7);
  MSK = 0x0F&(~MSK);
  
  switch(MSK)
  {
  case 1 : AddTask(&rtc_alarm); break;
  case 2 : break;
  case 3 : AddTask(&SatComParse); break;
  case 4 : AddTask(&PressAParse); break;//PRESS_A
  case 5 : AddTask(&PressBParse); break;//PRESS_B
  case 6 : AddTask(&RHTADCParse); break; //RHTADC
  case 7 : AddTask(&RADADCParse); break; //RADADC
  case 8 : AddTask(&SonicParse); break; //SONIC  
  case 9 : AddTask(&CupParse); break; //CUP
  case 10 : AddTask(&VaneParse); break; //VANE
  default : break;
  }  
  //strobe interrupt msk_strobe (XOR PORTD with 0100 [Pn2])
  PORTD ^= (1<<2);
}
