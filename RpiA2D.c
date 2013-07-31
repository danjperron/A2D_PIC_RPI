/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Simple 2 channels A/D conversion with I2C interface 
//
//
//   Date: 23 April 2013
//   programmer: Daniel Perron
//   Version: 1.0
//   Processor: PIC12F1840
//   Software: Microchip MPLAB IDE v8.90  with Hitech C (freeware version)
//   

/*

The MIT License (MIT)

Copyright (c)  2013 Daniel Perron

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

/*  A/D Command and pin description

   RA0  Analog 0
   RA1  SCL
   RA2  SDA
   RA3  MCLR
   RA4  Analog 1     (real analog 3)
   RA5  Trigger/Sync    I/O on input for edge detection interrupt in trigger mode. I/O on output  for start of conversion sync out.

Commands,

 00 : Control (R/W)
        (8 bits)
         bit 0:   Run/Stop   0:stop  1: run    (writing 1 to bit 0 will reset buffer data).
         bit 2..1:
                        0X :  Single mode .  Writing 1 to bit0 starts conversion. When conversion is done bit 0 is cleared. (buffer data hold last sample only)
                        10 :  Trigger mode.  Rising signal at RA5 start conversion. (RA5 is in input mode).
                        11:   Timer mode with sync out. Acquisition frequency start conversion and rise RA5 output  (RA5 is in output mode).

01 :Acquisition Timer (R/W)
     (16 bits)
       bit(0..15):    set the timing in 100us period. 
                           0 = not permitted
                           1 = not permitted 
			  Minimum = 2 (5K samples/sec)
                          Maximum = 65535  (0.153 samples/sec = 6.55 sec/sample)
       
02: Data count  (Read only)  number of data in buffer
     (8 bits)

03: Data (Read only)   read data buffer (FIFO method)
    (32bits) => 2x 16bits  first words is analog 0 and second is analog 1
    up to  7 data sample could be read at once  (32 bytes of data max on smbus.  7 * 4 + command byte)

    First word:
                       bit 0..9 		Analog 0 A/D value
                       bit 10..15 	0(zero)     

     Second word:
			bit 0..9		Analog 1 A/D value
			bit 10..11 	0(zero)
                        bit 12..14 	Overrun count  0=none 1..6= number of missed conversion   7= too many missed conversion
                        bit 15		Data is Valid   1=valid   0=underrun


 04: Packed Data (Read only)     33% transfer speed gain over command 03
    (24 bits) =>  hold  10bits Analog 0 and analog1 into 24 bits of data  (3 x 8 bits)
     up to 10 data sample could be read at once (32 bytes of data max on smbus.  10*3+ command byte)

                      	bit 0..9		Analog 0 A/D Value
			bit 10..19	Analog 1 A/D Value
			bit 20..22	Overrun count  0=none 1..6= number of missed conversion   7= too many missed conversion
			bit 23		Data is Valid   1=valid   0=underrun

   05:  eeprom address  (Write only) write to flash i2c Address (Default = 0x20)
   (8bits)
    bit 7..0:     I2C Address. (7 bits mode)   117 devices possible.
                      Addresss has to be between (0x3.. 0x77) inclusively
                 N.B. For protection against communication error, The I2C Address will change only if it is Flash (Command  9).
  
   06:  Timer Counter  (Read)
      (32 bits)
        Return number timer tick count since start of TimerMode command
 
  07:  Id & version number  8bits or 32 bits.
              bit 0..7:     return 0x01
              bit 8..15:   return 0xC3
              bit 16..23: major release version 
              bit 24..31: minor release version



   08:  oscillator  tune register (R/W)
     This is to adjust the internal PIC OScillator
    (8 bits signed )   (value between -32 to 31)
     N.B. this is a signed 6 bits register ! Do not use value outside (-32 to 31)




  09: Flash Settings . (8bits). (Write only)
        (16bits)
         value as to be 0xAA55 otherwise the writing is not done

        This will store the Settings structure into the eeprom.

*/

#include <htc.h>
#include <stddef.h>
#define _XTAL_FREQ 32000000

#ifndef BORV_LO
#define BORV_LO BORV_19
#endif

#define SSPIF SSP1IF

