#include "lib.h"

void main( void )
{  
  __delay_cycles(8000000); //delay required to ensure all other procs have booted  
//************************************************************ 
//** CLS processor delayed by 4.1ms in HW FUSES
//************************************************************  
  SystemState = INIT;
  SEMAPHORE = 0x00;
//************************************************************  
//** INIT PHASE  
//************************************************************      
  
  SyncState = 00;
  ClearScheduler();
  
  InitPwrMgmt();   //Init Power Control
  
  SPI_Enable(); //Init SPI
  USART_Init(_BAUD_USART_38400);
  USART_Flush();
  writeStatusFlash(0); //Unprotect flash ROM

  InitSys(); //SYSTEM BUFFER INIT

  ClearDataBuffer();    //DATA STORAGE INIT

  PowerUp(3); ConfigureMAX3100(3, _XTAL3686_BAUD_MAX3100_2400); ClearMAX3100(3);//IRIDIUM   
  PowerUp(4); ConfigureMAX3100(4, _XTAL3686_BAUD_MAX3100_9600); ClearMAX3100(4); //PRESS A
  PowerUp(5); ConfigureMAX3100(5, _XTAL3686_BAUD_MAX3100_9600); ClearMAX3100(5);//PRESS B
  PowerUp(6); ConfigureMAX3100(6, _XTAL3686_BAUD_MAX3100_9600); ClearMAX3100(6);//RHT ADC
  PowerUp(7); ConfigureMAX3100(7, _XTAL3686_BAUD_MAX3100_9600); ClearMAX3100(7);//RAD ADC  
  PowerUp(8); ConfigureMAX3100(8, _XTAL3686_BAUD_MAX3100_19200); ClearMAX3100(8);//SONIC  
  PowerUp(9); ConfigureMAX3100(9, _XTAL3686_BAUD_MAX3100_4800); ClearMAX3100(9);//CUP
  PowerUp(10); ConfigureMAX3100(10, _XTAL3686_BAUD_MAX3100_4800); ClearMAX3100(10);//VANE
  PowerUp(14); //turn on LED

  EventClockSetup();   //Init Clocks 
  ADCSetup(); //Init ADC

  InitStateMachines(); //Init all FSM
 
  SystemState = (0xE0&SystemState)|SYNC;
  ArmSystem();    //enable interrupts
  
//************************************************************  
//** LOOP PHASE  
//************************************************************    
  while((SystemState&LOOP)||(SystemState&SYNC))
  {    
    if(SEMAPHORE&0x01)
    {//scheduler or interrupt_task_list overrun; disable all interrupts
     //   complete all tasks on scheduler and re-enable interrupts
      __disable_interrupt();
      for(int i=0; i<SCHEDULER_SIZE; i++)
      {     
        CallTask();
      }
      SEMAPHORE = 0x0;
      
      //reset State Machines    
      RhtADCBufPtr=0; RhtADCState=0; 
      RadADCBufPtr=0; RadADCState=0; 
      PressABufPtr=0; PressAState=0;     
      PressBBufPtr=0; PressBState=0;     
      SonicBufPtr=0; SonicState=0;

      __enable_interrupt();
    }
    if(SCHP>=0) CallTask();
    else __sleep();
  }
  ClearScheduler();
//************************************************************  
//** HALT PHASE  
//************************************************************   
  while(1)
  {
    if(SatComState==1)
    {
      _time t;
      const char resp[19] = "IRIDIUM TIME: ERROR";    
      char timeStr[2];
      byte *tempPtr;
      byte temp;
      
      //enter modem pin and parse response
      while((SatComState==1)||(SatComState==7))
        if(SCHP>=0) CallTask();
      rtc_get(&t);
      if(SEMAPHORE & 0x08)
      {
        rtc_set(&IridiumTime);
        for(byte c=0; c<6; c++)
        {
          tempPtr = ((byte*)(&t))+c;
          temp = *tempPtr;
          sprintf(timeStr, "%02d", temp);
          USART_Transmit(timeStr[0]);
          USART_Transmit(timeStr[1]);
        }
        USART_Transmit(0x20);
        USART_Transmit('>');
        USART_Transmit(0x20);
        for(byte c=0; c<6; c++)
        {
          tempPtr = ((byte*)(&IridiumTime))+c;
          temp = *tempPtr;
          sprintf(timeStr, "%02d", temp);
          USART_Transmit(timeStr[0]);
          USART_Transmit(timeStr[1]);
        }
      }
      else
      {
        for(byte c=0; c<19; c++) USART_Transmit(resp[c]);                
      }      
      ClearScheduler();
    }
  }
}
