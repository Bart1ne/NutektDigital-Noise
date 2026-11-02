/*
    BSD 3-Clause License

    Copyright (c) 2018, KORG INC., 2023 Christopher Brand
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//*/

/*
 * File: noise.cpp
 *
 * Noise oscillator
 */

#include "userosc.h"
#include "noise.hpp"
#include "dsp/biquad.hpp"
#include "antialiasingfilter.hpp"

static Noise s_Noise;

// Moderate per-colour output trims, to be fine-tuned by listening on hardware.
// These are not exact loudness matching: different spectra sound different.
static inline float noise_trim(uint8_t type) {
  switch (type) {
    case Noise::k_flag_white:  return 0.65f;
    case Noise::k_flag_pink:   return 1.35f;
    case Noise::k_flag_brown:  return 3.00f;
    case Noise::k_flag_blue:   return 0.85f;
    case Noise::k_flag_violet: return 0.80f;
    default:                   return 1.00f; // grey
  }
}

// Fast, smooth approximate tanh saturation without expensive library calls.
static inline float soft_clip(float x) {
  if (x >= 3.f) return 1.f;
  if (x <= -3.f) return -1.f;
  const float sq = x * x;
  return x * (27.f + sq) / (27.f + 9.f * sq);
}

static inline float process_output(float sample, uint8_t type, float drive) {
  const float trimmed = sample * noise_trim(type);
  if (drive <= 0.0001f) {
    // Hard safety ceiling when DRIVE is off.
    if (trimmed > 0.98f) return 0.98f;
    if (trimmed < -0.98f) return -0.98f;
    return trimmed;
  }
  const float gain = 1.f + 4.f * drive;
  const float makeup = 1.f / (1.f + 0.35f * (gain - 1.f));
  return soft_clip(trimmed * gain) * makeup * 0.98f;
}


void OSC_INIT(uint32_t platform, uint32_t api)
{ 
  // prevents the compiler from complaining
  (void)platform;
  (void)api;
}

