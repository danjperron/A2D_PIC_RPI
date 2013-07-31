#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>
#include "I2CWrapper.h"
#include "I2C_A2D.h"


////////////////////////////////////////////
//
//    program to verify functionnality  of a PIC using A/D 
//    on raspberry pi I2C bus
//    to compile
//    
//     gcc -o A2DTest  A2DTest.c I2CWrapper.c
//
//
//   programmer : Daniel Perron
//   date       : May 3, 2013
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


struct timeval start, end, total ;
double elapse;


void DisplayVersion(int handle)
{
 A2D_Version Version;
 if(A2DReadVersion(handle,&Version)>=0)
   printf("Id = %x\nTag=%X\nVersion=%d.%d\n", Version.Id,Version.Tag,Version.Major,Version.Minor);
  else
   printf("Unable to read version\n");
 fflush(stdout);
}


void TestSingleShot(int handle)
{
  int loop;
  UnpackAnalog  unpackdata;
  
printf("\n------------  Test SingleShot mode\n");

 A2DMode(handle,A2D_MODE_OFF);		// STOP A/D
 
   for(loop=0 ;loop< 3 ;loop++)
   {
     // start conversion
     printf("\nStart single shot\n");

	 A2DMode(handle,A2D_MODE_SINGLE);  // single shot conversion
     sleep(1);
     // read A/d
     printf("%d data waiting\n ",A2DReadDataCount(handle)); // get number of point in buffer
	 A2DReadData(handle,1,&unpackdata);						// read one data point
     printf("A0 : %4d      A1 : %4d   Ovr : %d  Valid : %d  \n", unpackdata.A0,\
           unpackdata.A1, unpackdata.Overrun, unpackdata.Valid);
     printf("%d data waiting\n",A2DReadDataCount(handle)); // get number of point in buffer
    fflush(stdout);
   }
}

void TestSingleShotPack(int handle)
{
 int loop;
 PackAnalog  packdata;
 printf("\n--------------- Read A/D using PackData Command\n");fflush(stdout);
 for(loop=0 ;loop< 3 ;loop++)
   {
     // start conversion
  printf("\nStart single shot\n");

    A2DMode(handle,A2D_MODE_SINGLE);  // single shot conversion
     sleep(1);
// read A/d
    printf("%d data waiting\n ",A2DReadDataCount(handle)); // get number of point in buffer
    A2DReadPackData(handle,1,&packdata);						// read one data point
    printf("A0 : %4d      A1 : %4d   Ovr : %d  Valid : %d  \n", packdata.A0,\
           packdata.A1, packdata.Overrun,packdata.Valid);
    printf("%d data waiting\n ",A2DReadDataCount(handle)); // get number of point in buffer		   
    fflush(stdout);
   }
 }

   void TestSingleShotSpeed(int handle)
{
  int loop;
   PackAnalog  packdata;
   UnpackAnalog  unpackdata;
printf("\n--------  Find single shot Fastest speed \n");fflush(stdout);

  gettimeofday (&start, NULL) ;
  for(loop=0;loop<2000;loop++)
   {
    A2DMode(handle,A2D_MODE_SINGLE);  // single shot conversion
    A2DReadData(handle,1,&unpackdata);						// read one data point
   }
  gettimeofday (&end, NULL) ;
  timersub (&end, &start, &total) ;

#define TIMEVAL_CV(A) ((double) A.tv_sec + ((double)A.tv_usec * 1.0e-6))
  elapse = TIMEVAL_CV(total);
  printf("Data block  average samples/sec = %f\n",2000/elapse);

  gettimeofday (&start, NULL) ;
  int underrun=0;
  for(loop=0;loop<2000;loop++)
     {
      A2DMode(handle,A2D_MODE_SINGLE);  // single shot conversion
      A2DReadPackData(handle,1,&packdata);						// read one data point
     }
   gettimeofday (&end, NULL) ;
   timersub (&end, &start, &total) ;
 elapse = TIMEVAL_CV(total);
 printf("Pack Data block  average samples/sec = %f\n",2000/elapse);
}




  void TestTimerMode(int handle)
{
  int loop;
  UnpackAnalog unpackanalog[40];  // 40 samples possible in buffer

  unsigned int nsample,totsample,tempsample,isample;
  double delta;
  printf("\n--------------- Test timer mode\n");

  printf("Select  1000 samples/sec \n");
  printf("Timer= 10 => 1000 samples/sec\n");

  A2DMode(handle,A2D_MODE_OFF);
  A2DTimer(handle,10);

  A2DMode(handle,A2D_MODE_TIMER); // start timer mode

  gettimeofday (&start, NULL) ;
  totsample=0;
  nsample=0;
  delta=0;
   do {
        tempsample = A2DReadDataCount(handle);
        isample=0;

        if(tempsample > 1)
         {
            if(tempsample>7)
                isample=7;
               else
                isample=tempsample;
			A2DReadData(handle,isample,unpackanalog);  // max for isample is 7 ( 7 * 4==28) < 32
         }
        nsample+=isample;
        gettimeofday(&end,NULL);
        timersub(&end,&start,&total);
        elapse = TIMEVAL_CV(total);
        if((elapse - delta)> 1.0)
        {
          totsample+=nsample;
          printf("%.1f sec count=%d\n",elapse,totsample);fflush(stdout);

          delta=elapse;
          nsample=0;
     }
  } while (elapse  < 10.5);

A2DMode(handle,A2D_MODE_OFF); 
}



