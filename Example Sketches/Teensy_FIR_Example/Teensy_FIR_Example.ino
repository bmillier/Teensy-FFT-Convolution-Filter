// This sample program tests the FFT/convolution Audio library routine 
// In this case we generate a LP FIR filter with 513 taps use this to test the FFT/convolution routine
// The 513 tap filter coeficients are derived in function "calc_FIR_coeffs" 
// from DD4WH's Teensy SDR program

// IMPORTANT NOTE! You must load floating point CMSIS routines as explained in the readme file
// because the FFT/convolution audio library uses these CMSIS FP routines

// This program has a bypass function enabled via a switch on Pin 37
// FFT/convolution is perfromed only when Switch is CLOSED

// signal input is from audio board LINE IN LEFT

// this version uses overlap-add method 
// background for the routine  came from "The Scientist and Engineer's Guide to Digital Signal Processing" 
// by Steven W Smith ( www.dspguide.com)

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <arm_math.h>
#include <arm_const_structs.h>

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioInputI2S            i2s1;           //xy=142,103
AudioFilterConvolution       convolution;        //xy=271,97
AudioMixer4              mixer1;         //xy=405,99
AudioOutputI2S           i2s2;           //xy=551,106
AudioConnection          patchCord1(i2s1, 0, convolution, 0);
AudioConnection          patchCord2(convolution, 0, mixer1, 0);
AudioConnection          patchCord3(mixer1, 0, i2s2, 0);
AudioConnection          patchCord4(mixer1, 0, i2s2, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=306.5555419921875,178.22222900390625
										 // GUItool: end automatically generated code

#define BUFFER_SIZE 128
#define MAX_NUMCOEF 513
#define TPI           6.28318530717959f
#define PIH           1.57079632679490f
#define FOURPI        2.0 * TPI
#define SIXPI         3.0 * TPI
const int myInput = AUDIO_INPUT_LINEIN;
const uint32_t FFT_L = 1024;

int16_t cabinet_impulse[513];
float32_t FIR_Coef[513];
uint32_t i = 0;


void setup() {
	pinMode(37, INPUT_PULLUP);  // pass thru switch
	Serial.begin(115200);
	delay(5000);
	AudioMemory(8);
	// Enable the audio shield. select input. and enable output
	sgtl5000_1.enable();
	sgtl5000_1.inputSelect(myInput);
	sgtl5000_1.volume(0.7); // 
	sgtl5000_1.adcHighPassFilterDisable(); // does not help too much!
	sgtl5000_1.lineInLevel(14);
	sgtl5000_1.lineOutLevel(25);

	// this calculates a FIR LP filter to test convolution
	//- not needed when a cabinet impulse file loaded instead
	calc_FIR_coeffs(FIR_Coef, 513, (float32_t)2500, 40, 0, 0.0, 44100);      // Fc=2500 Hz, slope = 40 dB, S. Rate 44100
	Serial.println("FIR coefs generated");
	//change FIR file to Q15 format, to match the WAV file 16 bit integer format of the convolver files
	for (i = 0; i < 513; i++) {
		 cabinet_impulse[i] = FIR_Coef[i] * 32768;
	 }
	 Serial.println("FIR filter floats multiplied");

	convolution.begin(0);   // set to zero to disable audio processing until impulse has been loaded
	convolution.impulse(cabinet_impulse);  // generates Filter Mask and enables the audio stream

	Serial.println("FIR MASK generated");

}


void loop() {
	if (digitalRead(37) == 0) {
		convolution.passThrough(0);
	}
	else
	{
		convolution.passThrough(1);		
	}
}

void calc_FIR_coeffs (float * coeffs, int numCoeffs, float32_t fc, float32_t Astop, int type, float dfc, float Fsamprate)
    // pointer to coefficients variable, no. of coefficients to calculate, frequency where it happens, stopband attenuation in dB, 
    // filter type, half-filter bandwidth (only for bandpass and notch) 
 {  // modified by WMXZ and DD4WH after
    // Wheatley, M. (2011): CuteSDR Technical Manual. www.metronix.com, pages 118 - 120, FIR with Kaiser-Bessel Window
    // assess required number of coefficients by
    //     numCoeffs = (Astop - 8.0) / (2.285 * TPI * normFtrans);
    // selecting high-pass, numCoeffs is forced to an even number for better frequency response

     int ii,jj;
     float32_t Beta;
     float32_t izb;
     float fcf = fc;
     int nc = numCoeffs;
     fc = fc / Fsamprate;
     dfc = dfc / Fsamprate;
     // calculate Kaiser-Bessel window shape factor beta from stop-band attenuation
     if (Astop < 20.96)
       Beta = 0.0;
     else if (Astop >= 50.0)
       Beta = 0.1102 * (Astop - 8.71);
     else
       Beta = 0.5842 * powf((Astop - 20.96), 0.4) + 0.07886 * (Astop - 20.96);

     izb = Izero (Beta);
     if(type == 0) // low pass filter
//     {  fcf = fc;
     {  fcf = fc * 2.0;
      nc =  numCoeffs;
     }
     else if(type == 1) // high-pass filter
     {  fcf = -fc;
      nc =  2*(numCoeffs/2);
     }
     else if ((type == 2) || (type==3)) // band-pass filter
     {
       fcf = dfc;
       nc =  2*(numCoeffs/2); // maybe not needed
     }
     else if (type==4)  // Hilbert transform
   {
         nc =  2*(numCoeffs/2);
       // clear coefficients
       for(ii=0; ii< 2*(nc-1); ii++) coeffs[ii]=0;
       // set real delay
       coeffs[nc]=1;

       // set imaginary Hilbert coefficients
       for(ii=1; ii< (nc+1); ii+=2)
       {
         if(2*ii==nc) continue;
       float x =(float)(2*ii - nc)/(float)nc;
       float w = Izero(Beta*sqrtf(1.0f - x*x))/izb; // Kaiser window
       coeffs[2*ii+1] = 1.0f/(PIH*(float)(ii-nc/2)) * w ;
       }
       return;
   }

     for(ii= - nc, jj=0; ii< nc; ii+=2,jj++)
     {
       float x =(float)ii/(float)nc;
       float w = Izero(Beta*sqrtf(1.0f - x*x))/izb; // Kaiser window
       coeffs[jj] = fcf * m_sinc(ii,fcf) * w;
     }

     if(type==1)
     {
       coeffs[nc/2] += 1;
     }
     else if (type==2)
     {
         for(jj=0; jj< nc+1; jj++) coeffs[jj] *= 2.0f*cosf(PIH*(2*jj-nc)*fc);
     }
     else if (type==3)
     {
         for(jj=0; jj< nc+1; jj++) coeffs[jj] *= -2.0f*cosf(PIH*(2*jj-nc)*fc);
       coeffs[nc/2] += 1;
     }

} // END calc_FIR_coef

float32_t Izero (float32_t x) 
{
    float32_t x2 = x / 2.0;
    float32_t summe = 1.0;
    float32_t ds = 1.0;
    float32_t di = 1.0;
    float32_t errorlimit = 1e-9;
    float32_t tmp;
    do
    {
        tmp = x2 / di;
        tmp *= tmp;
        ds *= tmp;
        summe += ds;
        di += 1.0; 
    }   while (ds >= errorlimit * summe);
    return (summe);
}  // END Izero

float m_sinc(int m, float fc)
{  // fc is f_cut/(Fsamp/2)
  // m is between -M and M step 2
  //
  float x = m*PIH;
  if(m == 0)
    return 1.0f;
  else
    return sinf(x*fc)/(fc*x);
}







