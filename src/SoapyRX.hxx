/*
Copyright (c) 2017, Matthew H. Reilly (kb1vc)
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

#ifndef SoapyRX_HDR
#define SoapyRX_HDR
#include "SoDaBase.hxx"
#include "MultiMBox.hxx"
#include "Command.hxx"
#include "Params.hxx"
#include "UI.hxx"
#include "QuadratureOscillator.hxx"
#include "SoapyCtrl.hxx"

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Types.hpp>

namespace SoDa {
  /**
   * The Receive RF Path
   *
   * @image html SoDa_Radio_RX_Signal_Path.svg
   */
  class SoapyRX : public SoDaThread {
  public:
    /**
     * The constructor
     *
     * @param params a pointer to a params object that will tell us
     *        about sample rates and other configuration details.
     * @param usrp a pointer to the UHD USRP object that we are streaming data from.
     * @param _rx_stream data mailbox used to pass the baseband (IF3) shifted
     *        incoming RF samples to other units
     * @param _if_stream data mailbox used to pass the IF2 data to the spectrum units
     * @param _cmd_stream data mailbox used to carry command, query, and report messages
     */
    SoapyRX(Params * params, SoDa::SoapyCtrl * _ctrl, 
	   DatMBox * _rx_stream, DatMBox * _if_stream,
	   CmdMBox * _cmd_stream);

    /**
     * SoapyRX is a thread -- this is its run loop. 
     */
    void run();
    
  private:   
    void execCommand(Command * cmd); 
    void execGetCommand(Command * cmd); 
    void execSetCommand(Command * cmd); 
    void execRepCommand(Command * cmd);

    void startStream();
    void stopStream(); 

    /**
     * @brief implement a complex down converter with complex multiplication
     *
     * This is a complex downconverter in the manner presented in Lyons pp 457-458
     *
     * @param inout the input/output RF buffer
     */
    void doMixer(SoDaBuf * inout);
    void set3rdLOFreq(double IF_tuning);

    DatMBox * rx_stream;
    DatMBox * if_stream; 
    CmdMBox * cmd_stream;
    unsigned int cmd_subs; 

    // the controller
    SoDa::SoapyCtrl * ctrl; 
    
    // state for the SDR widget
    SoapySDR::Stream * rx_bits;
    SoapySDR::Device * radio; 

    unsigned int rx_buffer_size;

    // are we collecting?
    bool audio_rx_stream_enabled;

    SoDa::Command::ModulationType rx_modulation;
    
    // pointer to user interface box -- we send it FFT snapshots
    UI * ui;

    // IF tuner
    QuadratureOscillator IF_osc;
    double current_IF_tuning;
    double rx_sample_rate;

    // spectrum reporting
    bool enable_spectrum_report; 
    
    //debug hooks
    int outf[2];
    int scount;

    std::ofstream rf_dumpfile;
    std::ofstream if_dumpfile; 
    
  }; 
}


#endif
