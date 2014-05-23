#!/usr/bin/python
from twisted.internet import protocol, reactor
from twisted.application import internet, service
from twisted.protocols import basic
from twisted.python import log

from sys import stdout
from datetime import datetime, timedelta

from fennec_file import FennecFile

#logfile
log.startLogging(file(FennecFile.app_directory + '/log/fennec_' + datetime.now().strftime('%Y-%m-%d_%H:%M:%S') + '.log', 'w'))

#class to handle Fennec events
#class FennecProtocol(basic.LineReceiver):
class FennecProtocol(protocol.Protocol, basic._PauseableMixin):
   PACKET_LENGTH = 222 #should drop connection if more than 256 bytes comes at once 
   MAX_LENGTH = 222 #should drop connection if more than 256 bytes comes at once 

   #file handler - in use, will have the most recent Sys page
   #and the current data page open
   fennecfile = FennecFile() 

   def connectionMade(self):
      log.msg("connection from", self.transport.getPeer())
   #   self.setRawMode()

   #get a packet of data, not necessarily all 256 bytes of a message
   def dataReceived(self, recd):
      self.factory.dumpfile.write(recd);
      port = self.transport.getPeer().port;
      if(self.factory.last_incomplete_packet_time.get(port,False)):
         log.msg('Last incomplete packet from port ' + str(port) + ' at: ' +self.factory.last_incomplete_packet_time[port].isoformat())
      if(self.factory.last_incomplete_packet_time.get(port,False) and ((datetime.now() - self.factory.last_incomplete_packet_time[port]) > self.factory.max_time_delta)):
         #reset
         self.factory._recvd[port] ="" 
      #get method here returns empty string if the entry does not exist  else contents
      self.factory._recvd[port] = self.factory._recvd.get(port,"") + str(recd)
      self.factory.last_incomplete_packet_time[port] = datetime.now()
      log.msg('Received: ' + str(len(recd)) + " buffer[" + str(port) + "] at " +str(len(self.factory._recvd[port])))
      #log.msg('Data: ' + str(recd))
      dumpfile = open(FennecFile.app_directory + "/log/binlogfile", 'ab')
      dumpfile.write(str(recd))
      dumpfile.closed
      while self.factory._recvd.get(port,False) and len(self.factory._recvd[port]) > 0  and not self.paused:
         length = self.PACKET_LENGTH
         if len(self.factory._recvd[port]) < length:
            break
         packet = self.factory._recvd[port]
         #clear buffer
         del self.factory._recvd[port]
         self.stringReceived(packet) 
   
   
   #process message 
   def stringReceived(self, dataline):
      if(self.fennecfile.read(dataline)):
         #write file
         self.fennecfile.write()
         log.msg(self.fennecfile.unpack_data())
         if(not self.fennecfile.is_syspage()):
            mySonic = self.fennecfile.sonic_speed()
            if(len(self.fennecfile.Sys) > 0):
               #log.msg((self.fennecfile.get_date().isoformat()) 
               #+ (" %4.1lf\t %2.1lf\t" % tuple(self.fennecfile.pressure()))
               #+ ("%(SW up)1.4lf\t %(SW down)1.4lf\t %(LW up)1.4lf\t %(LW down)1.4lf\t %(Temperature #3)1.4lf\t %(Temperature #4)1.4lf\t %(Ground Flux)1.4lf\t %(Ground Flux Cal)1.4lf\t" %  self.fennecfile.radADC())
               #+ ("%02d\t %3.1lf\t %3.1lf\t %3.1lf\t %3.2lf\n" % ((self.fennecfile.Data.Sonic_Sample_No,) + (mySonic['speedMean'], mySonic['speedUMean'], mySonic['speedVMean'], mySonic['speedVar']))))
               #second data sample
               mySonic = self.fennecfile.sonic_speed(2)
               #log.msg((self.fennecfile.get_date(2).isoformat()) 
               #+ (" %4.1lf\t %2.1lf\t" % tuple(self.fennecfile.pressure()))
               #+ ("%(SW up)1.4lf\t %(SW down)1.4lf\t %(LW up)1.4lf\t %(LW down)1.4lf\t %(Temperature #3)1.4lf\t %(Temperature #4)1.4lf\t %(Ground Flux)1.4lf\t %(Ground Flux Cal)1.4lf\t" %  self.fennecfile.radADC(2))
               #+ ("%02d\t %3.1lf\t %3.1lf\t %3.1lf\t %3.2lf\n" % ((self.fennecfile.Data2.Sonic_Sample_No,) + (mySonic['speedMean'], mySonic['speedUMean'], mySonic['speedVMean'], mySonic['speedVar']))))
            else: #Sysypage is not set
               log.msg('WARNING: SYS page not set so data cannot be processed; saved'); 
         #ascii chr128 for ack
         self.transport.write(chr(128)+chr(13))
      else:
         #send NAK
         #ascii chr129 for nack
         log.msg("NAK sent")
         self.transport.write(chr(129)+chr(13))

         
         


   #def rawDataReceived(self, data):
   #   log.msg(data)
    #  stdout.write(data)

class FennecFactory(protocol.Factory):
   _recvd = {}
   protocol = FennecProtocol
   #packets must be separated by no more than 5secs.
   last_incomplete_packet_time = {}
   max_time_delta = timedelta(seconds=10)
   dumpfile = open(FennecFile.app_directory + "/log/binlogfile", 'ab')



application = service.Application('fennec')
#reactor.listenTCP(9080, FennecFactory())
#reactor.run()
factory = FennecFactory()
internet.TCPServer(9080, factory).setServiceParent(service.IServiceCollection(application))