__CONFIG(FOSC_INTOSC & WDTE_OFF & PWRTE_OFF & MCLRE_ON & BOREN_OFF & CP_OFF & CPD_OFF & CLKOUTEN_OFF & IESO_OFF & FCMEN_OFF);
__CONFIG(WRT_OFF & PLLEN_ON  & BORV_LO & LVP_OFF);
__IDLOC(0000);

#define IDTAG  0xE7
#define MARKERTAG 0xC3
#define MAJOR_VERSION 1
#define MINOR_VERSION 0




////////////////   eerom storage 

__EEPROM_DATA(0x20,4,0xff,0xff,0xff,0xff,0xff,0xff);  // needs to have 8 byte. This only hold the I2C Address and OSCTUNE for now

// set the __EEPROM_DATA  according to the following structure
typedef struct {
unsigned char  I2C_Address;
char OscTune;
}EepromSettingsStruct;

EepromSettingsStruct Settings;

// I know this is overkill for 2 bytes but this is my generic function
// just in case we add more stuff into Settings

void LoadSettings(void)
{
  unsigned char   idx;
  unsigned char   * pointer = (unsigned char *) &Settings;

  for(idx=0; idx < sizeof(Settings);idx++)
     *(pointer++) = eeprom_read(idx);
}

void SaveSettings(void)
{
  unsigned char   idx;
  unsigned char   * pointer = (unsigned char *) &Settings;

  for(idx=0; idx < sizeof(Settings);idx++)
      eeprom_write(idx, *(pointer++));
}

volatile bit SaveSettingsFlag;                                  // interruption ask to save settings




near unsigned char Command;				// This is the command you want to run from the I2C

////////////  FIFO DATA
#define BUF_SIZE  40
typedef union {
  struct{
          unsigned char lsb;
          unsigned char msb;
 };
          unsigned short word;
}FiFoUnion;

FiFoUnion FiFo_A0[BUF_SIZE]; 			// this is the 16 bits data buffer for analog sample 0
FiFoUnion FiFo_A1[BUF_SIZE]; 			// this is the 16 bits data buffer for analog sample 1

near volatile unsigned char  FirstIn=0;       // this is the First In  index
near volatile unsigned char  FirstOut=0;    // This is the First Out index

near volatile unsigned char  OverrunCount=0;              // This is the number of Overrun. (How many time we can't increase First In index because First Out  is full)

////////////  Timing
near volatile unsigned short  TargetTimer=10000;    // this is the Timer target value from command  1  Default = 10000 (1 sample/sec)
near volatile unsigned short CurrentTimer=10000;     // This  is the current value of the software timer. when it reaches Zero, a conversion start and it is reload with Reg_Timing.

near volatile bit CommandRun;                		     // This is the Run/Stop bit
near volatile bit Analog0Flag;


near volatile unsigned short _tempAnalogValue;   // hold analog channel 0 until we got channel 3 for full transaction storage into fifo.

typedef union{
  unsigned char  byte[4];
  unsigned short word[2];
  unsigned long  dword;
}IntegersStruct;



volatile  IntegersStruct TimerCounter;				// This is the total  number of start conversion in  TimerMode.
volatile  IntegersStruct _TimerCounter;
////////  I2C handling   variable

near volatile union {
 struct{
	unsigned    SingleMode : 1;			 // This is the Trigger mode flag. Command 0 mode 2
        unsigned    TriggerMode : 1;			// This is the  Timer mode flag.  Command 0 mode 3
        unsigned    TimerMode:1;                          // this tell the main program that we have to check the mode.
        unsigned	: 1;
        unsigned	: 1;
        unsigned	: 1;
        unsigned	: 1;
        unsigned	: 1;
      }bits;
   unsigned char byte;
}CommandMode;      
  


