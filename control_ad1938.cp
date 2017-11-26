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
#include"control_ad1938.h"

#include <SPI.h>


/*
reference
http://www.analog.com/media/en/technical-documentation/data-sheets/AD1938.pdf
http://www.analog.com/media/en/technical-documentation/application-notes/AN-1365.pdf
*/
/* SPI 3 byte register format
----------------------------------------------
|Global Address |R/W |Register Address |Data |
----------------------------------------------
|23:17          |16  |15:8             |  7:0|
----------------------------------------------
Address Register
0 		PLL and Clock Control 0
1 		PLL and Clock Control 1
2 		DAC Control 0
3 		DAC Control 1
4 		DAC Control 2
5 		DAC individual channel mutes
6 		DAC L1 volume control
7 		DAC R1 volume control
8 		DAC L2 volume control
9 		DAC R2 volume control
10 		DAC L3 volume control
11 		DAC R3 volume control
12 		DAC L4 volume control
13 		DAC R4 volume control
14 		ADC Control 0
15 		ADC Control 1
16 		ADC Control 2
*/

/*The global address for the AD1938 is 0x04, shifted left one bit due to the R/W bit.*/
#define AD1938_GLOBAL_ADDRESS   0x04

#define AD1938_WRITE_ADDRESS    (AD1938_GLOBAL_ADDRESS<<1)   
#define AD1938_READ_ADDRESS     ((AD1938_GLOBAL_ADDRESS<<1)|1)

#define AD1938_SPI_WRITE_BYTE_COUNT   3
#define AD1938_SPI_READ_BYTE_COUNT    2

/*------------------------------------------------------------------------------*/
/*PLL and Clock Control 0                                                       */
/*------------------------------------------------------------------------------*/
#define AD1938_PLL_CLK_CTRL0    0x00

/*
Bit Value Function Description
0 0 Normal operation PLL power-down
1 Power-down
2:1 00 INPUT 256 (× 44.1 kHz or 48 kHz) MCLKI/XI pin functionality (PLL active), master clock rate setting
    01 INPUT 384 (× 44.1 kHz or 48 kHz)
    10 INPUT 512 (× 44.1 kHz or 48 kHz)
    11 INPUT 768 (× 44.1 kHz or 48 kHz)
4:3 00 XTAL oscillator enabled MCLKO/XO pin, master clock rate setting
    01 256 × fS VCO output
    10 512 × fS VCO output
    11 Off
6:5 00 MCLKI/XI PLL input
    01 DLRCLK
    10 ALRCLK
    11 Reserved
7 0 Disable: ADC and DAC idle Internal master clock enable
  1 Enable: ADC and DAC active
*/

#define DIS_ADC_DAC         (0x00)
#define ENA_ADC_DAC         (0x80)

#define PLL_IN_MCLK         (0x00)
#define PLL_IN_DLRCLK       (0x20)
#define PLL_IN_ALRCLK       (0x40)

#define MCLK_OUT_XTAL       (0x00)
#define MCLK_OUT_256FS      (0x08)
#define MCLK_OUT_512FS      (0x10)
#define MCLK_OUT_OFF        (0x18)

#define INPUT256            (0x00)
#define INPUT384            (0x02)
#define INPUT512            (0x04)
#define INPUT768            (0x06)

#define PLL_PWR_UP          (0x00)
#define PLL_PWR_DWN         (0x01)

/*------------------------------------------------------------------------------*/
/*PLL and Clock Control 1                                                       */
/*------------------------------------------------------------------------------*/
#define AD1938_PLL_CLK_CTRL1     0x01
/*
Bit Value Function Description
0 0 PLL clock DAC clock source select
1 MCLK
1 0 PLL clock ADC clock source select
1 MCLK
2 0 Enabled On-chip voltage reference
1 Disabled
3 0 Not locked PLL lock indicator (read-only)
  1 Locked
7:4 0000 Reserved
*/
#define AD1938_PLL_LOCK     (0x08)

#define DIS_VREF            (0x04)
#define ENA_VREF            (0x00)

#define ADC_CLK_PLL         (0x00)
#define ADC_CLK_MCLK        (0x20)

#define DAC_CLK_PLL         (0x00)
#define DAC_CLK_MCLK        (0x01)

