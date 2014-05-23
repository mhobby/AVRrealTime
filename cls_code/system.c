#include "lib.h"

 
byte watchdog()
{
  byte status=0;
  if(!(SEMAPHORE & 0x20))
  {
    __watchdog_start();
    status = 1;
  }
  return status;
}

__monitor void __watchdog_start()
{
  SEMAPHORE |= 0x20;
  __watchdog_reset();
  WDTCSR |= (1<<WDCE)|(1<<WDE);
  WDTCSR =  WatchdogTimeout;
  WDTCSR |= (1<<WDIE);
  WATCHDOG_COUNTER = 0;
  __watchdog_reset();
}
                     
__monitor void __watchdog_stop()
{
  __watchdog_reset();  
  WDTCSR |= (1<<WDCE)|(1<<WDE);
  WDTCSR =  0;  
  SEMAPHORE &= ~(0x20);
}

void ClearScheduler()
{
  char i;
  for(i=0; i<SCHEDULER_SIZE; i++)
    SCHEDULER[i] = NULL;
  SCHP = -1;
}

__monitor void CallTask()
{
  byte task_complete;

  if(SCHP>=0)    
  {//check if tasks are waiting to be scheduled but not IDLE 
    task_complete = SCHEDULER[SCHP]();      
    if(task_complete==0)
    {//if task did not complete, reschedule
      AddTask(SCHEDULER[SCHP]);
    }
    RemoveTask();   
  } 
}
                
__monitor void AddTask(byte (*ptr2func)())
{
  char i;
  function_ptr temp_func;
  
  if(SCHP<SCHEDULER_SIZE+1) // addded 1 here for debug purposes
  {
    SCHP=SCHP+1;
    for(i=0;i<SCHP;i++)
    {
      temp_func = SCHEDULER[((SCHP)-i)-1];
      SCHEDULER[((SCHP)-i)]=temp_func;          
    }
    SCHEDULER[0]=ptr2func;
  }
  if(SCHP>=SCHEDULER_SIZE)
  {
    SEMAPHORE |= 0x01;
  }
}

__monitor void RemoveTask()
{
  SCHEDULER[SCHP] = 0;
  SCHP=SCHP-1;
}

void ClearDataBuffer()
{ 
  DataBuffer.station_id = 0;
  DataBuffer.rtc_time.secs=0;
  DataBuffer.rtc_time.mins=0;
  DataBuffer.rtc_time.hr=0;
  DataBuffer.rtc_time.date=0;
  DataBuffer.rtc_time.mon=0;
  DataBuffer.rtc_time.year=0;     
  
  DataBuffer.pressA_D1=0;
  DataBuffer.pressA_D2=0;
  DataBuffer.pressB_D1=0;
  DataBuffer.pressB_D2=0;
  
  for(char i=0; i<6; i++) DataBuffer.rht_adc[i]=0;
  for(char i=0; i<8; i++) DataBuffer.rad_adc[i]=0;
  
  DataBuffer.sonicSpdSum=0; 
  DataBuffer.sonicSpdMax=0; 
  DataBuffer.sonicUSum=0;    
  DataBuffer.sonicVSum=0; 
  DataBuffer.sonicSpd2Sum=0; 
  DataBuffer.sonicSpd3Sum=0; 
  DataBuffer.sonicSampleNo=0; 

  DataBuffer.cupSpdSum=0; 
  DataBuffer.cupSpdMax=0; 
  DataBuffer.cupVaneUSum=0;    
  DataBuffer.cupVaneVSum=0; 
  DataBuffer.cupSpd2Sum=0;  
  DataBuffer.cupSpd3Sum=0; 
  DataBuffer.cupVaneSampleNo=0; 
  
  DataBuffer.V_5Vreg = 0;
  DataBuffer.V_12Vreg = 0;
  DataBuffer.V_Batt = 0;
  DataBuffer.SysTemp = 0;
  DataBuffer.GroundProbe1 = 0; 
  DataBuffer.GroundProbe2 = 0;   
  DataBuffer.GndHeatFluxSC = 0;
  DataBuffer.clockSync = 0;
  for(char i=0; i<AUX_SIZE; i++) DataBuffer.aux[i]=0;
  DataBuffer.crc = 0;
}

