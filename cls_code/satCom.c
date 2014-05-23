#include "lib.h"


byte DTEAuth()
{
  const char cmd[14] = "AT+CPIN=\"1111\"";
  byte return_byte; 
  byte status = 0;
  byte byteRead;
  
  if(SatComTxPtr<14) status = WriteMAX3100(3, cmd[SatComTxPtr], &return_byte, &byteRead); 
  else if(SatComTxPtr==14) status = WriteMAX3100(3, 0x0D, &return_byte, &byteRead); 
  if(status)
  {
    SatComTxPtr++;
    if(SatComTxPtr<15) AddTask(&DTEAuth); 
    else
    {
      SatComStateAttempt++;
      SatComRxPtr = 0;
      SatComTxPtr = 0;
      WatchdogTimeout=WD_TIMEOUT_9_1;
      AddTask(watchdog);
    }    
  }
  return status;  	
}

byte CallInit()
{
  const char cmd[16] = "ATD+881600005104";
  byte return_byte;
  byte byteRead;
  byte status = 0;

  if(SatComTxPtr<16) status = WriteMAX3100(3, cmd[SatComTxPtr], &return_byte, &byteRead); 
  else if(SatComTxPtr==16) status = WriteMAX3100(3, 0x0D, &return_byte, &byteRead); 
  if(status)
  {
    SatComTxPtr++;
    if(SatComTxPtr<17) AddTask(&CallInit); 
    else
    {
      SatComStateAttempt++;
      SatComRxPtr = 0;
      SatComTxPtr = 0;        
      WatchdogTimeout=WD_TIMEOUT_9_1;
      AddTask(watchdog);
    }
  }
  return status;    
}

byte SysPageTx()
{
  byte return_byte, status;
  byte byteRead=0;
  if(!(SEMAPHORE&0x10))
  {
    if(SatComPagePtr==0)
    {
      //collect sys page into TxBuffer
      status = readFlash(0, TxBuffer, 256);
      //if sys page collected successfully; transmit first byte
      if(status) 
        status = WriteMAX3100(3, TxBuffer[SatComPagePtr], &return_byte, &byteRead);      
    }

    else //transmit byte in TxBuffer
      status = WriteMAX3100(3, TxBuffer[SatComPagePtr], &return_byte, &byteRead); 
    
    if(byteRead)
    {
      SatComTempBuf = return_byte;
      TempBufCtrl |= (1<<IRIDIUM_CHAN);
      AddTask(&SatComParse);
    }
    if(status)
    {
      SatComPagePtr++;     
      if(SatComPagePtr<222) AddTask(&SysPageTx); 
      else
      {    
        SatComStateAttempt++;
        SatComRxPtr = 0;
        SatComPagePtr = 0;       
        WatchdogTimeout=WD_TIMEOUT_9_1;
        AddTask(watchdog);
      }      
    }
    
  }
  else status =1;
  return status;    
}

byte DataPageTx()
{
  byte return_byte, status;
  byte byteRead;
  
  if(SatComPagePtr==0)
  {
    //collect sys page into TxBuffer
    status = readFlash(LTxPtr, TxBuffer, 256);
    //if data page collected successfully; transmit first byte
    if(status) 
      status = WriteMAX3100(3, TxBuffer[SatComPagePtr], &return_byte, &byteRead);   
  }
  else //transmit byte in TxBuffer
    status = WriteMAX3100(3, TxBuffer[SatComPagePtr], &return_byte, &byteRead); 
  
  if(byteRead)
  {
    SatComTempBuf = return_byte;
    TempBufCtrl |= (1<<IRIDIUM_CHAN);
    AddTask(&SatComParse);
  }  
  
  if(status)
  {
    SatComPagePtr++;    
    if(SatComPagePtr==111) SatComPagePtr=128;
    if(SatComPagePtr<239) AddTask(&DataPageTx);      
    else
    {   
      SatComStateAttempt++;
      SatComRxPtr = 0;
      SatComPagePtr = 0;   
      WatchdogTimeout=WD_TIMEOUT_9_1;
      AddTask(watchdog);   
    }          
  }
  return status;    
}

