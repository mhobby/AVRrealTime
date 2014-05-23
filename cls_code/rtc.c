#include "lib.h"
#define SAMPLE_PERIOD 200
#define GNDHFSC_PERIOD 54
#define GNDHFSC_ONTIME 3

byte rtc_alarm()
{
  static byte sampleCountdown=0;
  static unsigned int txCountdown=0; 
  static byte satCom_Operational=0;
  static unsigned int gndHeatFluxSC_Countdown=0; 
  static byte gndHeatFlux_InCal=0;
  static byte pressWatchdog=0;

  static _time lTime;
  byte status = 1;
  
  rtc_clr_interrupt();
    
  switch(0x1F&SystemState)
  {
  case SYNC : 
    rtc_get(&lTime); //get new time    
    switch(SyncState)
    {
    case 00:  //SYNC_INIT 
#ifndef NOBEEP      
      PowerUp(15);
#endif      
      pressWatchdog = 0; //initialise pressure watchdog timer
      PRESS_A_FSM_RESET(); 
      PRESS_B_FSM_RESET(); 
      AddTask(&RequestPressAData);  //Collect Pressure Cal Constants        
      AddTask(&RequestPressBData);  //Collect Pressure Cal Constants        
      SyncState=01;
      __delay_cycles(800000);
#ifndef NOBEEP
      PowerDn(15);
#endif      
      break;
    case 01:   //SYNC_RETRY
#ifndef NOBEEP
      PowerUp(15);
#endif
      if((PressAState&0x40)||(PressBState&0x40))
      {//no press cals received
        PRESS_A_FSM_RESET(); 
        PRESS_B_FSM_RESET(); 
        if((PressAState&0x40)&&((0x1F&PressAState)<5)) AddTask(&RequestPressAData);  
        if((PressBState&0x40)&&((0x1F&PressBState)<5)) AddTask(&RequestPressBData);  
        pressWatchdog++ ; //increment watchdog timer
        // if watchdog counts to 5, progress to next stage and assume HW failure
        if(pressWatchdog>5)
        {
          if(PressAState&0x40) LSysBuffer.ctrlA[0] |= (1<<4);  
          if(PressBState&0x40) LSysBuffer.ctrlA[0] |= (1<<5);             
          for(byte i=0; i<4; i++)
          {
            byte ref;
            ref =2*i;
            LSysBuffer.pressACals[i] = (PressACal[ref]<<8)+PressACal[ref+1];
            eSysBuffer[LSysPtr].pressACals[i] = (PressACal[ref]<<8)+PressACal[ref+1];
            LSysBuffer.pressBCals[i] = (PressBCal[ref]<<8)+PressBCal[ref+1];
            eSysBuffer[LSysPtr].pressBCals[i] = (PressBCal[ref]<<8)+PressBCal[ref+1];
          }
          eSysBuffer[LSysPtr].ctrlA[0] = LSysBuffer.ctrlA[0];            
          AddTask(&UpdateSysPage);
          SyncState=02; 
        }
      }
      else
      {
        SyncState=02; //press cals received
        for(byte i=0; i<4; i++)
        {
          byte ref;
          ref =2*i;
          LSysBuffer.pressACals[i] = (PressACal[ref]<<8)+PressACal[ref+1];
          eSysBuffer[LSysPtr].pressACals[i] = (PressACal[ref]<<8)+PressACal[ref+1];
          LSysBuffer.pressBCals[i] = (PressBCal[ref]<<8)+PressBCal[ref+1];
          eSysBuffer[LSysPtr].pressBCals[i] = (PressBCal[ref]<<8)+PressBCal[ref+1];
        } 
        AddTask(&UpdateSysPage);
      }
      __delay_cycles(800000);
#ifndef NOBEEP
      PowerDn(15);
#endif
      break;
    case 02:   //SYNC_COMPLET
      //Advance System State to LOOP        
#if SAMPLE_PERIOD==200      
      if((lTime.mins-(10*(lTime.mins/10))==0)&&(lTime.secs==1))
#else        
      if(lTime.secs-(SAMPLE_PERIOD*(lTime.secs/SAMPLE_PERIOD))==1)
#endif
      {       
#ifndef NOBEEP        
        PowerUp(15);
#endif
        SystemState=(0xE0&SystemState)|LOOP; 
        
        //reset products
        SonicSpdSum = SonicUSum = SonicVSum = SonicSpdSkwSum = SonicSpdVarSum = 0;
        SonicSamples = 0;
        CupSpdSum = CupVaneUSum = CupVaneVSum = CupSpdSkwSum = CupSpdVarSum = 0;
        CupSamples = 0;
        
        //flush streaming buffers
        SONIC_FSM_RESET(); FlushMAX3100(SONIC_CHAN);  
        CUP_FSM_RESET(); FlushMAX3100(CUP_CHAN);  //flush MAX3100 buffer
        VANE_FSM_RESET(); FlushMAX3100(VANE_CHAN);  //flush MAX3100 buffer
 
        //request new data from instruments
        PRESS_A_FSM_RESET(); AddTask(&RequestPressAData);
        PRESS_B_FSM_RESET(); AddTask(&RequestPressBData);        
        RHTADC_FSM_RESET(); AddTask(&RequestRHTADCData);
        RADADC_FSM_RESET(); AddTask(&RequestRADADCData);
        AddTask(&RequestADCData);                    
      }     
      break;
    }
    break;
  case LOOP :
#ifndef NOBEEP
    PowerDn(15);     //Turn Buzzer Off
#endif
    sampleCountdown++; //increment number of samples
    //txCountdown++; //increment number of seconds to Tx    

    if(sampleCountdown>=SAMPLE_PERIOD) 
    { 
      gndHeatFluxSC_Countdown++; //increment no. of seconds gnd heatflux sc on/off
    /*** NEW SAMPLE EPOCH ***/
      //copy data from instrument buffers to data buffer
      DataBuffer.station_id = eStationID;
      DataBuffer.rtc_time = lTime; 
      if(PressAState==10)
      {
        DataBuffer.pressA_D1 = (PressABuf[0]<<8)+PressABuf[1];
        DataBuffer.pressA_D2 = (PressABuf[2]<<8)+PressABuf[3];  
      }
      if(PressBState==10)
      {
        DataBuffer.pressB_D1 = (PressBBuf[0]<<8)+PressBBuf[1];
        DataBuffer.pressB_D2 = (PressBBuf[2]<<8)+PressBBuf[3];  
      }      
      
      if(RadADCState>=12)
      {
        for(byte i=0; i<8; i++) 
          DataBuffer.rad_adc[i] = ((int)RadADCBuf[2*i]<<8) + RadADCBuf[(2*i)+1];
        if(RadADC_SC_State>0) DataBuffer.GndHeatFluxSC=1;
      }
      if(RhtADCState==12)
      {
        for(byte i=0; i<3; i++) 
          DataBuffer.rht_adc[i] = ((int)RhtADCBuf[2*i]<<8) + RhtADCBuf[(2*i)+1];
        for(byte i=4; i<7; i++) 
          DataBuffer.rht_adc[i-1] = ((int)RhtADCBuf[2*i]<<8) + RhtADCBuf[(2*i)+1];
      }
      //copy sonic data to data buffer
      DataBuffer.sonicSpdSum = SonicSpdSum;
      DataBuffer.sonicSpd2Sum = SonicSpdVarSum;
      DataBuffer.sonicSpd3Sum = SonicSpdSkwSum;
      DataBuffer.sonicUSum = SonicUSum;
      DataBuffer.sonicVSum = SonicVSum;        
      DataBuffer.sonicSampleNo = SonicSamples;

      //copy cup\vane to data buffer      
      DataBuffer.cupSpdSum = CupSpdSum;
      DataBuffer.cupSpd2Sum = CupSpdVarSum;
      DataBuffer.cupSpd3Sum = CupSpdSkwSum;
      DataBuffer.cupVaneUSum = CupVaneUSum;
      DataBuffer.cupVaneVSum = CupVaneVSum;        
      DataBuffer.cupVaneSampleNo = CupSamples;
      
      DataBuffer.V_5Vreg = SysADCBuf[0];
      DataBuffer.V_12Vreg = SysADCBuf[1];
      DataBuffer.V_Batt = SysADCBuf[2];
      DataBuffer.GroundProbe1 = SysADCBuf[3];
      DataBuffer.GroundProbe2 = SysADCBuf[4];
      DataBuffer.SysTemp = SysADCBuf[5];
            
      for(byte i =0; i<AUX_SIZE; i++) DataBuffer.aux[i] = i;
      
      SEMAPHORE |= 0x80; //mark data buffer as now being full
      AddTask(&UploadDataBuffer); //schedule task to write DataBuf to disk

      sampleCountdown=0; //reset sample counter      
      
      SonicSpdSum = SonicUSum = SonicVSum = SonicSpdSkwSum = SonicSpdVarSum = 0;
      SonicSamples = 0;
      CupSpdSum = CupVaneUSum = CupVaneVSum = CupSpdSkwSum = CupSpdVarSum = 0;
      CupSamples = 0;
      
      //request 0.1Hz new data from instruments
      PRESS_A_FSM_RESET(); AddTask(&RequestPressAData);
      PRESS_B_FSM_RESET(); AddTask(&RequestPressBData);
      RHTADC_FSM_RESET(); AddTask(&RequestRHTADCData);
      RADADC_FSM_RESET(); AddTask(&RequestRADADCData);
      SYSADC_FSM_RESET(); AddTask(&RequestADCData); 
      
      //reset Star Count
      starCount=0;
    }
    else
    {
    /*** NON COMMUNICATIVE INSTRUMENTS ***/  
      if(((0x40&PressAState)>=5)&&(PressABufPtr==0))
      {
        FlushMAX3100(PRESSA_CHAN); PRESS_A_FSM_RESET(); AddTask(&RequestPressAData); //NO PRESSURE A DATA
      }
      if((0x40&(PressBState>=5))&&(PressBBufPtr==0))
      {
        FlushMAX3100(PRESSB_CHAN); PRESS_B_FSM_RESET(); AddTask(&RequestPressBData); //NO PRESSURE A DATA
      }      
      if((RhtADCState>=6)&&(RhtADCBufPtr==0))
      {
        FlushMAX3100(RHTADC_CHAN); RHTADC_FSM_RESET(); AddTask(&RequestRHTADCData); //NO TEMP/RH DATA
      }      
      if((RadADCState>=6)&&(RadADCBufPtr==0))
      {
        FlushMAX3100(RADADC_CHAN); RADADC_FSM_RESET(); AddTask(&RequestRADADCData); //NO RADIATION DATA
      }            
      /*
      Schedule tasks to Parse any data that may be available on input ports
      Tasks scheduled to ensure that interrupts do not hang and become unresponsive
      If no data is available then task will simply check buffer and return successful
      */  
      AddTask(&PressAParse);
      AddTask(&PressBParse);
      AddTask(&RADADCParse);
      AddTask(&RHTADCParse);      
      AddTask(&SonicParse); 
      AddTask(&CupParse);
      AddTask(&VaneParse);
    }
       
    rtc_get(&lTime); 
    if(SEMAPHORE&0x08)
    {//NEW IRIDIUM TIME AVAILABLE
      if((lTime.secs>IridiumTime.secs+1)||(lTime.secs<IridiumTime.secs-1))
      {//RTC is out of sync with Iridium, so update RTC
//        lTime=IridiumTime;
//        rtc_set(&IridiumTime);
//        sampleCountdown=0;
//        DataBuffer.clockSync=1;
      }
      SEMAPHORE &= ~0x08;
    }
   
    /*** TX CONTROL ***/
    //time slot control for Sat Tx
    // ensures that each station only transmits on specific time slot.
    if(lTime.secs<=30)
    {
      byte startUpComms=0;
      switch(LTxTimeSlot)
      {
      case 0 : if((lTime.mins<=4)) startUpComms=1; break;//00:00-04:30
      case 1 : if((lTime.mins<=9)&&(lTime.mins>=5)) startUpComms=1; break;//05:00-09:30
      case 2 : if((lTime.mins<=14)&&(lTime.mins>=10)) startUpComms=1; break;//10:00-14:30
      case 3 : if((lTime.mins<=19)&&(lTime.mins>=15)) startUpComms=1; break;//15:00-19:30
      case 4 : if((lTime.mins<=24)&&(lTime.mins>=20)) startUpComms=1; break;//20:00-24:30
      case 5 : if((lTime.mins<=29)&&(lTime.mins>=25)) startUpComms=1; break;//25:00-29:30
      case 6 : if((lTime.mins<=34)&&(lTime.mins>=30)) startUpComms=1; break;//30:00-34:30
      case 7 : if((lTime.mins<=39)&&(lTime.mins>=35)) startUpComms=1; break;//35:00-39:30
      case 8 : if((lTime.mins<=44)&&(lTime.mins>=40)) startUpComms=1; break;//40:00-44:30
      case 9 : if((lTime.mins<=49)&&(lTime.mins>=45)) startUpComms=1; break;//45:00-49:30
      case 10 : if((lTime.mins<=54)&&(lTime.mins>=50)) startUpComms=1; break;//50:00-54:30
      case 11 : if((lTime.mins<=59)&&(lTime.mins>=55)) startUpComms=1; break;//55:00-59:30
      }
      if(startUpComms)
      {        
        if(!satCom_Operational) //If Sat Comms are not operational
        {
          satCom_Operational=1;
          if(SatComState==0)
          {
            PowerUp(13);
            txCountdown=0;
          }
        }        
        if(txCountdown==5)
        {
          SatComState=1; 
          SatComStateAttempt = 0;
          SatComTxPtr = 0;
          AddTask(&DTEAuth);  
        }
        txCountdown++;        
      }
    }      
    else if(lTime.secs>30)
    {
      byte shutDownComms=0;
      switch(LTxTimeSlot)
      {
      case 0 : if((lTime.mins>=4)) shutDownComms=1; break;//00:00-04:30
      case 1 : if((lTime.mins>=9)||(lTime.mins<5)) shutDownComms=1; break;//05:00-09:30
      case 2 : if((lTime.mins>=14)||(lTime.mins<10)) shutDownComms=1; break;//10:00-14:30
      case 3 : if((lTime.mins>=19)||(lTime.mins<15)) shutDownComms=1; break;//15:00-19:30
      case 4 : if((lTime.mins>=24)||(lTime.mins<20)) shutDownComms=1; break;//20:00-24:30
      case 5 : if((lTime.mins>=29)||(lTime.mins<25)) shutDownComms=1; break;//25:00-29:30
      case 6 : if((lTime.mins>=34)||(lTime.mins<30)) shutDownComms=1; break;//30:00-34:30
      case 7 : if((lTime.mins>=39)||(lTime.mins<35)) shutDownComms=1; break;//35:00-39:30
      case 8 : if((lTime.mins>=44)||(lTime.mins<40)) shutDownComms=1; break;//40:00-44:30
      case 9 : if((lTime.mins>=49)||(lTime.mins<45)) shutDownComms=1; break;//45:00-49:30
      case 10 : if((lTime.mins>=54)||(lTime.mins<50)) shutDownComms=1; break;//50:00-54:30
      case 11 : if((lTime.mins>=59)||(lTime.mins<55)) shutDownComms=1; break;//55:00-59:30
      }
      if(shutDownComms)
      {
        PowerDn(13);
        SatComState=0; //IDLE
        satCom_Operational=0;
      }
    }
   
    if(gndHeatFluxSC_Countdown>=GNDHFSC_PERIOD)
    {
      if(!gndHeatFlux_InCal)
      {
        RadADC_SC_State=1;
        gndHeatFlux_InCal=1;
      }
      else if(gndHeatFluxSC_Countdown>=GNDHFSC_PERIOD+GNDHFSC_ONTIME)
      {
        gndHeatFlux_InCal=0;
        RadADC_SC_State=2;
        gndHeatFluxSC_Countdown=0;
      }
    }
    
    break;
  default :
    break;
  }  
  return status;
}

