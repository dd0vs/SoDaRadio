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
extern "C" {
#include "dem-gridlib.h"
}
#include "SoDaRadio_Top.h"
#include <wx/string.h>
#include <wx/wx.h>
#include <wx/colour.h>
#include "../src/Command.hxx"
#include <boost/format.hpp>
#include "SoDaLogo.xpm"

/**
 * @file SoDaRadio_Top.cxx
 *
 * @brief The toplevel methods for the SoDaRadio GUI
 *
 * @author Matt Reilly (kb1vc)
 */

namespace SoDaRadio_GUI {
  /**
   * The SoDaRadio object
   *
   * The SoDaRadio program provides a wxWidgets based GUI to control
   * the SoDa radio SDR control program. SoDaRadio is partitioned into
   * three major components
   *
   * @li The Application object defined in SoDaRadio_App that launches the 
   * main GUI control thread defined by SoDaRadio_Top
   * @li the RadioListenener object that connects to the SoDa::UI thread.
   * @li the wxWidgets GUI event loop that dispatches user requests
   * through the SoDaRadio_Top thread.
   */
  SoDaRadio_Top::SoDaRadio_Top( SoDa::GuiParams & params,  wxWindow* parent )
:
    SoDaRadioFrame( parent )
  {
    // revision string is initially empty
    SDR_version_string[0] = '\000';

    // make an icon.
    wxIcon logo((char **) &SoDaLogo);
    this->SetIcon(logo);

    debug_mode = false; 
    // setup the comm channel.
    std::string sock_basename = params.getServerSocketBasename(); 
    soda_radio = new SoDa::UD::ClientSocket(sock_basename + "_cmd");
    soda_fft = new SoDa::UD::ClientSocket(sock_basename + "_wfall");

    // create the listener thread
    if(debug_mode) {
      std::cerr << "Creating listener thread." << std::endl;
    }
    listener = new RadioListenerThread(this);
    // now launch it.
    if(debug_mode) {
      std::cerr << "Launching listener thread." << std::endl;
    }
    if(listener->Create() != wxTHREAD_NO_ERROR) {
      wxLogError(wxT("Couldn't create radio listener thread...")); 
    }

  
    // what is the default button background color? 
    default_button_bg_color = m_PTT->GetBackgroundColour();
  
    // set the initial radio state
    tx_on = false;
    cw_mode = false;
    dead_carrier = false; 
  

    config_tree = NULL;
    from_callsign = wxT("");
    from_grid = wxT("");  
    to_callsign = wxT("");
    to_grid = wxT("");  

    // create the periodogram plot
    // and init its controls
    cfreq_step = 25; 
    int xs, ys;
    FFTPanel->GetSize(&xs, &ys);
    if(debug_mode) {
      std::cerr << "size of FFT panel x,y " << xs << " " << ys << std::endl;
    }

    // xs = 1050;
    // ys = 200;
    pgram_plot = new XYPlot(FFTPanel, this, wxID_ANY, 
			    wxDefaultPosition,  wxSize(xs, ys), //wxDefaultSize, // 
			    XYPlot::DRAW_LABEL | XYPlot::DRAW_MARKERS |
			    XYPlot::DRAW_TITLE | XYPlot::DRAW_VERT_MARKER_BANDS 
			    );

    pgram_plot->SetXTicTemplate(wxT("%4.1f"), 1e-3);

    wfall_plot = new Waterfall(WfallPanel, this, wxID_ANY, 
			       wxDefaultPosition, wxSize(xs, ys), // wxDefaultSize, //
			       Waterfall::DRAW_LABEL | 
			       Waterfall::DRAW_TITLE 
			       );

    wfall_plot->SetXTicTemplate(wxT("%4.1f"), 1e-3);
    // broken wfall_plot->SetCenterFreqTemplate(wxT("%9.4f"));

    SetSpectrum(50.0);

    if(debug_mode) {
      std::cerr << "Running listener thread." << std::endl;
    }
    listener->Run(); 

    // setup the tuner
    tuner = new TuningDialog(this, this);

    // setup the controls dialog  
    tx_rf_outpower = 20.0;
    controls = new ControlsDialog(this, this);
    controls->setTXPower(tx_rf_outpower); 

    // setup the transverter offset dialog
    transconf = new TransverterConfigDialog(this, this);

    // setup the Log dialog
    logdialog = new LogDialog(this, this); 
  
    if(debug_mode) {
      std::cerr << "about to load configuration." << std::endl;
    }
    // load the configuration from a default file,
    // if available.
    std::string cfn = params.getConfigFileName(); 
    wxString config_filename(cfn.c_str(), wxConvUTF8);
    LoadSoDaConfig(config_filename);

    // now open the logfile, if any
    std::string lfn = params.getLogFileName();
    wxString log_filename(lfn.c_str(), wxConvUTF8);
    OpenSoDaLog(log_filename); 

    if(debug_mode) {
      std::cerr << "loaded configuration." << std::endl;
    }
  
    save_config_file_name = wxT("");

  
    // Now connect up a few events
    Connect(MSG_UPDATE_SPECTRUM, wxEVT_COMMAND_MENU_SELECTED,
	    wxCommandEventHandler(SoDaRadio_Top::OnUpdateSpectrumPlot));
    Connect(MSG_UPDATE_GPSLOC, wxEVT_COMMAND_MENU_SELECTED,
	    wxCommandEventHandler(SoDaRadio_Top::OnUpdateGPSLoc));
    Connect(MSG_UPDATE_GPSTIME, wxEVT_COMMAND_MENU_SELECTED,
	    wxCommandEventHandler(SoDaRadio_Top::OnUpdateGPSTime));
    Connect(MSG_TERMINATE_TX, wxEVT_COMMAND_MENU_SELECTED,
	    wxCommandEventHandler(SoDaRadio_Top::OnTerminateTX));

    // setup status bar -- hardwire the accelerators for now.
    m_ClueBar->SetStatusText(wxT("^C Set To Call        ^G Set To Grid        ^L Enter Log Comment        ^X Enter CW Text"), 0);
  }

