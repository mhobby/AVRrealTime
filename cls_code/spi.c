#include "lib.h"
void SPI_Enable()
{
  //set SPI to be master at basic speed of 500kHz   
  SPCR |= (1<<SPE0)|(1<<MSTR0);//|(1<<SPR00);   
  //set MOSI and SCLK line as output
  DDRB |= (1<<DDB5) | (1<<DDB7);
  //set SS IO lines to be outputs on processor
  DDRB |= (1<<DDB0) | (1<<DDB1) | (1<<DDB3) | (1<<DDB4);
  //set SS lines all high (00011011)
  PORTB |= _SPI_DIS;
}

void Set_SS_Lo(byte chan)
{
  switch(chan)
  {
  case 0 : //DATA STORAGE
    PORTB &= _SPI_CH0;
    break;
  case 1 : //RTC
    PORTB &= _SPI_CH1;
    break;
  case 2 : 
    PORTB &= _SPI_CH2;
    break;
  case 3 : //IRIDIUM
    PORTB &= _SPI_CH3;
    break;
  case 4 : //PRESSA
    PORTB &= _SPI_CH4;
    break;
  case 5 : //PRESSB
    PORTB &= _SPI_CH5;
    break;
  case 6 : //RhT ADC
    PORTB &= _SPI_CH6;
    break;    
  case 7 : //RAD ADC
    PORTB &= _SPI_CH7;
    break; 
  case 8 : //SONIC
    PORTB &= _SPI_CH8;
    break;  
  case 9 : //CUP
    PORTB &= _SPI_CH9;
    break;  
  case 10 : //VANE
    PORTB &= _SPI_CH10;
    break;      
  default :
    break;
  }
  SEMAPHORE |= 0x40;
}

void Set_SS_Hi()
{
  PORTB |= _SPI_DIS;
  SEMAPHORE &= 0xBF;
}

byte ConfigureMAX3100(byte chan, byte baud)
{
  byte status=0;
  byte return_byte[4]={0,0,0,0};
  __delay_cycles(8000); //delay CLS proc to give enough time for MAX3100 to power up
  if(!(SEMAPHORE & 0x40))
  {
    Set_SS_Lo(chan);
    {
      return_byte[0]=SPI_Clock(0xC4); //set interrupts to occur during Rx activity in ACTIVE and SHUTDOWN state
      return_byte[1]=SPI_Clock(baud);
    }
    Set_SS_Hi();
        
    Set_SS_Lo(chan);
    {
      return_byte[2]=SPI_Clock(0x40);
      return_byte[3]=SPI_Clock(0x00);      
    }
    Set_SS_Hi();
    if(return_byte[0]!=0x00) status = 1;
  }
  return status;
}

void FlushMAX3100(byte chan)
{
  byte buf;
  byte frameErr;
  byte moreData;  
  byte syncErr;  
  byte status;
  

  do
  {
    moreData = 0;
    frameErr = 0;
    status = ReadMAX3100(chan,&buf, &moreData, &frameErr, &syncErr);
  }while((moreData==1)&&(status==1));
}

byte WriteMAX3100(byte chan, byte dataOUT, byte* dataIN, byte* dataRx)
{
  byte status;
  byte return_byte[2]={0,0};
  
  if(!(SEMAPHORE & 0x40))
  {   
    Set_SS_Lo(chan);
    {
      return_byte[0] = SPI_Clock(0x80);  //MAX3100 write data command
      return_byte[1] = SPI_Clock(dataOUT);
    }
    Set_SS_Hi();
  }
  if(return_byte[0]&0x40) status = 1;  
  else status = 0; //Tx buffer was full so reschedule
  if(return_byte[0]&0x80) *dataRx=1;   
  else *dataRx=0;

  *dataIN = return_byte[1]; 
  return status;
}

byte ReadMAX3100(byte chan, byte* dataIN, byte* RxBufState, byte* frameErr, byte* syncErr)
{
  byte status=0;
  byte return_byte[4]={0,0,0,0};
  
  if(!(SEMAPHORE & 0x40))
  {
    status = 1;
    Set_SS_Lo(chan);
    {
      //collect data from Rx buffer
      return_byte[0] = SPI_Clock(0x00);  //MAX3100 read data command
      return_byte[1] = SPI_Clock(0x00);
    }
    Set_SS_Hi();
    
    if((return_byte[0]==0xFF)&&(return_byte[1]==0xFF))
    {//check for invalid response => expansion card not connected
       status = 1;  //output a successful response so as to not reschedule
    }
    else if(return_byte[0]&0x80) //check MAX3100 return control byte
    {//data was in the Rx buffer      
      *syncErr = 0;
      *dataIN = return_byte[1];
      
      //check to see if FIFO is now empty without emptying it
      Set_SS_Lo(chan);
      {
        //collect data from Rx buffer
        return_byte[2] = SPI_Clock(0x40);  //MAX3100 read config command
        return_byte[3] = SPI_Clock(0x00);
      }
      Set_SS_Hi();   
      
      if(return_byte[2]&0x80)
      {//signify FIFO not empty
        *RxBufState = 1;
      }
      
      //check for any Frame Errors
      if(return_byte[0]&0x04)
      {//frame error occurred...
        *frameErr = 1;
      }
      else *frameErr = 0;
    }
    else *syncErr = 1;
  }  
  return status;
}

void ClearMAX3100(byte chan)
{
  byte data;
  byte bytes_read=0;
  byte moreData=0;
  byte frameErr;
  byte syncErr;
  byte status=1;
  
  do
  {
    status = ReadMAX3100(chan,&data,&moreData,&frameErr,&syncErr);
    bytes_read++;
  }while((moreData)&&(bytes_read<8)&&(status==1));
}