near volatile bit   GotCommandFlag=0;	// I2C interrupts flag to specify we have the command bytes. We wont use bit since we don't want to use the same byte to hold the bit
near volatile unsigned char I2CCommand;                // I2C Command
near volatile unsigned char I2CByteCount;			// I2C Byte Data counter. This is use to verify when we should increment First out counter
near volatile  unsigned short I2CShortData;          // temporary variable to hold i2c short value;
near volatile unsigned char _tempFirstIn;            // temporary variable to hold FirstIn Fifo pointer.
//////////  I2C Initialization routine
void I2CInit()
{
SSP1IE=0;
SSP1CON1=0;
TRISA		= 0b00111111;	// ALL INPUT
// i2c init
SSP1ADD  = Settings.I2C_Address << 1;
SSP1CON1 = 0b00110110;
SSP1CON2 = 0b00000001;
SSP1CON3 = 0b00000011;
SSP1STAT = 0b11000000;
SSP1MSK  = 0xff;
SSP1IF=0;
// irq
  SSP1IE =1;
  GIE =1;
  PEIE =1;
CommandMode.byte=1;
CommandRun=0;
}


/////////////  I2C Write byte routine
void I2CWrite(unsigned char value)
{
   while(SSP1STATbits.BF);

    do 
    {
    SSP1CON1bits.WCOL=0;
    SSP1BUF = value;
    if(SSP1CON1bits.WCOL) continue;
//    SSP1CON1bits.CKP=1;
    break;
    }while(1);
} 


void Timer0Init()
{
 TMR0IE=0;  // disable interrupt for now
 PSA =1 ; // not prescaler
 TMR0CS= 0; // clock /4
 }


// delay of 20 us for A/D capacitor charge using timer0
// 32MHZ / 4 = 125ns => 20 us =   160 => 256-160= 96
#define DelayStart() TMR0=96; TMR0IF=0;TMR0IE=1;




void A2DInit()
{
ADCON1= 0b10100011;  // right justified, fosc/32 vref internal fixed.
ADCON0= 0b00000001; // enable a/d
ADIE=0;
ADIF=0;
FVRCON=0b11000010;  // Vref internal 2.048V on ADC
}

void A2DStart()
{
   // Star conversion for Analog 0
 Analog0Flag=0;                  // tell system that we are doing the analog 0 channel
 ADIE=0;                              // clear interrupt flag
 ADIF=0;
 ADON=1;
 ADCON0=1;                      //  channel 0
 DelayStart();                  // get 6us delay to charge cap. using timer0
}




void EnableTrigger()
{
 TRISAbits.TRISA5=1;                      //RA5 on input
 IOCAFbits.IOCAF5=0; 
  IOCAPbits.IOCAP5=1;
  IOCANbits.IOCAN5=0;
  INTCONbits.IOCIE=1;
}

void DisableTrigger()
{
 INTCONbits.IOCIE=0;
IOCANbits.IOCAN5=0;
 IOCAPbits.IOCAP5=0;
IOCAFbits.IOCAF5=0;
}

void EnableTimer()   
{

// we want 100us period so
// clock of 32Mhz give  125 ns   100us / 125ns = 800
// 800 / prescaler of 4 = 200 start count
 RA5=0;
 TRISAbits.TRISA5=0;     //RA5 output mode
RA5=0;
T2CON=0;
 PR2 = 200;   //adjust to 199
TMR2=0;
T2CON= 0b00000101; // Timer on prescaller 4
 // Enable IRQ
TMR2IF=0;
TimerCounter.dword=0;
TMR2IE=1;
OSCTUNE=Settings.OscTune;

}


void DisableTimer()
{
 TRISAbits.TRISA5=1;      
 TMR2IE=0;
 T2CON=0;
 TMR2IF=0;
}




