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

#include "BaseBandRX.hxx"
#include "OSFilter.hxx"
#include <fstream>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

SoDa::BaseBandRX::BaseBandRX(Params * params,
		       DatMBox * _rx_stream, CmdMBox * _cmd_stream,
		       AudioIfc * _audio_ifc) : SoDa::SoDaThread("BaseBandRX")
{
  audio_ifc = _audio_ifc; 
  rx_stream = _rx_stream;
  rx_subs = rx_stream->subscribe();

  cmd_stream = _cmd_stream; 
  cmd_subs = cmd_stream->subscribe();

  // set up some convenient defaults
  rx_modulation = SoDa::Command::USB;
  // what is the default sample rate and buffer size?
  // the audio rate is set and welded, as we have statically calculated
  // filter parameters. 
  audio_sample_rate = params->getAudioSampleRate(); 
  rf_sample_rate = params->getRXRate(); 
  audio_buffer_size = params->getAFBufferSize();
  rf_buffer_size = params->getRFBufferSize();

  // setup the audio level meter
  log_audio_buffer_size = log10((float) audio_buffer_size); 

  // setup the resampler now...
  buildFilterMap();

  // build the resamplers
  rf_resampler = new SoDa::TDResampler625x48<std::complex<float> >(150000.0);
  wbfm_resampler = new SoDa::TDResampler625x48<float>(1.0);  

  af_filter_selection = SoDa::Command::BW_6000;
  cur_audio_filter = filter_map[af_filter_selection];

  
  // initial af gain
  af_gain = 1.0;
  af_sidetone_gain = 1.0;
  cur_af_gain = &af_gain; 
  unsigned int i, j;

  // allocate the audio buffer pool
  bpool = new BufferPool<float>(audio_buffer_size);
  // tell the audio interface about it.
  audio_ifc->setRXBufferPool(bpool); 
  
  // prime the audio stream so that we don't fall behind
  // right away.
  for(j = 0; j < 6; j++) {
    sidetone_silence = bpool->getBuffer();
    for(i = 0; i < audio_buffer_size; i++) {
      sidetone_silence[i] = 0.0; 
    }
    if(j < 5) { // don't pend the last buffer, as we use it for background silence
      pendAudioBuffer(sidetone_silence); 
    }
  }
  // create hilbert transformer
  hilbert = new SoDa::HilbertTransformer(audio_buffer_size);

  // initialize the sample for the NBFM and WBFM demodulator
  last_phase_samp = 0.0;

  // setup the catchup mechanism that adjusts to differences
  // between the radio's clock frequency and the sound system's clock
  in_catchup = false;
  in_fallback = false;  
  // setup the random number generator.  Note that randomness
  // isn't nearly as important as an apparently long period.
  // The RNG is used to "steal" an audio sample out of the
  // stream in a pattern that won't be perceptible to human
  // hearing. 
  srandom(0x13245); 
  // we could do something mod(audio_buffer_size) but 2304 is
  // a tough point to make.  Since superlative randomness isn't
  // all that important, doing mod 2^k where 2^k is the largest
  // power of two less than the audio buffer size will ensure
  // reasonable distribution of the dropouts. (The trick is to
  // make the period undetectable.)
  for(catchup_rand_mask = 0x1;
      catchup_rand_mask < audio_buffer_size;
      catchup_rand_mask = ((catchup_rand_mask << 1) | 1));
  catchup_rand_mask = catchup_rand_mask >> 1;

  // debug help
  dbg_ctr = 0;


  audio_rx_stream_enabled = true;
  debugMsg("audio_rx_stream_enabled = true\n");  
  audio_rx_stream_needs_start = true;

  // log all audio to an output file (debug only....?)
  audio_save_enable = false;
  //audio_file.open("soda_audio.bin", std::ios::out | std::ios::binary);
  //audio_file2.open("soda_audio_iq_fm.bin", std::ios::out | std::ios::binary);

  // pend a null buffer or two just to keep the out stream from 
  // under-flowing
  pendNullBuffer(2);
}

