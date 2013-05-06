#!/usr/bin/env python
import smbus
import time
import struct
import array
import math
from ctypes import *

class StUnpackData(Structure):
    _pack_ = 1
    _fields_ = [("A0", c_uint,10),
                ("z0", c_uint,6),
                ("A1", c_uint,10),
                ("z1", c_uint,2),
                ("Overrun", c_uint,3),
                ("Valid", c_uint,1)]

class StPackData(Structure):
    _pack_ = 1
    _fields_ = [("A0", c_uint,10),
                ("A1", c_uint,10),
                ("Overrun", c_uint,3),
                ("Valid" , c_uint,1)]

class PackData(Union):
    _fields_ = [("Ulong", c_ulong,24),
                ("struct", StPackData)]
    def __init__(self,value):
      self.Ulong = value

class UnpackData(Union):
    _fields_ = [("Ulong", c_ulong),
                ("struct", StUnpackData)]
    def __init__(self,value):
       self.Ulong = value

class StA2D_Version(Structure):
   _pack_ = 1
   _fields_ = [("Id", c_uint,8),
               ("Tag",c_uint,8),
               ("Major", c_uint,8),
               ("Minor", c_uint,8)]

class A2D_Version(Union):
    _fields_ = [("Ulong", c_ulong),
                ("struct",StA2D_Version),
                ("Array",c_byte * 4)]
    def __init__(self,value):
        self.Ulong = value

# command definition

A2D_CMD_MODE=	0
A2D_CMD_TIMER=	1
A2D_CMD_DATA_NUMBER=	2
A2D_CMD_READ_DATA=	3
A2D_CMD_READ_PACK_DATA=	4
A2D_CMD_SLAVE_ADDRESS=	5
A2D_CMD_TIMER_COUNTER=	6
A2D_CMD_VERSION=	7
A2D_CMD_OSC_TUNE=	8
A2D_CMD_FLASH_SETTINGS=	9

# mode definition

A2D_MODE_OFF=		0
A2D_MODE_SINGLE=	3
A2D_MODE_TRIGGER=	5
A2D_MODE_TIMER=		7

bus = smbus.SMBus(1)



def A2DMode(Address,Mode):
   bus.write_byte_data(Address,A2D_CMD_MODE,Mode)


def A2DReadVersion(Address):
   _block=bus.read_i2c_block_data(Address,A2D_CMD_VERSION,4)
   Version= A2D_Version(_block[0] + (_block[1]<<8) + (_block[2]<<16) + (_block[3]<<24))
   return Version

def A2DReadUnpackData(Address):
   _block=bus.read_i2c_block_data(Address,A2D_CMD_READ_DATA,4)
   Data = UnpackData(_block[0] + (_block[1]<<8) + (_block[2]<<16) + (_block[3]<<24))
   return Data

def A2DReadUnpackDataBlock(Address,Number):
   DataList = []
   if Number < 1:
     return None
   if Number > 7:
     Number = 7
   _block=bus.read_i2c_block_data(Address,A2D_CMD_READ_DATA,Number * 4)
   for I in range(0 , Number):
      DataList.append( UnpackData(_block[I*4] + (_block[(I*4)+1]<<8) + (_block[(I*4)+2]<<16) + (_block[(I*4)+3]<<24)))
   return DataList

def A2DReadPackData(Address):
   _block=bus.read_i2c_block_data(Address,A2D_CMD_READ_PACK_DATA,3)
   Data = PackData(_block[0] + (_block[1]<<8) + (_block[2]<<16))
   return Data

def A2DReadPackDataBlock(Address,Number):
  DataList = []
  if Number < 1:
    return None
  if Number > 10:
    Number = 10
  _block=bus.read_i2c_block_data(Address,A2D_CMD_READ_PACK_DATA,Number * 3)
  for I in range(0 , Number):
     DataList.append( PackData(_block[I*3] + (_block[(I*3)+1]<<8) + (_block[(I*3)+2]<<16)))
  return DataList


def A2DTimer(Address , TimerValue):
   bus.write_word_data(Address, A2D_CMD_TIMER, TimerValue)

def A2DReadDataCount(Address):
   return bus.read_byte_data(Address,A2D_CMD_DATA_NUMBER)

def A2DWriteOscTune(Address, osc):
   if (osc >= (-32)) and (osc < 32):
     bus.write_byte_data(Address,A2D_CMD_OSC_TUNE,osc)

def A2DReadOscTune(Address):
   data = bus.read_byte_data(Address,A2D_CMD_OSC_TUNE)
# convert unsigned byte to sign byte
   if data > 31:
     data &= 31;
     data -= 32;
   return data

