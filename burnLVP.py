#!/usr/bin/env python

################################
#
# burnLVP.py  
#
#
# Program to burn pic12F1840 using LVP mode with a Rasberry Pi
#
#
# programmer : Daniel Perron
# Date       : June 30, 2013
# Version    : 1.0
#
# source code:  https://github.com/danjperron/A2D_PIC_RPI
#
#

#////////////////////////////////////  GPL LICENSE ///////////////////////////////////
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#   



from time import sleep
import RPi.GPIO as GPIO
import sys, termios, atexit
from intelhex import IntelHex
from select import select   


#set io pin

#CLK GPIO 4 
PIC_CLK = 7

#DATA GPIO 8
PIC_DATA = 24

#MCLR GPIO 9
PIC_MCLR = 21


#compatible PIC id

PIC12F1840 = 0x1b80
PIC16F1847 = 0x1480
PIC12LF1840 = 0x1bc0
PIC16LF1847 = 0x14A0

PIC16F1826  = 0x2780
PIC16F1827  = 0x27A0
PIC16LF1826 = 0x2880
PIC16LF1827 = 0x28A0
PIC16F1823  = 0x2720
PIC16LF1823 = 0x2820
PIC12F1822  = 0x2700
PIC12LF1822 = 0x2800
PIC16F1824  = 0x2740
PIC16LF1824 = 0x2840
PIC16F1825  = 0x2760
PIC16LF1825 = 0x2860
PIC16F1828  = 0x27C0
PIC16LF1828 = 0x28C0
PIC16F1829  = 0x27E0
PIC16LF1829 = 0x28E0




# command definition
C_LOAD_CONFIG = 0
C_LOAD_PROGRAM = 2 
C_LOAD_DATA   =  3
C_READ_PROGRAM = 4
C_READ_DATA    = 5
C_INC_ADDRESS = 6
C_RESET_ADDRESS = 0x16
C_BEGIN_INT_PROG = 8
C_BEGIN_EXT_PROG = 0x18
C_END_EXT_PROG = 0xa
C_BULK_ERASE_PROGRAM = 9
C_BULK_ERASE_DATA = 0xB

def Release_VPP():
   GPIO.setup(PIC_DATA, GPIO.IN)
   GPIO.setup(PIC_CLK,  GPIO.IN)
   GPIO.output(PIC_MCLR, False)
   print "VPP OFF"


def Set_VPP():
   print "VPP OFF"
   #held MCLR Low
   GPIO.setup(PIC_MCLR,GPIO.OUT)
   GPIO.output(PIC_MCLR,False)
   sleep(0.1)
   #ok PIC_CLK=out& HIGH, PIC_DATA=out & LOW
   GPIO.setup(PIC_CLK, GPIO.OUT)
   GPIO.output(PIC_CLK,False)
 
   #MCLR LOW 
   GPIO.setup(PIC_DATA, GPIO.OUT)
   GPIO.output(PIC_DATA,False)
   print "VPP ON"
   GPIO.output(PIC_MCLR,True)
   sleep(0.1)

def Set_LVP():
   #held MCLR HIGH
   GPIO.setup(PIC_MCLR,GPIO.OUT)
   GPIO.output(PIC_MCLR,True)
   sleep(0.1)
   #ok PIC_CLK=out& HIGH, PIC_DATA=out & LOW
   GPIO.setup(PIC_CLK, GPIO.OUT)
   GPIO.output(PIC_CLK,False)

   #MCLR LOW 
   GPIO.setup(PIC_DATA, GPIO.OUT)
   GPIO.output(PIC_DATA,False)
   print "LVP ON"
   GPIO.output(PIC_MCLR,False)
   sleep(0.3)

def Release_LVP():
   GPIO.setup(PIC_DATA,GPIO.IN)
   GPIO.setup(PIC_CLK,GPIO.IN)
   GPIO.output(PIC_MCLR,True)
   print "LVP OFF"

