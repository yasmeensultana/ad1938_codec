

//AD1938 codec configuration using Teensy spi drivers
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>

#include"control_ad1938.h"

/*
The daisy chain connection of two ad1938 and Teensy are as follows
AD1938 (slave)->AD1938(master)->Teensy(slave)
*/


int clatch_slave =6;//ad1938 slave spi latch
int clatch =7;       //ad1938 master spi latch
int cout=12;         //spi miso
int cin =11;         //spi  mosi
int cclk =14;        //spi clock
int reset_pin_slave =16; //ad1938 slave reset pin
int reset_pin =17;        //ad1938 master reset pin

AudioControlAD1938       ad1938master;
AudioControlAD1938       ad1938slave;

int main(void)
{
  /* Reset and initialize the spi drivers */
  ad1938master.spiInit( clatch, reset_pin, cout, cin, cclk);
  /*configure daisy chain mode for 16 channels*/
  ad1938master.config(FS_48000,BITS_16,I2S_TDM_16CH,AD1938_I2S_MASTER);
 
  ad1938master.volume(1); /* (0.0 to 1.0)0 mute, 1 full volume*/
  ad1938master.enable();
  
  
  /* Reset and initialize the spi drivers */
  ad1938slave.spiInit( clatch_slave, reset_pin_slave, cout, cin, cclk);
  ad1938slave.config(FS_48000,BITS_16,I2S_TDM_16CH,AD1938_I2S_SLAVE);
  
  ad1938slave.volume(1);
  ad1938slave.enable();
}
