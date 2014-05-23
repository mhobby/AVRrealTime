#include "lib.h"

byte RequestRADADCData()
{
  byte status=0;
  byte return_byte;
  byte cmd;
  byte dummy;

  switch(0x1F&RadADCState)
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
  
  if((0x1F&RadADCState)<6)
  {    
    status = WriteMAX3100(7, cmd, &return_byte, &dummy); 
    if(status)
    {
      RadADCState++;
      if((0x1F&RadADCState)<6) AddTask(&RequestRADADCData); 
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

byte RADADCParse()
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
    
    status = ReadMAX3100(7,&buf, &moreData, &frameErr, &syncErr); 

    if((syncErr!=1)&&(status==1))
    {
      switch(0x1F&RadADCState)
      {
      case 6: if(buf==0x24) RadADCState++; break; //'wait' for data state
      case 7: RadADCState++; break;
      case 8: 
        if(buf==0x44) RadADCState++;   //should be a 'D'
        else
        {
          RADADC_FSM_RESET();  //otherwise reset state machine
        }
        break;
    case 9:
        RadADCBuf[RadADCBufPtr++]=buf; 
        if(RadADCBufPtr>=18)
        {
          RadADCState++;
        }
        break;
      case 10:
        if(buf==0x0D) RadADCState++;
        else
        {//otherwise reset state machine                  
          RADADC_FSM_RESET();  
        }
        break;
      case 11: 
        if(buf==0x0A) 
        {
          RadADCState++; 
          if(RadADC_SC_State==1) 
          {
            if(!(RadADCState&0x40)) AddTask(FluxPlateSCOn);            
          }
          else if(RadADC_SC_State==2)
          {
            if(RadADCState&0x40) AddTask(FluxPlateSCOff);          
          }
          
        }
        break; 
      case 18: //Power On/Off responses
        if(buf==0x24) RadADCState++; 
        break;
      case 19:
        if(buf==0x0A)
        {
          RadADCState++; 
          if(RadADCState&0x40) RadADCState&=~0x40;
          else RadADCState|=0x40; 
        }
        break;
      default: break;
      }
      
      if(frameErr)
      {//reset state machine
        RADADC_FSM_RESET();  
      }    
    }
    if(syncErr)
    {
      syncErr=99;
    }
  }while(moreData&&status); 

  return status;
}

byte FluxPlateSCOn()
{
  byte status=0;
  byte return_byte;
  byte cmd;
  byte dummy;

  switch(0x1F&RadADCState)
  {
  case 12 : cmd=0x3F; break;
  case 13 : cmd=0x01; break;
  case 14 : cmd=0x54; break;
  case 15 : cmd=0x04; break;
  case 16 : cmd=0x0D; break;
  case 17 : cmd=0x0A; break;
  }
  
  if((0x1F&RadADCState)<18)
  {    
    status = WriteMAX3100(7, cmd, &return_byte, &dummy); 
    if(status)
    {
      RadADCState++;
      if((0x1F&RadADCState)<18) AddTask(&FluxPlateSCOn); 
    }
  }
  return status;  
}

byte FluxPlateSCOff()
{
  byte status=0;
  byte return_byte;
  byte cmd;
  byte dummy;

  switch(0x1F&RadADCState)
  {
  case 12 : cmd=0x3F; break;
  case 13 : cmd=0x01; break;
  case 14 : cmd=0x54; break;
  case 15 : cmd=0x00; break;
  case 16 : cmd=0x0D; break;
  case 17 : cmd=0x0A; break;
  }
  
  if((0x1F&RadADCState)<18)
  {    
    status = WriteMAX3100(7, cmd, &return_byte, &dummy); 
    if(status)
    {
      RadADCState++;
      if((0x1F&RadADCState)<18) AddTask(&FluxPlateSCOff); 
    }
  }
  return status;  
}