byte rtc_write(byte cmd, byte data)
{//write to the RTC
  byte status=0;
  if(!(SEMAPHORE & 0x40))
  {
    SPCR |= (1<<CPOL0)|(1<<CPHA0);
    Set_SS_Lo(1);
    { 
      SPI_Clock(cmd);  
      SPI_Clock(data);
    }
    Set_SS_Hi();
    SPCR &= ~(1<<CPOL0)&~(1<<CPHA0); 
    status = 1;
  }
  return status;
}

byte rtc_read(byte cmd, byte* data)
{//read from the RTC
  byte status=0;
  if(!(SEMAPHORE & 0x40))
  {
    SPCR |= (1<<CPOL0)|(1<<CPHA0);
    Set_SS_Lo(1);
    { 
      SPI_Clock(cmd);  
      *data = SPI_Clock(0xFF);
    }
    Set_SS_Hi();
    SPCR &= ~(1<<CPOL0)&~(1<<CPHA0); 
    status = 1;
  }
  return status;
}

byte rtc_init()
{//initialise RTC and setup alarm interrupt
  byte status[9]={0,0,0,0,0,0,0,0,0};
  byte return_byte = 0;
  
  status[0]=rtc_write(ALM1_SEC_W, 0x80);
  status[1]=rtc_write(ALM1_MIN_W, 0x80);  
  status[2]=rtc_write(ALM1_HR_W, 0x80);
  status[3]=rtc_write(ALM1_DAT_W, 0x80);
  
  status[4]=rtc_write(ALM2_MIN_W, 0x80);
  status[5]=rtc_write(ALM2_DAT_W, 0x80);
  status[6]=rtc_write(ALM2_HR_W, 0x80);
  
  status[7]=rtc_write(CTRL_W, 0x05);
  
  status[8]=rtc_write(CTRL_STAT_W, 0x00);
  
  if((status[0]==1)&&(status[1]==1)&&(status[2]==1)&&(status[3]==1)&&(status[4]==1)&&(status[5]==1)&&(status[6]==1)&&(status[7]==1))
    return_byte=1;
  return return_byte;
}

