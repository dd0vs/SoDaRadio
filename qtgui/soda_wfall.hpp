#ifndef SODAWFALL_H
#define SODAWFALL_H

#include <QWidget>
#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_spectrogram.h>
#include <qwt/qwt_raster_data.h>
#include <qwt/qwt_plot_picker.h>
#include <qwt/qwt_plot_grid.h>
#include <qwt/qwt_plot_panner.h>
#include <qwt/qwt_scale_draw.h>
#include <qwt/qwt_plot_marker.h>
#include <qwt/qwt_plot_shapeitem.h>


#include <QMouseEvent>
#include <QPointF>

#include <iostream>
#include <boost/format.hpp>

#include "soda_wfall_picker.hpp"
#include "soda_freq_scale_draw.hpp"
#include "soda_wfall_data.hpp"

#include <cmath>

class SoDaPlotSpectrogram ;

class SoDaWFall : public QwtPlot
{
  Q_OBJECT
  
 public:
  explicit SoDaWFall(QWidget *parent = 0);
  ~SoDaWFall();

  double freqCenter() { return center_freq; }	      
  public slots:
    void updateData(double cf, float * y);
    void pickPoint(const QPointF & pos);

    void setFreqMarker(double freq); 

    void setDynamicRange(double drange);
    void setRefLevel(int rlvl);
    void setFreqCenter(double cf, bool check_boundary = false);
    void setFreqSpan(double fs, bool check_boundary = false);
    void setFreqSpanKHz(double fs) { setFreqSpan(1e3 * fs, true); }     
    void scrollRight(bool v) { 
      (void) v;
      setFreqCenter(center_freq + freq_span * 0.25, true); 
    }
    void scrollLeft(bool v) { 
      (void) v;
      setFreqCenter(center_freq - freq_span * 0.25, true); }
    void configureSpectrum(double cfreq, double span, long buckets) {
      setFreqCenter(cfreq); 
      // setFreqSpan(span);
      spectrum_input_span = span; 
      wfall_data->setSpectrumDimensions(cfreq, span, buckets); 
    }
    void setMarkerOffset(double lo, double hi);

 private:
    void setZAxis();
    void setMarkers(double lo, double hi) { 
      wfall_data->setMarkers(lo, hi); 
    }
    double marker_lo_offset; 
    double marker_hi_offset; 
    
 public:
    
 signals:
    void xClick(double x);

 protected:

    double correctCenterFreq(double cfreq);
    
    double center_freq; 
    double freq_span;
    double spectrum_input_span;     
    double last_input_cfreq;
    double marker_freq; 


    SoDaWFallData * wfall_data ;
    SoDaPlotSpectrogram * sgram; 

    SoDaWFallPicker * picker_p;     

    SoDaFreqScaleDraw * freq_scale_p; 

    QwtScaleWidget * right_axis; 
 private:
    void initPlot();

};

#endif
