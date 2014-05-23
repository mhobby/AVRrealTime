#define _BAUD_USART_9600 51
#define _BAUD_USART_19200 25
#define _BAUD_USART_38400 12

#define USR_DATA 1
#define USR_TIME 2
#define USR_SYNC 3

void USART_Init(unsigned int baud);
void USART_Transmit(byte data);
byte USART_Receive();
byte USART_Flush();

byte PCParse();

byte LTxData(byte dataRqst);