byte rtc_check()
{//check alarm interrupt is set to go off every second
  byte status=0;
  byte return_byte[9] = {0,0,0,0,0,0,0,0,0};
  rtc_read(ALM1_SEC_R, &return_byte[0]);
  rtc_read(ALM1_MIN_R, &return_byte[1]);
  rtc_read(ALM1_HR_R, &return_byte[2]);
  rtc_read(ALM1_DAT_R, &return_byte[3]);

  rtc_read(ALM2_DAT_R, &return_byte[4]);
  rtc_read(ALM2_HR_R, &return_byte[5]);  
  rtc_read(ALM2_MIN_R, &return_byte[6]);    

  rtc_read(CTRL_R, &return_byte[7]);
  rtc_read(CTRL_STAT_R, &return_byte[8]);

  status = 1;
  
  return status;
}

byte rtc_clr_interrupt()
{//clear interrupt from RTC
  byte status = 0;
  status = rtc_write(CTRL_STAT_W, 0x00);
  return status;
}

byte rtc_set(_time* newTime)
{//set RTC time and date
  byte ss, mm, hh, dd, mt, yy;
  byte status[6] = {0,0,0,0,0,0};
  byte return_status = 0;
  
  ss=newTime->secs;
  mm=newTime->mins;
  hh=newTime->hr;
  dd=newTime->date;
  mt=newTime->mon;
  yy=newTime->year;
  
  status[0] = rtc_write_secs(ss);
  status[1] = rtc_write_mins(mm);
  status[2] = rtc_write_hrs(hh);
  status[3] = rtc_write_date(dd);
  status[4] = rtc_write_mon(mt);
  status[5] = rtc_write_yr(yy); 
  if((status[0]&&status[1]&&status[2]&&status[3]&&status[4]&&status[5])==1)
    return_status = 1;
  return return_status;
}

