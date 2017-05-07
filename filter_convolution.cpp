/* Audio Library for Teensy 3.X
 * filter convolution written by Brian Millier
 *
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

#include "filter_convolution.h"

/******************************************************************/
//                A u d i o FilterConvolution
// Written by Brian Millier Mar 2017
/*******************************************************************/


boolean AudioFilterConvolution::begin(int status)
{
	enabled = status;
  return(true);
}

void AudioFilterConvolution::passThrough(int stat)
{
  passThru=stat;
}

void AudioFilterConvolution::impulse(int16_t *coefs) {
	arm_q15_to_float(coefs, FIR_coef, 513); // convert int_buffer to float 32bit
	int k = 0;
	int i = 0;
	enabled = 0; // shut off audio stream while impulse is loading
	for (i = 0; i < (FFT_length / 2) + 1; i++)
	{
		FIR_filter_mask[k++] = FIR_coef[i];
		FIR_filter_mask[k++] = 0;
	}

	for (i = FFT_length + 1; i < FFT_length * 2; i++)
	{
		FIR_filter_mask[i] = 0.0;
	}
	arm_cfft_f32( &arm_cfft_sR_f32_len1024, FIR_filter_mask, 0, 1);
	for (int i = 0; i < 1024; i++) {
	//	Serial.println(FIR_filter_mask[i] * 32768);
	}
	// for 1st time thru, zero out the last sample buffer to 0
	memset(last_sample_buffer_L, 0, sizeof(last_sample_buffer_L));
	state = 0;
	enabled = 1;  //enable audio stream again
}

void AudioFilterConvolution::update(void)
{
	audio_block_t *block;
	short *bp;

	if (enabled != 1 ) return;
	block = receiveWritable(0);   // MUST be Writable, as convolution results are written into block
	if (block) {
		switch (state) {
		case 0:
			bp = block->data;
			for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
				buffer[i] = *bp++;
			}
			bp = block->data;
			for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
				*bp++ = tbuffer[i];   // tbuffer contains results of last FFT/multiply/iFFT processing (convolution filtering)
			}
			transmit(block, 0);
			release(block);
			state = 1;
			break;
		case 1:
			bp = block->data;
			for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
				buffer[128+i] = *bp++;
			}
			bp = block->data;
			for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
				*bp++ = tbuffer[i+128]; // tbuffer contains results of last FFT/multiply/iFFT processing (convolution filtering)
			}
			transmit(block, 0);
			release(block);
			state = 2;
			break;
		case 2:
			bp = block->data;
			for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
				buffer[256 + i] = *bp++;
			}
			bp = block->data;
			for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
				*bp++ = tbuffer[i+256];  // tbuffer contains results of last FFT/multiply/iFFT processing (convolution filtering)
			}
			transmit(block, 0);
			release(block);
			state = 3;
			break;
		case 3:
			bp = block->data;
			for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
				buffer[384 + i] = *bp++;
			}
			bp = block->data;
			for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
				*bp++ = tbuffer[i + 384];  // tbuffer contains results of last FFT/multiply/iFFT processing (convolution filtering)
			}
			transmit(block, 0);
			release(block);
			state = 0;
			// 4 blocks are in- now do the FFT1024,complex multiply and iFFT1024  on 512samples of data
			// using the overlap/add method
			// 1st convert Q15 samples to float
			arm_q15_to_float(buffer, float_buffer_L, 512); 
		   // float_buffer samples are now standardized from > -1.0 to < 1.0
			if (passThru ==0) {
				memset(FFT_buffer + 1024, 0, sizeof(FFT_buffer) / 2);    // zero pad last half of array- necessary to prevent aliasing in FFT
				//fill FFT_buffer with current audio samples
				k = 0;
				for (i = 0; i < 512; i++)
				{
					FFT_buffer[k++] = float_buffer_L[i];   // real
					FFT_buffer[k++] = float_buffer_L[i];   // imag
				}
				// calculations are performed in-place in FFT routines
				arm_cfft_f32(&arm_cfft_sR_f32_len1024, FFT_buffer, 0, 1);// perform complex FFT
				arm_cmplx_mult_cmplx_f32(FFT_buffer, FIR_filter_mask, iFFT_buffer, FFT_length);   // complex multiplication in Freq domain = convolution in time domain
				arm_cfft_f32(&arm_cfft_sR_f32_len1024, iFFT_buffer, 1, 1);  // perform complex inverse FFT
				k = 0;
				l = 1024;
				for (int i = 0; i < 512; i++) {
					float_buffer_L[i] = last_sample_buffer_L[i] + iFFT_buffer[k++];   // this performs the "ADD" in overlap/Add
					last_sample_buffer_L[i] = iFFT_buffer[l++];				// this saves 512 samples (overlap) for next time around
					k++;
					l++;
				}
			} //end if passTHru
			// convert floats to Q15 and save in temporary array tbuffer
			arm_float_to_q15(&float_buffer_L[0], &tbuffer[0], BUFFER_SIZE*4);
			break;
		}
	}
}