void OSC_CYCLE(const user_osc_param_t * const params,
               int32_t *yn,
               const uint32_t frames)
{
  Noise::State &s = s_Noise.state;
  const uint8_t flags = s.flags;
  const uint8_t osc = s.noise_type;

  q31_t * __restrict y = (q31_t *)yn;
  const q31_t * y_e = y + frames;

  if (osc == Noise::k_flag_white){
    float preUpSampleBuffer [frames] = {0};
    float postUpSampleBuffer [frames *2] = {0};
    float postDownSampleBuffer [frames] = {0};

    // fill preUpsampleBuffer
    for (int i = 0; i < frames; i++){
      preUpSampleBuffer[i] = osc_white();
    }

    // upsample (2x, going from 48kHz to 96kHz), populate postUpsampleBuffer
    s_Noise.aAFilter.upsample(preUpSampleBuffer, postUpSampleBuffer, frames);

    // do any processing needed (none)

    // decimate to postDownsampleBuffer (1/2x, going from 96kHz to 48kHz)
    s_Noise.aAFilter.decimate(postUpSampleBuffer, postDownSampleBuffer, frames);

    // copy into real buffer
    for (int i = 0; i < frames; i++){
      *(y++) = f32_to_q31(process_output(postDownSampleBuffer[i], osc, s.drive));
    }    
  }
  else if (osc == Noise::k_flag_pink){
    // Voss - McCartney algorithm, this might be able to be implemented cleaner 
    // or we could do a -3db/oct filter on white noise
    float preUpSampleBuffer [frames] = {0};
    float postUpSampleBuffer [frames *2] = {0};
    float postDownSampleBuffer [frames] = {0};

    uint8_t counter = s.counter;
    s.counter= s.counter + (frames % 128);    
    if (s.counter>127){
      s.counter = 0;
    }

    // fill preUpSampleBuffer
    for (int i = 0; i < frames; i++){
      float osc_white_total=osc_white(); // row -1 aadded in every counter

      if (counter % 2 == 0){
        s.row_0=osc_white();
      }
      else if ((counter-1) % 4 == 0){
        s.row_1=osc_white();        
      }
      else if ((counter -3) % 8 == 0){
        s.row_2=osc_white();
      }
      else if ((counter - 7) % 16 == 0){
        s.row_3=osc_white();
      }
      else if ((counter - 15) % 32 == 0){
        s.row_4=osc_white();
      }
      else if (counter==33 || counter==97){
        s.row_5=osc_white();
      }
      else if (counter==63) {
        s.row_6=osc_white();
      }
      osc_white_total +=s.row_0 + s.row_1 + s.row_2 + s.row_3 + s.row_4 + s.row_5 +s.row_6; 
      preUpSampleBuffer[i]=(osc_white_total/8.f);

      counter+=1;
      if (counter>127.f){
        counter = 0.f;
      }
    }

    // upsample (2x, going from 48kHz to 96kHz), populate postUpsampleBuffer
    s_Noise.aAFilter.upsample(preUpSampleBuffer, postUpSampleBuffer, frames);

    // do any processing needed (none)

    // decimate to postDownsampleBuffer (1/2x, going from 96kHz to 48kHz)
    s_Noise.aAFilter.decimate(postUpSampleBuffer, postDownSampleBuffer, frames);

    // copy into real buffer
    for (int i = 0; i < frames; i++){
      *(y++) = f32_to_q31(process_output(postDownSampleBuffer[i], osc, s.drive));
    }
  }
  else if (osc == Noise::k_flag_brown){
    // 6.02db/octave low pass filter on white noise, use first order filter
    const float ampAdjust = 1.99f;  // brown is a bit quiet so lets boost it some

    float preUpSampleBuffer [frames] = {0};
    float postUpSampleBuffer [frames *2] = {0};
    float postDownSampleBuffer [frames] = {0};

    // fill preUpsampleBuffer
    for (int i = 0; i < frames; i++){
      preUpSampleBuffer[i] = ampAdjust * s_Noise.brownFilter.process_fo(osc_white());
    }

    // upsample (2x, going from 48kHz to 96kHz), populate postUpsampleBuffer
    s_Noise.aAFilter.upsample(preUpSampleBuffer, postUpSampleBuffer, frames);

    // do any processing needed (none)

    // decimate to postDownsampleBuffer (1/2x, going from 96kHz to 48kHz)
    s_Noise.aAFilter.decimate(postUpSampleBuffer, postDownSampleBuffer, frames);

    // copy into real buffer
    for (int i = 0; i < frames; i++){
      *(y++) = f32_to_q31(process_output(postDownSampleBuffer[i], osc, s.drive));
    }    
  }
  else if (osc == Noise::k_flag_blue){
    // we are going to use pink noise and take the difference of successive samples, aka, pink noise with a first differential operator
    // Voss - McCartney algorithm, this might be able to be implemented cleaner
    const float ampAdjust = 1.99f;  // lets boost it some

    float preUpSampleBuffer [frames] = {0};
    float postUpSampleBuffer [frames *2] = {0};
    float postDownSampleBuffer [frames] = {0};

    uint8_t blueCounter = s.counter;
    s.counter= s.counter + (frames % 128);    
    if (s.counter>127){
      s.counter = 0;
    }

    // fill preUpSampleBuffer
    for (int i = 0; i < frames; i++){
      float osc_white_total=osc_white(); // row -1 aadded in every counter

      if (blueCounter % 2 == 0){
        s.blueRow_0=osc_white();
      }
      else if ((blueCounter-1) % 4 == 0){
        s.blueRow_1=osc_white();        
      }
      else if ((blueCounter -3) % 8 == 0){
        s.blueRow_2=osc_white();
      }
      else if ((blueCounter - 7) % 16 == 0){
        s.blueRow_3=osc_white();
      }
      else if ((blueCounter - 15) % 32 == 0){
        s.blueRow_4=osc_white();
      }
      else if (blueCounter==33 || blueCounter==97){
        s.blueRow_5=osc_white();
      }
      else if (blueCounter==63) {
        s.blueRow_6=osc_white();
      }
      osc_white_total +=s.blueRow_0 + s.blueRow_1 + s.blueRow_2 + s.blueRow_3 + s.blueRow_4 + s.blueRow_5 +s.blueRow_6; 
      preUpSampleBuffer[i]=ampAdjust * ((osc_white_total/8.f) - s.prev_sample);
      s.prev_sample = osc_white_total/8.f;

      blueCounter+=1;
      if (blueCounter>127.f){
        blueCounter = 0.f;
      }
    }

    // upsample (2x, going from 48kHz to 96kHz), populate postUpsampleBuffer
    s_Noise.aAFilter.upsample(preUpSampleBuffer, postUpSampleBuffer, frames);

    // do any processing needed (none)

    // decimate to postDownsampleBuffer (1/2x, going from 96kHz to 48kHz)
    s_Noise.aAFilter.decimate(postUpSampleBuffer, postDownSampleBuffer, frames);

    // copy into real buffer
    for (int i = 0; i < frames; i++){
      *(y++) = f32_to_q31(process_output(postDownSampleBuffer[i], osc, s.drive));
    }
  }
  else if (osc == Noise::k_flag_violet){
   // 6.02db/octave high pass filter on white noise, use first order filter
    const float ampAdjust = 1.99f;  // lets boost it some

    float preUpSampleBuffer [frames] = {0};
    float postUpSampleBuffer [frames *2] = {0};
    float postDownSampleBuffer [frames] = {0};

    // fill preUpsampleBuffer
    for (int i = 0; i < frames; i++){
      preUpSampleBuffer[i] = ampAdjust * s_Noise.violetFilter.process_fo(osc_white());
    }

    // upsample (2x, going from 48kHz to 96kHz), populate postUpsampleBuffer
    s_Noise.aAFilter.upsample(preUpSampleBuffer, postUpSampleBuffer, frames);

    // do any processing needed (none)

    // decimate to postDownsampleBuffer (1/2x, going from 96kHz to 48kHz)
    s_Noise.aAFilter.decimate(postUpSampleBuffer, postDownSampleBuffer, frames);

    // copy into real buffer
    for (int i = 0; i < frames; i++){
      *(y++) = f32_to_q31(process_output(postDownSampleBuffer[i], osc, s.drive));
    }    
  }
  else{
    // grey
    const float ampAdjust = 1.99f;  // lets boost it some

    float preUpSampleBuffer [frames] = {0};
    float postUpSampleBuffer [frames *2] = {0};
    float postDownSampleBuffer [frames] = {0};

  // fill preUpsampleBuffer
    for (int i = 0; i < frames; i++){
      float whiteNoise = osc_white();
      float intermediate;
      
      intermediate = s_Noise.greyLPFilter.process_so(whiteNoise);
      intermediate += s_Noise.greyHP3Filter.process_so(s_Noise.greyHP2Filter.process_so(s_Noise.greyHP1Filter.process_so(whiteNoise)));

      preUpSampleBuffer[i] = ampAdjust * (intermediate/2.f);
    }

    // upsample (2x, going from 48kHz to 96kHz), populate postUpsampleBuffer
    s_Noise.aAFilter.upsample(preUpSampleBuffer, postUpSampleBuffer, frames);

    // do any processing needed (none)

    // decimate to postDownsampleBuffer (1/2x, going from 96kHz to 48kHz)
    s_Noise.aAFilter.decimate(postUpSampleBuffer, postDownSampleBuffer, frames);

    // copy into real buffer
    for (int i = 0; i < frames; i++){
      *(y++) = f32_to_q31(process_output(postDownSampleBuffer[i], osc, s.drive));
    }
  }
}