byte rtc_get(_time* time)
{//get RTC time and date
  byte status[6] = {0,0,0,0,0,0};
  byte return_status = 0;
  status[0] = rtc_read_secs(&(time->secs));
  status[1] = rtc_read_mins(&(time->mins));
  status[2] = rtc_read_hrs(&(time->hr));
  status[3] = rtc_read_date(&(time->date));
  status[4] = rtc_read_mon(&(time->mon));
  status[5] = rtc_read_yr(&(time->year));  
  if((status[0]&&status[1]&&status[2]&&status[3]&&status[4]&&status[5])==1)
    return_status = 1;
  return return_status;  
}

byte rtc_read_secs(byte* secs)
{
  byte status=0;
  byte return_byte[2]={0,0};  
  if(!(SEMAPHORE & 0x40))
  {
    SPCR |= (1<<CPOL0)|(1<<CPHA0);
    Set_SS_Lo(1);
    {
      return_byte[0] = SPI_Clock(READ_SEC);  
      return_byte[1] = SPI_Clock(0xFF);
    }
    Set_SS_Hi();
    SPCR &= ~(1<<CPOL0)&~(1<<CPHA0);
    *secs = ((0x07&(return_byte[1]>>4))*10)+(0x0F&return_byte[1]);
    status = 1;
  }
  return status;
}

