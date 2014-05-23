#include "lib.h"

byte RequestRHTADCData()
{
  byte status=0;
  byte return_byte;
  byte cmd;
  byte dummy;

  switch(RhtADCState)
  {
  case 0 : cmd=0x3F; break;
  case 1 : cmd=0x01; break;
  case 2 : cmd=0x44; break;
  case 3 : cmd=0x00; break;
  case 4 : cmd=0x0D; break;
  case 5 : cmd=0x0A; break;
  default : 
    cmd = 0xFF;
    break;  
  }
  
  if(RhtADCState<6)
  {    
    status = WriteMAX3100(6, cmd, &return_byte, &dummy); 
    if(status)
    {
      RhtADCState++;
      if(RhtADCState<6) AddTask(&RequestRHTADCData); 
    }
    else
    {
      status=0;
    }      
  }
  else
  {
    status = 0;
  }
  return status;  
}

byte RHTADCParse()
{
  byte status = 0;
  byte buf; 
  byte frameErr;
  byte moreData;
  byte syncErr;
   
  do
  {
    moreData = 0;
    frameErr = 0;
    syncErr = 0;
    
    status = ReadMAX3100(6,&buf, &moreData, &frameErr, &syncErr); 

    if((syncErr!=1)&&(status==1))
    {
      switch(RhtADCState)
      {
      case 6: if(buf==0x24) RhtADCState++; break; //'wait' for data state
      case 7: RhtADCState++; break;
      case 8: 
        if(buf==0x44) RhtADCState++;   //should be a 'D'
        else
        {
          RHTADC_FSM_RESET();  //otherwise reset state machine
        }
        break;
      case 9:
        RhtADCBuf[RhtADCBufPtr++]=buf; 
        if(RhtADCBufPtr>=18)
        {
          RhtADCState++;
        }
        break;
      case 10:
        if(buf==0x0D) RhtADCState++;
        else
        {//otherwise reset state machine                  
          RHTADC_FSM_RESET();  
        }
        break;
      case 11: if(buf==0x0A) RhtADCState++; break; 
      }
      
      if(frameErr)
      {//reset state machine
        RHTADC_FSM_RESET();  
      }    
    }

    if(syncErr)
    {
      syncErr=99;
    }    
  }while(moreData&&status); 

  return status;
}
