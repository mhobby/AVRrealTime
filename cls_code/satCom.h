byte DTEAuth();
byte CallInit();
byte SysPageTx();
byte DataPageTx();
byte SatExit();
byte SatCSQChk();
byte HangUp();
byte Sat_MSSTM();
byte SatComParse();
void ClearSatRxBuf();


#define DTE_ATTEMPT 3
#define PROTOCOL_ATTEMPT 3
#define CALL_ATTEMPT 10
#define SYSPAGE_ATTEMPT 3
#define DATAPAGE_ATTEMPT 3
#define EXIT_ATTEMPT 3
#define HANGUP_ATTEMPT 3
#define MSSTM_ATTEMPT 3

#define ACK_WAIT 2
