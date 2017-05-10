# Teensy-FFT-Convolution-Filter
Convolution filter Audio library object using FFT/iFFT 
Brian Millier May 2017

This object works with the PJRC Teensy Audio library using the Teensy 3.6. I drew upon DD4WH's SDR code in coming up with this library.Thanks Frank.

 What is this good for?
	My main goal in writing this was to provide a way for the Teensy to do guitar amplifier cabinet simulation, using commercially available/freeware impulse files(in WAV format). For this application, the latency (important to guitar players) is approx. 19.5 milliseconds.
	This routine can also be used to provide the very sharp filtering characteristics provided by a 513-tap FIR filter.

Will this work with the standard Teensyduino distribution?

Some hardware Floating Point CMSIS routines are used by this object. Teensyduino must have some files replaced/edited
to accomodate this.To do this, please follow the detailed instructions found in Post #15 on the Teensy Forum at:

https://forum.pjrc.com/threads/40590-Teensy-Convolution-SDR-(Software-Defined-Radio)?highlight=SDR

(Ignore item #1, about Si5351, as it doesn't apply here)

If you are hesitant about making changes to your Arduino/Teensyduino environment, you can install a second, dedicated Arduino IDE on your PC by downloading the Arduino ZIP file and unzipping it into a new folder ("Arduino-FPCMSIS" for example). Then run the Teensyduino installer and point to this folder when it asks you for the Arduino IDE location.
 
Toolchain used:  Arduino 1.8.0 and Teensyduino 1.34
  
 
How does it work?
	This object is a single channel filter which performs a 513-tap convolution on the audio stream that is fed to it (at the standard Teensy Audio library 44,100 Hz Sample Rate). Since a 513 tap convolution would require more calculations than a Teensy 3.6 could perform in real time, at this rate, an alternate method is used. When a time domain signal is transformed into the frequency domain, the math-heavy convolution process can be replaced by a multiplication of the signal and Filter Mask arrays. The combination of the transforms into the frequency domain and back again, plus the array multiplication, turns out to be much quicker than a straight 513-tap convolution (thanks to the efficiency of the FFT algorithm in general, and especially so for the optimized one found in the CMSIS library). 

To use this object, you must first have/generate a 513-tap filter coeficient array. If you are doing standard FIR filtering, you can use FIR calculation tool found on the web, or use the one I include in the FIR_Example sketch included in this repository.( Thanks to DD4WH for this routine). You pass it the filter center frequency, slope etc. and it fills up a 513 point floating point array. 
	 I wrote this object primarily to do guitar cabinet simulation via convolution. The impulse files that are commonly available to simulate various guitar cabinets are WAV files, with integer coeficients. Therefore, I tailored the FilterConvolution object to accept the filter coeficients in 16-bit integer format. Therefore, in the FIR_example sketch, the generated floating point FIR coeficients are converted to integer format by multiplying each value by 32768.

If you are doing guitar cabinet simulation, you must load the impulse WAV file onto a microSD card, and place it in the socket on the Teensy 3.6 module. If you want to use the Cabinet_Simulation_Example sketch, un-modified, this file should be named MG.wav, as that is the name of the file that I open in this sketch. You can of course modify your sketch to handle any impulse WAV file that you have available- even switching amongst impulse files using switches, for example. 

I note that the impulse files that I have seen can be very large: 700 KB or greater, even though they only contain 513 actual coeficient values. The rest of the file is padded with zeros. I don't know why they are made this way. Also, since they are WAV format files, they contain header information at the start of the file, and the actual data doesn't always start at any given offset. In the example sketch, I search the file from the beginning for the presence of the ID string "data", and then start reading the data 4 bytes beyond that ID (skipping the long integer data size field). I haven't looked at all of the cabinet simulation impulse files that are out there, so if the sketch doesn't work with your file, load it into a hex editor of some sort, and check to see if it follows the standard WAV file format.The WAV file format can be studied by Googling "WAV file format".

Whichever way you obtain your filter coeficients, that array must be passed through a 1024 point FFT routine to provide the Filter Mask needed for the FFT/convolution routine.That is done in the sketch as follows:
  
convolution.begin(0);   // set to zero to disable audio processing until impulse has been loaded
convolution.impulse(cabinet_impulse);  // generates Filter Mask and enables the audio stream 

where "convolution" is an instance of the AudioFilterConvolution object, and "cabinet_impulse" is a pointer to a 513 point integer array that has been pre-loaded with the filter coeficients. The actual 513-point Filter Mask that is generated is a private array variable within the AudioFilterConvolution class, and is not visible in the example sketch itself.

The FFT/convolution filtering is performed in the AudioFilterConvolution.update routine, as is the case for all Audio library objects. To do the 513 point convolution requires 1) a 513 point Filter Mask 2) 512 samples of incoming audio 3) 1024 point FFT + array multiplication + 1024 point inverse FFT. However, the Audio library is designed with a block size of 128 audio samples. Therefore, the update routine has to accumulate 4 such blocks before doing the FFT/convolution. It will then operate on those 4 blocks and produce 512 output samples. Each time through the update routine, only one 128 sample block can be sent back into the audio stream. In practice, the update routine maintains a  buffer of input data, operating on the last 512 input samples that have come in, and also maintains a pointer into the 512 sample filter output. It sends out those 512 samples, 128 samples for each invokation of the update routine. 
  
There is a pass-through function for this object. In the main sketch this is accessed as follows:

void loop() {

if (digitalRead(37) == 0) {

convolution.passThrough(0);

}

else

{

convolution.passThrough(1);	

}

}


Assuming that Pin 37 has been defined as INPUT_PULLUP, then the filtering will be enabled when the switch is closed, and the audio signal will pass through un-modified when the switch is open.

You must add my filter_convolution.h and filter_convolution.cpp files to the folder containing the standard Audio library files. This folder will be located under whatever folder you have installed the Arduino/Teensyduino IDE- modified for FP CMSIS- the path is as follows:

c:\your arduino folder\hardware\teensy\avr\libraries\Audio

Also, in that folder, edit Audio.h by adding the following line at the end

#include "filter_convolution.h" // library file added by Brian Millier

Like any custom audio library objects that you add yourself, this one will NOT show up in the Audio System Design Tool found on the PJRC site. Probably the easist way to generate the setup/connection code needed to incorporate this filter into your audio configuration, is to draw your configuration using the Design Tool web program, but place an fir filter. Import this configuration into your sketch. Then, within the "// GUItool: begin automatically generated code" section replace 

AudioFilterFIR           fir1;           

 with 

AudioFilterConvolution       convolution;        

and, on 2 AudioConnection lines, replace instances of "fir1" with "convolution" 

If you want the convolution filter keywords to be highlighted in orange in the Arduino IDE, like all other Audio library objects, you can add the following line to the keywords file (contained in the Audio folder).
AudioFilterConvolution	KEYWORD2

It is a bit difficult to determine how much of the processor's time is actually being used by this object, due to the way in which it has to process the data. When I placed a 

Serial.println (AudioProcessorUsage()) 

in the loop() section of a sample sketch, it reported 1%  three quarters of the time and 64% about one-quarter of the time (when it was being printed out once per second).
	Since the FFT/convolution routine is only being done once for every 4 convolution.update() routine calls, this large variation is likely due to this fact. I think that to provide gapless audio output, one would have to assume that the 64% figure is what must be considered if other Audio objects were to be used in addition to this one.