double  CalculateTimerRate(int handle,signed char osc)
{
   //start 10000 sample/sec
   // run a little , record value
   // run for 2 second record value

   // return  Samples/sec
     unsigned short  Count1,Count2;
     double rate;

     A2DSetOscTune(handle,osc);  // set tuning frequency clock

     A2DMode(handle,A2D_MODE_OFF);
     A2DTimer(handle,2); // set 5K/sec
     A2DMode(handle,A2D_MODE_TIMER); //start conversion using timer

     usleep(500000);   // wait one se
     gettimeofday(&start,NULL);
     Count1= A2DReadTimerCounterWord(handle);
     sleep(2);
     gettimeofday(&end,NULL);
     Count2= A2DReadTimerCounterWord(handle);
     A2DMode(handle,A2D_MODE_OFF);


     timersub(&end,&start,&total);
     elapse = TIMEVAL_CV(total); 
 
     rate =  ((double) (Count2 - Count1)) / elapse;

     printf("OscTune=%d  Rate=%.1f Sample/sec count =%d elapse=%f\n",osc, rate,(Count2-Count1),elapse);fflush(stdout);
    return rate;

 }

void AdjustOscillator(int handle)
{

  signed char Best;
  double  BestDelta;
  double  CurrentDelta;
  signed char osc;
  double Target=5000.0;
  // scan  the current settings
  //  go down or up until value became worst
  // store the best Value

  printf("\n--------------- Adjust Oscillator\n");


  Best =  A2DReadOscTune(handle);
  osc = Best;
  A2DSetOscTune(handle,Best);
  BestDelta =  CalculateTimerRate(handle,osc) - Target;

  if(BestDelta  > 0.0) 
   {
       do
        {
          // let's reduce  the frequency
	  osc--;
          if(osc < (-32))  break; // can't go lower than -32
          CurrentDelta = CalculateTimerRate(handle,osc) - Target;
          if(fabs(BestDelta) < fabs(CurrentDelta)) break; // not better! just quit!
          BestDelta=CurrentDelta;
          Best=osc;
        }while(1);
  }
  else
  {
       do
        {
          // let's reduce  the frequency
          osc++;
          if(osc > 31)  break; // can't go  higher than 31
          CurrentDelta = CalculateTimerRate(handle,osc) - Target;
          if(fabs(BestDelta) < fabs(CurrentDelta)) break; // not better! just quit!
          BestDelta=CurrentDelta;
          Best=osc;
        }while(1);

  }

printf("Best  osctune = %d    frequency %.0f %+.3f%%\n", Best,Target, BestDelta * 100.0 / 5000.0);
fflush(stdout);

       A2DSetOscTune(handle,Best);
       A2DFlashEeprom(handle); 


}


void  TestMaxDataTransfer(int handle)
{
// force 10K sample per second
// record only  data transfer
  int loop;
  UnpackAnalog unpackanalog[40];  // 40 samples possible in buffer
  double Rate;
  unsigned int totsample,tempsample;

  printf("\n--------------  Test Max Data Transfer using buffer \n");
  printf("Select  5K samples/sec \n");

  A2DMode(handle,A2D_MODE_OFF);
  A2DTimer(handle,2);

  A2DMode(handle,A2D_MODE_TIMER); // start timer mode
  gettimeofday (&start, NULL) ;
  totsample=0;
   do {
        tempsample = A2DReadDataCount(handle);

        if(tempsample > 1)
         {
            if(tempsample>7)
              {
                 A2DReadData(handle,7,unpackanalog);     // max for isample is 7 ( 7 * 4==28) < 32
                 totsample+=7;
                  break;
              }
               else
              {
                 A2DReadData(handle,tempsample,unpackanalog);
                 totsample+=tempsample;
              }
         }
      }while(totsample<10000);
       gettimeofday(&end,NULL);
       timersub(&end,&start,&total);
       elapse = TIMEVAL_CV(total);

       A2DMode(handle,A2D_MODE_OFF);

      Rate =  (double) totsample / elapse;
      printf("Buffer Data block  average samples/sec = %f\n",Rate);
}

