#include "lib.h"

void ADCSetup()
{
  ADCSRA |= (1<<ADEN);
  ADCSRA |= (1<<ADPS2) | (1<<ADPS1);    //set ADC clock to 1/64 of system clock
  ADCSRA |= (1<<ADIE);
  ADMUX = (1<<REFS0) | (1<<REFS1); //set ADC to be with reference to internal 2.56V ref
  SysADCChan=2;
}

byte RequestADCData()
{  
  byte status=1;
  ADMUX = (ADMUX&0xC0) | SysADCChan; //set ADC channel
  ADCSRA |= (1<<ADSC); //start conversion  
  return status;
}

byte ADCParse()
{
  byte status=1;
  byte val;
  val = ADCL;  
  switch(SysADCChan)
  {
  case 0:
    SysADCBuf[0]=(ADCH<<8)+val; //5V_REG
    SysADCChan=1;
    AddTask(&RequestADCData);
    break;
  case 1: //12V_REG
    SysADCBuf[1]=(ADCH<<8)+val;
    SysADCChan=2;
    AddTask(&RequestADCData);   
    break;
  case 2: //BATTERY_VOLTAGE
    SysADCBuf[2]=(ADCH<<8)+val;
    SysADCChan=3;
    AddTask(&RequestADCData);
    break;
  case 3: //GROUND PROBE 1
    SysADCBuf[3]=(ADCH<<8)+val;
    SysADCChan=4;
    AddTask(&RequestADCData);
    break;
  case 4: //GROUND PROBE 2
    SysADCBuf[4]=(ADCH<<8)+val;
    SysADCChan=5;
    AddTask(&RequestADCData);
    break;    
  case 5: //Thermistor
    SysADCBuf[5]=(ADCH<<8)+val;
    SysADCChan=6;
  }  
  return status;  
}