  bool SoDaRadio_Top::CreateSpectrumTrace(double * freqs, float * powers, unsigned int len)
  {
    if(pgram_plot == NULL) return false; 
    if(wfall_plot == NULL) return false;
  
    pgram_trace = new XYPlot::Trace(freqs, powers, len, 1, wxT(""));
    wfall_plot->RegisterBuffers(freqs, powers, len); 
    wxColor t1(0xff, 0x00, 0x00, 0x80); 
    pgram_plot->AddTrace(0, t1, pgram_trace);
    return true; 
  }

  ControlsDialog::ControlsDialog(wxWindow * parent, SoDaRadio_Top * radio) :
    m_ControlsDialog(parent)
  {
    // remember the toplevel control object.
    radio_top = radio;
  }

  wxString freq2wxString(double freq)
  {
    int GHz = floor(freq / 1e9);
    int MHz = floor((freq - ((double)GHz) * 1e9) / 1e6);
    int KHz = floor((freq - ((double)GHz) * 1e9 - ((double) MHz) * 1e6) / 1e3);
    int Hz = floor((freq - ((double)GHz) * 1e9 - ((double) MHz) * 1e6 - ((double) KHz) * 1e3));

    // now print as GHz,MHz.KHz Hz
    wxString s;
    if(GHz > 0) {
      s.Printf(wxT("%2d,%03d.%03d %03d"), GHz, MHz, KHz, Hz);
    }
    else {
      s.Printf(wxT("   %03d.%03d %03d"), MHz, KHz, Hz);
    }

    return s;
  }

  void SoDaRadio_Top::UpdateRXFreq(double freq)
  {
    // update the rx frequency field and all display markers.
    rx_frequency = freq;

    // update the tuner
    tuner->newRXFreq();
  
    // now udpate the display
    wxString freqstring = freq2wxString(freq);
  
    m_RXFreqText->SetLabel(freqstring);

    // and update the radio
    SoDa::Command ncmd(SoDa::Command::SET, SoDa::Command::RX_RETUNE_FREQ,
		       applyRXTVOffset(rx_frequency));
    sendMsg(&ncmd);

  }