///////   Interrupts I2C handler
void ssp_handlerB(void)
{
    unsigned int i, stat;
    unsigned char data;

        if (SSPOV == 1)
        {
            SSPOV = 0;  //clear overflow
            data = SSPBUF;
        }
        else
        {
            stat = SSPSTAT;
            stat = stat & 0b00101101;
            //find which state we are in
            //state 1 Master just wrote our address
            if ((stat ^ 0b00001001) == 0) //S=1 && RW==0 && DA==0 && BF==1
            {
//                for (i=0; i<BUF_SIZE; ++i) //clear buffer
//                    buf[i] = 0;
                I2CByteCount=0;
                GotCommandFlag=0;
                //ReportAddress = SSPBUF; //read address so we know later if read or write
                data=SSPBUF;
            }
            //state 2 Master just wrote data
            else if ((stat ^ 0b00101001) == 0) //S=1 && RW==0 && DA==1 && BF==1
            {
                data = SSPBUF;
                if(GotCommandFlag)
                 {

                       if(I2CCommand==0)
                         {
                                 //  run mode 
                               if(I2CByteCount==0)
                                 {

                                    CommandRun= (data &1)==1 ? 1 : 0;
                                      if(CommandRun==0)
                                      {
                                          DisableTrigger();
                                          DisableTimer();
                                     }
                                    else
                                     {                                     
					       CurrentTimer= TargetTimer;
                                           FirstIn=0;
                                           FirstOut=0;
                                           OverrunCount=0;
                                    if((data & 4) ==0)
                                    {  // single capture mode
                                        CommandMode.byte = 1;
                                   
                                        TRISA5 = 1;
                                        DisableTrigger();
                                        DisableTimer();
                                        A2DStart();
                                   }
                                  else if((data & 2) == 0)
                                    {  // Trigger mode
                                          CommandMode.byte= 2;
                                           DisableTimer();
                                           EnableTrigger();
                                    }
                                 else
                                   {  // timer mode
                                          CommandMode.byte=4;
                                           DisableTrigger();
                                           EnableTimer();
                                   }
                                  }
                                  }
                         }
                       else if(I2CCommand==1)
                        {  // Timer settings
                                   if(I2CByteCount==0)
                                          I2CShortData= data;
                                   else if(I2CByteCount ==1)
                                     {
                                         I2CShortData += ((unsigned short) data << 8);
                                         if(I2CShortData < 2)
                                         TargetTimer = 2;   // minimum is 5Khz
                                        else
                                         TargetTimer = I2CShortData;
                                     } 
                                   else
                                            I2CByteCount=1;  // limit the byte count since it is a word       
                       }
                        else if(I2CCommand==5)
                            {   // change I2C_channel
                                if(I2CByteCount==0)
                                 if(data > 0x2)
                                   if(data < 0x78)
                                       {
                                         // ok we are allowed to change Adress
                                         // will be only effective if it is save on flash
                                                 Settings.I2C_Address=data;
                                                 // tell main program to save and reload
                                        }
                            } 
                         else if(I2CCommand==8)
                             {  //  Oscillator tune
                                      if(I2CByteCount==0)
                                         {
                                           if((data & 0x20)==0x20)
                                               data |= 0xC0;
                                              else
                                               data &= 0x1F;
                                            Settings.OscTune = data;

                                         }
                             }  


                         else if(I2CCommand==9)
                             {  //  write flash
                                                                             
                                       if(I2CByteCount==0)
                                         {
                                             if(data != 0x55)
                                                    I2CByteCount=2;
                                         }
                                     else if(I2CByteCount==1)
                                         {
                                              if(data == 0xaa)
                                                SaveSettingsFlag=1;
                                         }
                             }  
                    I2CByteCount++;
                 }
              else
               {
                   I2CCommand = data;
                   GotCommandFlag=1;
               } 
        }
     		  //state 3  & 4 Master want to read data 
              else if ((stat & 0b00001100) == 0b00001100)     
//            else if ((stat ^ 0b00001100) == 0) //S=1 && RW==1 && DA==0 && BF==0
//            else if ((stat ^ 0b00101100) == 0) //S=1 && RW==1 && DA==1 && BF==0            
{
                if((stat & 0b00100000)==0)
               {
                I2CByteCount=1;
                //ReportAddress = SSPBUF;
                data=SSPBUF;
               }

                  if(I2CCommand==0)
                         { // reconstruct Control
                             if(I2CByteCount==1)
                              {
                               data = CommandRun;
                               if(CommandMode.bits.TriggerMode==1)
                                  data |= 0x4;
                              else if(CommandMode.bits.TimerMode==1)
                                  data |= 0x7;
                              }
                             else
                                   data=0;
                       }
                  else if(I2CCommand ==1)
                   {// timer
                        if(I2CByteCount==1)
                             data = TargetTimer & 0xff;
                        else if(I2CByteCount == 2)
                             data = TargetTimer >> 8;
                        else 
                               data=0;
                   }   
                  else if(I2CCommand==2)
                   {// number of data in fifo
                          if(I2CByteCount==1)
                              {
                                  if(FirstIn >= FirstOut)
                                      data = FirstIn - FirstOut;
                                  else
                                      data = BUF_SIZE - FirstOut + FirstIn;
                              } 
                           else
                                    data=0;
                   }
                  else                   if(I2CCommand==3)
                    { //32 bit data in fifo

                           if(I2CByteCount==1)
                               {
                                  _tempFirstIn=FirstIn;
                                  if(_tempFirstIn == FirstOut)
                                   data=0;
                                 else
                                   data = FiFo_A0[FirstOut].lsb;
                               }
                           else if(I2CByteCount==2)
                               {
                                    if(_tempFirstIn == FirstOut)
                                     data=0;
                                    else
                                      data = FiFo_A0[FirstOut].msb;
                              }
                           else if(I2CByteCount==3)
                               {
                                    if(_tempFirstIn == FirstOut)
                                     data=0;
                                    else
                                   data = FiFo_A1[FirstOut].lsb;
                              }
                           else if(I2CByteCount==4)
                               {
                                    if(_tempFirstIn == FirstOut)
                                     data=0;
                                    else
                                     {
                                      data = FiFo_A1[FirstOut].msb << 2;  // get overrun and valid stuff
                                      data &= 0xf0;                         
                                      data |= (FiFo_A1[FirstOut].msb) & 0x3; // get bit 8&9
                                      if(CommandMode.bits.SingleMode==1)
                                       FirstOut=1;
                                       else
                                            FirstOut++;
                                      if(FirstOut>=BUF_SIZE) FirstOut=0;                                 
                                      I2CByteCount=0;
                                  }
                              }
                    }   
                  else if(I2CCommand==4)
                    { //24 bit Pack data in fifo
                          if(I2CByteCount==1)
                               {
                                  _tempFirstIn=FirstIn;
                                  if(_tempFirstIn == FirstOut)
                                   data=0;
                                 else
                                   data = FiFo_A0[FirstOut].lsb;
                               }
                           else if(I2CByteCount==2)
                               {
                                    if(_tempFirstIn == FirstOut)
                                     data=0;
                                    else
                                     {
                                      data = (FiFo_A0[FirstOut].msb) & 0x03;
                                      data|=  (FiFo_A1[FirstOut].lsb << 2) & 0xfc;
                                    }
                              }
                           else if(I2CByteCount==3)
                               {
                                    if(_tempFirstIn == FirstOut)
                                     data=0;
                                    else
                                     {
                                      data = (FiFo_A1[FirstOut].word >>6);
                    				if(CommandMode.bits.SingleMode==1)
                                       FirstOut=1;
                                       else
                                      FirstOut++;
                                      if(FirstOut>=BUF_SIZE) FirstOut=0;   
                                      I2CByteCount=0;
                                  }
                              }
                    }   

 

                  else if(I2CCommand==6)
                     {
                        if(I2CByteCount==1)
                         {
                           _TimerCounter.dword=TimerCounter.dword;
                           data = _TimerCounter.byte[0];
                         }
                        else if(I2CByteCount<5)
                           data = _TimerCounter.byte[I2CByteCount-1];
                        else
                          data=0;
                    }
                 else if(I2CCommand==7)
                  {// version
	
 						if(I2CByteCount==1)
                         {
                           data = IDTAG;
                         }  
                        else if(I2CByteCount ==2)
                          {
                           data =  MARKERTAG;
                          }
                        else if(I2CByteCount==3)
                          {
                           data = MAJOR_VERSION;
                          } 
                        else if(I2CByteCount==4)
                          {
                           data = MINOR_VERSION;
                          }
                        else
                           data=0;
                 }
                else if(I2CCommand==8)
                    {  // OSC TUNE
                        if(I2CByteCount==1)
                             data = Settings.OscTune;
                       else
                          data= 0;
                  }
                I2CByteCount++;
//                WCOL = 0;   //clear write collision flag
//                SSPBUF = data ; //data to send
                 I2CWrite(data);
            }
            //state 5 Master sends NACK to end message
//            else if ((stat ^ 0b00101000) == 0) //S=1 && RW==0 && DA==1 && BF==0
//            {
//                /dat = SSPBUF;
//            }
            else //undefined, clear buffer
            {   WCOL=0;
                data = SSPBUF;
            }
        }
    CKP = 1; //release the clk
    SSPIF = 0; //clear interupt flag
    }
  










