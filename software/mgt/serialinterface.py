import serial
import string
import sys
import time

from logger import flogger,getLogger

class SerialInterface:
    log = getLogger('SerialInterface')
    
    @flogger(log)
    def  __init__ ( self, path2device, baudrate, timeout=0):
        self.ser = serial.Serial(path2device, baudrate)
        self.ser.flushInput()
        self.ser.flushOutput()
        if timeout:
            self.ser.setTimeout(timeout)

    def writeMessage(self,message):
        enc = "\\0" + message.replace('\\','\\\\') + "\\1";
        self.log.debug('writing %s' % list(enc))
        self.ser.write(enc)

    def readMessage(self):
        data = ""
        escaped = False
        stop = False
        start = False

        while True:
            c = self.ser.read(1)
            if len(c) == 0:             #A timout occured
                self.log.warning('TIMEOUT')
                return False
            #print "c=", list(c)
        #    continue
            if escaped:
                if c == '0':
                    start = True
                elif c == '1':
                    stop = True
                elif c == '\\':
                    d = '\\'
                escaped = False
            elif c == '\\':
                escaped = 1
            else:
                d = c
                
            if start:
                start = False
            elif stop:
                if data[0] == 'D':
                    self.log.info('Debug: %s'%(data[1:]))
                    data = ""
                    stop = False
                else:
                    self.log.debug('received message: len=%d data=%s'%(len(data), list(data)))
                    return data
            elif escaped == False:
                data += str(d)
if __name__ == '__main__': 
    s = SerialInterface("/dev/ttyUSB0",115200);
    #m = "P"+ chr(0x22)+chr(0x23)+chr(0)+chr(1)+chr(1)+'a'
    #s.writeMessage(m)
    while 1:
        data = s.readMessage()
        if data[0] == 'P' and data[2] == '\x01':
            #time.sleep(0.1)
            m = "P"+data[2]+data[1]+chr(0)+data[4]+chr(1)+data[6]
            #s.writeMessage(m)