  void SoDaRadio_Top::UpdateTXFreq(double freq)
  {
    // update the tx frequency field and all display markers.
    tx_frequency = freq;

    // update the tuner
    tuner->newTXFreq(); 
  
    // now udpate the display
    wxString freqstring = freq2wxString(freq); 
    m_TXFreqText->SetLabel(freqstring);

    // and update the radio
    SoDa::Command ncmd(SoDa::Command::SET, SoDa::Command::TX_RETUNE_FREQ,
		       applyRXTVOffset(tx_frequency));
    sendMsg(&ncmd);
  }

  void SoDaRadio_Top::setGPSLoc(double lat, double lon)
  {
    std::string slat = (boost::format("%6.3f") % lat).str();
    std::string slon = (boost::format("%7.3f") % lon).str();
    wxString wxslat(slat.c_str(), wxConvUTF8);
    wxString wxslon(slon.c_str(), wxConvUTF8);

    GPS_Lat_Str = wxslat;
    GPS_Lon_Str = wxslon;

    DMSpoint latlon;
    DEM_FloattoDMS(lat, lon, &latlon);
    char gridbuf[128];
    DEM_DMStoGrid(gridbuf, &latlon);
  

    GPS_Grid_Str = wxString::FromUTF8(gridbuf);

    // now post the update
    wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED,
			 SoDaRadio_Top::MSG_UPDATE_GPSLOC);
    AddPendingEvent(event); 
  }

  void SoDaRadio_Top::setGPSTime(unsigned char h, unsigned char m, unsigned char s)
  {

    std::string stim = (boost::format("%02d:%02d:%02d")
			% ((unsigned int) h)
			% ((unsigned int) m)
			% ((unsigned int) s)).str();
    wxString wxstim(stim.c_str(), wxConvUTF8);

    GPS_UTC_Str = wxstim;

    // now post the update
    wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED,
			 SoDaRadio_Top::MSG_UPDATE_GPSTIME);
    AddPendingEvent(event); 
  
  }


  void TransverterConfigDialog::UpdateSettings()
  {
    if(radio_top->GetLOMult() != 0) {
      TransverterModeEna->SetValue(true);
      wxString loval;
      loval.Printf(wxT("%f"), radio_top->GetLONominalBase() * 1e-6);
      TransverterLOFreq->SetValue(loval);
    }
    else {
      TransverterModeEna->SetValue(false);    
    }

  
  }

  void TransverterConfigDialog::ReadSettings()
  {
    if(TransverterModeEna->IsChecked()) {
      double v;
      v = (double) TransverterLOMult->GetValue();
      radio_top->SetLOMult(v);
      if(TransverterLOFreq->GetValue().ToDouble(&v)) {
	radio_top->SetLONominalBase(v * 1e6); 
      }
    }
    else {
      radio_top->SetLOMult(0); 
    }
  }



  TuningDialog::TuningDialog(wxWindow * parent, SoDaRadio_Top * radio) :
    m_TuningDialog(parent)
  {
    // build map for digits.
    TunerDigit *lrx, *ltx;
    int i;
    lrx = ltx = NULL;

    // remember the toplevel control object.
    radio_top = radio;

    // we're really controlling the rx and tx freqs in
    // the toplevel widget.  This used to be much cleaner,
    // but then we demoted the tuning box to a separate
    // dialog. 
    double * rx_freq_p = &(radio_top->rx_frequency); 
    double * tx_freq_p = &(radio_top->tx_frequency);

    // connect all the tuner pieces. 
    wxTextCtrl * rdtxt[11], * tdtxt[11];

    rdtxt[10] = digitTextR10; 
    rdtxt[9] = digitTextR9; 
    rdtxt[8] = digitTextR8; 
    rdtxt[7] = digitTextR7; 
    rdtxt[6] = digitTextR6; 
    rdtxt[5] = digitTextR5; 
    rdtxt[4] = digitTextR4; 
    rdtxt[3] = digitTextR3; 
    rdtxt[2] = digitTextR2; 
    rdtxt[1] = digitTextR1; 
    rdtxt[0] = digitTextR0; 

    tdtxt[10] = digitTextT10; 
    tdtxt[9] = digitTextT9; 
    tdtxt[8] = digitTextT8; 
    tdtxt[7] = digitTextT7; 
    tdtxt[6] = digitTextT6; 
    tdtxt[5] = digitTextT5; 
    tdtxt[4] = digitTextT4; 
    tdtxt[3] = digitTextT3; 
    tdtxt[2] = digitTextT2; 
    tdtxt[1] = digitTextT1; 
    tdtxt[0] = digitTextT0; 
  
    for(i = 10; i >= 0; i--) {
      lrx = rx[i] = new TunerDigit(rx_freq_p, i, lrx, rdtxt[i]);
      ltx = tx[i] = new TunerDigit(tx_freq_p, i, ltx, tdtxt[i]);
    }
  
    for(i = 10; i >= 0; i--) {
      rx[i]->setLast(rx[0]); 
      tx[i]->setLast(tx[0]); 
    }
  
    rx_tuner[m_DigitUpR10] = rx[10]; 
    rx_tuner[m_DigitUpR9] = rx[9]; 
    rx_tuner[m_DigitUpR8] = rx[8]; 
    rx_tuner[m_DigitUpR7] = rx[7]; 
    rx_tuner[m_DigitUpR6] = rx[6]; 
    rx_tuner[m_DigitUpR5] = rx[5]; 
    rx_tuner[m_DigitUpR4] = rx[4]; 
    rx_tuner[m_DigitUpR3] = rx[3]; 
    rx_tuner[m_DigitUpR2] = rx[2]; 
    rx_tuner[m_DigitUpR1] = rx[1]; 
    rx_tuner[m_DigitUpR0] = rx[0];   

    rx_tuner[m_DigitDownR10] = rx[10]; 
    rx_tuner[m_DigitDownR9] = rx[9]; 
    rx_tuner[m_DigitDownR8] = rx[8]; 
    rx_tuner[m_DigitDownR7] = rx[7]; 
    rx_tuner[m_DigitDownR6] = rx[6]; 
    rx_tuner[m_DigitDownR5] = rx[5]; 
    rx_tuner[m_DigitDownR4] = rx[4]; 
    rx_tuner[m_DigitDownR3] = rx[3]; 
    rx_tuner[m_DigitDownR2] = rx[2]; 
    rx_tuner[m_DigitDownR1] = rx[1]; 
    rx_tuner[m_DigitDownR0] = rx[0];   

    tx_tuner[m_DigitUpT10] = tx[10]; 
    tx_tuner[m_DigitUpT9] = tx[9]; 
    tx_tuner[m_DigitUpT8] = tx[8]; 
    tx_tuner[m_DigitUpT7] = tx[7]; 
    tx_tuner[m_DigitUpT6] = tx[6]; 
    tx_tuner[m_DigitUpT5] = tx[5]; 
    tx_tuner[m_DigitUpT4] = tx[4]; 
    tx_tuner[m_DigitUpT3] = tx[3]; 
    tx_tuner[m_DigitUpT2] = tx[2]; 
    tx_tuner[m_DigitUpT1] = tx[1]; 
    tx_tuner[m_DigitUpT0] = tx[0];   

    tx_tuner[m_DigitDownT10] = tx[10]; 
    tx_tuner[m_DigitDownT9] = tx[9]; 
    tx_tuner[m_DigitDownT8] = tx[8]; 
    tx_tuner[m_DigitDownT7] = tx[7]; 
    tx_tuner[m_DigitDownT6] = tx[6]; 
    tx_tuner[m_DigitDownT5] = tx[5]; 
    tx_tuner[m_DigitDownT4] = tx[4]; 
    tx_tuner[m_DigitDownT3] = tx[3]; 
    tx_tuner[m_DigitDownT2] = tx[2]; 
    tx_tuner[m_DigitDownT1] = tx[1]; 
    tx_tuner[m_DigitDownT0] = tx[0];   
  }
    
}
