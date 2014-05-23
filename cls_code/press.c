#include "lib.h"

byte requestPressData(byte pressSens, byte* pressSensState)
{
  byte status=0;
  byte return_byte;
  byte cmd, chan;
  byte dummy;

  switch(0x1F&*pressSensState)
  {
  case 0 : cmd=0x3F; break;
  case 1 : cmd=0x01; break;
  case 2 : 
    if(*pressSensState&0x40) cmd=0x50; //'P'
    else cmd=0x58; //'X'
    break;
  case 3 : cmd=0x0D; break;
  case 4 : cmd=0x0A; break;
  default : 
    cmd = 0xFF;
    break;  
  }
  
  switch(pressSens)
  {
  case 1 : chan=4; break;
  case 2 : chan=5; break;
  }

  if((0x1F&*pressSensState)<5)
  {    
    status = WriteMAX3100(chan, cmd, &return_byte, &dummy); 
    if(status)
    {
      (*pressSensState)++;
      if((0x1F&(*pressSensState))<5)
      {       
        switch(pressSens)
        {
        case 1 : AddTask(&RequestPressAData); break;
        case 2 : AddTask(&RequestPressBData); break;
        default : break;
        }
      }
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

byte RequestPressAData()
{    
  byte status;
  status = requestPressData(1, &PressAState);
  return status;
}

byte RequestPressBData()
{    
  byte status;
  status = requestPressData(2, &PressBState);
  return status;
}

byte pressParse(byte pressSens, char dataType)
{
  byte status = 0;
  byte buf;
  
  byte chan;
  char pressBufMaxSize;
  byte* pressBuf;
  byte* pressBufPtr;
  byte* pressSensState;
  
  byte frameErr;
  byte moreData;
  byte syncErr;
  
  switch(pressSens)
  {
  case 1 : 
    if(PressAState&0x40) pressBuf=&PressACal[0];
    else pressBuf=&PressABuf[0];    
    chan=4; 
    pressBufPtr=&PressABufPtr; 
    pressSensState=&PressAState; 
    break;
  case 2 : 
    if(PressBState&0x40) pressBuf=&PressBCal[0];
    else pressBuf=&PressBBuf[0];
    chan=5; 
    pressBufPtr=&PressBBufPtr; 
    pressSensState=&PressBState; 
    break;
  }
  if(dataType == 'P')
  {
    pressBufMaxSize = 8;
  }
  else
  {
    pressBufMaxSize = 4;
  }
  
  do
  {
    moreData = 0;
    frameErr = 0;
    syncErr = 0;
    
    status = ReadMAX3100(chan,&buf, &moreData, &frameErr, &syncErr); 

    if((syncErr!=1)&&(status==1))
    {
      switch(0x0F&*pressSensState)
      {
      case 5: if(buf==0x24) (*pressSensState)++; break; //'wait' for data state
      case 6: (*pressSensState)++; break;
      case 7: 
        if(buf==dataType) (*pressSensState)++;   //should be an 'X' or a 'P'
        else PRESS_A_FSM_RESET();  //otherwise reset state machine
        break;
      case 8: 
        pressBuf[(*pressBufPtr)++]=buf; 
        if(*pressBufPtr>=pressBufMaxSize) (*pressSensState)++;
        break;
      case 9:
        if(buf==0x0D) (*pressSensState)++;
        else
        {//otherwise reset state machine                  
          switch(pressSens)
          {
          case 1 : PRESS_A_FSM_RESET();  break;
          case 2 : PRESS_B_FSM_RESET();  break;
          }
        }
        break;
      case 10: if(buf==0x0A) (*pressSensState) &=~0x40; break;
      }
      
      if(frameErr)
      {//reset state machine
        switch(pressSens)
        {
        case 1 : PRESS_A_FSM_RESET();  break;
        case 2 : PRESS_B_FSM_RESET();  break;
        }
      }    
    }
  }while(moreData&&status);  
  return status;
}

byte PressAParse()
{
  byte status;
  if(PressAState&0x40) status = pressParse(1, 'P');
  else status = pressParse(1, 'X');
  return status;
}

byte PressBParse()
{
  byte status;
  if(PressBState&0x40) status = pressParse(2, 'P');
  else status = pressParse(2, 'X');
  return status;
}
