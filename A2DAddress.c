#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>
#include "I2CWrapper.h"
#include "I2C_A2D.h"


////////////////////////////////////////////
//
//    program to set I2C slave address
//
//     usage  A2DAdress  Current_address new_address
//
//  to compile  gcc -o A2DAddress  A2DAddress.c I2CWrapper.c
//
////////////////////////////////////  GPL LICENSE ///////////////////////////////////

/*

The MIT License (MIT)

Copyright (c) 2013 Daniel Perron

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


int ValidateAddress(int Address)
{

    if((Address < 3) || ( Address > 0x77)) return 0;
    return 1;
}


int ValidateDevice(int handle)
{
   //read  IdTAG  check for 0xe7;

   if(I2CWrapperReadByte(handle,7)== 0xe7) return(1);
   return(0);
}


int main(int argc , char * argv[])
{
   int i2c_handle;

   int  CurrentAddress;
   int  NewAddress;
   int  rcode;

   const BUS = 1;

   if(argc !=3)
    {

      printf("Usage:\n         A2DAdress  [Current Address]  [New Address]\n");
      return(-1);
    }


       if(strstr(argv[1],"0x")==NULL)
          CurrentAddress=atoi(argv[1]);
        else
          CurrentAddress=strtoul(argv[1],0,16);

       if(strstr(argv[1],"0x")==NULL)
         NewAddress=atoi(argv[2]);
        else
         NewAddress=strtoul(argv[2],0,16);


    if(!ValidateAddress(CurrentAddress))
     {
       fprintf(stderr,"Current I2C Address=%X is  invalid!\n",CurrentAddress);
       return (-1);
     }

    if(!ValidateAddress(CurrentAddress))
     {
       fprintf(stderr,"New I2C Address=%X is  invalid!\n",CurrentAddress);
       return(-1);
     }



  i2c_handle = I2CWrapperOpen(BUS,CurrentAddress);

    if(i2c_handle <0)
     {
       fprintf(stderr,"Unable to open I2C smbus\n");
       return(-1);
     }


    if(!ValidateDevice(i2c_handle))
     {
      fprintf(stderr,"Slave device bot found at  %02X is invalid",CurrentAddress);
      return(-1); 
     }


      A2DSetSlaveAddress(i2c_handle,NewAddress);  // change address  and flash
    
      sleep(1);// wait 1 second

      I2CWrapperSlaveAddress(i2c_handle,NewAddress); //change I2C Slave device communication

     if(!ValidateDevice(i2c_handle))
     {
      fprintf(stderr,"Unable to find new slave Address at %02X",NewAddress);
      return(-1);
     }

   printf(" Device at %X is now at %X\n",CurrentAddress,NewAddress);

return 0;
}