/*------------------------------------------------------------------------------*/
/*        DAC Control 0                                                         */
/*------------------------------------------------------------------------------*/
#define AD1938_DAC_CTRL0         0x02
/*
Bit Value Function Description
0 0 Normal Power-down
1 Power-down
2:1 00 32 kHz/44.1 kHz/48 kHz Sample rate
    01 64 kHz/88.2 kHz/96 kHz
    10 128 kHz/176.4 kHz/192 kHz
    11 Reserved
5:3 000 1 SDATA delay (BCLK periods)
    001 0
    010 8
    011 12
    100 16
    101 Reserved
    110 Reserved
    111 Reserved
7:6 00 Stereo (normal) Serial format
    01 TDM (daisy chain)
    10 DAC AUX mode (ADC-, DAC-, TDM-coupled)
    11 Dual-line TDM
*/
#define DAC_FMT_I2S         (0x00)
#define DAC_FMT_TDM         (0x40)
#define DAC_FMT_AUX         (0x80)
#define DAC_FMT_DUALTDM     (0xc0)


#define DAC_BCLK_DLY_1      (0x00)
#define DAC_BCLK_DLY_0      (0x08)
#define DAC_BCLK_DLY_8      (0x01)
#define DAC_BCLK_DLY_12     (0x18)
#define DAC_BCLK_DLY_16     (0x20)

#define DAC_SR_48K          (0x00)
#define DAC_SR_96K          (0x02)
#define DAC_SR_192K         (0x04)

#define DAC_PWR_UP          (0x00)
#define DAC_PWR_DWN         (0x01)
/*------------------------------------------------------------------------------*/
/*        DAC Control 1                                                         */
/*------------------------------------------------------------------------------*/
#define AD1938_DAC_CTRL1        0x03

/*
Bit Value Function Description
0   0 Latch in mid cycle (normal) BCLK active edge (TDM in)
1 Latch in at end of cycle (pipeline)
2:1 00 64 (2 channels) BCLKs per frame
    01 128 (4 channels)
    10 256 (8 channels)
    11 512 (16 channels)
3 0 Left low LRCLK polarity
  1 Left high
4 0 Slave LRCLK master/slave
  1 Master
5 0 Slave BCLK master/slave
  1 Master
6 0 DBCLK pin BCLK source
  1 Internally generated
7 0 Normal BCLK polarity
  1 Inverted
*/

#define DAC_BCLK_POL_NORM       (0x00)
#define DAC_BCLK_POL_INV        (0x80)

#define DAC_BCLK_SRC_PIN        (0x00)
#define DAC_BCLK_SRC_INTERNAL   (0x40)

#define DAC_BCLK_SLAVE          (0x00)
#define DAC_BCLK_MASTER         (0x20)

#define DAC_LRCLK_SLAVE         (0x00)
#define DAC_LRCLK_MASTER        (0x10)

#define DAC_LRCLK_POL_NORM      (0x00)
#define DAC_LRCLK_POL_INV       (0x08)

#define DAC_CHANNELS_2          (0x00)
#define DAC_CHANNELS_4          (0x02)
#define DAC_CHANNELS_8          (0x04)
#define DAC_CHANNELS_16         (0x06)

#define DAC_LATCH_MID           (0x00)
#define DAC_LATCH_END           (0x01)
/*------------------------------------------------------------------------------*/
/*       DAC Control 2                                                          */
/*------------------------------------------------------------------------------*/
/*
Bit Value Function Description
0   0 Unmute Master mute
1 Mute
2:1 00 Flat De-emphasis (32 kHz/44.1 kHz/48 kHz mode only)
    01 48 kHz curve
    10 44.1 kHz curve
    11 32 kHz curve
4:3 00 24 Word width
    01 20
    10 Reserved
    11 16
5 0 Noninverted DAC output polarity
  1 Inverted
7:6 00 Reserved
*/
#define AD1938_DAC_CTRL2        0x04
#define DAC_OUT_POL_NORM    (0x00)
#define DAC_OUT_POL_INV     (0x20)

#define DAC_WIDTH_24        (0x00)
#define DAC_WIDTH_20        (0x08)
#define DAC_WIDTH_16        (0x18)

#define DAC_DEEMPH_FLAT     (0x00)
#define DAC_DEEMPH_48K      (0x02)
#define DAC_DEEMPH_44_1K    (0x04)
#define DAC_DEEMPH_32K      (0x06)

