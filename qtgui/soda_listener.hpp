#ifndef SODA_LISTENER_HEADER
#define SODA_LISTENER_HEADER
#include <QObject>
#include <QString>
#include <QtNetwork/QtNetwork>
#include <iostream>
#include <boost/format.hpp>
#include <errno.h>

#include "../src/Command.hxx"


class SoDaListener : public QObject {
  Q_OBJECT

public:
  SoDaListener(QObject * parent = 0, QString socket_basename = "tmp");
  ~SoDaListener() {
  }

  // initiate transfers on the socket.
  
  void start(); 
signals:
  // when new spectrum data arrives
  void updateData(double cfreq, float * y);

  void configureSpectrum(double cfreq, double span, long buckets);
  
  void addModulation(QString modtype, int mod_id);
  void addFilterWidth(double lo, double hi);
  void addFilterName(QString filter_name, int filt_id);
  void repMarkerOffset(double lo, double hi); 
  
  void addRXAntName(const QString & ant_name);
  void addTXAntName(const QString & ant_name);  
  
  void repFilterEdges(double lo, double hi);

  void repGPSLatLon(double lat, double lon);
  void repGPSTime(int hh, int mm, int ss);

  void repGPSLock(bool is_locked);

  void repSDRVersion(const QString & version);
  void repHWMBVersion(const QString & version);

					      
public slots:
  void setRXFreq(double freq);
  void setTXFreq(double freq);

  void setModulation(int mod_id); 

  void setAFFilter(int id); 

  void setRXGain(int gain);
  void setTXGain(int gain);
  void setAFGain(int gain);
  void setAFSidetoneGain(int gain);  

  void setRXAnt(const QString & antname);
  void setTXAnt(const QString & antname);

  double getRXFreq() { return current_rx_freq; }
  double getTXFreq() { return current_tx_freq; }

  void setSpectrumCenter(double freq); 
  void setSpectrumAvgWindow(int window);
  void setSpectrumUpdateRate(int rate);  


  void closeRadio();


protected:
  double current_rx_freq; 
  double current_tx_freq; 
  int get(char* buf, int maxlen); 
  bool get(SoDa::Command & cmd); 
  int put(const char * buf, int len); 
  bool put(const SoDa::Command & cmd);

  bool handleREP(const SoDa::Command & cmd);
  bool handleSET(const SoDa::Command & cmd);
  bool handleGET(const SoDa::Command & cmd); 

private:
  void setupSpectrumBuffer(double cfreq, double span, long buflen);
  long spect_buffer_len;
  float * spect_buffer; 		      
  double spect_center_freq; 				    
protected slots:  
  void processCmd();
  void cmdErrorHandler(QLocalSocket::LocalSocketError err) {
    std::cerr << "Error [" << err << "]\n";
  }

  void processSpectrum();
  
private:
  QLocalSocket * cmd_socket;
  QLocalSocket * spect_socket; 
  bool quit; 
};

#endif