static void interrupt isr(void){
volatile near unsigned char _temp;


////////////////////////////////////////// timer0 interrupt 
// Timer 0 is use to delay A/D start of conversion 
if(TMR0IE==1) 
if(TMR0IF==1)
  {
     TMR0IE=0;   	// turn interrupt off
     TMR0IF=0; 
    ADIF=0;
     ADIE=1;
     ADGO=1;        // start A/D
  }

////////////////////////////////////////////  Timer2 interrupt
 if(TMR2IE==1)
 if(TMR2IF==1)
  {
    TMR2IF=0;

    if(CommandMode.bits.TimerMode ==1)
      {
          if(CurrentTimer >1)
                CurrentTimer--;
          else
            {
               RA5=1;
               A2DStart();
               CurrentTimer=TargetTimer;
               TimerCounter.dword++;
            }
      }
   }        

//////////////////////////////////////////  I2C Interrupt
  if(SSP1IF==1)
   {
     ssp_handlerB();
  }
 

/////////////////////////////////////////////  End of A/D conversion Interrupt
// A/D   will convert channel 0 and channel 3
// there is a delay between channel 0 and 3.    12 TAD + Timer0 interrupt + code to deal with interrupt     (12 + 6 us + .. )
  if(ADIE==1)  
  if(ADIF==1)
  {// A/D CONVERSION
         if(Analog0Flag==0)
        {
          ADCON0= 0b0001101;            // select  analog channel 3
          DelayStart();			//use timer0 interrupt for 8us delay
          if(CommandMode.bits.TimerMode==1)   RA5=0; //  ok toggle off RA5
           _tempAnalogValue= ADRES;  // store temporary the Analog 0 value
        }
   
     else
          {
             _temp=FirstIn;
             _temp++;
             if(_temp==FirstOut)
               {
                OverrunCount++;
               if(OverrunCount>7)
                     OverrunCount=7;
                
               }
             else
              {
                 FiFo_A0[FirstIn].word= _tempAnalogValue;                                         // store analog 0 value
              _tempAnalogValue= ADRES;		  					   // retreive analog 3
                _tempAnalogValue |= (unsigned short) OverrunCount << 12; //add the overrun count
                 FiFo_A1[FirstIn].word= _tempAnalogValue | 0x2000;                         // store value and set it valid
                  OverrunCount=0;
                 FirstIn++;
                 if(FirstIn>=BUF_SIZE) FirstIn=0;
              }
                 ADCON0=0;
                 ADIE=0;                 
           }
    Analog0Flag=1;
    ADIF=0;
  }

///////////////////////////////////////////  Edge detection interrupt
 if(IOCAF5==1)
  {
          IOCAF5=0;
          A2DStart();
   }

}


// ************************************   MAIN **********************************************************
void main(void){

 	unsigned char  Counter=0; 
 
	OSCCON		= 0b11110000;	// 32MHz
	OPTION_REG	= 0b00000011;	// pullups on, TMR0 @ Fosc/4/16 ( need to interrupt on every 80 clock 16*5)
	ANSELA		= 0b10001;	    // no analog pins
	PORTA   		= 0b00000000;	
	WPUA			= 0b00100000;	// pull-up ON  RA5
	TRISA			= 0b00011111;	// ALL INPUT  RA5 OUTPUT
	VREGCON		= 0b00000010;	// lower power sleep please
    INTCON			= 0b00000000;	// no interrupt  


// get the I2C address from the eeprom
  LoadSettings();

CommandRun=0;

Timer0Init();		// TIMER0 is used to delay A/D conversion needed for capacitor charge.
A2DInit();		//  Set A/D with  internal 2.048 Volt Reference. Range is then 0..2.048V
I2CInit();            // Enable I2C services.

   while(1){

     if(SaveSettingsFlag==1)   // Do we need to save settings in eeprom;
           {
                SSP1ADD  = Settings.I2C_Address << 1;
                SaveSettings();
                SaveSettingsFlag=0;
           } 
     }
}
