#include <stdlib.h>
#include <stdio.h>
#include <inavr.h> 
#ifndef ENABLE_BIT_DEFINITIONS
#define ENABLE_BIT_DEFINITIONS
// Enable the bit definitions in the iom644.h file
#endif
#include <iom644p.h>  

#ifndef DATA_TYPES_DEF
  typedef unsigned char byte;

//define SYSTEM STATES
#define INIT 1
#define SYNC 2
#define LOOP 4
#define HALT 8

//for ARM STATE only IOP flag is replaced by CANCEL flag...
#define CANCEL_PROGRAM 128 //set when all programs are cancelled

#define DEBUG
//#define NOBEEP

#define AUX_SIZE 17

//time data
  typedef struct
  {	
    byte date;
    byte mon;
    byte year;
    byte hr;  
    byte mins;
    byte secs;    
  }_time;  
  
//system data
  typedef struct
  {
    byte station_id; //0
    _time bootTime; //1
    byte bootDataStartAddr[4]; //7
    byte bootDataStopAddr[4]; //11
    byte ctrlA[4]; //15
    int bytesTx; //19
    int pressACals[4]; //21
    int pressBCals[4]; //29
    byte ctrlB[2]; //37
    byte storageCycle; //39
    byte rsvd[2]; //40
  }_sys;

  //MISSION_DATA packets
  typedef struct
  {	
    byte station_id; //1 byte
    _time rtc_time; //6 bytes
   
    int pressA_D1; //2 bytes
    int pressA_D2; //2 bytes
    int pressB_D1; //2 bytes
    int pressB_D2; //2 bytes    
    
    int rht_adc[6]; //12 bytes
    int rad_adc[8]; //16 bytes
    
    int sonicSpdSum ; // 2 bytes
    int sonicSpdMax ; // 2 bytes
    signed long sonicUSum; // 4 bytes    
    signed long sonicVSum; // 4 bytes
    unsigned long sonicSpd2Sum; // 4 bytes
    long long sonicSpd3Sum; // 8 bytes
    byte sonicSampleNo; // 1 byte
                          // 25 bytes
    
    int cupSpdSum ; // 2 bytes
    int cupSpdMax ; // 2 bytes
    signed long cupVaneUSum; // 4 bytes    
    signed long cupVaneVSum; // 4 bytes
    unsigned long cupSpd2Sum; // 4 bytes
    long long cupSpd3Sum; // 8 bytes
    unsigned int cupVaneSampleNo; // 2 byte
                          // 26 bytes
    
    unsigned int V_5Vreg;  //2 bytes
    unsigned int V_12Vreg;  //2 bytes
    unsigned int V_Batt;  //2 bytes
    unsigned int SysTemp;  //2 bytes
    
    byte clockSync; //1 byte
    byte GndHeatFluxSC; //1 byte
    unsigned int GroundProbe1; //2 bytes
    unsigned int GroundProbe2; //2 bytes    
    byte pageFlag;
    unsigned int crc;
    byte aux[AUX_SIZE];
  }_data; //128 bytes
   
  typedef byte(*function_ptr)();

  #define DATA_TYPES_DEF
#endif

#ifndef SYS_DEF
  #include "system.h"
  #define SYS_DEF
#endif
  
#ifndef LOOKUP_DEF
  #include "lookupTables.h"
  #define LOOKUP_DEF
#endif  

#ifndef SPI_DEF
  #include "spi.h"
  #define SPI_DEF
#endif
  
#ifndef FLASH_DEF
  #include "flash.h"
  #define FLASH_DEF
#endif  
  
#ifndef SATCOM_DEF
  #include "satCom.h"
  #define SATCOM_DEF
#endif  
  
#ifndef RTC_DEF
  #include "rtc.h"
  #define RTC_DEF
#endif

#ifndef RAD_DEF
  #include "rad.h"
  #define RAD_DEF
#endif  
  
#ifndef ADC_DEF
  #include "adc.h"
  #define ADC_DEF
#endif  
  
