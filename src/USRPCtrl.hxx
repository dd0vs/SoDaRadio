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

 ///
 ///  @file USRPCtrl.hxx
 ///  @brief Thread class that owns the USRP control channel and functions
 ///
 ///
 ///  @author M. H. Reilly (kb1vc)
 ///  @date   July 2013
 ///

#ifndef USRPCTRL_HDR
#define USRPCTRL_HDR
#include "SoDaBase.hxx"
#include "MultiMBox.hxx"
#include "Command.hxx"
#include "Params.hxx"
#include "RangeMap.hxx"

#include <uhd/utils/thread_priority.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/usrp/dboard_base.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/types/tune_result.hpp>
#include <uhd/utils/msg.hpp>

namespace SoDa {
  ///  @class USRPCtrl
  /// 
  ///  Though libuhd is designed to be re-entrant, there are some indications
  ///  that all control functions (set_freq, gain, and other operations)
  ///  should originate from a single thread.  USRPCtrl does that.
  ///
  ///  USRPCtrl listens on the Command Stream message channel for
  ///  requests from other components (including the SoDa::UI listener)
  ///  and dumps status and completion reports back onto
  ///  the command stream channel. 
  class USRPCtrl : public SoDaThread {
  public:
    /// Constructor
    /// Build a USRPCtrl thread
    /// @param params Pointer to a parameter object with all the initial settings
    /// and identification for the attached USRP
    /// @param _cmd_stream Pointer to the command stream message channel.
    USRPCtrl(Params * params, CmdMBox * _cmd_stream);
    /// start the thread
    void run();

    /// return a pointer to the multi_usrp object -- used by
    /// RX and TX processes to find the associated USRP widget.
    /// @return a pointer to the USRP radio object
    uhd::usrp::multi_usrp::sptr getUSRP() { return usrp; }

    /// Temporary message handler, while we're looking for
    /// "natural" front-end VCO settings.
    static void freq_search_message_handler(uhd::msg::type_t type, const std::string & msg);

    /// This is the more permanent message handler... 
    static void normal_message_handler(uhd::msg::type_t type, const std::string & msg);

    /// This is a singleton object -- the last (and only, we hope) such object
    /// to be created sets a static pointer to itself.  This looks pretty gross, but
    /// it is necessary to provide context to the error message handlers.
    static SoDa::USRPCtrl * singleton_ctrl_obj;
    
  private:
    Params * params;

    /// The B200 and B210 need some special handling, as they
    /// don't have frontend lock indications (as of 3.7.0)
    /// and need a special sample rate.
    bool is_B2xx;
    bool is_B210; ///< the B210 has two tx channels -- use the second for a Transverter LO -- see USRPLO

    /// Parse an incoming command and dispatch.
    /// @param cmd a command record
    void execCommand(Command * cmd);
    /// Dispatch an incoming GET command
    /// @param cmd a command record
    void execGetCommand(Command * cmd); 
    /// Dispatch an incoming SET command
    /// @param cmd a command record
    void execSetCommand(Command * cmd); 
    /// Dispatch an incoming REPort command
    /// @param cmd a command record
    void execRepCommand(Command * cmd); 

    /// get the number of seconds since the "Epoch"
    /// @return relative time in seconds
    double getTime();

    /// is the identified (rx or tx) front-end LO locked?
    /// If not, set the tuning frequency to "the right thing"
    /// @param req the requested frequency (and tuning discipline)
    /// @param sel 'r' for RX LO, 't' for TX LO
    /// @param cur tuning result, if the LO was locked. 
    uhd::tune_result_t checkLock(uhd::tune_request_t & req,
				 char sel,
				 uhd::tune_result_t & cur);


    /// Set the front-end (LO + DDS) frequency to 'freq'
    /// This includes setting the PLL front end synthesizer
    /// as well as the FPGA resident digital synthesizer.
    /// @param freq target frequency (LO and DDS combined)
    /// @param sel 'r' for RX LO, 't' for TX LO
    /// @param set_if_freq if TRUE, tell the USRPRX thread to reset
    /// its front-end frequency so that it can adjust its own oscillator.
    void set1stLOFreq(double freq, char sel, bool set_if_freq = false);

    
    CmdMBox * cmd_stream; ///< command stream channel
    unsigned int subid;   ///< subscriber ID for this thread's connection to the command channel

