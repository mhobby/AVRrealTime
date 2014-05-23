#define STAT_W 1
#define STAT_R 5
#define WEL 6
#define PROG 2
#define READ 3
#define ERASE 32

byte writeStatusFlash(byte dataOut);
byte programFlash(unsigned long addr, byte* dataIn, unsigned int byteNo);
byte readFlash(unsigned long addr, byte* dataOut, unsigned int byteNo);
byte eraseFlash(unsigned long addr);
byte checkEpe(byte* epeError);

byte writeFlash(unsigned long addr, byte* data, unsigned int bytes);