def SendMagic():
   magic = 0x4d434850
   GPIO.setup(PIC_DATA, GPIO.OUT)
   for loop in range(33):
      GPIO.output(PIC_CLK,True)
      GPIO.output(PIC_DATA,(magic & 1) == 1)
#  use pass like a delay
      pass
      GPIO.output(PIC_CLK,False)
      pass
      magic = magic >> 1


def SendCommand(Command):
   GPIO.setup(PIC_DATA, GPIO.OUT)
   for loop in range(6):
     GPIO.output(PIC_CLK,True)
     GPIO.output(PIC_DATA,(Command & 1)==1)
     pass
     GPIO.output(PIC_CLK,False)
     pass
     Command = Command >> 1;


def ReadWord():
    GPIO.setup(PIC_DATA,GPIO.IN)
    Value = 0
    for loop in range(16):
      GPIO.output(PIC_CLK,True)
      pass
      if GPIO.input(PIC_DATA):
        Value =  Value + (1 << loop)
      GPIO.output(PIC_CLK,False)
      pass
    Value = (Value >> 1) & 0x3FFF;
    return Value;
        

def LoadWord(Value):
  GPIO.setup(PIC_DATA,GPIO.OUT)
  Value = (Value << 1) & 0x7FFE
  for loop in range(16):
     GPIO.output(PIC_CLK,True)
     GPIO.output(PIC_DATA,(Value & 1)==1)
     pass
     GPIO.output(PIC_CLK,False)
     pass
     Value = Value >> 1;



def Pic12_BulkErase():
   print "Bulk Erase Program",
   SendCommand(C_RESET_ADDRESS)
   SendCommand(C_LOAD_CONFIG)
   LoadWord(0x3fff)
   SendCommand(C_BULK_ERASE_PROGRAM)
   sleep(0.1)
   print ", Data.",
   SendCommand(C_BULK_ERASE_DATA)
   sleep(0.1)
   print ".... done."


def Pic12_ProgramBlankCheck(program_size):
   print "Program blank check",
   SendCommand(C_RESET_ADDRESS)
   for l in range(program_size):
     SendCommand(C_READ_PROGRAM)
     Value = ReadWord()
     if  Value != 0x3fff :
       print "*** CPU program at Address ", hex(l), " = ", hex(Value), " Failed!"
       return False
     if (l % 128)==0 :
       sys.stdout.write('.')
       sys.stdout.flush()
     SendCommand(C_INC_ADDRESS)
   print "Passed!"
   return True

def Pic12_DataBlankCheck(data_size):
   print "Data Blank check",
   SendCommand(C_RESET_ADDRESS)
   for l in range(data_size):
     SendCommand(C_READ_DATA)
     Value = ReadWord()
     if  Value != 0xff :
        print "*** CPU eeprom data  at Address ", hex(l), " = ", hex(Value), "Failed!"
        return False
     if (l % 32)==0 :
       sys.stdout.write('.')
       sys.stdout.flush()
     SendCommand(C_INC_ADDRESS)
   print "Passed!"
   return True


def Pic12_ProgramBurn(pic_data, program_base, program_size):
   print "Writing Program",
   SendCommand(C_RESET_ADDRESS)
   for l in range( program_size):
      if pic_data.get(l*2+ program_base) != None :
        if pic_data.get(l*2+ program_base+1) != None :
          Value = pic_data.get(l*2+ program_base) + ( 256 * pic_data.get(l*2+ program_base+1))
          Value = Value & 0x3fff
          SendCommand(C_LOAD_PROGRAM)
          LoadWord(Value)
          SendCommand(C_BEGIN_INT_PROG)
          sleep(0.005)
          SendCommand(C_READ_PROGRAM)
          RValue = ReadWord()
          if Value != RValue :
            print "Program address:", hex(l) , " write ", hex(Value), " read ", hex(RValue), " Failed!"
            return False
      if (l % 128)==0 :
        sys.stdout.write('.')
        sys.stdout.flush()
      SendCommand(C_INC_ADDRESS)
   print "Done."
   return True

