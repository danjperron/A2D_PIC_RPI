A2D_PIC_RPI
===========

  A small PIC12F1840   will be use to get  2 channels 10 bit analog converter using the  i2c smbus.
This is to be used with the Raspberry Pi board.

  Up to 5K samples per seconds  is possible. Each sample conversion includes  2 channels.


  The I2C bus permits to use  more than one device. Up to 117 in theory.
  This way it is easy to implement more Analog channels by adding more PIC cpu with a different I2C address.
  A special command allow to change the I2C address and store it into the build-in eeprom.


   Three modes are available,

    -Single shot 
      In this mode, a command is send to start a conversion

    -Timer mode
     A special timer will be use to start the conversionA2 on each n * 100us time slot. This is a continuous mode.

    -Trigger mode
      On each rising of the port RA5 an conversion is done. This could be use with an other PIC to create a
      synchronisation between peripherals.


   Files Information

   Main CPU program
    - RpiA2D.c        This is the PIC program written in C.
    - RpiA2D.hex      This is the Hex file needed to burn program into the cpu.
  
      
   Test program
    - A2DTest.c       This is the test program to check and demonstrate the function in C .
    - I2CWrapper.c    This is the functions wrapper to comunicate using I2C needed in A2DTest.c .
    - I2CWrapper.h    This is the header of I2CWrapper.c
    - I2C_A2D.h       This is the header definition for the A/D converter communication protocol.

    - AdTest.py       This is the test program written in python to demonstrate how to use it.

   Schematic
    - RpiA2D.png      This is the schematic on how to connect on cpu.
    - RpiA2D_2.png    This is the schematic on how to connect multiple cpus.


    

Analog converter  using PIC processeur using I2C bus