void SoDa::BaseBandRX::demodulateWBFM(SoDaBuf * rxbuf, SoDa::Command::ModulationType mod, float af_gain)
{
  (void) mod;
  // now allocate a new audio buffer from the buffer ring
  float * audio_buffer = bpool->getBuffer();
  float demod_out[rf_buffer_size];
  unsigned int i;

  std::complex<float> * dbuf = rxbuf->getComplexBuf();

  // Interestingly, arctan based demodulation (see Lyons p 486 for instance)
  // performs much better than the approximation that avoids the atan call.
  // Texts that talk about atan generally don't talk about the problem of
  // rollover, where the sign changes from atan(samp[n]) and atan(samp[n+1]).
  // In this case, dphase will be much bigger than M_PI, and it should be
  // "corrected".  We're really trying to find the angular diference between
  // samples, so the wraparound is important. 
  for(i = 0; i < rf_buffer_size; i++) {
    // do the atan demod
    // measure the phase of the incoming signal.
    float phase = arg(dbuf[i]);
    float dphase = phase - last_phase_samp;
    if(dphase < -M_PI) dphase += 2.0 * M_PI;
    if(dphase > M_PI) dphase -= 2.0 * M_PI;
    demod_out[i] = af_gain * dphase; 
    last_phase_samp = phase; 
  }
  // now downsample it
  wbfm_resampler->apply(demod_out, audio_buffer, rf_buffer_size, audio_buffer_size);
  // do a median filter to eliminate the pops.
  // better not. fmMedianFilter.apply(audio_buffer, audio_buffer, audio_buffer_size); 
  // gain was arrived at by trial and error.  
  fm_audio_filter->apply(audio_buffer, audio_buffer, 0.0012);
  // then send it to the audio port.
  pendAudioBuffer(audio_buffer);
}

void SoDa::BaseBandRX::demodulateNBFM(std::complex<float> * dbuf, SoDa::Command::ModulationType mod, float af_gain)
{
  (void) mod;
  // now allocate a new audio buffer from the buffer ring
  float * audio_buffer = bpool->getBuffer();
  std::complex<float> demod_out[audio_buffer_size];

  // Interestingly, arctan based demodulation (see Lyons p 486 for instance)
  // performs much better than the approximation that avoids the atan call.
  // Texts that talk about atan generally don't talk about the problem of
  // rollover, where the sign changes from atan(samp[n]) and atan(samp[n+1]).
  // In this case, dphase will be much bigger than M_PI, and it should be
  // "corrected".  We're really trying to find the angular diference between
  // samples, so the wraparound is important. 
  unsigned int i; 
  for(i = 0; i < audio_buffer_size; i++) {
    // do the atan demod
    // measure the phase of the incoming signal.
    float phase = arg(dbuf[i]);
    float dphase = phase - last_phase_samp;
    if(dphase < -M_PI) dphase += 2.0 * M_PI;
    if(dphase > M_PI) dphase -= 2.0 * M_PI;
    demod_out[i] = af_gain * dphase; 
    last_phase_samp = phase; 
  }

  cur_audio_filter->apply(demod_out, demod_out, 25.0);
  
  if(audio_save_enable) {
    audio_file2.write((char*) demod_out, audio_buffer_size * sizeof(std::complex<float>));
  }
  for(i = 0; i < audio_buffer_size; i++) {
    audio_buffer[i] = demod_out[i].real(); 
  }
  // do a median filter to eliminate the pops.
  // maybe not... fmMedianFilter.apply(audio_buffer, audio_buffer, audio_buffer_size); 

  // then send it to the audio port.
  pendAudioBuffer(audio_buffer);
}

void SoDa::BaseBandRX::demodulateSSB(std::complex<float> * dbuf, SoDa::Command::ModulationType mod)
{
  // now allocate a new audio buffer from the buffer ring
  float * audio_buffer = bpool->getBuffer();

  // shift the Q channel by pi/2
  // note that this hilbert filter transforms the Q channel and delays the I channel
  hilbert->applyIQ(dbuf, dbuf); 

  // then add/subtract I/Q to a single real channel
  float sbmul = ((mod == SoDa::Command::LSB) || (mod == SoDa::Command::CW_L)) ? 1.0 : -1.0;
  unsigned int i; 
  for(i = 0; i < audio_buffer_size; i++) {
    audio_buffer[i] = (float) (dbuf[i].real() + sbmul * dbuf[i].imag()); 
  }
  // then send it to the audio port.
  pendAudioBuffer(audio_buffer);
}

