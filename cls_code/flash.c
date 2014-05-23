#include "lib.h"

byte readStatusFlash()
{
  byte return_byte[2] = {0,0};
  Set_SS_Lo(0);
  {
    return_byte[0]=SPI_Clock(STAT_R); 
    return_byte[1]=SPI_Clock(0xFF);
  }
  Set_SS_Hi();  
  return return_byte[1];
}

void welFlash()
{ 
  Set_SS_Lo(0);
  {
    SPI_Clock(WEL); 
  }
  Set_SS_Hi();
}


byte writeStatusFlash(byte dataOut)
{
  byte status=0;
  if(!(SEMAPHORE & 0x40))
  {  
    welFlash();
    Set_SS_Lo(0);
    {
      SPI_Clock(STAT_W); 
      SPI_Clock(dataOut);
    }
    Set_SS_Hi();
    status =1;    
  }
  return status;
}

byte programFlash(unsigned long addr, byte* dataIn, unsigned int byteNo)
{
  byte status, flashStatus;
  status =0;
  if(!(SEMAPHORE & 0x40))
  {  
    flashStatus = readStatusFlash();
    if(flashStatus&0x01) status =0;
    else
    {  
      welFlash();
      Set_SS_Lo(0);
      {
        SPI_Clock(PROG); 
        SPI_Clock((0x00FF0000&addr)>>16);
        SPI_Clock((0x0000FF00&addr)>>8);
        SPI_Clock(0x000000FF&addr);
        for(int i=0; i<byteNo; i++) SPI_Clock(dataIn[i]);
      }
      Set_SS_Hi();  
      status =1;
    }
  }
  return status;
}

byte readFlash(unsigned long addr, byte* dataOut, unsigned int byteNo)
{
  byte flashStatus, status;

  status =0;
  if(!(SEMAPHORE & 0x40))
  {  
    flashStatus = readStatusFlash();
    if(flashStatus&0x01) status =0;
    else
    {
      Set_SS_Lo(0);
      {
        SPI_Clock(READ); 
        SPI_Clock((0x00FF0000&addr)>>16);
        SPI_Clock((0x0000FF00&addr)>>8);
        SPI_Clock(0x000000FF&addr);
        for(int i=0; i<byteNo; i++) dataOut[i] = SPI_Clock(0xFF);
      }
      Set_SS_Hi();
      status =1;
    }
  }
  return status;
}

byte eraseFlash(unsigned long addr)
{
  byte status, flashStatus;
  status = 0;
  if(!(SEMAPHORE & 0x40))
  {  
    flashStatus = readStatusFlash();
    if(flashStatus&0x01) status =0;
    else
    {  
      welFlash();
      Set_SS_Lo(0);
      {
        SPI_Clock(ERASE); 
        SPI_Clock((0x00FF0000&addr)>>16);
        SPI_Clock((0x0000FF00&addr)>>8);
        SPI_Clock(0x000000FF&addr);
      }
      Set_SS_Hi();  
      status = 1;
    }
  }
  return status;
}

byte checkEpe(byte* epeError)
{
  byte status, flashStatus;
  status = 0;
  if(!(SEMAPHORE & 0x40))
  {  
    flashStatus = readStatusFlash();
    if(flashStatus&0x01) status = 0;
    else 
    {
      status = 1;
      *epeError = (flashStatus&0x20)>>5;
    }
  }
  return status;
}

byte writeFlash(unsigned long addr, byte* data, unsigned int bytes)
{
  byte status, readStatus, eraseStatus, programStatus;
  byte tempBuf;

  status = readStatus = eraseStatus = programStatus = 0;

  if(!(SEMAPHORE & 0x40))
  {  
    readStatus=readFlash(addr, &tempBuf, 1);
    if(readStatus)
    {
      if(tempBuf!=0xFF) eraseStatus=eraseFlash(addr);
      else eraseStatus=1;
      
      if(eraseStatus) programStatus=programFlash(addr, data, bytes);
      
      if(programStatus) status =1;
    }
  }
  return status;
}