#define DAC_UNMUTE_ALL      (0x00)
#define DAC_MUTE_ALL        (0x01)
/*------------------------------------------------------------------------------*/
/* DAC individual channel mutes                                                */
/*------------------------------------------------------------------------------*/
#define AD1938_DAC_CHNL_MUTE    0x05
/*
Bit Value Function Description
0 0 Unmute DAC 1 left mute
  1 Mute
1 0 Unmute DAC 1 right mute
  1 Mute
2 0 Unmute DAC 2 left mute
  1 Mute
3 0 Unmute DAC 2 right mute
  1 Mute
4 0 Unmute DAC 3 left mute
  1 Mute
5 0 Unmute DAC 3 right mute
  1 Mute
6 0 Unmute DAC 4 left mute
  1 Mute
7 0 Unmute DAC 4 right mute
  1 Mute
*/
#define DACMUTE_R4    (0x80)
#define DACMUTE_L4    (0x40)
#define DACMUTE_R3    (0x20)
#define DACMUTE_L3    (0x10)
#define DACMUTE_R2    (0x08)
#define DACMUTE_L2    (0x04)
#define DACMUTE_R1    (0x02)
#define DACMUTE_L1    (0x01)

/*
Bit Value Function Description
7:0        0        No attenuation DAC volume control
1 to 254  -3/8 dB per step
255       Full attenuation

*/
#define DACVOL_MIN      (0xFF)
#define DACVOL_LOW      (0xC0)
#define DACVOL_MED      (0x80)
#define DACVOL_HI       (0x40)
#define DACVOL_MAX      (0x00) // 0db Volume
#define DACVOL_MASK     (0xFF)  
/*------------------------------------------------------------------------------*/
/*DAC L1 volume control                                                         */
/*------------------------------------------------------------------------------*/
#define AD1938_DAC_L1_VOL       0x06
/*------------------------------------------------------------------------------*/
/*DAC R1 volume control                                                         */
/*------------------------------------------------------------------------------*/
#define AD1938_DAC_R1_VOL       0x07
/*------------------------------------------------------------------------------*/
/*DAC L2 volume control                                                         */
/*------------------------------------------------------------------------------*/
#define AD1938_DAC_L2_VOL       0x08
/*------------------------------------------------------------------------------*/
/*DAC R2 volume control                                                         */
/*------------------------------------------------------------------------------*/
#define AD1938_DAC_R2_VOL       0x09
/*------------------------------------------------------------------------------*/
/*DAC L3 volume control                                                         */
/*------------------------------------------------------------------------------*/
#define AD1938_DAC_L3_VOL       0x0a

/*------------------------------------------------------------------------------*/
/*DAC R3 volume control                                                         */
/*------------------------------------------------------------------------------*/
#define AD1938_DAC_R3_VOL       0x0b

/*------------------------------------------------------------------------------*/
/*DAC L4 volume control                                                         */
/*------------------------------------------------------------------------------*/
#define AD1938_DAC_L4_VOL       0x0c
/*------------------------------------------------------------------------------*/
/*DAC R4 volume control                                                         */
/*------------------------------------------------------------------------------*/
#define AD1938_DAC_R4_VOL       0x0d

/*------------------------------------------------------------------------------*/
/*        ADC Control 0                                                         */
/*------------------------------------------------------------------------------*/
#define AD1938_ADC_CTRL0        0x0e
/*
Bit Value Function Description
0 0 Normal Power-down
  1 Power down
1 0 Off High-pass filter
  1 On
2 0 Unmute ADC L1 mute
  1 Mute
3 0 Unmute ADC R1 mute
  1 Mute
4 0 Unmute ADC L2 mute
  1 Mute
5 0 Unmute ADC R2 mute
  1 Mute
7:6 00 32 kHz/44.1 kHz/48 kHz Output sample rate
    01 64 kHz/88.2 kHz/96 kHz
    10 128 kHz/176.4 kHz/192 kHz
    11 Reserved
*/
#define ADC_SR_48K          (0x00)
#define ADC_SR_96K          (0x40)
#define ADC_SR_192K         (0x80)

#define ADC_R2_UNMUTE       (0x00)
#define ADC_R2_MUTE         (0x20)

