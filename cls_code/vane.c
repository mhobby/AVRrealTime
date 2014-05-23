#include "lib.h"

byte VaneParse()
{
  byte status = 0;
  byte buf;
  byte moreData;
  byte frameErr;
  byte syncErr;
  
  char tempStr[10];
  byte i;
  
  do
  {
    moreData=0; 
    frameErr=0; 
    syncErr=0;
    status = ReadMAX3100(10,&buf, &moreData, &frameErr, &syncErr); 

    if((syncErr!=1)&&(status==1))
    {
      switch(VaneState) //pkt: $WIMV,<dir>,<spd>,M,A<CR><LF>
      {
      case 0: if(buf==0x24) VaneState=1; break; //'wait' for $; indicating new pkt
      case 1: 
        if(buf==0x57) //W?
        {
          VaneState=2;      
        }
        else VANE_FSM_RESET();   //reset state machine  
        break;
      case 2: 
        if(buf==0x49) //I?
        {
          VaneState=3;      
        }
        else VANE_FSM_RESET();   //reset state machine  
        break;
      case 3: 
        if(buf==0x4D) //M?
        {
          VaneState=4;      
        }
        else VANE_FSM_RESET();   //reset state machine  
        break;
      case 4: 
        if(buf==0x57) //W?
        {
          VaneState=5;      
        }
        else VANE_FSM_RESET();   //reset state machine  
        break;
      case 5: 
        if(buf==0x56) //V?
        {
          VaneState=6;      
        }
        else VANE_FSM_RESET();   //reset state machine  
        break;
      case 6: 
        if(buf==0x2c) //,?
        {
          VaneState=7;      
        }
        else VANE_FSM_RESET();   //reset state machine  
        break;        
      case 7: //DATA
        if(buf==0x0D)
        {
          VaneState=8;   
        }
        else VaneBuf[VaneBufPtr++]=buf;  //Data
        if(VaneBufPtr>VANE_BUF_SIZE) VANE_FSM_RESET();   //reset state machine  
        break;
      case 8:
        if(buf==0x0A)
        {          
          for(i=0; i<3; i++) tempStr[i]=VaneBuf[i];
          tempStr[i]='\n';
          VaneDir = atoi(tempStr);
          VaneState=9;
        }
        else VANE_FSM_RESET();   //reset state machine   
      default: break;
      }      
      if(frameErr)
      {
        VANE_FSM_RESET();   //reset state machine             
      }         
    }
  }while(moreData&&status);     
  return status;
}