void  TestMaxPackDataTransfer(int handle)
{
// force 10K sample per second
// record only  data transfer
  int loop;
  PackAnalog packanalog[40];  // 40 samples possible in buffer
  double Rate;
  unsigned int totsample,tempsample;

  printf("\n--------------  Test Max Pack Data Transfer using buffer \n");
  printf("Select  5K samples/sec \n");

  A2DMode(handle,A2D_MODE_OFF);
  A2DTimer(handle,2);

  A2DMode(handle,A2D_MODE_TIMER); // start timer mode

  gettimeofday (&start, NULL) ;
  totsample=0;
   do {
        tempsample = A2DReadDataCount(handle);

        if(tempsample > 1)
         {

            if(tempsample>10)
              {



                 A2DReadPackData(handle,10,packanalog);     // max for packdata  is 10 ( 10 * 3 ==30) < 32
                 totsample+=10;
                 break;
              }
               else
              {
                 A2DReadPackData(handle,tempsample,packanalog);
                totsample+=tempsample;
             }
         }
      }while(totsample<10000);
       gettimeofday(&end,NULL);
       timersub(&end,&start,&total);
       elapse = TIMEVAL_CV(total);

       A2DMode(handle,A2D_MODE_OFF);

      Rate =  (double) totsample / elapse;
      printf("Buffer Pack Data block  average samples/sec = %f\n",Rate);
}

void  TestTriggerMode(int handle)
{
//  Use 2 Pics
// First PIC at 0x20 in timer mode
// second PIC at 0x21 in trigger mode

  int loop;
  UnpackAnalog unpackanalog[40];  // 40 samples possible in buffer


  unsigned int nsample,totsample,tempsample,isample;
  double delta;
  printf("\n--------------- Test Trigger mode\n");

  printf("Select  1000 samples/sec on PIC at 0x20\n");
  printf("Timer= 10 => 1000 samples/sec\n");

  A2DMode(handle,A2D_MODE_OFF);
  A2DTimer(handle,10);


  I2CWrapperSlaveAddress(handle,0x21); // change I2C  device. Use the one at 0x21

  A2DMode(handle,A2D_MODE_TRIGGER);  // start  trigger mode


  I2CWrapperSlaveAddress(handle,0x20); // change I2C  device to 0x20
  A2DMode(handle,A2D_MODE_TIMER); // start timer mode at 1Khz
  I2CWrapperSlaveAddress(handle,0x21); // change I2C  device to 0x21


  gettimeofday (&start, NULL) ;
  totsample=0;
  nsample=0;
  delta=0;
   do {
        tempsample = A2DReadDataCount(handle);
        isample=0;

        if(tempsample > 1)
         {
            if(tempsample>6)
                isample=7;
               else
                isample=tempsample;
                        A2DReadData(handle,isample,unpackanalog);  // max for isample is 7 ( 7 * 4==28) < 32
         }
        nsample+=isample;
        gettimeofday(&end,NULL);
        timersub(&end,&start,&total);
        elapse = TIMEVAL_CV(total);
        if((elapse - delta)> 1.0)
        {
          totsample+=nsample;
          printf("%.1f sec count=%d\n",elapse,totsample);fflush(stdout);

          delta=elapse;
          nsample=0;
     }
  } while (elapse  < 10.5);

A2DMode(handle,A2D_MODE_OFF);

I2CWrapperSlaveAddress(handle,0x20); // but main i2c device back
A2DMode(handle,A2D_MODE_OFF);
}




int main(void)
{
   int i2c_handle;
   unsigned char AD_address=0x20;

   const BUS = 1;
   int I2C_Current_Slave_Adress=0x20;

    i2c_handle = I2CWrapperOpen(BUS,I2C_Current_Slave_Adress);
	if(i2c_handle <0) return -1;

    printf("I2C Address= 0x%02X\n",AD_address);fflush(stdout);

   DisplayVersion(i2c_handle);
   TestSingleShot(i2c_handle);
   TestSingleShotPack(i2c_handle);
//   TestSingleShotSpeed(i2c_handle);
   AdjustOscillator(i2c_handle);
   TestTimerMode(i2c_handle);
//   TestMaxDataTransfer(i2c_handle);
//   TestMaxPackDataTransfer(i2c_handle);
//   TestTriggerMode(i2c_handle);
   close(i2c_handle);
return 0;
}
