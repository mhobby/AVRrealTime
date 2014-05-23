#define CLEAR 0
#define CALL 1
#define ADD 2

#define MAX3100_POWER_DELAY 1000

#define SCHEDULER_SIZE 32
#define MAX_INTERRUPT_TASKS 10

#define WD_TIMEOUT_9_1 0x21
#define WD_TIMEOUT_4_6 0x20
#define WD_TIMEOUT_2_3 0x07

byte watchdog();
void __watchdog_start();                   
void __watchdog_stop();

void ClearScheduler();
void CallTask();

__monitor void AddTask(byte (*ptr2func)());
void AddInterruptTask(byte (*ptr2func)());
__monitor void RemoveTask();

__monitor void ClearDataBuffer();
void InitSys();
byte UpdateSysPage();
byte UploadDataBuffer();

void InitPwrOnTime();
void InitPwrMgmt();
__monitor void PowerUp(byte chan);
__monitor void PowerDn(byte chan);

void ToggleBuzzer();

void EventClockSetup();
void StartEventClock();

void InitStateMachines();
void ArmSystem();

void EEPROM_write(unsigned int uiAddress, unsigned char ucData);
unsigned char EEPROM_read(unsigned int uiAddress);