void InitSys()
{ 
  byte i, status;
  _time lTime;  
  enum sys_ptr tSysPtr;
  
  //initialise WatchDog
  MCUSR &= ~(1<<WDRF);
  WDTCSR |= (1<<WDCE)|(1<<WDE);
  WDTCSR =  0;

  TempBufCtrl = 0;

  //initialise data system  
  LSysPtr = eSysPtr;
  LDataPtr = eDataPtr-256;
  LTxPtr = eTxPtr; 
      
  switch(LSysPtr)
  {
  case SYS0: eSysPtr = SYS1; tSysPtr = SYS4; break;
  case SYS1: eSysPtr = SYS2; tSysPtr = SYS0; break;
  case SYS2: eSysPtr = SYS3; tSysPtr = SYS1; break;
  case SYS3: eSysPtr = SYS4; tSysPtr = SYS2; break;
  case SYS4: eSysPtr = SYS0; tSysPtr = SYS3; break;
  }  
  
  status = rtc_get(&lTime); //get time
  if(!status) LSysBuffer.ctrlA[0] |= (1<<1);  

  LSysBuffer.station_id = eStationID;

  switch(LSysBuffer.station_id)
  { 
  case 0x82: break; //dumb;
  case 0x83: LTxTimeSlot=0; break; 
  case 0x84: break; //dumb
  case 0x85: LTxTimeSlot=1; break;
  case 0x86: LTxTimeSlot=2; break;
  case 0x87: LTxTimeSlot=3; break;
  case 0x88: LTxTimeSlot=4; break;
  case 0x89: LTxTimeSlot=5; break;
  case 0x8A: LTxTimeSlot=6; break;
  case 0x8B: LTxTimeSlot=7; break;
  case 0x8C: LTxTimeSlot=8; break;
  case 0x8D: LTxTimeSlot=9; break;  
  case 0x8E: LTxTimeSlot=10; break;  
  case 0x8F: LTxTimeSlot=11; break;    
  }  

  LSysBuffer.bootTime = lTime;  

  eSysBuffer[tSysPtr].bootDataStopAddr[0] = (0xFF000000&LDataPtr)>>24;  
  eSysBuffer[tSysPtr].bootDataStopAddr[1] = (0x00FF0000&LDataPtr)>>16;
  eSysBuffer[tSysPtr].bootDataStopAddr[2] = (0x0000FF00&LDataPtr)>>8; 
  eSysBuffer[tSysPtr].bootDataStopAddr[3] = (0x000000FF&LDataPtr);

  LDataPtr+=256;
  LSysBuffer.bootDataStartAddr[0] = (0xFF000000&LDataPtr)>>24;
  LSysBuffer.bootDataStartAddr[1] = (0x00FF0000&LDataPtr)>>16;
  LSysBuffer.bootDataStartAddr[2] = (0x0000FF00&LDataPtr)>>8;
  LSysBuffer.bootDataStartAddr[3] = (0x000000FF&LDataPtr);

  LSysBuffer.storageCycle = eSysBuffer[tSysPtr].storageCycle;
  
  LSysBuffer.bootDataStopAddr[0] = 0;
  LSysBuffer.bootDataStopAddr[1] = 0;
  LSysBuffer.bootDataStopAddr[2] = 0;
  LSysBuffer.bootDataStopAddr[3] = 0;

  LSysBuffer.bytesTx = 0;

  for(i=0; i<4; i++) LSysBuffer.pressACals[i]=0;
  for(i=0; i<4; i++) LSysBuffer.pressBCals[i]=0;
  for(i=0; i<6; i++) LSysBuffer.ctrlA[i]=0;
  for(i=0; i<2; i++) LSysBuffer.ctrlB[i]=0;  
  for(i=0; i<3; i++) LSysBuffer.rsvd[i]=0;
  
  eSysBuffer[LSysPtr] = LSysBuffer;
  while(!UpdateSysPage());
}