byte SatExit()
{
  const char cmd[3] = "+++";
  byte return_byte;
  byte status = 0;
  byte byteRead;

  status = WriteMAX3100(3, cmd[SatComTxPtr], &return_byte, &byteRead); 
  
  if(byteRead)
  {
    SatComTempBuf = return_byte;
    TempBufCtrl |= (1<<IRIDIUM_CHAN);
    AddTask(&SatComParse);
  }
  
  if(status)
  {
    SatComTxPtr++;
    if(SatComTxPtr<3) AddTask(&SatExit); 
    else
    {
      SatComStateAttempt++;
      SatComRxPtr = 0;
      SatComTxPtr = 0;           
      WatchdogTimeout=WD_TIMEOUT_9_1;
      AddTask(watchdog);
    }
  }
  return status;      
}

byte SatCSQChk()
{
  const char cmd[6] = "AT+CSQ";
  byte return_byte;
  byte status = 0;
  byte byteRead;

  if(SatComTxPtr<6) status = WriteMAX3100(3, cmd[SatComTxPtr], &return_byte, &byteRead); 
  else if(SatComTxPtr==6) status = WriteMAX3100(3, 0x0D, &return_byte, &byteRead);
  if(status)
  {
    SatComTxPtr++;
    if(SatComTxPtr<7) AddTask(&SatCSQChk); 
    else
    {
      SatComStateAttempt++;
      SatComRxPtr = 0;
      SatComTxPtr = 0;           
      WatchdogTimeout=WD_TIMEOUT_9_1;
      AddTask(watchdog);
    }
  }
  return status;   
}  


byte HangUp()
{
  const char cmd[4] = "ATH0";
  byte return_byte;
  byte status = 0;
  byte byteRead;

  if(SatComTxPtr<4) status = WriteMAX3100(3, cmd[SatComTxPtr], &return_byte, &byteRead); 
  else if(SatComTxPtr==4) status = WriteMAX3100(3, 0x0D, &return_byte, &byteRead); 

  if(byteRead)
  {
    SatComTempBuf = return_byte;
    TempBufCtrl |= (1<<IRIDIUM_CHAN);
    AddTask(&SatComParse);
  }
  
  if(status)
  {
    SatComTxPtr++;
    if(SatComTxPtr<5) AddTask(&HangUp); 
    else
    {
      SatComStateAttempt++;
      SatComRxPtr = 0;
      SatComTxPtr = 0;   
      WatchdogTimeout=WD_TIMEOUT_9_1;
      AddTask(watchdog);
    }
  }
  return status;    
}

byte Sat_MSSTM()
{
  const char cmd[8] = "AT-MSSTM";
  byte return_byte;
  byte status = 0;
  byte byteRead;

  if(SatComTxPtr<8) status = WriteMAX3100(3, cmd[SatComTxPtr], &return_byte, &byteRead); 
  else if(SatComTxPtr==8) status = WriteMAX3100(3, 0x0D, &return_byte, &byteRead);
  if(status)
  {
    SatComTxPtr++;
    if(SatComTxPtr<9) AddTask(&Sat_MSSTM); 
    else
    {
      SatComStateAttempt++;
      SatComRxPtr = 0;
      SatComTxPtr = 0;           
      WatchdogTimeout=WD_TIMEOUT_9_1;
      AddTask(watchdog);
    }
  }
  return status;   
}  

