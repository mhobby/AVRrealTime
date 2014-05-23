#define ALM1_SEC_R 0x07
#define ALM1_MIN_R 0x08
#define ALM1_HR_R 0x09
#define ALM1_DAT_R 0x0A

#define ALM2_MIN_R 0x0B
#define ALM2_HR_R 0x0C
#define ALM2_DAT_R 0x0D

#define CTRL_R 0x0E
#define CTRL_STAT_R 0x0F

#define ALM1_SEC_W 0x87
#define ALM1_MIN_W 0x88
#define ALM1_HR_W 0x89
#define ALM1_DAT_W 0x8A

#define ALM2_MIN_W 0x8B
#define ALM2_HR_W 0x8C
#define ALM2_DAT_W 0x8D

#define CTRL_W 0x8E
#define CTRL_STAT_W 0x8F

#define READ_SEC 0x00
#define READ_MIN 0x01
#define READ_HRS 0x02
#define READ_DAT 0x04
#define READ_MON 0x05
#define READ_YR 0x06

#define WRITE_SEC 0x80
#define WRITE_MIN 0x81
#define WRITE_HRS 0x82
#define WRITE_DAT 0x84
#define WRITE_MON 0x85
#define WRITE_YR 0x86

byte rtc_alarm();

byte rtc_write(byte cmd, byte data);
byte rtc_read(byte cmd, byte* data);

byte rtc_init();
byte rtc_check();

byte rtc_mode();

byte rtc_clr_interrupt();

byte rtc_set(_time* newTime);
byte rtc_get(_time* time);

byte rtc_read_secs(byte* secs);
byte rtc_read_mins(byte* mins);
byte rtc_read_hrs(byte* hrs);
byte rtc_read_date(byte* date);
byte rtc_read_mon(byte* mon);
byte rtc_read_yr(byte* year);

byte rtc_write_secs(byte secs);
byte rtc_write_mins(byte mins);
byte rtc_write_hrs(byte hrs);
byte rtc_write_date(byte date);
byte rtc_write_mon(byte mon);
byte rtc_write_yr(byte year);