#define ADC_L2_UNMUTE       (0x00)
#define ADC_L2_MUTE         (0x10)

#define ADC_R1_UNMUTE       (0x00)
#define ADC_R1_MUTE         (0x08)

#define ADC_L1_UNMUTE       (0x00)
#define ADC_L1_MUTE         (0x04)

#define ADC_HP_FILT_OFF     (0x00)
#define ADC_HP_FILT_ON      (0x02)

#define ADC_PWR_UP          (0x00)
#define ADC_PWR_DWN         (0x01)
/*------------------------------------------------------------------------------*/
/*        ADC Control 01                                                        */
/*------------------------------------------------------------------------------*/
#define AD1938_ADC_CTRL1        0x0f
/*
Bit Value Function Description
1:0 00 24 Word width
    01 20
    10 Reserved
    11 16
4:2 000 1 SDATA delay (BCLK periods)
    001 0
    010 8
    011 12
    100 16
    101 Reserved
    110 Reserved
    111 Reserved
6:5 00 Stereo Serial format
    01 TDM (daisy chain)
    10 ADC AUX mode (ADC-, DAC-, TDM-coupled)
    11 Reserved
7 0 Latch in mid cycle (normal) BCLK active edge (TDM in)
  1 Latch in at end of cycle (pipeline)
*/
#define ADC_LATCH_MID       (0x00)
#define ADC_LATCH_END       (0x80)

#define ADC_FMT_I2S         (0x00)
#define ADC_FMT_TDM         (0x20)
#define ADC_FMT_AUX         (0x40)

#define ADC_BCLK_DLY_1      (0x00)
#define ADC_BCLK_DLY_0      (0x04)
#define ADC_BCLK_DLY_8      (0x08)
#define ADC_BCLK_DLY_12     (0x0c)
#define ADC_BCLK_DLY_16     (0x10)

#define ADC_WIDTH_24        (0x00)
#define ADC_WIDTH_20        (0x01)
#define ADC_WIDTH_16        (0x03)
/*------------------------------------------------------------------------------*/
/*        ADC Control 2                                                         */
/*------------------------------------------------------------------------------*/
#define AD1938_ADC_CTRL2        0x10
/*
Bit Value Function Description
0 0 50/50 (allows 32, 24, 20, or 16 bit clocks (BCLKs) per channel) LRCLK format
  1 Pulse (32 BCLKs per channel)
1 0 Drive out on falling edge (DEF) BCLK polarity
  1 Drive out on rising edge
2 0 Left low LRCLK polarity
  1 Left high
3 0 Slave LRCLK master/slave
  1 Master
5:4 00 64 BCLKs per frame
    01 128
    10 256
    11 512
6 0 Slave BCLK master/slave
  1 Master
7 0 ABCLK pin BCLK source
  1 Internally generated
*/

#define ADC_BCLK_SRC_PIN        (0x00)
#define ADC_BCLK_SRC_INTERNAL   (0x80)

#define ADC_BCLK_SLAVE          (0x00)
#define ADC_BCLK_MASTER         (0x40)

#define ADC_CHANNELS_2          (0x00)
#define ADC_CHANNELS_4          (0x10)
#define ADC_CHANNELS_8          (0x20)
#define ADC_CHANNELS_16         (0x30)

#define ADC_LRCLK_SLAVE         (0x00)
#define ADC_LRCLK_MASTER        (0x08)

#define ADC_LRCLK_POL_NORM      (0x00)
#define ADC_LRCLK_POL_INV       (0x04)

#define ADC_BCLK_POL_NORM       (0x00)
#define ADC_BCLK_POL_INV        (0x02)

#define ADC_LRCLK_FMT_50_50     (0x00)
#define ADC_LRCLK_FMT_PULSE     (0x01)