void SoDa::BaseBandRX::demodulateAM(std::complex<float> * dbuf)
{
  // now allocate a new audio buffer from the buffer ring
  float * audio_buffer = bpool->getBuffer();

  unsigned int i;
  float maxval = 0.0;
  float sumsq = 0.0; 
  for(i = 0; i < audio_buffer_size; i++) {
    float v = 0.5 * abs(dbuf[i]);
    if(v > maxval) maxval = v; 
    sumsq += v * v; 
    audio_buffer[i] = v; 
  }
  sumsq = sqrt(sumsq / ((float) audio_buffer_size));
  // if((dbg_ctr & 0xff) == 0) {
  //   std::cerr << boost::format("maxval = %f rms = %f\n") % maxval % sumsq;
  // }

  // audio is biased above DC... it really really needs to get its DC component removed. 
  am_audio_filter->apply(audio_buffer, audio_buffer); 

  // then send it to the audio port.
  pendAudioBuffer(audio_buffer);
}

void SoDa::BaseBandRX::demodulate(SoDaBuf * rxbuf)
{
  // First we downsample and apply the audio filter unless this is a WBFM signal.
  std::complex<float> dbufi[audio_buffer_size]; 
  std::complex<float> dbufo[audio_buffer_size]; 
  // Note that audio_buffer_size must be (sample_length / decimation rate)
  
  if((rx_modulation != SoDa::Command::WBFM) && (rx_modulation != SoDa::Command::NBFM)) {
    rf_resampler->apply(rxbuf->getComplexBuf(), dbufi, rf_buffer_size, audio_buffer_size);
    
    // now do the low pass filter
    if(rx_modulation == SoDa::Command::AM) {
      am_pre_filter->apply(dbufi, dbufo, *cur_af_gain); 
    }
    else {
      cur_audio_filter->apply(dbufi, dbufo, *cur_af_gain);
    }
  }
  else if(rx_modulation == SoDa::Command::NBFM) {
    // first, bandpass the RF down to about 25 kHz wide...
    std::complex<float> * rfbuf = rxbuf->getComplexBuf();
    nbfm_pre_filter->apply(rfbuf, rfbuf, 1.0);
    rf_resampler->apply(rfbuf, dbufo, rf_buffer_size, audio_buffer_size);
  }

 
  switch(rx_modulation) {
  case SoDa::Command::LSB:
  case SoDa::Command::CW_L:
    demodulateSSB(dbufo, SoDa::Command::LSB); 
    break; 
  case SoDa::Command::USB:
  case SoDa::Command::CW_U:
    demodulateSSB(dbufo, SoDa::Command::USB); 
    break;
  case SoDa::Command::NBFM:
    demodulateNBFM(dbufo, SoDa::Command::NBFM, *cur_af_gain);
    break; 
  case SoDa::Command::WBFM:
    demodulateWBFM(rxbuf, SoDa::Command::NBFM, *cur_af_gain);
    break; 
  case SoDa::Command::AM:
    demodulateAM(dbufo); 
    break; 
  default:
    // all other modes are unsupported just for now.
    throw(new SoDa::SoDaException("Unsupported Modulation Mode in RX", this)); 
    break; 
  }
}

void SoDa::BaseBandRX::repAFFilterShape() {
  std::pair<double, double> fshape = cur_audio_filter->getFilterEdges();  
  switch (rx_modulation) {
  case SoDa::Command::USB:
  case SoDa::Command::CW_U:
      cmd_stream->put(new Command(Command::REP, Command::RX_AF_FILTER_SHAPE, 
				  fshape.first, fshape.second));
      break; 
  case SoDa::Command::LSB:
  case SoDa::Command::CW_L:
      cmd_stream->put(new Command(Command::REP, Command::RX_AF_FILTER_SHAPE, 
				  -fshape.first, -fshape.second));
      break; 
  case SoDa::Command::AM:
      cmd_stream->put(new Command(Command::REP, Command::RX_AF_FILTER_SHAPE, 
				  -fshape.second, fshape.second));
      break; 
  default:
      cmd_stream->put(new Command(Command::REP, Command::RX_AF_FILTER_SHAPE, 
				  -100, 100));
    
  }
}

