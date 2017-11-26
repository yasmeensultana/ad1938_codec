
/* AD1938 Audio Codec  control library
*
* Copyright (c) 2017, Yasmeen Sultana
*
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice, development funding notice, and this permission
* notice shall be included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/

#ifndef _CONTROL_AD1938_H_
#define _CONTROL_AD1938_H_

#include "AudioControl.h"

/*SPI */
#define AD1938_SPI_CLK_FREQ      1000000
#define AD1938_SPI_CHIP_SEL      7  /*teensy 3.6 gpio*/
#define AD1938_SPI_SCK           14 /*teensy 3.6 gpio*/
#define AD1938_RESET             17 /*teensy 3.6 gpio*/

/*sampling rate*/
typedef enum 
{
  FS_32000,
  FS_44100,
  FS_48000,
  FS_64000,
  FS_88200,
  FS_96000,
  FS_128000,
  FS_176400,
  FS_192000,  
}Te_samplingRate;

/*number of bits per sample*/
typedef enum 
{
  BITS_16,
  BITS_20, 
  BITS_24,
}Te_bitsPerSample;

/*I2s clock mode*/
typedef enum 
{
  AD1938_I2S_SLAVE,
  AD1938_I2S_MASTER, 
}Te_i2sClockMode;

/*I2s number of channels */
typedef enum 
{
  I2S_STEREO_2CH,
  I2S_TDM_8CH, 
  I2S_TDM_16CH
}Te_i2sNumChannels;


class AudioControlAD1938 : public AudioControl
{
public:
	bool spiInit(int clatch,int reset,int cout,int cin,int cclk);
	bool config(Te_samplingRate sampleRate,Te_bitsPerSample wordLen,Te_i2sNumChannels numChannels, Te_i2sClockMode mode);
	bool enable(void);
	bool disable(void);
	bool volume(float volume);
	bool adcMute(bool mute);
	bool dacMute(bool mute);
	void readAllreg(void);
	bool inputSelect(int n) {
		return (n == 0) ? true : false;
	}
	bool inputLevel( float volume) {
		 return false;
	}
private:
	int ad1938_clatch;
	int ad1938_reset;
	int ad1938_cout;
	int ad1938_cin;
	int ad1938_cclk;
	Te_i2sClockMode   i2sMode;
	Te_bitsPerSample  wordLen;
	Te_i2sNumChannels numChannels;
	Te_samplingRate   samplingRate;
	
protected:
	bool spi_write_reg(unsigned char reg, unsigned char val);
	unsigned char spi_read_reg(unsigned char reg);
	bool isPllLocked();
	bool dacVolume(int dac_num,int volume);
	
};



#endif // !_CONTROL_AD1938_H_