/*------------------------------------------------------------------------------*/
/*        init(void)                                                             */
/*------------------------------------------------------------------------------*/
bool AudioControlAD1938::spiInit(int clatch,int reset,int cout,int cin,int cclk)
{
	 ad1938_clatch = clatch;
	 ad1938_reset  = reset;
	 ad1938_cout   = cout;
	 ad1938_cin    = cin;
	 ad1938_cclk   = cclk;
	 
	 /**/
	 pinMode(ad1938_clatch, OUTPUT);
	 pinMode(ad1938_reset, OUTPUT);
	 
	 /*SPI clock pin set*/
	// SPI.setMOSI(cin); 
	//SPI.setMISO(cout);  
	SPI.setSCK(cclk);/*SPI clock alternate pin 14*/
	SPI.begin();
	 /*reset codec*/
	digitalWrite(ad1938_reset, LOW);
	delay(200);
	digitalWrite(ad1938_reset, HIGH);
	delay(400);//wait for 300ms to load the code
	
	
	
	return true;
	 
}
/*------------------------------------------------------------------------------*/
/*        spi_read_reg                                                          */
/*------------------------------------------------------------------------------*/
unsigned char AudioControlAD1938::spi_read_reg(unsigned char reg)
{
	unsigned char result = 0;
	unsigned char data[AD1938_SPI_WRITE_BYTE_COUNT];

	data[0] = AD1938_READ_ADDRESS;
	data[1] = reg;
	data[2] = 0x0;

	// and configure settings
	SPI.beginTransaction(SPISettings(AD1938_SPI_CLK_FREQ, MSBFIRST, SPI_MODE3));
	// take the chip select low to select the device:
	digitalWrite(ad1938_clatch, LOW);

	SPI.transfer(&data[0], AD1938_SPI_READ_BYTE_COUNT); //Send register location , read byte is 2

    // send a value of 0 to read the first byte returned:
	result =(unsigned char) SPI.transfer(0x00);

	// take the chip select high to de-select:
	digitalWrite(ad1938_clatch, HIGH);
	// release control of the SPI port
	SPI.endTransaction();
	

	return(result);

}

/*------------------------------------------------------------------------------*/
/*        spi_write_reg                                                         */
/*------------------------------------------------------------------------------*/
bool AudioControlAD1938::spi_write_reg(unsigned char reg, unsigned char val)
{
	unsigned char data[AD1938_SPI_WRITE_BYTE_COUNT];

	/*fill the buffer as per AD1938 format*/
	data[0] = AD1938_WRITE_ADDRESS;
	data[1] = reg;          
	data[2] = val;
	
	// and configure settings
	SPI.beginTransaction(SPISettings(AD1938_SPI_CLK_FREQ, MSBFIRST, SPI_MODE3));//3
																				// take the chip select low to select the device:
	digitalWrite(ad1938_clatch, LOW);

	SPI.transfer(&data[0], AD1938_SPI_WRITE_BYTE_COUNT);
	// take the chip select high to de-select:
	digitalWrite(ad1938_clatch, HIGH);
	// release control of the SPI port
	SPI.endTransaction();

	return true;

}

