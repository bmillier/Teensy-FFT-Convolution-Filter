// Teensy 3.6  FFT/convolution library example 
// This sketch loads in a guitar cabinet simulation file (in WAV format) 
// from the BUILT-IN SD card socket
// The filename I am using is MG.wav (see line #77)

// IMPORTANT NOTE! You must load floating point CMSIS routines as explained in the readme file
// because the FFT/convolution audio library uses these CMSIS FP routines

// This program has a bypass function enabled via a switch on Pin 37
// FFT/convolution is performed only when Switch is CLOSED

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
#include <filter_convolution.h>
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

const int myInput = AUDIO_INPUT_LINEIN;
const uint32_t FFT_L = 1024;

int16_t cabinet_impulse[2000];
uint32_t dataID[1];
uint32_t i = 0;


void setup() {
	pinMode(37, INPUT_PULLUP);  // pass thru switch
	// setI2SFreq(44100); // could use other sample rates if the FIR impulse values were changed accordingly
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

	if (!(SD.begin(BUILTIN_SDCARD))) {
		while (1) {
			Serial.println("Unable to access the SD card");
			delay(500);
		}
	}

	Serial.println("SD card OK");
	File datafile = SD.open("MG.WAV", FILE_READ);
	// As WAV data offset used in various cabinet simulation files may vary
	// I scan thru file to find the ID string "data", which preceeds the data subchunk
	// and then read 513 samples from the start of the Data subchunk
	// if string "data" is NOT found, the program will scan past end of file and hang.

	if (datafile) {
		int i = 16;
		boolean IDFound = false;
		while (!IDFound) {
		datafile.seek(i);
		datafile.read(dataID, 4);
		if (dataID[0] == 0x61746164) {     // this big-endian integer = the string 'data'
			IDFound = true;
		}
		i += 8; // skip past the ID filed and data chunk size field
	}
		
//		Serial.println(i);
		datafile.seek(i);
		datafile.read(cabinet_impulse, 1026);
	}
/*	
	for (i = 0; i < 513; i++) {
		Serial.println(cabinet_impulse[i]);
	}
*/

	convolution.begin(0);   // set to zero to disable audio processing until impulse has been loaded
	convolution.impulse(cabinet_impulse);  // generates Filter Mask using FFT and enables the audio stream

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
