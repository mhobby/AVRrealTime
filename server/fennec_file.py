import array,struct
from namedtuple24 import namedtuple
from twisted.python import log

from sys import stdout
from datetime import datetime
import time
import calendar
import os, errno, stat

import crcmod.predefined

def mkdir_p(path):
    #function to simulate "mkdir -p"
    try:
        os.umask(0)
        os.makedirs(path,stat.S_IRWXG | stat.S_IRWXU)
    except OSError, exc: 
        if exc.errno == errno.EEXIST:
            pass
        else: raise


#class to handle Fennec file format
class FennecFile:
   #constants & Magic Numbers
   year_offset = 2000 #year 0 according to the station
   radADC_divisor = 65536
   app_directory = '/opt/fennec'
   
   data = array.array('B') 
   # see http://docs.python.org/library/struct.html#format-characters for meanings
   data_struct_fmt = '<B6B20H2lLqB2H2lLq5H2B2HBH' #qqq is placeholder for unused bytes
   sys_struct_fmt = '<B6B6B6BH4H4H2B3B'     # leading < is significant; forces little-endian byte order

   data_tuple = namedtuple('Data', 'station_id RTC_day RTC_month RTC_year RTC_hour RTC_min RTC_sec pressureA_D1 pressureA_D2 pressureB_D1 pressureB_D2 Temperature_1 Humidity_1 Temperature_2 Humidity_2 Therm_1 Therm_2 SW_up SW_down LW_up LW_down Temperature_3 Temperature_4 Ground_Flux Ground_Flux_Cal Sonic_Speed_Sum Sonic_Speed_Max Sonic_U_Sum Sonic_V_Sum Sonic_Speed2_Sum Sonic_Speed3_Sum Sonic_Sample_No Cup_Speed_Sum Cup_Speed_Max Cup_Vane_U_Sum Cup_Vane_V_Sum Cup_Vane_Speed2_Sum Cup_Vane_Speed3_Sum Cup_Vane_Sample_No V_5VREG V_12VREG VBATT SysTemp aux1 aux2 Ground_Probe_1 Ground_Probe_2 pageFlag crc ')

   sys_tuple = namedtuple('Sys','station_id bootTime_day bootTime_month bootTime_year bootTime_hour bootTime_min bootTime_sec lastTxTime_day lastTxTime_month lastTxTime_year lastTxTime_hour lastTxTime_min lastTxTime_sec lastIridiumTxTime_day lastIridiumTxTime_month lastIridiumTxTime_year lastIridiumTxTime_hour lastIridiumTxTime_min lastIridiumTxTime_sec bytesTx pressACals_1 pressACals_2 pressACals_3 pressACals_4 pressBCals_1 pressBCals_2 pressBCals_3 pressBCals_4 ctrl_0 ctrl_1 rsvd_0 rsvd_1 rsvd_2' )

   #replaced by named tuples when instantiated
   Data = []
   Data2 = []
   Sys = []

 
   def read(self, dataline): #the 222-chr page from the station
      a = array.array('B') # B is unsigned 8-bit 
      #check length
      if(len(dataline) != 222):
         #must be exactly 222
         return False
      #trims to 222 characters - protocol calls for *exactly* 222
      # characters follwed by an ACK from the server if the checksum
      # is valid
      a.fromstring(dataline)
      self.data = a
      if self.checksum(a):
         return True
      else:
         return False # i.e. checksum failed; should trigger NACK


   def checksum(self, data_array):
      #CRC should go here!

      try: 
         self.unpack_data()
      except:
         log.msg(self.Data)
         log.msg('Unpack data failed')
         return False #data structure error of some sort

      if(not self.is_syspage()):
         log.msg('DATA PAGE...')
	 #print data_array[0:126].tostring()
         crc16 = crcmod.predefined.Crc('crc-16')
         crc16.update(data_array[0:109].tostring())
         if(not (crc16.crcValue == self.Data.crc)):
            #ignore crc if datapage is full of 0xFF (CRC=30828 if this is the case)
            if(not (crc16.crcValue == 30828)):
               log.msg('Checksum failed')
               log.msg(crc16.crcValue)
               log.msg(self.Data.crc)
               return False 

         crc16 = crcmod.predefined.Crc('crc-16')
         crc16.update(data_array[111:220].tostring())
         if(not (crc16.crcValue == self.Data2.crc)):
            #ignore crc if datapage is full of 0xFF (CRC=30828 if this is the case)
            if(not (crc16.crcValue == 30828)):
               log.msg('Checksum failed')
               log.msg(crc16.crcValue)
               log.msg(self.Data2.crc)
               return False 
      
      try:
         self.get_date()
      except:
