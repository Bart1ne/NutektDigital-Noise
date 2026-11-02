#pragma once
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
 *  File: noise.hpp
 *
 *  Noise Synthesizer
 *
 */

#include "userosc.h"
#include "biquad.hpp"
#include "antialiasingfilter.hpp"

struct Noise{
  enum {
    k_flags_none   = 0,
    k_flag_reset  = 1<<1
  };

  enum {
    k_flag_white  = 0,
    k_flag_pink   = 1<<1,
    k_flag_brown  = 1<<2,
    k_flag_blue   = 1<<3,
    k_flag_violet = 1<<4,
    k_flag_grey   = 1<<5
  };

  struct State{
    float w0;
    float phase;
    float duty;
    float angle;
    float lfo, lfoz;
    uint8_t noise_type;
    float drive; // 0..1, controlled by SHIFT+SHAPE

    // for pink noise generation
    uint8_t counter;
    //  float row_minus_one;  recalculated every sample, no need to cache
    float row_0;
    float row_1;
    float row_2;
    float row_3;
    float row_4;
    float row_5;
    float row_6;

    // for blue noise generation
    float blueCounter;
    float prev_sample;
    //  float row_minus_one;  recalculated every sample, no need to cache
    float blueRow_0;
    float blueRow_1;
    float blueRow_2;
    float blueRow_3;
    float blueRow_4;
    float blueRow_5;
    float blueRow_6;

    uint8_t flags;
  };

  Noise(void) {
    init();
  }

  void init(void) {
    state = State();
    state.flags = k_flags_none;
    state.noise_type = k_flag_white;
    state.drive = 0.f;
    state.counter=0.0f;
    state.blueCounter=0.f;

    brownFilter.mCoeffs.setFOLP(tan(PI* brownFilter.mCoeffs.wc(16.35f, k_samplerate_recipf)));
    violetFilter.mCoeffs.setFOHP(tan(PI* violetFilter.mCoeffs.wc(16744.04f, k_samplerate_recipf)));

    greyLPFilter.mCoeffs.setSOLP(tan(PI* greyLPFilter.mCoeffs.wc(500.f,k_samplerate_recipf)), 1.f);
    greyHP1Filter.mCoeffs.setSOHP(tan(PI* greyHP1Filter.mCoeffs.wc(10000.f,k_samplerate_recipf)), 1.f);  
    greyHP2Filter.mCoeffs.setSOHP(tan(PI* greyHP2Filter.mCoeffs.wc(10000.f,k_samplerate_recipf)), 1.f);  
    greyHP3Filter.mCoeffs.setSOHP(tan(PI* greyHP3Filter.mCoeffs.wc(10000.f,k_samplerate_recipf)), 1.f);  

    aAFilter.init(24000.f);
  }

  State state;

  dsp::BiQuad brownFilter;
  dsp::BiQuad violetFilter;
  dsp::BiQuad greyLPFilter;
  dsp::BiQuad greyHP1Filter, greyHP2Filter, greyHP3Filter;
  AntiAliasingFilter aAFilter;
};