#ifndef RHT_DEF
  #include "rht.h"
  #define RHT_DEF
#endif  
  
#ifndef PRESS_DEF
  #include "press.h"
  #define PRESS_DEF
#endif 
  
#ifndef SONIC_DEF
  #include "sonic.h"
  #define SONIC_DEF
#endif 
  
#ifndef CUP_DEF
  #include "cup.h"
  #define CUP_DEF
#endif  
  
#ifndef VANE_DEF
  #include "vane.h"
  #define VANE_DEF
#endif   
  
#ifndef USART_DEF
  #include "usart.h"
  #define USART_DEF
#endif  
 
  enum sys_ptr { SYS0, SYS1, SYS2, SYS3, SYS4, SYS5};

//eeprom definitions
__eeprom byte eStationID @0x00;
__eeprom enum sys_ptr eSysPtr @0x01;
__eeprom _sys eSysBuffer[5] @0x02;
__eeprom unsigned long eDataPtr @0xFE;
__eeprom unsigned long eTxPtr @0x102;
__eeprom byte eTxTimeSlot @0x106;

//system
__no_init byte SystemState @0x100;
__no_init byte SyncState @0x101;
__no_init byte WatchdogTimeout @0x102;
__no_init enum sys_ptr LSysPtr @0x103;
__no_init _sys LSysBuffer @ 0x104;

//POWER CONTROL
__no_init unsigned int PwrState @0x12E;
__no_init unsigned int PwrOnTime[16] @0x130; //MAX3100 expected operation time

//DATA STORAGE
__no_init unsigned long LDataPtr @0x150;
__no_init _data DataBuffer @0x154;   
__no_init unsigned long LTxPtr @0x1D4;
__no_init byte TxBuffer[256] @0x1D8;

__no_init byte SatComState @0x2D8;
__no_init byte SatComStateAttempt @0x2D9;
__no_init byte SatComTxPtr @0x2DA;
__no_init byte SatComRxBuf[32] @0x2DB;
__no_init byte SatComTempBuf @0x2FB;
__no_init byte SatComRxPtr @0x2FC;
__no_init byte SatComPagePtr @0x2FD;

__no_init byte LTxTimeSlot @0x2FE;

#define PRESS_BUF_SIZE 4
#define RHTADC_BUF_SIZE 18
#define RADADC_BUF_SIZE 18
#define SONIC_BUF_SIZE 18
#define CUP_BUF_SIZE 15
#define VANE_BUF_SIZE 15



#define IRIDIUM_CHAN 3
#define PRESSA_CHAN 4
#define PRESSB_CHAN 5
#define RHTADC_CHAN 6
#define RADADC_CHAN 7
#define SONIC_CHAN 8
#define CUP_CHAN 9
#define VANE_CHAN 10

//RhT
__no_init byte RhtADCBuf[RHTADC_BUF_SIZE] @0x2FF;
__no_init byte RhtADCBufPtr @0x311;
__no_init byte RhtADCState @0x312; //BIT 8: ERROR STATE, BIT 7: STATE MACHINE see docs
inline RHTADC_FSM_RESET()
{
  RhtADCBufPtr = 0;
  RhtADCState = 0;
}

//RAD
__no_init byte RadADCBuf[RADADC_BUF_SIZE] @0x313;
__no_init byte RadADCBufPtr @0x325;
__no_init byte RadADCState @0x326; //BIT 8: ERROR STATE, 
                                   //   BIT 7: FLUX PLATE HEATER ON
                                   //   BIT 6: STATE MACHINE see docs
inline RADADC_FSM_RESET()
{
  RadADCBufPtr = 0;
  RadADCState &= ~0x3F;
}
__no_init byte RadADC_SC_State @0x327;