byte UpdateSysPage()
{
  byte status=0;
  byte sysPage[256];

  for(byte i=0; i<252; i++)
  {
    sysPage[i] = EEPROM_read(2+i);
  }
  for(byte i=0; i<4; i++) sysPage[i+252]=0; //signify that this is a Sys Page

  status=writeFlash(0, sysPage, 256);
  return status;
}

byte CalculateCRC()
{
  byte status=0;
  unsigned int crc=0x0000;
  byte t;
  for(byte i=0; i<109; i++)
  {
    t = (crc&0x00FF)^*(((byte*)&DataBuffer)+i);
    crc = crc >> 8;
    crc = crc ^ CRC_LOOKUP[t];
  }
  DataBuffer.crc = crc;
  return status;
}

byte UploadDataBuffer()
{
  byte status;
  DataBuffer.pageFlag=0xFF;
  CalculateCRC();
  
  status = writeFlash(LDataPtr, (byte*)&DataBuffer, sizeof(_data));
  if(status)
  {
    LDataPtr= LDataPtr + 128;
//    if(LSysBuffer.storageCycle<1)
//    { //allow storage to be cycled once only! equivalent to 1 x 144 = 144 days
      if(LDataPtr>=0x7FFFFF)
      { //end of storage      
        LDataPtr=0x100;   
        //reset TxPtr to start of memory space here too.
        // all data not transmitted to this point will not be transmitted.
        // this can be improved but is kept simple in this instance
        eTxPtr = LTxPtr = 0x100; 
        eSysBuffer[LSysPtr].storageCycle++;
        LSysBuffer.storageCycle++;        
        AddTask(&UpdateSysPage);      
      }
 //   }
 //   else
//    { //during third (and subsequent) storage cycle (i.e: after at least 288 days)
//      if(LDataPtr>=0x10000) //cycle storage at 288 + n(1.185) days
//      { //end of storage      
//        LDataPtr=0x100;   
//        //reset TxPtr to start of memory space here too.
//        // all data not transmitted to this point will not be transmitted.
//        // this can be improved but is kept simple in this instance
//        eTxPtr = LTxPtr = 0x100; 
//        eSysBuffer[LSysPtr].storageCycle++;
//        LSysBuffer.storageCycle++;        
//        AddTask(&UpdateSysPage);      
//      }
//    }
    eDataPtr = LDataPtr;
    ClearDataBuffer();
    SEMAPHORE &= ~(0x80);
  }
  return status;
}

void InitPwrOnTime()
{
  //expected time devices will have powered up
  for(char i=0; i<15; i++)
    PwrOnTime[i] =0;
}

void InitPwrMgmt()
{
  //setup pin for power mask strobe and set high, with interrupt MSK
  PORTD |= 0xF8;
  DDRD |= 0xF8;
  
  //reset power proc
  PwrState = 0x0; //set to zero; all channels OFF
  InitPwrOnTime();
}

__monitor void PowerUp(byte chan)
{  
  //if power is down ... 
  if(!(PwrState&(1<<chan)))
  {    
    PwrState |= (1<<chan); //record state of power channel
    PORTD = (chan<<4)|(0x0F&PORTD); //set up channel number on PWR MSK
    PORTD ^= (1<<3);     //toggle PWR strobe
    //PWRON_TIME[chan-3]=TCNT1+MAX3100_POWER_DELAY;
  }
}

__monitor void PowerDn(byte chan)
{  
  //if power is up ... then toggle PWR_PROC
  if(PwrState&(1<<chan))
  {
    //record state of power channel
    PwrState &= ~(1<<chan);
    //set up channel number on PWR MSK
    PORTD = (0xF0&(chan<<4))|(0x0F&PORTD);
    //toggle PWR strobe
    PORTD ^= (1<<3);  
if(chan==13)
  __delay_cycles(1000);
  }
}

void ToggleBuzzer()
{
  if(PwrState&0x8000) PowerDn(15);
  else PowerUp(15);
}