void SoDa::BaseBandRX::execSetCommand(SoDa::Command * cmd)
{
  SoDa::Command::AudioFilterBW fbw;
  SoDa::Command::ModulationType txmod; 
  switch (cmd->target) {
  case SoDa::Command::RX_MODE:
    rx_modulation = SoDa::Command::ModulationType(cmd->iparms[0]);
    repAFFilterShape();    
    break;
  case SoDa::Command::TX_MODE:
    txmod = SoDa::Command::ModulationType(cmd->iparms[0]);
    if((txmod == SoDa::Command::CW_L) || (txmod == SoDa::Command::CW_U)) {
      sidetone_stream_enabled = true;
    }
    else {
      sidetone_stream_enabled = false; 
    }
    break; 
  case SoDa::Command::TX_STATE: // SET TX_ON
    if(cmd->iparms[0] == 1) {
      // flush the audio buffers that have RX info that we
      // aren't going to need anymore.
      debugMsg("In TX ON");      
      flushAudioBuffers(); 
      if (sidetone_stream_enabled) {
	debugMsg("sidetone mode\n"); 
	cur_af_gain = &af_sidetone_gain;
      }
      else {
	audio_rx_stream_enabled = false;
	debugMsg("audio_rx_stream_enabled = false\n");
	// 	audio_ifc->sleepOut();
      }
    }
    if(cmd->iparms[0] == 2) { // the CTRL unit has done the setup.... 
      debugMsg("In RX ON");
      cur_af_gain = &af_gain; 
      audio_rx_stream_enabled = true;
      debugMsg("audio_rx_stream_enabled = true\n");      
    }
    break;
  case SoDa::Command::RX_AF_FILTER: // set af filter bw.
    fbw = (SoDa::Command::AudioFilterBW) cmd->iparms[0];
    if(filter_map.find(fbw) != filter_map.end()) {
      cur_audio_filter = filter_map[fbw];
      af_filter_selection = fbw; 
    }
    else {
      // if unsupported -- use widest. 
      cur_audio_filter = filter_map[SoDa::Command::BW_6000]; 
      af_filter_selection = SoDa::Command::BW_6000;
    }
    {
      cmd_stream->put(new Command(Command::REP, Command::RX_AF_FILTER, 
				  af_filter_selection));
      repAFFilterShape();
    }
    break; 
  case SoDa::Command::RX_AF_GAIN: // set audio gain. 
    af_gain = powf(10.0, 0.25 * (cmd->dparms[0] - 50.0));
    cmd_stream->put(new Command(Command::REP, Command::RX_AF_GAIN, 
				50. + 4.0 * log10(af_gain)));
    break; 
  case SoDa::Command::RX_AF_SIDETONE_GAIN: // set audio gain. 
    af_sidetone_gain = powf(10.0, 0.25 * (cmd->dparms[0] - 50.0));
    cmd_stream->put(new Command(Command::REP, Command::RX_AF_SIDETONE_GAIN, 
				50. + 4.0 * log10(af_sidetone_gain)));
    break; 
  default:
    break; 
  }
}

void SoDa::BaseBandRX::execGetCommand(SoDa::Command * cmd)
{
  switch (cmd->target) {
  case SoDa::Command::RX_AF_FILTER: // set af filter bw.
    cmd_stream->put(new Command(Command::REP, Command::RX_AF_FILTER, 
				af_filter_selection));
    break;
  case SoDa::Command::RX_AF_GAIN: // set af filter bw.
    cmd_stream->put(new Command(Command::REP, Command::RX_AF_GAIN, 
				50.0 + 4.0 * log10(af_gain)));
    break;
  case SoDa::Command::DBG_REP: // report status
    SoDa::Command::UnitSelector us;
    us = SoDa::Command::UnitSelector(cmd->iparms[0]);
    if(us == SoDa::Command::BaseBandRX) {
      std::cerr << boost::format("%s ready_buffers.size = %d free_buffers.size = %d\n") % getObjName() % readyAudioBuffers() % free_buffers.size();
    }
    break; 
  default:
    break; 
  }

}