def Pic12_DataBurn(pic_data, data_base, data_size):
   print "Writing Data",
   SendCommand(C_RESET_ADDRESS)
   for l in range( data_size):
      if pic_data.get(l + data_base) != None :
        Value = pic_data.get(l + data_base)
        SendCommand(C_LOAD_DATA)
        LoadWord(Value)
        SendCommand(C_BEGIN_INT_PROG)
        sleep(0.003)
        SendCommand(C_READ_DATA)
        RValue = ReadWord()
        if Value != RValue :
          print "Data address:", hex(l) , " write ", hex(Value), " read ", hex(RValue), " Failed!"
          return False
      if (l % 32)==0 :
        sys.stdout.write('.')
        sys.stdout.flush()
      SendCommand(C_INC_ADDRESS)
   print "Done."
   return True

def Pic12_ProgramCheck(pic_data, program_base, program_size):
   print "Program check ",
   SendCommand(C_RESET_ADDRESS)
   for l in range(program_size):
      if pic_data.get(l*2+ program_base) != None :
        if pic_data.get(l*2+ program_base+1) != None :
          Value = pic_data.get(l*2+ program_base) + ( 256 * pic_data.get(l*2+ program_base+1))
          Value = Value & 0x3fff
	  SendCommand(C_READ_PROGRAM)
          RValue = ReadWord()
          if Value != RValue :
            print "Program address:", hex(l) , " write ", hex(Value), " read ", hex(RValue)
            return False
      if (l % 128)==0 :
        sys.stdout.write('.')
        sys.stdout.flush()
      SendCommand(C_INC_ADDRESS)
   print "Passed!"
   return True



def Pic12_DataCheck(pic_data, data_base, data_size):
   print "Data check ",
   SendCommand(C_RESET_ADDRESS)
   for l in range(data_size):
      if pic_data.get(l+ data_base) != None :
        Value = pic_data.get(l+data_base)
        SendCommand(C_READ_DATA)
        RValue = ReadWord()
        if Value != RValue :
           print "Data address:", hex(l) , " write ", hex(Value), " read ", hex(RValue)
           return False
      if (l % 32)==0 :
        sys.stdout.write('.')
        sys.stdout.flush()
      SendCommand(C_INC_ADDRESS)
   print "Passed!"
   return True


def Pic12_ConfigBurn(pic_data, config_base):
   print "Writing Config",
   SendCommand(C_RESET_ADDRESS)
   SendCommand(C_LOAD_CONFIG)
   LoadWord(0x3fff)
   #user id first
   for l in range(4):
      if pic_data.get(l*2+ config_base) != None :
        if pic_data.get(l*2+ config_base+1) != None :
          Value = pic_data.get(l*2+ config_base) + ( 256 * pic_data.get(l*2+ config_base+1))
          Value = Value & 0x3fff
          SendCommand(C_LOAD_PROGRAM)
          LoadWord(Value)
          SendCommand(C_BEGIN_INT_PROG)
          sleep(0.005)
          SendCommand(C_READ_PROGRAM)
          RValue = ReadWord()
          if Value != RValue :
            print "User Id Location:", hex(l) , " write ", hex(Value), " read ", hex(RValue), " Failed!"
            return False
        sys.stdout.write('.')
        sys.stdout.flush()
      SendCommand(C_INC_ADDRESS)
   #ok we are at 08004
   #skip 0x8004 .. 0x8006
   SendCommand(C_INC_ADDRESS)
   SendCommand(C_INC_ADDRESS)
   SendCommand(C_INC_ADDRESS)
   # now the configuration word 1& 2  at 0x8007 ( hex file at 0x1000E)
   for l in range(7,9):
      if pic_data.get(l*2+ config_base) != None :
        if pic_data.get(l*2+ config_base+1) != None :
          Value = pic_data.get(l*2+ config_base) + ( 256 * pic_data.get(l*2+ config_base+1))
          Value = Value & 0x3fff
          SendCommand(C_LOAD_PROGRAM)
          if l is 8:
            #catch21 force LVP programming to be always ON
            Value = Value | 0x2000
          LoadWord(Value)
          SendCommand(C_BEGIN_INT_PROG)
          sleep(0.005)
          SendCommand(C_READ_PROGRAM)
          RValue = ReadWord()
          if Value != RValue :
            print "Config Word ", l-6 , " write ", hex(Value), " read ", hex(RValue), " Failed!"
            return False
        sys.stdout.write('.')
        sys.stdout.flush()
      SendCommand(C_INC_ADDRESS)
   print "Done."
   return True