byte rtc_read_mins(byte* mins)
{
  byte status=0;
  byte return_byte[2]={0,0};  
  if(!(SEMAPHORE & 0x40))
  {
    SPCR |= (1<<CPOL0)|(1<<CPHA0);
    Set_SS_Lo(1);
    {
      return_byte[0] = SPI_Clock(READ_MIN);  
      return_byte[1] = SPI_Clock(0xFF);
    }
    Set_SS_Hi();
    SPCR &= ~(1<<CPOL0)&~(1<<CPHA0);
    *mins = ((0x07&(return_byte[1]>>4))*10)+(0x0F&return_byte[1]);
    status = 1;
  }
  return status;    
}

byte rtc_read_hrs(byte* hrs)
{
  byte status=0;
  byte return_byte[2]={0,0};  
  if(!(SEMAPHORE & 0x40))
  {
    SPCR |= (1<<CPOL0)|(1<<CPHA0);
    Set_SS_Lo(1);
    {
      return_byte[0] = SPI_Clock(READ_HRS);  
      return_byte[1] = SPI_Clock(0xFF);
    }
    Set_SS_Hi();
    SPCR &= ~(1<<CPOL0)&~(1<<CPHA0);
    *hrs = ((0x03&(return_byte[1]>>4))*10);
    *hrs += (0x0F&return_byte[1]);
    status = 1;
  }
  return status;   
}