/*------------------------------------------------------------------------------*/
/*        init(void)                                                             */
/*------------------------------------------------------------------------------*/
bool AudioControlAD1938::config(Te_samplingRate sampleRate,
                                Te_bitsPerSample wordLen,
								Te_i2sNumChannels numChannels, 
								Te_i2sClockMode mode)
{
	
    unsigned char dac_fs = 0;
	unsigned char adc_fs = 0;
	unsigned char dac_mode = 0;
	unsigned char adc_mode = 0;
	unsigned char dac_wl = 0;
	unsigned char adc_wl = 0;
	unsigned char dac_channels = 0;
	unsigned char adc_channels = 0;
	
	
	i2sMode     = mode;
	wordLen     = wordLen;
	numChannels = numChannels;
	samplingRate = sampleRate;
	
	switch(sampleRate)
	{
	 case FS_32000:
	 case FS_44100:
	 case FS_48000:
	 {
		dac_fs = DAC_SR_48K;
		adc_fs = ADC_SR_48K;
	 }
		break;
     case FS_64000:
     case FS_88200:
     case FS_96000:
	 {
		dac_fs = DAC_SR_96K;
		adc_fs = ADC_SR_96K;
	 }
		break;
     case FS_128000:
     case FS_176400:
     case FS_192000:
	 {
		dac_fs = DAC_SR_192K;
		adc_fs = ADC_SR_192K;
	 }
		break;
	 default:
	 {
		dac_fs = DAC_SR_48K;
		adc_fs = ADC_SR_48K;
	 }
	}
	
	switch(wordLen)
	{
	 case BITS_16:
	 {
		dac_wl = DAC_WIDTH_16;
		adc_wl = ADC_WIDTH_16;
		
		
	 }
	break;
     case BITS_20:
	 {
		dac_wl = DAC_WIDTH_20;
		adc_wl = ADC_WIDTH_20;
		
	 }
	 break;
     case BITS_24:
	 {
		dac_wl = DAC_WIDTH_24;
		adc_wl = ADC_WIDTH_24;
		
	 }
   
	break;
	
	 default:
	 {
		dac_wl = DAC_WIDTH_24;
		adc_wl = ADC_WIDTH_24;
	
	 }
	}
	
	switch(numChannels)
	{
	 case I2S_STEREO_2CH:
	 {
		dac_mode = DAC_FMT_I2S;
		adc_mode = ADC_FMT_I2S;
		dac_channels = DAC_CHANNELS_2;
	    adc_channels = ADC_CHANNELS_2;
		
	 }
		break;
     case I2S_TDM_8CH:
	 {
		dac_mode = DAC_FMT_TDM;
		adc_mode = ADC_FMT_TDM;
		dac_channels = DAC_CHANNELS_8;
	    adc_channels = ADC_CHANNELS_8;
		
	 }
		break;
     case I2S_TDM_16CH:
	 {
		dac_mode = DAC_FMT_TDM;
		adc_mode = ADC_FMT_TDM;
		dac_channels = DAC_CHANNELS_16;
	    adc_channels = ADC_CHANNELS_16;
		
	 }
	break;
	 default:
	 {
		dac_mode = DAC_FMT_I2S;
		adc_mode = ADC_FMT_I2S;
		dac_channels = DAC_CHANNELS_2;
	    adc_channels = ADC_CHANNELS_2;
	 }
	}
	
	if(mode == AD1938_I2S_SLAVE)
	{
	
	//0 PLL and Clock Control 0
	spi_write_reg(AD1938_PLL_CLK_CTRL0, (DIS_ADC_DAC | INPUT512 | PLL_IN_ALRCLK | MCLK_OUT_OFF |PLL_PWR_DWN));

	
    //1 PLL and Clock Control 1
	spi_write_reg(AD1938_PLL_CLK_CTRL1, (DAC_CLK_PLL | ADC_CLK_PLL | ENA_VREF));

	//2 DAC Control 0
	spi_write_reg(AD1938_DAC_CTRL0, (dac_mode | DAC_BCLK_DLY_1 | dac_fs | DAC_PWR_UP));

    //3 DAC Control 1
	spi_write_reg(AD1938_DAC_CTRL1, ( DAC_BCLK_SRC_PIN|DAC_BCLK_SLAVE| DAC_LRCLK_SLAVE  |DAC_LRCLK_POL_NORM | dac_channels | DAC_LATCH_MID));
	//4 DAC Control 2
	spi_write_reg(AD1938_DAC_CTRL2, dac_wl);

	//5 DAC individual channel mutes
	spi_write_reg(AD1938_DAC_CHNL_MUTE, 0x00);/*mute*/

	//6 DAC L1 volume control
	spi_write_reg(AD1938_DAC_L1_VOL, DACVOL_MAX);

	//7 DAC R1 volume control
	spi_write_reg(AD1938_DAC_R1_VOL, DACVOL_MAX);

	//8 DAC L2 volume control
	spi_write_reg(AD1938_DAC_L2_VOL, DACVOL_MAX);

	//9 DAC R2 volume control
	spi_write_reg(AD1938_DAC_R2_VOL, DACVOL_MAX);

	//10 DAC L3 volume control
	spi_write_reg(AD1938_DAC_L3_VOL, DACVOL_MAX);

	//11 DAC R3 volume control
	spi_write_reg(AD1938_DAC_R3_VOL, DACVOL_MAX);

	//12 DAC L4 volume control
	spi_write_reg(AD1938_DAC_L4_VOL, DACVOL_MAX);

	//13 DAC R4 volume control
	spi_write_reg(AD1938_DAC_R4_VOL, DACVOL_MAX);


	//14 ADC Control 0
	spi_write_reg(AD1938_ADC_CTRL0, adc_fs);

	//15 ADC Control 1
	spi_write_reg(AD1938_ADC_CTRL1, (ADC_LATCH_MID | adc_mode | ADC_BCLK_DLY_1 | adc_wl));

	//16 ADC Control 2
	spi_write_reg(AD1938_ADC_CTRL2, ( ADC_BCLK_SRC_PIN|ADC_BCLK_SLAVE | adc_channels | ADC_LRCLK_SLAVE  | ADC_LRCLK_FMT_50_50|ADC_LRCLK_POL_NORM|ADC_BCLK_POL_NORM));		
	
	}
	else
	{
		//0 PLL and Clock Control 0
		spi_write_reg(AD1938_PLL_CLK_CTRL0, (DIS_ADC_DAC | INPUT512 | PLL_IN_MCLK | MCLK_OUT_XTAL |PLL_PWR_DWN));
			
		//1 PLL and Clock Control 1
		spi_write_reg(AD1938_PLL_CLK_CTRL1, (DAC_CLK_MCLK | ADC_CLK_MCLK | ENA_VREF));
		
		//2 DAC Control 0
		spi_write_reg(AD1938_DAC_CTRL0, (dac_mode | DAC_BCLK_DLY_1 | dac_fs | DAC_PWR_UP));
		
		 //3 DAC Control 1
		spi_write_reg(AD1938_DAC_CTRL1, ( DAC_BCLK_SRC_INTERNAL|DAC_BCLK_SLAVE| DAC_LRCLK_SLAVE  |DAC_LRCLK_POL_NORM | dac_channels | DAC_LATCH_MID));
		
		//4 DAC Control 2
		spi_write_reg(AD1938_DAC_CTRL2, dac_wl);
		
		//5 DAC individual channel mutes
		spi_write_reg(AD1938_DAC_CHNL_MUTE, 0x00);/*unmute*/

		//6 DAC L1 volume control
		spi_write_reg(AD1938_DAC_L1_VOL, DACVOL_MAX);

		//7 DAC R1 volume control
		spi_write_reg(AD1938_DAC_R1_VOL, DACVOL_MAX);

		//8 DAC L2 volume control
		spi_write_reg(AD1938_DAC_L2_VOL, DACVOL_MAX);

		//9 DAC R2 volume control
		spi_write_reg(AD1938_DAC_R2_VOL, DACVOL_MAX);

		//10 DAC L3 volume control
		spi_write_reg(AD1938_DAC_L3_VOL, DACVOL_MAX);

		//11 DAC R3 volume control
		spi_write_reg(AD1938_DAC_R3_VOL, DACVOL_MAX);

		//12 DAC L4 volume control
		spi_write_reg(AD1938_DAC_L4_VOL, DACVOL_MAX);

		//13 DAC R4 volume control
		spi_write_reg(AD1938_DAC_R4_VOL, DACVOL_MAX);
		
		//14 ADC Control 0
		spi_write_reg(AD1938_ADC_CTRL0, adc_fs);

		//15 ADC Control 1
		spi_write_reg(AD1938_ADC_CTRL1, (ADC_LATCH_MID | adc_mode | ADC_BCLK_DLY_1 | adc_wl));

		//16 ADC Control 2
		spi_write_reg(AD1938_ADC_CTRL2, ( ADC_BCLK_SRC_INTERNAL|ADC_BCLK_MASTER | adc_channels | ADC_LRCLK_MASTER  | ADC_LRCLK_FMT_50_50|ADC_LRCLK_POL_NORM|ADC_BCLK_POL_NORM));
	}
	return true;

}

