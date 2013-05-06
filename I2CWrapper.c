#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/input.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <string.h>


////////////////////////////////////   I2CWrapperOpen
//
//    Open I2C I/O
//
//    Inputs,
//
//    BUS :  select  bus  (Rpi is 0 or 1)
//    SlaveAddress:  between 0x3 .. 0x77
//
//    Return,
//
//    IO handle for  open()
//

int ExitOnFail=1;
int DisplayFailMessage=1;

void FailMessage(char *msg)
{
  if(DisplayFailMessage)
     {
       fprintf(stderr,msg);
       fflush(stderr);
     }

   if(ExitOnFail)
      exit(0);
}


int I2CWrapperOpen(int BUS, int SlaveAddress)
    {
	  int handle;
	  char I2C_dev[256];

	  if(BUS < 0) return -1;
	  if(BUS > 1) return -1;

	  sprintf(I2C_dev,"/dev/i2c-%d", BUS);

	  handle = open(I2C_dev, O_RDWR);
	  if(handle < 0)
	   {
	     FailMessage("Failed to open the i2c bus \n");
	    return -1;
	   }
	  if(I2CWrapperSlaveAddress(handle,SlaveAddress) < 0)
	   {
	      close(handle);
		  return -2;
	   }
	   return handle;
	}





////////////////////////////////////   I2CWrapperSlaveAddress
//
//    Specify which Slave device you want to talk to
//
//     inputs,
//
//     handle:   IO handle
//     SlaveAddress:
//
//    Return integer
//
//    0  ok
//    < 0 error
//

int I2CWrapperSlaveAddress(int handle, int SlaveAddress)
    {
       if(SlaveAddress < 3) return -1;
       if(SlaveAddress > 0x77)return -1;

	   if (ioctl(handle, I2C_SLAVE, SlaveAddress) < 0)
	    {
               FailMessage("Failed to acquire bus access and/or talk to slave.\n");
		return -1;
		}
        return 0; // ok
    }

////////////////////////////////////   I2CWrapperReadBlock
//
//    Read N byte from the I2C (maximum of 31 bytes possible)
//
//     inputs,
//
//     handle:   IO handle
//     cmd:  Specify which is the device command (more or less the device function or register)
//     size:     Number of bytes to read
//     array:    the pointer array
//
//    Return integer
//
//    number of byte reed.
//    < 0 error
//
int I2CWrapperReadBlock(int handle, unsigned char cmd, unsigned char  size,  void * array)
{
struct i2c_smbus_ioctl_data  blk;
union i2c_smbus_data i2cdata;

  blk.read_write=1;
  blk.command=cmd;
  blk.size= I2C_SMBUS_I2C_BLOCK_DATA;
  blk.data=&i2cdata;
  i2cdata.block[0]=size;

  if(ioctl(handle,I2C_SMBUS,&blk)<0){
    FailMessage("Unable to Read  I2C data\n");
    return -1;
    }

 memcpy(array,&i2cdata.block[1],size);
 return   i2cdata.block[0];
}

////////////////////////////////////   I2CWrapperReadByte
//
//    Read 1 byte  from the I2C
//
//     inputs,
//
//     handle:   IO handle
//     cmd:  Specify which is the device command (more or less the device function or register)
//
//    Return  byte data (in int),  if <0 error
//
//    Check I2CWrapperErrorFlag for error
//
int  I2CWrapperReadByte(int handle, unsigned char cmd)
{
  struct i2c_smbus_ioctl_data  blk;
  union i2c_smbus_data i2cdata;

  blk.read_write=1;
  blk.command=cmd;
  blk.size=I2C_SMBUS_BYTE_DATA;
  blk.data=&i2cdata;

  if(ioctl(handle,I2C_SMBUS,&blk)<0){
    FailMessage("Unable to Read  I2C data\n");
    return -1;
    }
  return   ((int) i2cdata.byte);
}


////////////////////////////////////   I2CWrapperWriteByte
//
//    Write 1 byte  to the I2C device
//
//     inputs,
//
//     handle:   IO handle
//     cmd:  Specify which is the device command (more or less the device function or register)
//     value:    byte value
//    Return   number of byte written if <0 error
//
//    Check I2CWrapperErrorFlag for error
//
int I2CWrapperWriteByte(int handle,unsigned char cmd, unsigned char value)
{
 struct i2c_smbus_ioctl_data  blk;
 union i2c_smbus_data i2cdata;

 i2cdata.byte=value;
 blk.read_write=0;
 blk.command=cmd;
 blk.size=I2C_SMBUS_BYTE_DATA;
 blk.data= &i2cdata;

  if(ioctl(handle,I2C_SMBUS,&blk)<0){
    FailMessage("Unable to write I2C byte data\n");
    return -1;
    }
 return 1;
}


////////////////////////////////////   I2CWrapperReadWord
//
//    Read 2 bytes  from the I2C
//
//     inputs,
//
//     handle:   IO handle
//     cmd:  Specify which is the device command (more or less the device function or register)
//
//    Return  word data  in ( short long format), if < 0 error
//
//    Check I2CWrapperErrorFlag for error
//
int   I2CWrapperReadWord(int handle, unsigned char cmd)
{
 struct i2c_smbus_ioctl_data  blk;
 union i2c_smbus_data i2cdata;


 blk.read_write=1;
 blk.command=cmd;
 blk.size=I2C_SMBUS_WORD_DATA;
 blk.data=&i2cdata;

 if(ioctl(handle,I2C_SMBUS,&blk)<0){
    FailMessage("Unable to Read  I2C data\n");
    return -1;
    }
 return  (int) i2cdata.word;
}


////////////////////////////////////   I2CWrapperWriteWord
//
//    Write 2 bytes  to the I2C device
//
//     inputs,
//
//     handle:   IO handle
//     cmd:  Specify which is the device command (more or less the device function or register)
//     value:    byte value
//    Return   number of byte written if <0 error
//
//    Check I2CWrapperErrorFlag for error
//
int I2CWrapperWriteWord(int handle,unsigned char cmd, unsigned short value)
{
 struct i2c_smbus_ioctl_data  blk;
  union i2c_smbus_data i2cdata;

  i2cdata.word=value;
  blk.read_write=0;
  blk.command=cmd;
  blk.size=I2C_SMBUS_WORD_DATA;
  blk.data= &i2cdata;

  if(ioctl(handle,I2C_SMBUS,&blk)<0){
    FailMessage("Unable to write I2C data\n");
    return -1;
    }
 return 2;
}


