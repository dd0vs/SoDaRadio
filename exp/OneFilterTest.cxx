/*
Copyright (c) 2012, Matthew H. Reilly (kb1vc)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#define SAMPLE_RATE 48000
#include <complex>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "OSFilter.hxx"
#include <time.h>
#include <fftw3.h>
#include <math.h>


#include <boost/format.hpp>

using namespace std;
#include <sys/time.h>

// the filters are all built for a 48KHz sample rate.
// now let's see what happens..

int main(int argc, char * argv[])
{
  (void) argc; (void) argv; 
  
  int i, j;
  const int samp_len = 65536;
  int inbuflen = 2304; 

  double sample_rate = 48000.0; 
  
  SoDa::OSFilter SSBFilter(80.0, 150.0, 2300.0, 2400.0, 512, 1.0, sample_rate, inbuflen); 
  SoDa::OSFilter CWFilter(300.0, 400.0, 900.0, 1000.0, 512, 1.0, sample_rate, inbuflen); 

  complex<float> in[samp_len];
  complex<float> ssb_out[samp_len];
  complex<float> cw_out[samp_len];


  // sweep the frequency from -22kHz to 22kHz 
  double freq; 
  for(freq = -22e3; freq < 22e3; freq += 5.0) {
    double phase_step = 2.0 * M_PI * freq / ((double) sample_rate); 
    double ang = 0.0; 
    for(i = 0; i < samp_len; i++) {
      in[i] = complex<float>(cos(ang), sin(ang)); 
      ang += phase_step; 
      if(ang > M_PI) ang = ang - 2.0 * M_PI;
      if(ang < -M_PI) ang = ang + 2.0 * M_PI; 
    }
    for(i = 0; i < (samp_len - inbuflen); i += inbuflen) {
      SSBFilter.apply(&(in[i]), &(ssb_out[i]));
      CWFilter.apply(&(in[i]), &(cw_out[i])); 
    }

    // now measure the phase and amplitude
    // do this somewhere near the middle... 

    double ssb_gain, ssb_phase, cw_gain, cw_phase; 
    for(i = 4000, j = 0; j < 5; i += 1000, j++) {
      ssb_gain = std::abs(ssb_out[i]) / std::abs(in[i]);
      ssb_phase = std::arg(ssb_out[i]) - std::arg(in[i]);
      cw_gain = std::abs(cw_out[i]) / std::abs(in[i]);
      cw_phase = std::arg(cw_out[i]) - std::arg(in[i]);
      if(ssb_phase > M_PI) ssb_phase -= 2.0*M_PI;
      if(ssb_phase < -M_PI) ssb_phase += 2.0*M_PI; 
      if(cw_phase > M_PI) cw_phase -= 2.0*M_PI;
      if(cw_phase < -M_PI) cw_phase += 2.0*M_PI;
      std::cout << boost::format("%12.8f %6g %6g %6g %6g\n") % freq % ssb_gain % ssb_phase % cw_gain % cw_phase; 
    }

  }


  return 0; 
}
