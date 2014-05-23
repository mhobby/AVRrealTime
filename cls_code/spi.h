#define _SPI_DIS 0xFF //XXX1 1X11
#define _SPI_CH0 0xE4 //XXX0 0X00
#define _SPI_CH1 0xF4 //XXX1 0X00

#define _SPI_CH2 0xEC //XXX0 1X00
#define _SPI_CH3 0xFC //XXX1 1X00
#define _SPI_CH4 0xE6 //XXX0 0X10
#define _SPI_CH5 0xF6 //XXX1 0X10
#define _SPI_CH6 0xEA //XXX0 1X10
#define _SPI_CH7 0xFE //XXX1 1X10
#define _SPI_CH8 0xE1 //XXX0 0X01
#define _SPI_CH9 0xF1 //XXX1 0X01
#define _SPI_CH10 0xE9 //XXX0 1X01

#define	_XTAL18432_BAUD_MAX3100_4800 0x0B
#define	_XTAL18432_BAUD_MAX3100_9600 0x0A
#define	_XTAL18432_BAUD_MAX3100_19200 0x09
#define	_XTAL18432_BAUD_MAX3100_38400 0x08

#define	_XTAL3686_BAUD_MAX3100_2400 0x0D
#define	_XTAL3686_BAUD_MAX3100_4800 0x0C
#define	_XTAL3686_BAUD_MAX3100_9600 0x0B
#define	_XTAL3686_BAUD_MAX3100_19200 0x0A
#define	_XTAL3686_BAUD_MAX3100_38400 0x09


#define SPI_HOLD 0000

void SPI_Enable();
void Set_SS_Lo(byte chan);
void Set_SS_Hi();

inline byte SPI_Clock(byte dataOUT)
{
  byte dataIN;
  SPDR = dataOUT;
  while(!(SPSR & (1<<SPIF0)));
  dataIN = SPDR;
  return dataIN;
}

byte ConfigureMAX3100(byte chan, byte baud);
void FlushMAX3100(byte chan);
byte WriteMAX3100(byte chan, byte dataOUT, byte* dataIN, byte* dataRx);
byte ReadMAX3100(byte chan, byte* dataIN, byte* moreData, byte* frameErr, byte* syncErr);
void ClearMAX3100(byte chan);
