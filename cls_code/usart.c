#include "lib.h"

void USART_Init(unsigned int baud)
{
  /* Set baud rate */
  UBRR0 = baud;
  /* Enable receiver and transmitter */
  UCSR0B = (1<<RXEN0)|(1<<TXEN0)|(1<<RXCIE0);
  /* Set frame format: 8data, 1stop bit */
  UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);  
}

void USART_Transmit(byte data)
{
  while ( !( UCSR0A & (1<<UDRE0)));
  UDR0 = data;
}

byte USART_Receive(byte* data)
{
  byte status = 1;
  if(!(UCSR0A & (1<<RXC0))) status = 0;
  *data = UDR0;
  return status;
}

byte USART_Flush()
{
  byte dummy;
  while ( UCSR0A & (1<<RXC0) ) dummy = UDR0;
  return dummy;
}

void TxHalt()
{
  SystemState=HALT;
  SMCR &= ~(1<<SE); //disable CLS proc to sleep
  USART_Transmit('H');
  USART_Transmit('A');
  USART_Transmit('L');
  USART_Transmit('T');
  USART_Transmit(0x0D);
  USART_Transmit(0x0A);    
  USART_Flush();   //clear UART buffer 
}

void TxOK()
{
  USART_Transmit('O');
  USART_Transmit('K');
  USART_Transmit(0x0D);
  USART_Transmit(0x0A);    
  USART_Flush();   //clear UART buffer   
}

byte PCParse()
{
  static byte PC_STATE;
  byte status=0;
  static byte dataRqst;
  byte buf; 
  static byte usrRqst=0;
  static byte timeBuf[12];
  static byte timeBufPtr=0;
  _time t;

  status = USART_Receive(&buf);
  
  if(status)
  {
    if((0x1F&SystemState)==LOOP)
    {
      if(buf==0x2A)
      {
        starCount++;
        if(starCount==3) { TxHalt(); starCount = 0; }
      }
      UCSR0B |= (1<<RXCIE0); //reenable UART interrupt     
    }
    else if(SystemState==HALT)
    {
      switch(PC_STATE)    
      {
      case 0 : 
        switch(buf)
        {
        case 0x3F : PC_STATE=1; break;
        case 0x2A : starCount++; if(starCount==3) { TxHalt(); starCount = 0; } break;
        default : PC_STATE=0; starCount=0; break;
        }
        break;
      case 1 : 
        switch(buf)
        {
        case 'A': 
          usrRqst=USR_DATA; 
          PC_STATE = 2;
          break;
        case 'T':        
          timeBufPtr=0;
          usrRqst=USR_TIME;
          PC_STATE = 2; 
          break;
        case 'I':
          usrRqst=USR_SYNC;
          PC_STATE = 2;
          break;
        default : 
          PC_STATE=0; 
          break;
        }
        break;
      case 2 : 
        switch(usrRqst)
        {
        case USR_DATA : 
          dataRqst=buf-0x30; 
          PC_STATE = 3; 
          break;
        case USR_TIME : 
          timeBuf[timeBufPtr++]=buf; 
          if(timeBufPtr>=12) PC_STATE = 3;
          break;
        case USR_SYNC:
          PC_STATE = 3;
          break;
        }
        break;        
      case 3 : if(buf==0x0D) PC_STATE = 4; else PC_STATE=0; break;    
      case 4 : 
        if(buf==0x0A)
        {
          switch(usrRqst)
          {
          case USR_DATA :
            LTxData(dataRqst);
            for(byte i=0; i<5; i++) USART_Transmit(0x2A);    
            PC_STATE = 0;
            break;
          case USR_TIME :            
            t.date=(10*(timeBuf[0]-0x30))+(timeBuf[1]-0x30);
            t.mon=(10*(timeBuf[2]-0x30))+(timeBuf[3]-0x30);
            t.year=(10*(timeBuf[4]-0x30))+(timeBuf[5]-0x30);
            t.hr=(10*(timeBuf[6]-0x30))+(timeBuf[7]-0x30);
            t.mins=(10*(timeBuf[8]-0x30))+(timeBuf[9]-0x30);
            t.secs=(10*(timeBuf[10]-0x30))+(timeBuf[11]-0x30);
            if(rtc_set(&t)) TxOK();
            break;
          case USR_SYNC :                      
            //power up modem
            PowerUp(13);
            AddTask(&DTEAuth);    
            SatComState=1;
          }
        }
        PC_STATE=0;
        break;
    }
  }
}
return status;
}

byte LTxData(byte dataRqst)
{
  unsigned long addr;
  byte status;
  status = 0;
  //check to see if dataRqst is an actual number
  if((dataRqst<1)||(dataRqst>9)) addr = 0x100; 
  //if it is a number check to ensure that a request has not been 
  // made for data that does not exist. If so, then force request to be for all 
  // available data
  else if(LDataPtr<=((dataRqst*128)+0x100)) addr=0x100;
  // if request is sensible, update start address; addr accordingly
  else addr = LDataPtr-(dataRqst<<7);
  
  //Read SysPage
  status = readFlash(0, TxBuffer, 256);
  if(status)
    for(int i=0; i<256; i++) USART_Transmit(TxBuffer[i]);
  ToggleBuzzer();
  
  while(addr<(LDataPtr))
  {
    status = readFlash(addr, TxBuffer, 256);
    if(status)
    {
      for(int i=0; i<256; i++)
      {
        USART_Transmit(TxBuffer[i]);
      }
      addr=addr+256;
    }
    ToggleBuzzer();    
  }
  PowerDn(15);
  return status;
}