void SoDa::BaseBandRX::execRepCommand(SoDa::Command * cmd)
{
  (void) cmd; 
}

void SoDa::BaseBandRX::run()
{
  bool exitflag = false;
  SoDaBuf * rxbuf;
  Command * cmd; 

  int trim_count = 0; 
  int add_count = 0;     

  int null_audio_buf_count = 0;
  int sleep_count = 0; 
  int catchup_count = 0;

  int restart_count = 0;

  while(!exitflag) {
    bool did_work = false;
    bool did_audio_work = false; 

    if((cmd = cmd_stream->get(cmd_subs)) != NULL) {
      // process the command.
      execCommand(cmd);
      did_work = true; 
      exitflag |= (cmd->target == Command::STOP); 
      cmd_stream->free(cmd); 
    }

    // Send audio buffers to the audio output
    if(readyAudioBuffers()) {
      if(readyAudioBuffers() < 2) {
	// this is an emergency -- pend a few empty buffers
	pendNullBuffer(3);
      }
      if(!in_catchup && (readyAudioBuffers() > 8)) {
	// If the audio output has fallen 8 buffers behind, (about 400mS)
	// then go into catchup mode (shorten each outbound buffer), 
	// where we'll gain about 0.4 mS
	// on each 2304 sample frame.
	std::cerr << "BaseBandRX: enter catchup mode\n";		
	in_catchup = true;
	in_fallback = false; 
	catchup_count++; 
      }
      if(in_catchup && (readyAudioBuffers() < 6)) { /// 6)) {
	std::cerr << "BaseBandRX: exit catchup mode\n";
	in_catchup = false; 
      }
      if(!in_fallback && (readyAudioBuffers() < 3)) {
	std::cerr << "BaseBandRX: enter fallback mode\n";	
	in_fallback = true; 
	in_catchup = false; 
      }
      if(in_fallback && (readyAudioBuffers() > 5)) {
	std::cerr << "BaseBandRX: exit fallback mode\n";	
	in_fallback = false;
      }

      int rbsize = readyAudioBuffers(); 
      if(rbsize > 1) {

	if(audio_rx_stream_needs_start) {
	  audio_ifc->wakeOut(); 
	  audio_rx_stream_needs_start = false; 
	  restart_count++; 
	}
	  
	while(1) {
	  if(audio_ifc->sendBufferReady(audio_buffer_size + (in_fallback ? 1 : 0))) {
	    float * outb = getNextAudioBuffer();
	    if(outb == NULL) {
	      break; 
	    }

	    did_work = true;
	    did_audio_work = true; 

	    if(in_catchup) {
	      // this is where the random generator comes in.
	      int trim = (random() & catchup_rand_mask);
	      // drop out one "randomly" selected sample in the first (power of two) part of the buffer.
	      audio_ifc->send(outb, trim);
	      audio_ifc->send(&(outb[trim+1]), (audio_buffer_size - (trim + 1))); 
	      trim_count++; 
	      catchup_count++; 
	    }
	    else if(in_fallback) {
	      // duplicate one "randomly" selected sample in the first (power of two) part of the buffer.
	      int dup = (random() & catchup_rand_mask);
	      audio_ifc->send(outb, dup);
	      audio_ifc->send(&(outb[dup]), (audio_buffer_size - dup)); 
	      add_count++; 
	    }
	    else {
	      audio_ifc->send(outb, audio_buffer_size);
	    }

	    // is this a problem?  Is it possible to free the buffer too soon? 
	    bpool->freeBuffer(outb);

	  }
	  else {
	    null_audio_buf_count++; 
	    if(rbsize > 10) {
	      // what is the state of the audio ifc? 
	      debugMsg(boost::format("Audio State [%s] rbsize = %d\n") % audio_ifc->currentPlaybackState() % rbsize);
	    }
	    break; 
	  }
	}
      }
    }

    // now look for incoming buffers from the rx_stream. 
    int bcount = 0; 
    for(bcount = 0; (bcount < 2) && ((rxbuf = rx_stream->get(rx_subs)) != NULL); bcount++) {
      if(rxbuf == NULL) break; 
      did_work = true; 
      // if we're in TX mode, we should just pend silence and ignore the incoming buffer
      // otherwise, demodulate it.

      if(audio_rx_stream_enabled) {
	// demodulate the buffer.
	demodulate(rxbuf); 
      }
      else {
	pendNullBuffer();
      }
      // now free the buffer up.
      rx_stream->free(rxbuf); 
    }


    if(!did_audio_work && !did_work) {
      usleep(1000); 
      sleep_count++; 
    }
  }
  // close(outdump); 

  if(audio_save_enable) {
    audio_file.close();
    audio_file2.close();    
  }
}