/*------------------------------------------------------------------------------*/
/*        isPllLocked(void)                                                     */
/*------------------------------------------------------------------------------*/
bool AudioControlAD1938::isPllLocked(void)
{
	return ((spi_read_reg(AD1938_PLL_CLK_CTRL1)>>3)&0x1);

}
/*------------------------------------------------------------------------------*/
/*        enable(void)                                                            */
/*------------------------------------------------------------------------------*/
bool AudioControlAD1938::enable(void)
{

	if(i2sMode == AD1938_I2S_SLAVE)
	{
		spi_write_reg(AD1938_PLL_CLK_CTRL0, (ENA_ADC_DAC | INPUT512 | PLL_IN_DLRCLK | MCLK_OUT_OFF | PLL_PWR_UP));	
	}
	else
	{
		spi_write_reg(AD1938_PLL_CLK_CTRL0, (ENA_ADC_DAC | INPUT512 | PLL_IN_MCLK | MCLK_OUT_XTAL |PLL_PWR_UP));
	}
	
	spi_write_reg(AD1938_DAC_CHNL_MUTE, 0);/*un mute*/
		
    return true;
}

/*------------------------------------------------------------------------------*/
/*        disable(void)                                                          */
/*------------------------------------------------------------------------------*/
bool AudioControlAD1938::disable(void)
{
	unsigned char reg_value;
	reg_value = spi_read_reg(AD1938_PLL_CLK_CTRL0);
	reg_value = (reg_value&0x7e);/*mask the last and first bits*/
	spi_write_reg(AD1938_PLL_CLK_CTRL0, (DIS_ADC_DAC |  reg_value| PLL_PWR_DWN));
	
	reg_value = spi_read_reg(AD1938_DAC_CTRL0);
	
	spi_write_reg(AD1938_DAC_CTRL0, ((reg_value&0xfe)|DAC_PWR_DWN));
	
	reg_value = spi_read_reg(AD1938_ADC_CTRL0);
	
	spi_write_reg(AD1938_DAC_CTRL0, ((reg_value&0xfe)|ADC_PWR_DWN));
	
   return true;
}
/*------------------------------------------------------------------------------*/
/*        dacVolume(int dac_num, float volume)                                                             */
/*------------------------------------------------------------------------------*/
bool AudioControlAD1938::dacVolume(int dac_num, int volume)
{

		switch (dac_num)
		{
		case 0://DAC0
			spi_write_reg(AD1938_DAC_L1_VOL, volume);
			spi_write_reg(AD1938_DAC_R1_VOL, volume);
			break;
		case 1://DAC1
			spi_write_reg(AD1938_DAC_L2_VOL, volume);
			spi_write_reg(AD1938_DAC_R2_VOL, volume);
			break;
		case 2://DAC2
			spi_write_reg(AD1938_DAC_L3_VOL, volume);
			spi_write_reg(AD1938_DAC_R3_VOL, volume);
			break;
		case 3://DAC3
			spi_write_reg(AD1938_DAC_L4_VOL, volume);
			spi_write_reg(AD1938_DAC_R4_VOL, volume);
			break;

		}

	

	return true;

}
/*------------------------------------------------------------------------------*/
/*        volume( float volume)                                                             */
/*------------------------------------------------------------------------------*/
bool AudioControlAD1938::volume(float volume)
{
	int vol = 0;
	 
	vol =(int)((1.0-volume)*255);
	if (vol<0)
		vol=0;
	if(vol>255)
		vol =255;
	
	dacVolume(0,vol);
	dacVolume(1,vol);
	dacVolume(2,vol);
	dacVolume(3,vol);

	return true;

}