//ADC
__no_init unsigned int SysADCBuf[6] @0x328;
__no_init byte SysADCChan @0x334;
inline SYSADC_FSM_RESET()
{
  SysADCBuf[0]=0;
  SysADCBuf[1]=0;
  SysADCBuf[2]=0;
  SysADCBuf[3]=0;
  SysADCBuf[4]=0;
  SysADCBuf[5]=0;
  SysADCChan=0;
}
//PRESS
__no_init byte PressABuf[PRESS_BUF_SIZE] @0x335;
__no_init byte PressABufPtr @0x339;
__no_init byte PressAState @0x33A;
  //BIT 8: ERROR STATE, 
  //BIT 7: UN CALIBRATED STATE, 
  //BIT 0..6: STATE MACHINE see docs
__no_init byte PressACal[8] @0x33B;

__no_init byte PressBBuf[PRESS_BUF_SIZE] @0x343;
__no_init byte PressBBufPtr @0x347;
__no_init byte PressBState @0x348;
  //BIT 8: ERROR STATE, 
  //BIT 7: UN CALIBRATED STATE, 
  //BIT 0..6: STATE MACHINE see docs
__no_init byte PressBCal[8] @0x349;

inline PRESS_A_FSM_RESET()
{
  PressABufPtr = 0;
  PressAState &= ~0x1F;
}
inline PRESS_B_FSM_RESET()
{
  PressBBufPtr = 0;
  PressBState &= ~0x1F;  
}

//Sonic
__no_init byte SonicBuf[SONIC_BUF_SIZE] @0x351;
__no_init byte SonicBufPtr @0x363;
__no_init int SonicSpd @0x364;
__no_init int SonicSpdMax @0x366;
__no_init int SonicSpdSum @0x368;
__no_init long SonicUSum @0x36A;
__no_init long SonicVSum @0x36E;
__no_init unsigned long SonicSpdSkwSum @0x372;
__no_init unsigned long SonicSpdVarSum @0x376;
__no_init byte SonicSamples @0x37A;
__no_init byte SonicState @0x37B; //BIT 8: ERROR STATE, BIT 7: STATE MACHINE see docs

inline SONIC_FSM_RESET()
{
  SonicBufPtr = 0;
  SonicState = 0;
}

//Cup
__no_init byte CupBuf[CUP_BUF_SIZE] @0x37C;
__no_init byte CupBufPtr @0x38B;
__no_init int CupSpd @0x38C;
__no_init int CupSpdMax @0x38E;
__no_init int CupSpdSum @0x390;
__no_init unsigned long CupSpdSkwSum @0x392;
__no_init unsigned long CupSpdVarSum @0x396;
__no_init int CupSamples @0x39A;
__no_init byte CupState @0x39C;
inline CUP_FSM_RESET()
{
  CupBufPtr = 0;
  CupState = 0;
}

//Vane
__no_init byte VaneBuf[VANE_BUF_SIZE] @0x39D;
__no_init byte VaneBufPtr @0x3AC;
__no_init int VaneDir @0x3AD;
__no_init byte VaneState @0x3AF;
inline VANE_FSM_RESET()
{
  VaneBufPtr = 0;
  VaneState = 0;
}
__no_init long CupVaneUSum @0x3B0;
__no_init long CupVaneVSum @0x3B4;


//RTOS
__no_init _time IridiumTime @0x875;
__no_init byte TempBufCtrl @0x87B;
__no_init byte starCount @0x87C;
__no_init byte WATCHDOG_COUNTER @0x87D;
__no_init volatile function_ptr SCHEDULER[2*SCHEDULER_SIZE] @0x87E; 
__no_init volatile signed char SCHP @0x8FE;
__no_init byte SEMAPHORE @0x8FF;
/*
SEMAPHORE - 
  0000 0001 : SCHEDULER OVERRUN
  0000 0010 : 
  0000 0100 : 
  0000 1000 : 
  0001 0000 : CARRIER LOSS MID PAGE TX
  0010 0000 : WATCHDOG BUSY
  0100 0000 : SPI BUS BUSY
  1000 0000 : DATA BUFFER FULL
*/


#define IDLE 0x80
#define RESYNC 0x40

#define STX 0
#define TX_INIT 1
#define ARG1_INIT 2
#define ARG2_INIT 3
#define ETX 4