#         log.msg(self.Data)
         log.msg('Invalid date')
         return False #invalid date

      return True

   def is_syspage(self):
      #if byte 253 is 0 then it's a syspage; if it's 255 it's a datapage
      if(self.data[219] == 0):
         return True
      else:
         return False 

   def unpack_data(self):
      log.msg("unpacking data")
#      log.msg(self.data)
      #reads the packed binary string into a named tuple for easy access
      if(self.is_syspage()):
         tempsys = []
         dates_of_tempsys = []
         for i in range(0,5): #syspage can have up to 5 entries
	     structsize = struct.calcsize(self.sys_struct_fmt)
             unpacked_struct = self.sys_tuple._make(struct.unpack(self.sys_struct_fmt,self.data.tostring()[(i * structsize):((i+1)*structsize)]))
             if((unpacked_struct.bootTime_day != 0)and(unpacked_struct.bootTime_day != 255)): #empty stanzas are zero-filled so "boot day" 255 means empty stanza
                tempsys.insert(i,unpacked_struct)
                dates_of_tempsys.insert(i, datetime(unpacked_struct.bootTime_year + self.year_offset, unpacked_struct.bootTime_month, unpacked_struct.bootTime_day, unpacked_struct.bootTime_hour, unpacked_struct.bootTime_min, unpacked_struct.bootTime_sec))
         #print dates_of_tempsys, dates_of_tempsys.index(max(dates_of_tempsys))
         log.msg("Using Sys data in position " + str(dates_of_tempsys.index(max(dates_of_tempsys))))
         self.Sys = tempsys[dates_of_tempsys.index(max(dates_of_tempsys))]
         return self.Sys
      else:
	 self.Data = self.data_tuple._make(struct.unpack(self.data_struct_fmt,self.data.tostring()[0:struct.calcsize(self.data_struct_fmt)]))
	 self.Data2 = self.data_tuple._make(struct.unpack(self.data_struct_fmt,self.data.tostring()[111:(111 + struct.calcsize(self.data_struct_fmt))]))
	 return self.Data

   def get_station_id(self):
      #returns the station id
      if(self.is_syspage()):
         return self.Sys.station_id
      else:
         return self.Data.station_id
   
   def get_date(self, sample=1):
      if(self.is_syspage()):
         #RTC time
         #print self.data[1:6]
         #print(self.sys_struct.unpack(self.data.tostring()[0:42]))
         debug=datetime(self.Sys.bootTime_year + self.year_offset, self.Sys.bootTime_month, self.Sys.bootTime_day, self.Sys.bootTime_hour, self.Sys.bootTime_min, self.Sys.bootTime_sec); 
         return debug
         #return datetime(self.data[6] + 2000, self.data[5], self.data[4], self.data[3], self.data[2], self.data[1]); 
      else:
         #ergo is datapage
         #stdout.write(self.data[1:6)         
         if sample==1:
            sampledata = self.Data
         else:
            sampledata = self.Data2
         
         debug=datetime(sampledata.RTC_year + self.year_offset, sampledata.RTC_month, sampledata.RTC_day, sampledata.RTC_hour, sampledata.RTC_min, sampledata.RTC_sec);          
         #station date/time corrections
         if((self.get_station_id()==141)or(self.get_station_id()==135)):
            log.msg('Correcting Date...')
            julianDate=calendar.timegm(time.strptime("%04d%02d%02d%02d%02d%02d" % (debug.year,debug.month,debug.day,debug.hour,debug.minute,debug.second), "%Y%m%d%H%M%S")) 
            adjJulianDate=julianDate+86400
            adjDate=time.gmtime(adjJulianDate)
            debug=debug.replace(year=adjDate[0])
            debug=debug.replace(month=adjDate[1])
            debug=debug.replace(day=adjDate[2])
         return debug
         #return datetime(self.data[2] + 2000, self.data[1], self.data[0], self.data[3], self.data[4], self.data[5]); 

   #writes file
   def write(self):
      if(self.is_syspage()):
	filename = 'sys.bin'
      else:
         filename = 'data.bin'
      filedate = self.get_date()
      filedir = filedate.strftime('%Y')+'/'+filedate.strftime('%m')+'/'+filedate.strftime('%d')+'/station_'+str(self.get_station_id())
      mkdir_p(self.app_directory +'/data/' + filedir)
      filehandle = open(self.app_directory + '/data/' + filedir + '/' +self.get_date().isoformat() + filename, "w" )
      #os.chmod(self.app_directory + '/data/' + filedir + '/..',stat.S_IRWXG | stat.S_IRWXU)
      os.chmod(self.app_directory + '/data/' + filedir + '/' +self.get_date().isoformat() + filename, 0660) 
      return self.data.tofile(filehandle)

   def pressure(self):
      pressure =[0,0]
      pressCals = [0,0,0,0,0,0]

      pressCals[0] = (0xFFFE&self.Sys.pressACals_1)>>1;
      pressCals[1] = ((0x003F&self.Sys.pressACals_3)<<6)+(0x003F&self.Sys.pressACals_4);
      pressCals[2] = (0xFFC0&self.Sys.pressACals_4)>>6;
      pressCals[3] = (0xFFC0&self.Sys.pressACals_3)>>6;
      pressCals[4] = ((0x01&self.Sys.pressACals_1)<<10)+((0xFFC0&self.Sys.pressACals_2)>>6);
      pressCals[5] = (0x003F&self.Sys.pressACals_2);
   
      dT = (self.Data.pressureA_D2-(8*pressCals[4]+20224));
      off = (pressCals[1]*4.0)+(((pressCals[3]-512.0)*dT)/4096.0);
      sens = pressCals[0] + ((pressCals[2]*dT)/1024.0) + 24576.0;
      pressure[0] = ((sens * (self.Data.pressureA_D1-7168.0))/16384.0) - off;
      pressure[1] = 0.1*(200.0+ (dT*((pressCals[5]+50.0)/1024.0)));
      pressure[0] = (pressure[0]/32.0)+250.0; 

      return pressure

   def sonic_speed(self, sample=1):
      if sample==1:
         sampledata = self.Data
      else:
         sampledata = self.Data2
      mySonic = {} #dictionary
      if(sampledata.Sonic_Sample_No != 0):
         tempShortInt = sampledata.Sonic_Speed_Sum
         mySonic['speedMean'] = (tempShortInt * 0.1)/sampledata.Sonic_Sample_No;

         tempInt = sampledata.Sonic_U_Sum
         mySonic['speedUMean'] = (tempInt * 0.1)/sampledata.Sonic_Sample_No;

         tempInt = sampledata.Sonic_V_Sum
         mySonic['speedVMean'] = (tempInt * 0.1)/sampledata.Sonic_Sample_No;

         tempInt = sampledata.Sonic_Speed2_Sum
         mySonic['speedVar'] = ((tempInt * 0.01)/sampledata.Sonic_Sample_No) - pow(mySonic['speedMean'],2);
      else:
         mySonic['speedMean'] = -999.9;
         mySonic['speedUMean']= -999.9;
         mySonic['speedVMean']= -999.9;
         mySonic['speedVar']  = -999.9;

      return mySonic

   def radADC(self, sample=1):
      if sample==1:
         sampledata = self.Data
      else:
         sampledata = self.Data2
      radADC = {}
      radADC['SW up']=float(5* sampledata.SW_up) / self.radADC_divisor
      radADC['SW down']=float(5* sampledata.SW_down) / self.radADC_divisor
      radADC['LW up']=float(5* sampledata.LW_up) / self.radADC_divisor
      radADC['LW down']=float(5* sampledata.LW_down) / self.radADC_divisor
      radADC['Temperature #3']=float(5* sampledata.Temperature_3) / self.radADC_divisor
      radADC['Temperature #4']=float(5* sampledata.Temperature_4) / self.radADC_divisor
      radADC['Ground Flux']=float(5* sampledata.Ground_Flux) / self.radADC_divisor
      radADC['Ground Flux Cal']=float(5* sampledata.Ground_Flux_Cal) / self.radADC_divisor
      return radADC