byte SatComParse()
{
  byte status = 0;
  byte buf;
  byte moreData;
  byte frameErr;
  byte syncErr;
  byte response=0;
  unsigned long long IridiumMSSTM;
  byte i;
  
  do
  {
    moreData=0; 
    frameErr=0; 
    syncErr=0;
    
    if(TempBufCtrl&(1<<IRIDIUM_CHAN))
    {
      status = 1;
      moreData = 1;
      TempBufCtrl &= ~(1<<IRIDIUM_CHAN);
      buf = SatComTempBuf;
    }
    else   
      status = ReadMAX3100(3,&buf, &moreData, &frameErr, &syncErr); 
    
    if((syncErr!=1)&&(status==1))
    {
      //XON/XOFF checks
      if(buf==0x13) ;//XOFF
      else if(buf==0x11) ;//XON
      else
      {
        switch(SatComState)
        {
        case 1: //DTE Authentication;
          SatComRxBuf[SatComRxPtr++]=buf;
          if(SatComRxPtr==30) SatComRxPtr=0;//error!!! buffer overload!
          if(buf==0x0D) 
          {
            __watchdog_stop(); 
            for(byte i=0; i<SatComRxPtr-1; i++) 
            {
              response = response * 10;
              response += SatComRxBuf[i]-0x30;                        
            }
            ClearSatRxBuf();
            switch(response)
            {
            case 0: 
              if(SystemState&LOOP)
              {
                SatComState=8;
                WatchdogTimeout=WD_TIMEOUT_2_3;
                AddTask(watchdog);              
              }
              else
              {
                SatComState=7;
                AddTask(&Sat_MSSTM); 
              }
              break;//OK
            case 4: 
              if(SatComStateAttempt<DTE_ATTEMPT)
              { //ERROR
                AddTask(&DTEAuth);
              }
              else
              {
                if(SystemState&LOOP)
                {
                  AddTask(&HangUp);
                  SatComStateAttempt = 0; 
                  SatComState=6;          
                }
                else SatComState=0;
              }
              break;
            default : break;
            }
            SatComRxPtr=0;
          }          
          break;
        case 2: //CALL STATE
          SatComRxBuf[SatComRxPtr++]=buf;
          if(SatComRxPtr==30) SatComRxPtr=0;//error!!! buffer overload!
          if(buf==0x0D) 
          {
            __watchdog_stop(); 
            for(byte i=0; i<SatComRxPtr-1; i++) 
            {
              response = response * 10;
              response += SatComRxBuf[i]-0x30;                        
            }
            
            switch(response)
            {
            case 3:  //NO CARRIER
              if(SatComStateAttempt<CALL_ATTEMPT)
              {//WAIT FOR WATCHDOG TO TIMEOUT BEFORE RETRYING 
                SatComState=8; //WAIT
                WatchdogTimeout=WD_TIMEOUT_9_1;
                AddTask(watchdog);                   
              }
              else
              { 
                AddTask(&HangUp); 
                SatComStateAttempt = 0; 
                SatComState=6;                      
              }
              break;
            case 4: //ERROR
              if(SatComStateAttempt<CALL_ATTEMPT)
              {
                AddTask(&CallInit);                      
              }
              else
              { 
                AddTask(&HangUp); SatComStateAttempt = 0; SatComState=6;                      
              }
              break; 
            case 10:  //CONNECT 2400
              SatComState=3; 
              SatComStateAttempt = 0; 
              SEMAPHORE&=~0x10; 
              AddTask(&SysPageTx);       
              break;            
            default : 
              if(SatComStateAttempt<CALL_ATTEMPT)
              {              
                AddTask(&CallInit);
              }
              break;
            }
            ClearSatRxBuf();
            SatComRxPtr=0;
          }          
          break;                   
        case 3:  //SYS PAGE TX
          SatComRxBuf[SatComRxPtr++]=buf;
          if(SatComRxPtr==30) SatComRxPtr=0;//error!!! buffer overload!
          if(buf==0x0D) 
          {
            __watchdog_stop();             
            switch(SatComRxBuf[0])
            {
            case 0x33: 
              SatComState=2; 
              AddTask(&CallInit);      
              break; //NO CARRIER
            case 0x80: 
              SatComState=4; 
              SatComStateAttempt = 0; 
              AddTask(&DataPageTx);     
              break;//ACK
            case 0x81: 
              //IF SYS PAGE NOT RECEIVED BY SERVER AFTER x ATTEMPTS
               // ADVANCE TO DATA PAGE ANYWAY. 
               // NEED TO FLAG IN NEXT DATA PAGE
              if(SatComStateAttempt<SYSPAGE_ATTEMPT)
              {
                AddTask(&SysPageTx);                     
              }
              else
              { 
                SatComStateAttempt = 0; 
                AddTask(&DataPageTx);     
              }
              break;//NACK
            default : break;
            }
            ClearSatRxBuf();
            SatComRxPtr=0;
          }
          break;                                     
        case 4:  //DATA PAGE TX
          SatComRxBuf[SatComRxPtr++]=buf;
          if(SatComRxPtr==30) SatComRxPtr=0;//error!!! buffer overload!
          if(buf==0x0D) 
          {
            __watchdog_stop();             
            switch(SatComRxBuf[0])
            {
            case 0x33:
              SatComState=2; 
              AddTask(&CallInit);      
              break; //NO CARRIER
            case 0x80: 
              SatComStateAttempt = 0;              
              LTxPtr = LTxPtr + 256;
              eTxPtr = LTxPtr;
              if(LTxPtr +128 < LDataPtr)
              {
                AddTask(&DataPageTx);                     
              }
              else
              {
                SatComState=5; 
                AddTask(&SatExit);
              }              
              break;
            case 0x81: 
              if(SatComStateAttempt<DATAPAGE_ATTEMPT)
              {
                AddTask(&DataPageTx);              
              }
              else
              {//COULDN'T GET DATA PAGE TO SERVER AFTER x ATTEMPTS
               // ADVANCE TO NEXT PAGE ANYWAY. 
               // NEED TO FLAG IN NEXT DATA PAGE
                SatComStateAttempt = 0;              
                LTxPtr = LTxPtr + 256;
                eTxPtr = LTxPtr;
                if(LTxPtr +128 < LDataPtr)
                {
                  AddTask(&DataPageTx);             
                }
                else
                {
                  AddTask(&SatExit);
                  SatComState=5;                 
                }
              }
              break;//NACK
            default : break;
            }
            ClearSatRxBuf();
            SatComRxPtr=0;
          }
          break;           
      case 5: // EXIT 
          SatComRxBuf[SatComRxPtr++]=buf;
          if(SatComRxPtr==30) SatComRxPtr=0;//error!!! buffer overload!
          if(buf==0x0D) 
          {
            __watchdog_stop();             
            switch(SatComRxBuf[0])
            {            
            case '0': //OK 
              SatComState=6; 
              SatComStateAttempt=0; 
              AddTask(&HangUp);                           
              break;  
            case 0x80 : break; //ACK
            case 0x81 : break; //NACK
            default : 
              if(SatComStateAttempt<EXIT_ATTEMPT)
              {
                AddTask(&SatExit);              
              }
              else
              { 
                SatComStateAttempt = 0; 
                SatComState=0;                
                PowerDn(13);            
              }
              break;
            }
            ClearSatRxBuf();
            SatComRxPtr=0;
          }
          break;    
        case 6: //HANG UP STATE
          SatComRxBuf[SatComRxPtr++]=buf;
          if(SatComRxPtr==30) SatComRxPtr=0;//error!!! buffer overload!
          if(buf==0x0D) 
          {
            __watchdog_stop();             
            switch(SatComRxBuf[0])
            {
            case '0': 
              if(SEMAPHORE&0x10)
              {//HANG UP CAUSED BY DATA ACK/NACK TIMEOUT
                SatComState=2; 
                SatComStateAttempt = 0;
                AddTask(&CallInit);          
              }
              else
              {                
                SatComStateAttempt = 0;
                SatComState=0;                               
                PowerDn(13);
              }
              break;//OK
            case '4': 
              if(SatComStateAttempt<HANGUP_ATTEMPT)
              {
                AddTask(&HangUp); //ERROR
              }
              else
              { 
                SatComStateAttempt = 0;
                SatComState=0;
                PowerDn(13);            
              }
              break;
            default : break;
            }
            ClearSatRxBuf();
            SatComRxPtr=0;
          }          
          break;   
        case 7:
          SatComRxBuf[SatComRxPtr++]=buf;
          if(SatComRxPtr==30) SatComRxPtr=0;//error!!! buffer overload!
          if(buf==0x0D) 
          {
            if(SatComRxPtr>9)
            {
              __watchdog_stop();             
              if((SatComRxBuf[0]=='-')&&(SatComRxBuf[2]=='S')&&(SatComRxBuf[4]=='T'))
              {
                if((SatComRxBuf[8]=='n')&&(SatComRxBuf[11]=='n'))
                {//no network service
                  SatComStateAttempt = 0;
                  SatComState=0;                
                  PowerDn(13);
                }
                else
                {
                  IridiumMSSTM=0;
                  i=8;
                  while((SatComRxBuf[i]!=0x0D)&&(i<16))
                  {
                    IridiumMSSTM = IridiumMSSTM << 4;
                    if((SatComRxBuf[i]>=0x30)&&(SatComRxBuf[i]<=0x39))
                      IridiumMSSTM += SatComRxBuf[i]-0x30;                        
                    else 
                      switch(SatComRxBuf[i])
                      {
                      case 'a': IridiumMSSTM += 10; break;
                      case 'b': IridiumMSSTM += 11; break;
                      case 'c': IridiumMSSTM += 12; break;
                      case 'd': IridiumMSSTM += 13; break;
                      case 'e': IridiumMSSTM += 14; break;
                      case 'f': IridiumMSSTM += 15; break;                        
                      }
                    i++;
                  }
                  //Irridium MSSTM - 
                  //    - no. of 90ms since March 8, 2007, 03:50:21.00 GMT
                  IridiumMSSTM=IridiumMSSTM*9;
                  IridiumMSSTM=IridiumMSSTM/100;                  
                  IridiumMSSTM+=5802621;
                  unsigned long yearRm;
                  unsigned int dateRm, hrRm;
                  unsigned int dayOfYear;
                  IridiumTime.year=(IridiumMSSTM/31536000);
                  if(IridiumTime.year>1) yearRm=IridiumMSSTM-(((IridiumTime.year)*31536000)+86400);
                  else if(IridiumTime.year>5) yearRm=IridiumMSSTM-(((IridiumTime.year)*31536000)+172800);
                  IridiumTime.year+=7;

                  dayOfYear=(yearRm/86400);
                  dateRm=yearRm-(dayOfYear*86400);

                  if(dayOfYear<=31){IridiumTime.mon=1; IridiumTime.date=dayOfYear;} //january
                  else if(dayOfYear<=59){IridiumTime.mon=2; IridiumTime.date=dayOfYear-31;} //february
                  else if(dayOfYear<=90){IridiumTime.mon=3; IridiumTime.date=dayOfYear-59;} //march                  
                  else if(dayOfYear<=120){IridiumTime.mon=4; IridiumTime.date=dayOfYear-90;} //april
                  else if(dayOfYear<=151){IridiumTime.mon=5; IridiumTime.date=dayOfYear-121;} //may                  
                  else if(dayOfYear<=181){IridiumTime.mon=6; IridiumTime.date=dayOfYear-151;} //june                  
                  else if(dayOfYear<=212){IridiumTime.mon=7; IridiumTime.date=dayOfYear-181;} //july
                  else if(dayOfYear<=243){IridiumTime.mon=8; IridiumTime.date=dayOfYear-212;} //august
                  else if(dayOfYear<=273){IridiumTime.mon=9; IridiumTime.date=dayOfYear-243;} //september
                  else if(dayOfYear<=304){IridiumTime.mon=10; IridiumTime.date=dayOfYear-273;} //october
                  else if(dayOfYear<=334){IridiumTime.mon=11; IridiumTime.date=dayOfYear-304;} //november
                  else{IridiumTime.mon=12; IridiumTime.date=dayOfYear-334;} //december                  
                  
                  IridiumTime.hr=(dateRm/3600);
                  hrRm=dateRm-(IridiumTime.hr*3600);                  
                  
                  IridiumTime.mins=(hrRm/60);
                  IridiumTime.secs=(hrRm-(IridiumTime.mins*60));                                    
                  SEMAPHORE |= 0x08;
                }                
              }
              else if(SatComStateAttempt<MSSTM_ATTEMPT)
              {
                AddTask(&Sat_MSSTM); //ERROR
              }
            }
            SatComStateAttempt = 0;
            SatComState=0;
            PowerDn(13);                        
          }
          break;
        default :break;
        }
      }
    }    
  }while(moreData&&status);
  return status;
}

void ClearSatRxBuf()
{
  for(byte i=0; i<30; i++) SatComRxBuf[i]=0xFF;
}
