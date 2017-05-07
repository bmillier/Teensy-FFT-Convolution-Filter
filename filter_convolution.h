/* Audio Library for Teensy 3.X (only works with floating point library enabled in toolchain & tested with Teensy 3.6 only )
 * filter convolution routine  written by Brian Millier
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef filter_convolution_h_
#define filter_convolution_h_

#include "Arduino.h"
#include "AudioStream.h"
#include <arm_math.h>
#include <arm_const_structs.h>

/******************************************************************/
//                A u d i o FilterConvolution
// Written by Brian Millier mar 2017
/******************************************************************/

class AudioFilterConvolution : 
public AudioStream
{
public:
	AudioFilterConvolution(void) :
		AudioStream(1, inputQueueArray)                       //, num_chorus(2)   
	{
	}

  boolean begin(int status);
  virtual void update(void);
  void passThrough(int stat);
  void impulse(int16_t *coefs);
  
private:
#define BUFFER_SIZE 128
  audio_block_t *inputQueueArray[1];
  int16_t *sp_L;
  volatile uint8_t state;
  int i;
  int k;
  int l;
  int passThru;
  int enabled;
  const uint32_t FFT_length = 1024;
  float32_t FIR_coef[2048] __attribute__((aligned(4)));
  float32_t FIR_filter_mask[2048] __attribute__((aligned(4)));
  int16_t buffer[2048] __attribute__((aligned(4)));
  int16_t tbuffer[2048]__attribute__((aligned(4)));
  float32_t FFT_buffer[2048] __attribute__((aligned(4)));
  float32_t iFFT_buffer[2048] __attribute__((aligned(4)));
  float32_t float_buffer_L[512]__attribute__((aligned(4)));
  float32_t last_sample_buffer_L[512];
  };

#endif