/*------------------------------------------------------------------------------*/
/*        dacMute                                                               */
/*------------------------------------------------------------------------------*/
bool AudioControlAD1938::dacMute(bool mute)
{
	if (mute == true)
	{
		spi_write_reg(AD1938_DAC_CHNL_MUTE, 0xff);/*mute*/
	}
	else
	{
		spi_write_reg(AD1938_DAC_CHNL_MUTE, 00);/*unmute*/
	}
	return true;

}

/*------------------------------------------------------------------------------*/
/*        adcMute                                                               */
/*------------------------------------------------------------------------------*/
bool AudioControlAD1938::adcMute(bool mute)
{
	unsigned char reg_value;
	reg_value = spi_read_reg(AD1938_ADC_CTRL0);
	if (mute == true)
	{
		spi_write_reg(AD1938_ADC_CTRL0, (reg_value&0xc3)|0x3c);/*mute*/
	}
	else
	{
		spi_write_reg(AD1938_ADC_CTRL0, (reg_value&0xc3));/*unmute*/
	}
	return true;

}

/*------------------------------------------------------------------------------*/
/*        readAllreg                                                         */
/*------------------------------------------------------------------------------*/
void AudioControlAD1938::readAllreg(void)
{
	int i =0;
	unsigned char reg_val=0;
	 Serial.print("\n readAllreg\n");
	for (i = 0;i<17;i++)
	{
	   reg_val = spi_read_reg(i);
	   Serial.print("\n");
	   Serial.print(i);
	   Serial.print("\t");
	   Serial.print(reg_val, HEX);
	}
	
}
/*------------------------------------------------------------------------------*/
/*        end of file                                                           */
/*------------------------------------------------------------------------------*/