byte rtc_read_date(byte* date)
{
  byte status=0;
  byte return_byte[2]={0,0};  
  if(!(SEMAPHORE & 0x40))
  {
    SPCR |= (1<<CPOL0)|(1<<CPHA0);
    Set_SS_Lo(1);
    {
      return_byte[0] = SPI_Clock(READ_DAT);  
      return_byte[1] = SPI_Clock(0xFF);
    }
    Set_SS_Hi();
    SPCR &= ~(1<<CPOL0)&~(1<<CPHA0);
    *date = ((0x03&(return_byte[1]>>4))*10)+(0x0F&return_byte[1]);
    status = 1;
  }
  return status;    
}

byte rtc_read_mon(byte* mon)
{
  byte status=0;
  byte return_byte[2]={0,0};  
  if(!(SEMAPHORE & 0x40))
  {
    SPCR |= (1<<CPOL0)|(1<<CPHA0);
    Set_SS_Lo(1);
    {
      return_byte[0] = SPI_Clock(READ_MON);  
      return_byte[1] = SPI_Clock(0xFF);
    }
    Set_SS_Hi();
    SPCR &= ~(1<<CPOL0)&~(1<<CPHA0);
    *mon = ((0x01&(return_byte[1]>>4))*10)+(0x0F&return_byte[1]);    
    status = 1;
  }
  return status;    
}

