#include "lib.h"

byte CupParse()
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
    status = ReadMAX3100(9,&buf, &moreData, &frameErr, &syncErr); 

    if((syncErr!=1)&&(status==1))
    {
      switch(CupState) //pkt: $WIMV,<dir>,<spd>,M,A<CR><LF>
      {
      case 0: if(buf==0x24) CupState=1; break; //'wait' for $; indicating new pkt
      case 1: 
        if(buf==0x57) //W?
        {
          CupState=2;      
        }
        else CUP_FSM_RESET();   //reset state machine  
        break;
      case 2: 
        if(buf==0x49) //I?
        {
          CupState=3;      
        }
        else CUP_FSM_RESET();   //reset state machine  
        break;
      case 3: 
        if(buf==0x4D) //M?
        {
          CupState=4;      
        }
        else CUP_FSM_RESET();   //reset state machine  
        break;
      case 4: 
        if(buf==0x57) //W?
        {
          CupState=5;      
        }
        else CUP_FSM_RESET();   //reset state machine  
        break;
      case 5: 
        if(buf==0x56) //V?
        {
          CupState=6;      
        }
        else CUP_FSM_RESET();   //reset state machine  
        break;
      case 6: 
        if(buf==0x2c) //,?
        {
          CupState=7;      
        }
        else CUP_FSM_RESET();   //reset state machine  
        break;        
      case 7: //DATA
        if(buf==0x0D)
        {
          CupState=8;   
        }
        else CupBuf[CupBufPtr++]=buf;  //Data
        if(CupBufPtr>CUP_BUF_SIZE) CUP_FSM_RESET();   //reset state machine  
        break;
      case 8:
        if(buf==0x0A)
        {          
          for(i=0; i<2; i++) tempStr[i]=CupBuf[i+6];
          for(i=2; i<3; i++) tempStr[i]=CupBuf[i+7];
          tempStr[i]='\n';
          CupSpd = atoi(tempStr);
          CupState=9;
          AddTask(&CalcCupProd);
        }
        else CUP_FSM_RESET();   //reset state machine   
      default: break;
      }      
      if(frameErr)
      {
        CUP_FSM_RESET();   //reset state machine             
      }         
    }
  }while(moreData&&status);     
  return status;
}


byte CalcCupProd()
{
  byte status =1;
  unsigned long cupSpdVar;
  signed long cupSpdSkw;
  CupSamples++;
  CupSpdSum = CupSpdSum + CupSpd;
  cupSpdVar = (unsigned long)CupSpd * (unsigned long)CupSpd;
  cupSpdSkw = cupSpdVar * CupSpd;
  CupSpdSkwSum = CupSpdSkwSum + cupSpdSkw;
  CupSpdVarSum = CupSpdVarSum + cupSpdVar;
  CupState = 10;
  AddTask(&CalcCupVaneUV);
  return status;
}

byte CalcCupVaneUV()
{
  byte status;
  long u, v;
  if((CupState==10)&&(VaneState==9))
  {
    CalcUV(CupSpd, VaneDir, &u, &v);
    CupVaneUSum = CupVaneUSum + u;
    CupVaneVSum = CupVaneVSum + v;
    status =1;
    CupState = 0;
    VaneState = 0;
  }
  else status =0; //vane data not ready so reschedule
  return status;
}