def Pic12_ConfigCheck(pic_data, config_base):
   print "Config Check",
   SendCommand(C_RESET_ADDRESS)
   SendCommand(C_LOAD_CONFIG)
   LoadWord(0x3fff)
   #user id first
   for l in range(4):
      if pic_data.get(l*2+ config_base) != None :
        if pic_data.get(l*2+ config_base+1) != None :
          Value = pic_data.get(l*2+ config_base) + ( 256 * pic_data.get(l*2+ config_base+1))
          Value = Value & 0x3fff 
          SendCommand(C_READ_PROGRAM)
          RValue = ReadWord()
          if Value != RValue :
            print "User Id Location:", hex(l) , " write ", hex(Value), " read ", hex(RValue), " Failed!"
            return False
        sys.stdout.write('.')
        sys.stdout.flush()
      SendCommand(C_INC_ADDRESS)
   #ok we are at 08004
   #skip 0x8004 .. 0x8006
   SendCommand(C_INC_ADDRESS)
   SendCommand(C_INC_ADDRESS)
   SendCommand(C_INC_ADDRESS)
   # now the configuration word 1& 2  at 0x8007 ( hex file at 0x1000E)
   for l in range(7,9):
      if pic_data.get(l*2+ config_base) != None :
        if pic_data.get(l*2+ config_base+1) != None :
          Value = pic_data.get(l*2+ config_base) + ( 256 * pic_data.get(l*2+ config_base+1))
          Value = Value & 0x3fff
          if l is 8:
            #catch21 force LVP programming to be always ON
            Value = Value | 0x2000
          SendCommand(C_READ_PROGRAM)
          RValue = ReadWord()
          if Value != RValue :
            print "Config Word ", l-6 , " write ", hex(Value), " read ", hex(RValue), " Failed!"
            return False
        sys.stdout.write('.')
        sys.stdout.flush()
      SendCommand(C_INC_ADDRESS)
   print "Passed!"
   return True


#just check if the user forget to set LVP flag enable
#if not just give a warning since we force LVP enable

def Pic12_CheckLVP(pic_data, config_base):
  #specify config word2
  l=8 
  if pic_data.get(l*2+ config_base) != None :
    if pic_data.get(l*2+ config_base+1) != None :
      Value = pic_data.get(l*2+ config_base) + ( 256 * pic_data.get(l*2+ config_base+1))
      Value = Value & 0x3fff
      return((Value & 0x2000)== 0x2000)
  return True
   

#============ MAIN ==============

if __name__ == '__main__':
  if len(sys.argv) is 2:
    HexFile = sys.argv[1]
  elif len(sys.argv) is 1:
    HexFile = ''
  else:
    print 'Usage: %s file.hex' % sys.argv[0]
    quit()


## load hex file if it exist
FileData =  IntelHex()
if len(HexFile) > 0 :
   try:
     FileData.loadhex(HexFile)
   except IOError:
     print 'Error in file "', HexFile, '"'
     quit()

PicData = FileData.todict()       
print 'File "', HexFile, '" loaded'
#set GPIO MODE

GPIO.setwarnings(False)
GPIO.setmode(GPIO.BOARD)



GPIO.setup(PIC_MCLR,GPIO.OUT)
GPIO.setup(PIC_CLK,GPIO.IN)
GPIO.setup(PIC_DATA,GPIO.IN)


#enter Program Mode