void OSC_NOTEON(const user_osc_param_t * const params)
{  
  s_Noise.state.flags |= Noise::k_flag_reset;
}

void OSC_NOTEOFF(const user_osc_param_t * const params)
{
  // does nothing, prevents the compiler from complaining we are not using params
  (void)params;
}

void OSC_PARAM(uint16_t index, uint16_t value)
{  
  Noise::State &s = s_Noise.state;

  switch (index) {
  case k_user_osc_param_id1:
  case k_user_osc_param_id2:    
  case k_user_osc_param_id3:
  case k_user_osc_param_id4:
  case k_user_osc_param_id5:    
  case k_user_osc_param_id6:
    break;
    
  case k_user_osc_param_shape:
    {
      // 10bit parameter  (1024 possible values?)
      // we have 6 noise types
      const float user_osc_param = param_val_to_f32(value);
      
      if (user_osc_param<=.170){
        s.noise_type = Noise::k_flag_white;
      }
      else if (user_osc_param>.170 && user_osc_param<=.340){
        s.noise_type = Noise::k_flag_pink;
      }
      else if (user_osc_param>.340 && user_osc_param<=.510){
        s.noise_type = Noise::k_flag_brown;
      }
      else if (user_osc_param>.510 && user_osc_param<=.680){
        s.noise_type = Noise::k_flag_blue;
      }
      else if (user_osc_param>.680 && user_osc_param<=.850){
        s.noise_type = Noise::k_flag_violet;
      }
      else{
        s.noise_type = Noise::k_flag_grey;
      }
      break;
    }
  case k_user_osc_param_shiftshape:
    s.drive = param_val_to_f32(value);
    break;
    
  default:
    break;
  }
}