def A2DReadTimerCounterWord(Address):
   data = bus.read_word_data(Address, A2D_CMD_TIMER_COUNTER)
   return data

SlaveAddress1 = 0x20
SlaveAddress2 = 0x21

###########    Test Functions ######


def DisplayVersion():
   Version = A2DReadVersion(SlaveAddress1)
   print "Id = " + hex(Version.struct.Id) 
   print "Tag= " + hex(Version.struct.Tag) 
   print "Version= " + str(Version.struct.Major) + "." + str(Version.struct.Minor)


def TestSingleConversion():
   print "##########  single conversion test"
   print "Start single conversion"
   A2DMode(SlaveAddress1,A2D_MODE_SINGLE)   
   print "Read Unpack data"
   Data = A2DReadUnpackData(SlaveAddress1)
   print "0: " + str(Data.struct.A0) + "  1: " + str(Data.struct.A1) + "  Overrun : " + str(Data.struct.Overrun) + "  Valid: " + str(Data.struct.Valid)
   print "Start single conversion"
   A2DMode(SlaveAddress1,A2D_MODE_SINGLE)   
   print "Read Pack data"
   Data = A2DReadPackData(SlaveAddress1)
   print "0: " + str(Data.struct.A0) + "  1: " + str(Data.struct.A1) + "  Overrun : " + str(Data.struct.Overrun) + "  Valid: " + str(Data.struct.Valid)



def CalculateTimerRate(Address , osc):
   A2DMode(Address,A2D_MODE_OFF)
   A2DWriteOscTune(Address,osc)
# set timer to 5K sample /sec
   A2DTimer(Address,2)
   A2DMode(Address,A2D_MODE_TIMER)
   t_s = time.time()
   Count1 = A2DReadTimerCounterWord(Address)
#  sleep for 2 seconds
   time.sleep(2)
   t_e = time.time()
   Count2 = A2DReadTimerCounterWord(Address)
   rate = (Count2 - Count1) / (t_e - t_s)
   print "OscTune= {0}  Rate= {1:0.1f} Samples/sec  count= {2}  elapse = {3:.1f}".format(osc, rate, Count2 - Count1, t_e - t_s)
   return rate

def AdjustOscillator():
   print "########### adjust oscillator"
   Target=5000.0
   Best = A2DReadOscTune(SlaveAddress1)
   osc = Best
   BestDelta = CalculateTimerRate(SlaveAddress1,osc) - Target     
   if BestDelta > 0.0:
      while True:
        osc = osc - 1
        if osc < (-32) :
           break
        CurrentDelta = CalculateTimerRate(SlaveAddress1,osc) - Target
        if (math.fabs(BestDelta) < math.fabs(CurrentDelta)):
           break
        BestDelta = CurrentDelta
        Best = osc
   else:
      while True:
        osc = osc + 1
        if osc > 32 :
           break 
        CurrentDelta = CalculateTimerRate(SlaveAddress1,osc) - Target
        if (math.fabs(BestDelta) < math.fabs(CurrentDelta)):    
           break
        BestDelta = CurrentDelta
        Best = osc
   PercentError = BestDelta * 100.0 / Target
   print "Best  Osc Tune = {0}  frequency {1:.0f} {2:+.3f}%".format(Best,Target, PercentError)



def TestTimerMode():
   print "##########  Test Timer mode "
   print "Set Timer interval to 10 ms 0 (100 samples /sec)"
   A2DTimer(SlaveAddress1, 100)
   print "Start Timer Mode"
   A2DMode(SlaveAddress1,A2D_MODE_TIMER)
   
   totsample=0
   nsample=0
   delta=0
   elapse=0
   t_s= time.time()
   
   while elapse < 10.5:
      DataCount = A2DReadDataCount(SlaveAddress1)
      isample=0;
      
      if(DataCount > 1):
        if(DataCount>7):
          isample=7
        else:
          isample= DataCount;
#        print "isample = {0} elapse={1}".format(isample,elapse)
        Data=A2DReadPackDataBlock(SlaveAddress1,isample)
        nsample+= isample
      t_e = time.time()
      elapse = t_e - t_s
      if (elapse - delta) > 1.0:
        delta=elapse
        totsample+=nsample
        nsample=0
        print "{0:0.1f} sec  count={1}".format(elapse,totsample),
        idx= len(Data)
	if idx >0:
           idx-=1
#       display the latest data
        print " latest block index: " + str(idx) +"  0: " + str(Data[0].struct.A0) + "  1: " + str(Data[0].struct.A1) + "  Overrun : " + str(Data[0].struct.Overrun) + "  Valid: " + str(Data[0].struct.Valid)
        
 

######################  MAIN ############

DisplayVersion()
TestSingleConversion()
AdjustOscillator()
TestTimerMode()
