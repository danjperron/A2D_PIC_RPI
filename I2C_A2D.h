#pragma once


typedef struct{
  unsigned short A0 :10;
  unsigned short A1 :10;
  unsigned char  Overrun :3;
  unsigned char  Valid :1;
}__attribute__((packed)) PackAnalog;

typedef struct{
   unsigned short A0 :10;
   unsigned char  z0 :6;
   unsigned short A1 :10;
   unsigned char  z1 :2;
   unsigned char  Overrun :3;
   unsigned char  Valid :1;
}__attribute__((packed)) UnpackAnalog;

typedef struct  {
 unsigned char Id;
 unsigned char Tag;
 unsigned char Major;
 unsigned char Minor;
}__attribute__((packed))A2D_Version;


#define A2D_CMD_MODE		0
#define A2D_CMD_TIMER		1
#define A2D_CMD_DATA_NUMBER 	2
#define A2D_CMD_READ_DATA 	3
#define A2D_CMD_READ_PACK_DATA 	4
#define A2D_CMD_SLAVE_ADDRESS 	5
#define A2D_CMD_TIMER_COUNTER	6
#define A2D_CMD_VERSION		7
#define A2D_CMD_OSC_TUNE	8
#define A2D_CMD_FLASH_SETTINGS  9

#define A2D_MODE_OFF		0
#define A2D_MODE_SINGLE		3
#define A2D_MODE_TRIGGER     	5
#define A2D_MODE_TIMER		7


#define A2DMode(HDL,MD) 		I2CWrapperWriteByte(HDL,A2D_CMD_MODE,MD)
#define A2DReadVersion(HDL,VN) 		I2CWrapperReadBlock(HDL,A2D_CMD_VERSION,sizeof(A2D_Version),VN)
#define A2DReadDataCount(HDL)  		I2CWrapperReadByte(HDL,A2D_CMD_DATA_NUMBER)
#define A2DReadData(HDL,NDATA,ARRAY) 	I2CWrapperReadBlock(HDL,A2D_CMD_READ_DATA, NDATA * 4, ARRAY)
#define A2DReadPackData(HDL,NDATA,ARRAY) I2CWrapperReadBlock(HDL,A2D_CMD_READ_PACK_DATA, NDATA * 3, ARRAY)
#define A2DTimer(HDL,VALUE)		I2CWrapperWriteWord(HDL,A2D_CMD_TIMER,VALUE)
#define A2DReadTimerCounter(HDL,ARRAY) 	I2CWrapperReadBlock(HDL,A2D_CMD_TIMER_COUNTER,4,ARRAY)
#define A2DReadTimerCounterWord(HDL)    I2CWrapperReadWord(HDL,A2D_CMD_TIMER_COUNTER)
#define A2DFlashEeprom(HDL)		I2CWrapperWriteWord(HDL,A2D_CMD_FLASH_SETTINGS,0xAA55)
#define A2DSetSlaveAddress(HDL,VALUE)  	I2CWrapperWriteByte(HDL,A2D_CMD_SLAVE_ADDRESS,VALUE);A2DFlashEeprom(HDL)
#define A2DReadOscTune(HDL)            (char)I2CWrapperReadByte(HDL,A2D_CMD_OSC_TUNE)
#define A2DSetOscTune(HDL,VALUE)	I2CWrapperWriteByte(HDL,A2D_CMD_OSC_TUNE,(unsigned char)VALUE)