byte rtc_read_yr(byte* year)
{
  byte status=0;
  byte return_byte[2]={0,0};  
  if(!(SEMAPHORE & 0x40))
  {
    SPCR |= (1<<CPOL0)|(1<<CPHA0);
    Set_SS_Lo(1);
    {
      return_byte[0] = SPI_Clock(READ_YR);  
      return_byte[1] = SPI_Clock(0xFF);      
    }
    Set_SS_Hi();
    SPCR &= ~(1<<CPOL0)&~(1<<CPHA0);    
    *year = ((0x0F&(return_byte[1]>>4))*10)+(0x0F&return_byte[1]);
    status = 1;
  }
  return status;    
}

byte rtc_write_secs(byte secs)
{
  byte status=0;
  byte data;
    
  if(!(SEMAPHORE & 0x40))
  {
    data =  ((secs/10)<<4)+(secs-(10*(secs/10)));
    SPCR |= (1<<CPOL0)|(1<<CPHA0);
    Set_SS_Lo(1);
    {
       SPI_Clock(WRITE_SEC);  
       SPI_Clock(data);
    }
    Set_SS_Hi();
    SPCR &= ~(1<<CPOL0)&~(1<<CPHA0); 
    status = 1;
  }
  return status;    
}

byte rtc_write_mins(byte mins)
{
  byte status=0;
  byte data=0;
  if(!(SEMAPHORE & 0x40))
  {
    data = ((mins/10)<<4);
    data += (mins-(10*(mins/10)));
    SPCR |= (1<<CPOL0)|(1<<CPHA0);
    Set_SS_Lo(1);
    {
      SPI_Clock(WRITE_MIN);  
      SPI_Clock(data);
    }
    Set_SS_Hi();
    SPCR &= ~(1<<CPOL0)&~(1<<CPHA0); 
    status = 1;
  }
  return status;    
}

byte rtc_write_hrs(byte hrs)
{
  byte status=0;
  byte data =0;
  if(!(SEMAPHORE & 0x40))
  {
    data = ((hrs/10)<<4);
    data += hrs - (10*(hrs/10));
    SPCR |= (1<<CPOL0)|(1<<CPHA0);    
    Set_SS_Lo(1);
    {
      SPI_Clock(WRITE_HRS);  
      SPI_Clock(data);
    }
    Set_SS_Hi();
    SPCR &= ~(1<<CPOL0)&~(1<<CPHA0); 
    status = 1;
  }
  return status;    
}

byte rtc_write_date(byte date)
{
  byte status=0;
  byte data = 0;
  if(!(SEMAPHORE & 0x40))
  {
    data = ((date/10)<<4);
    data += (date-(10*(date/10)));
    SPCR |= (1<<CPOL0)|(1<<CPHA0);       
    Set_SS_Lo(1);
    {
      SPI_Clock(WRITE_DAT);  
      SPI_Clock(data);
    }
    Set_SS_Hi();
    SPCR &= ~(1<<CPOL0)&~(1<<CPHA0);     
    status = 1;
  }
  return status;    
}

byte rtc_write_mon(byte mon)
{
  byte status=0;
  byte data=0;

  if(!(SEMAPHORE & 0x40))
  {
    data = ((mon/10)<<4);
    data += (mon-(10*(mon/10)));
    SPCR |= (1<<CPOL0)|(1<<CPHA0);    
    Set_SS_Lo(1);
    {      
      SPI_Clock(WRITE_MON);  
      SPI_Clock(data);
    }
    Set_SS_Hi();
    SPCR &= ~(1<<CPOL0)&~(1<<CPHA0);       
    status = 1;
  }
  return status;    
}

byte rtc_write_yr(byte year)
{
  byte status=0;
  byte data=0;
  if(!(SEMAPHORE & 0x40))
  {
    data = ((year/10)<<4);
    data += (year-(10*(year/10)));
    SPCR |= (1<<CPOL0)|(1<<CPHA0);      
    Set_SS_Lo(1);
    {
      SPI_Clock(WRITE_YR);  
      SPI_Clock(data);
    }
    Set_SS_Hi();
    SPCR &= ~(1<<CPOL0)&~(1<<CPHA0);    
    status = 1;
  }
  return status;    
}