    // USRP stuff.
    uhd::usrp::multi_usrp::sptr usrp; ///< to which USRP unit is this connected?
    uhd::usrp::dboard_iface::sptr dboard;  ///< the daughterboard we're controlling
    // need this for TX/RX enable.
    uhd::property_tree::sptr tx_fe_subtree; ///< property tree from daughtercard module
    // need this for TX/RX enable.
    uhd::property_tree::sptr rx_fe_subtree; ///< property tree from daughtercard module

    // Capability Flags --
    bool supports_tx_gpio; ///< does this unit support GPIO signals?  (B2xx does not as of 3.7.0)
    
    // parallel IO to turn on RX ena and TX ena
    // specific to WBX right now.

    /// Initialize the GPIO control registers to set the
    /// direction and enables for the TX/RX relay output and sense input
    void initControlGPIO();

    /// get the state of the TXEna bit
    /// @return true if the TX relay is activated. 
    bool getTXEna();
    /// get the state of the TX relay confirm bit
    /// @return true if the TX relay sense input is asserted
    bool getTXRelayOn(); 
    /// turn TX on/off
    /// @param val true to enable the transmitter, false otherwise.
    void setTXEna(bool val);


    /// set the transvert LO frequency and power
    void setTransverterLOFreqPower(double freq, double power);
    void enableTransverterLO();
    void disableTransverterLO();
    
    /// we use TX_IO bit 12 to turn on the TX relay
    /// we use TX_IO bit 11 to monitor the TX relay
    static const unsigned int TX_RELAY_CTL; ///< mask for RELAY control bit
    static const unsigned int TX_RELAY_MON; ///< mask for RELAY sense bit
    
    uhd::tune_result_t last_rx_tune_result; ///< RX tune result -- actual LO and DSP freq
    uhd::tune_result_t last_tx_tune_result; ///< TX tune result
    uhd::tune_result_t saved_rx_tune_result; ///< previous RX tune result -- used for transverter LO "calibration" function

    double first_gettime; ///< timestamps are relative to the first timestamp.

    // gain settings
    double rx_rf_gain; ///< rf gain for RX front end amp/attenuator
    double tx_rf_gain; ///< rf gain for final TX amp

    uhd::gain_range_t rx_rf_gain_range; ///< property of the device -- what is the min/maximum RX gain setting?
    uhd::gain_range_t tx_rf_gain_range; ///< property of the device -- what is the min/maximum TX gain setting?

    uhd::freq_range_t rx_rf_freq_range; ///< property of the device -- what is the min/maximum RX frequency?
    uhd::freq_range_t tx_rf_freq_range; ///< property of the device -- what is the min/maximum TX frequency?

    // state of the box
    bool tx_on; ///< if true, we are transmitting.

    double last_rx_req_freq;  ///< remember the last setting -- useful for "calibration check" 
    
    // we horse the TX tuning around when we switch to RX
    // to move the transmit birdie out of band.
    double tx_freq; ///< remember current tx freq 
    double tx_freq_rxmode_offset; ///< when in RX mode, move tx off frequency to put the tx birdie out of band, when in TX mode, this is 0
    static const double rxmode_offset = 1e6; ///< tx offset when in RX mode

    double tx_samp_rate; ///< sample rate to USRP TX chain. 
    std::string tx_ant;  ///< TX antenna choice (usually has to be TX or TX/RX1?

    std::string motherboard_name; ///< The model name of the USRP unit

    // transverter local oscillator support.
    bool tvrt_lo_capable; ///< if true, this unit can implement a local transverter oscillator.
    bool tvrt_lo_mode; ///< if true, set the transmit frequency, with some knowledge of the tvrt LO.
    double tvrt_lo_gain; ///< output power for the second transmit channel (used for transverter LO)
    double tvrt_lo_freq; ///< the frequency of the second transmit channel oscillator
    double tvrt_lo_fe_freq; ///< the frequency of the second transmit channel front-end oscillator
    
    // enables verbose messages
    bool debug_mode; ///< print stuff when we are in debug mode

    // integer tuning mode is helped by a map of LO capabilities.

    /// @brief getNearestStep returns the nearest integer-N step
    /// to the supplied frequency that is at least <offset> Hz below it.
    /// @param freq -- target tuning frequency
    /// @param offset -- returned frequency will be < freq - offset
    double getNearestStep(double freq, double offset = 0.0); 
    SoDa::RangeMap<double, double> lo_step_map; /// map from range to bottom of range...rx_rf
    /// @brief initialize the step map by probing the LO tuning over the range
    void initStepMap();
    bool supports_IntN_Mode; 
  };
}


#endif
