#include "lib.h"

byte SonicParse()
{
  byte status = 0;
  byte buf;
  byte moreData;
  byte frameErr;
  byte syncErr;

  do
  {
    moreData=0; 
    frameErr=0; 
    syncErr=0;

    status = ReadMAX3100(8,&buf, &moreData, &frameErr, &syncErr); 

    if((syncErr!=1)&&(status==1))
    {
      switch(SonicState)
      {
      case 0: if(buf==0x30) SonicState=1; break; //'wait' for data state
      case 1: 
        if(buf==0x72) //r?0
        {
          SonicState=2;      
        }
        else SONIC_FSM_RESET();   //reset state machine  
        break;
      case 2: 
        if(buf==0x31) //1?
        {
          SonicState=3;      
        }
        else SONIC_FSM_RESET();   //reset state machine  
        break;
      case 3: 
        if(buf==0x2c) //comma?
        {
          SonicState=4;      
        }
        else SONIC_FSM_RESET();   //reset state machine  
        break;
      case 4: //DATA
        if(buf==0x0D)
        {
          SonicState=5;   
        }
        else SonicBuf[SonicBufPtr++]=buf;  //Data
        if(SonicBufPtr>SONIC_BUF_SIZE)
        {
          SONIC_FSM_RESET();   //reset state machine  
        }
        break;
      case 5:
        if(buf==0x0A)
        {
          SonicState=6;          
          AddTask(&CalcSonicProd);           
        }
        else SONIC_FSM_RESET();   //reset state machine   
      default: break;
      }      
      if(frameErr)
      {
          SONIC_FSM_RESET();   //reset state machine             
      }         
    }
  }while(moreData&&status);   
  return status;
}

byte CalcSonicProd()
{
  byte status = 1;
  byte i,j;
  unsigned long sonicSpdVar;
  signed long sonicSpdSkw;  
  
  i=0;
  j=0;
  char tempStr[10];

  SonicBufPtr = 3;
  
  while((SonicBuf[SonicBufPtr]!='=')&&(i<15)) { i++; SonicBufPtr++; };
  SonicBufPtr++;
  while((SonicBuf[SonicBufPtr]!='.')&&(j<6)) tempStr[j++] = SonicBuf[SonicBufPtr++];
  SonicBufPtr++;
  tempStr[j++] = SonicBuf[SonicBufPtr++];
  
  if(j>7) 
  {
    SONIC_FSM_RESET();
  }
  else
  {
    tempStr[j] = '\n';

    SonicSpd = atoi(tempStr);
    SonicSpdSum = SonicSpdSum + SonicSpd;  
    sonicSpdVar = (unsigned long)SonicSpd * (unsigned long)SonicSpd;
    sonicSpdSkw = sonicSpdVar * SonicSpd;
    SonicSpdSkwSum = SonicSpdSkwSum + sonicSpdSkw;
    SonicSpdVarSum = SonicSpdVarSum + sonicSpdVar;
    if(SonicSamples==0) SonicSpdMax=SonicSpd;
    else if(SonicSpd>SonicSpdMax) SonicSpdMax=SonicSpd;
    SonicSamples++;
    SonicState++; 
    AddTask(&CalcSonicUV);
  }
  return status;
}

void CalcUV(int spd, int dir, signed long* _U, signed long* _V)
{
  signed char uMult, vMult;
  
  if((dir>=0)&&(dir<90))
  {//NE quadrant (0 to 89deg)
    uMult = 1;
    vMult = 1;
    dir = dir;
    *_U = cosine_lookup[90-dir]*(long)uMult*(long)spd; //sin
    *_V = cosine_lookup[dir]*(long)vMult*(long)spd; //cos
  }
  else if((dir>=90)&&(dir<180))
  {//SE quadrant (90 to 179)
    uMult = 1;
    vMult = -1;      
    dir = dir-90;
    *_U = cosine_lookup[dir]*(long)uMult*(long)spd; //cos
    *_V = cosine_lookup[90-dir]*(long)vMult*(long)spd; //sin
  }
  else if((dir>=180)&&(dir<270))
  {//SW quadrant (180 to 269)
    uMult = -1;
    vMult = -1;      
    dir = dir-180;
    *_U = cosine_lookup[90-dir]*(long)uMult*(long)spd; //sin
    *_V = cosine_lookup[dir]*(long)vMult*(long)spd; //cos
  }
  else if((dir>=270)&&(dir<360))
  {//NW quadrant (270 to 359)
    uMult = -1;
    vMult = 1;      
    dir = dir-270;      
    *_U = cosine_lookup[dir]*(long)uMult*(long)spd; //cos
    *_V = cosine_lookup[90-dir]*(long)vMult*(long)spd; //sin
  }
  else
  {//error!!!
    uMult = 0;
    vMult = 0;
    dir = 0;
    *_U = cosine_lookup[90-dir]*(long)uMult*(long)spd; //sin
    *_V = cosine_lookup[dir]*(long)vMult*(long)spd; //cos    
  }
}

byte CalcSonicUV()
{
  byte status = 1;
  byte j;
  int sonicDir;
  long u, v;
  
  SonicBufPtr = 3;
  j=0;
  
  char tempStr[10];
  
  while((SonicBuf[SonicBufPtr]!='D')&&(j<5)) tempStr[j++] = SonicBuf[SonicBufPtr++];
  if(j>6) 
  {
    SONIC_FSM_RESET();
  }
  else
  {
    tempStr[j] = '\n';
    sonicDir = atoi(tempStr);
    CalcUV(SonicSpd, sonicDir, &u, &v);
    SonicUSum = SonicUSum + u;
    SonicVSum = SonicVSum + v;
    SonicState=0;
    SonicBufPtr=0;
  }
  return status;
}