#Set_VPP()
#sleep(0.1)
#get cpu id
Set_LVP()
SendMagic()
SendCommand(0)
LoadWord(0x3fff)
for l in range(6):
   SendCommand(C_INC_ADDRESS)
SendCommand(C_READ_PROGRAM)
CpuId  = ReadWord()
CpuRevision = CpuId & 0x1f
CpuId = CpuId & 0x3FE0

#define default Program and EEPROM data size
ProgramSize = 0
DataSize    = 0
ProgramBase = 0
DataBase    = 0x1e000
ConfigBase  = 0x10000


print "Cpu :", hex(CpuId), " :",
if CpuId == PIC12F1840:
   print "PIC12F1840",
   ProgramSize = 4096
   DataSize    = 256 
elif CpuId == PIC16F1847:
   print "PIC16F1847",
   ProgramSize = 8192
   DataSize    = 256
elif CpuId == PIC16LF1847:
   print "PIC16LF1847",
   ProgramSize = 8192
   DataSize    = 256
elif CpuId == PIC12LF1840:
   print "PIC12LF1840",
   ProgramSize = 4096
   DataSize    = 256
elif CpuId == PIC16F1826:
   print "PIC16F1826",
   ProgramSize = 2048
elif CpuId == PIC16F1827:
   print "PIC16F1827",
   ProgramSize = 4096
elif CpuId == PIC16LF1826:
   print "PIC16LF1826",
   ProgramSize = 2048
elif CpuId == PIC16LF1827:
   print "PIC16LF1827",
   ProgramSize = 4096
elif CpuId == PIC16F1823:
   print "PIC16F1823",
   ProgramSize = 2048
elif CpuId == PIC16LF1823:
   print "PIC16LF1823",
   ProgramSize = 2048
elif CpuId == PIC12F1822:
   print "PIC16F1822",
   ProgramSize = 2048
elif CpuId == PIC12LF1822:
   print "PIC16LF1822",
   ProgramSize = 2048
elif CpuId == PIC16F1824:
   print "PIC16F1824",
   ProgramSize = 4096
elif CpuId == PIC16LF1824:
   print "PIC16LF1824",
   ProgramSize = 4096
elif CpuId == PIC16F1825:
   print "PIC16F1825",
   ProgramSize = 8192
elif CpuId == PIC16LF1825:
   print "PIC16LF1825",
   ProgramSize = 8192
elif CpuId == PIC16F1828:
   print "PIC16F1828",
   ProgramSize = 4095
elif CpuId == PIC16LF1828:
   print "PIC16LF1828",
   ProgramSize = 4095
elif CpuId == PIC16F1829:
   print "PIC16F1829",
   ProgramSize = 8192
elif CpuId == PIC16LF1829:
   print "PIC16LF1829",
   ProgramSize = 8192   
else:
   print "Invalid"
#   Release_VPP()
   Release_LVP()
   quit()
print "Revision ", hex(CpuRevision)

print "ProgramSize =", hex(ProgramSize)
print "DataSize    =", hex(DataSize)


#if(CpuId == PIC12F1840 or CpuId == PIC12LF1840 or CpuId == PIC16F1847 or CpuId == PIC16LF1847):
if ProgramSize > 0 : 
  Pic12_BulkErase()
  if Pic12_ProgramBlankCheck(ProgramSize):
    if Pic12_DataBlankCheck(DataSize):
      if Pic12_ProgramBurn(PicData,ProgramBase,ProgramSize):
        if Pic12_ProgramCheck(PicData,ProgramBase,ProgramSize):
          if Pic12_DataBurn(PicData,DataBase,DataSize):
            if Pic12_DataCheck(PicData,DataBase,DataSize):
              if Pic12_ConfigBurn(PicData,ConfigBase):
                if Pic12_ConfigCheck(PicData,ConfigBase):
                  if not Pic12_CheckLVP(PicData,ConfigBase):
                     print " *** Warning ***   LVP not set in Hex file. LVP was force!"
                  print "No Error.   All Done!"

#elif (cpuid == ???):
# put other cpu type here
# and burning method here

Release_LVP()   