int SoDa::BaseBandRX::readyAudioBuffers() 
{
  std::lock_guard<std::mutex> lock(ready_mutex);
  return ready_buffers.size();
}

void SoDa::BaseBandRX::pendNullBuffer(int count) {
  for(int b = 0; b < count; b++) {
    float * nullbuf = bpool->getBuffer();
    for(int i = 0; i < audio_buffer_size; i++) {
      nullbuf[i] = 0.0; 
    }
    pendAudioBuffer(nullbuf);
  }
}

void SoDa::BaseBandRX::pendAudioBuffer(float * b)
{
  {
    std::lock_guard<std::mutex> lock(ready_mutex);
    ready_buffers.push(b);
  }
  if(audio_save_enable) {
    audio_file.write((char*) b, audio_buffer_size * sizeof(float));
  }
  float al = 1.0e-19; // really small...
  for(int i = 0; i < audio_buffer_size; i++) {
    al += b[i] * b[i]; 
  }
  audio_level = 10.0 * (log10(al / af_gain) - log_audio_buffer_size); 
}

float * SoDa::BaseBandRX::getNextAudioBuffer()
{
  std::lock_guard<std::mutex> lock(ready_mutex); 
  if(ready_buffers.empty()) return NULL;
  float * ret;
  ret = ready_buffers.front();
  ready_buffers.pop();
  return ret; 
}

void SoDa::BaseBandRX::flushAudioBuffers()
{
  std::lock_guard<std::mutex> lock(ready_mutex); 
  while(!ready_buffers.empty()) {
    float * v = ready_buffers.front(); 
    ready_buffers.pop();
    bpool->freeBuffer(v);
  }
  return;
}

void SoDa::BaseBandRX::buildFilterMap()
{
  // Each filter is 512 samples long... (a really big filter)
  // The Overlap and Save buffer needs to be long enough to make this all
  // work
  
  filter_map[SoDa::Command::BW_2000] = new SoDa::OSFilter(200.0, 300.0, 2300.0, 2400.0, 512, 1.0, audio_sample_rate, audio_buffer_size);
  filter_map[SoDa::Command::BW_WSPR] = new SoDa::OSFilter(1100.0, 1400.0, 1600.0, 1900.0, 512, 1.0, audio_sample_rate, audio_buffer_size);  
  filter_map[SoDa::Command::BW_500] = new SoDa::OSFilter(300.0, 400.0, 900.0, 1000.0, 512, 1.0, audio_sample_rate, audio_buffer_size);

  filter_map[SoDa::Command::BW_100] = new SoDa::OSFilter(300.0, 400.0, 500.0, 600.0, 512, 1.0, audio_sample_rate, audio_buffer_size);

  filter_map[SoDa::Command::BW_6000] = new SoDa::OSFilter(200.0, 300.0, 6300.0, 6400.0, 512, 1.0, audio_sample_rate, audio_buffer_size);
  filter_map[SoDa::Command::BW_PASS] = new SoDa::OSFilter(0.0, 10.0, 15000.0, 18000.0, 512, 1.0, audio_sample_rate, audio_buffer_size);

  fm_audio_filter = new SoDa::OSFilter(50.0, 100.0, 8000.0, 9000.0, 512, 1.0, audio_sample_rate, audio_buffer_size);
  am_audio_filter = filter_map[SoDa::Command::BW_6000]; 

  am_pre_filter = new SoDa::OSFilter(0.0, 0.0, 8000.0, 9000.0, 512, 1.0, audio_sample_rate, audio_buffer_size);

  nbfm_pre_filter = new SoDa::OSFilter(0.0, 0.0, 25000.0, 32000.0, 512, 1.0, rf_sample_rate, rf_buffer_size);


}