void EventClockSetup()
{//set to 32us counter
  TCCR1B =  (1<<CS12) ;   //CPU clock / 256
  TCCR1B |=  (1<<CS10) ;   //CPU clock
  TIMSK1 |= (1<<TOIE1);
}

void InitStateMachines()
{
  //Iridium
  SatComTxPtr=0;
  SatComPagePtr=0;
  SatComRxPtr=0;
  SatComState=0;
  SatComStateAttempt=0;
  FlushMAX3100(3);
  
  //RhT ADC
  RhtADCState=0x00;
  RhtADCBufPtr = 0x00;
  FlushMAX3100(6);   //flush MAX3100 buffer
  
  //RAD ADC
  RadADCState=0x00;
  RadADCBufPtr= 0x00;
  RadADC_SC_State=0;
  FlushMAX3100(7);    //flush MAX3100 buffer
  
  //PRESS
  PressAState = 0x40;
  PressABufPtr = 0x00;
  for(byte i=0; i<10; i++) PressACal[i] = 0;
  PressBState = 0x40;
  PressBBufPtr = 0x00;
  for(byte i=0; i<10; i++) PressBCal[i] = 0;
  FlushMAX3100(4);   //flush MAX3100 buffers
  FlushMAX3100(5);
  
  //SONIC
  SonicState=0x00;
  SonicBufPtr = 0x00;
  SonicSpdSum = SonicUSum = SonicVSum = SonicSpdSkwSum = SonicSpdVarSum = 0;
  SonicSamples = 0;
  FlushMAX3100(8);  //flush MAX3100 buffer
  
  //CUP
  CupState=0x00;
  CupBufPtr = 0x00;
  CupSpdSum = CupVaneUSum = CupVaneVSum = CupSpdSkwSum = CupSpdVarSum = 0;
  CupSamples = 0;
  FlushMAX3100(9);  //flush MAX3100 buffer
    
  //VANE
  VaneState=0x00;
  VaneBufPtr = 0x00;
  FlushMAX3100(10);  //flush MAX3100 buffer
}

void ArmSystem()
{ 
  //setup pins for interrupt MSK in
  DDRC &= ~(0xC3);
  
  //setup pings for MSK_STROBE
  DDRD |= (1<<2);
  
  //set external interrupt 2 to be negative edge triggered
  EICRA |= (1<<ISC21);
  //enable external interrupt 2 only
  EIMSK |= (1<<INT2);
  
  //setup SLEEP to be POWER DOWN
  SMCR |= (1<<SM1);  
  SMCR |= (1<<SE); //enable CLS proc to sleep

  //clear existing interrupts
  do
  {
    PORTD ^= (1<<2); //toggle MSK_STROBE
    __delay_cycles(1000);         
  }while(!(PINB&0x04));
  
  //reset interrupt sources  
  //n.b - time from reset of source to enabling interrupts is 39 clock cycles
  //   => 4.875us. Interrupt Processor takes longer than this to process an 
  //  interrupt so no race condition SHOULD occur...
  
  ClearMAX3100(6); //SONIC
  rtc_init(); 
  rtc_check();
  __enable_interrupt();
}

void EEPROM_write(unsigned int uiAddress, unsigned char ucData)
{//ref: ATmega324 datasheet example
  // takes 3.4ms per byte to complete

  // Wait for completion of previous write 
  while(EECR & (1<<EEWE));
  // Set up address and Data Registers 
  EEAR = uiAddress;
  EEDR = ucData;
  // Write logical one to EEMPE 
  EECR |= (1<<EEMWE);
  // Start eeprom write by setting EEPE 
  EECR |= (1<<EEWE);
}

unsigned char EEPROM_read(unsigned int uiAddress)
{//ref: ATmega324 datasheet example
  // Wait for completion of previous write 
  while(EECR & (1<<EEWE));
  // Set up address register 
  EEAR = uiAddress;
  // Start eeprom read by writing EERE 
  EECR |= (1<<EERE);
  // Return data from Data Register 
  return EEDR;
}
