#include "soda_spect.h"
#include <iostream>
#include <boost/format.hpp>
#include <cmath>

#include <qwt/qwt_plot_grid.h>
#include <qwt/qwt_scale_engine.h>



SoDaSpect::SoDaSpect(QWidget *parent) :
    QwtPlot(parent)
{
    initPlot();
}

SoDaSpect::~SoDaSpect()
{
  if(freqs != NULL) delete[] freqs; 
  if(vals != NULL) delete[] vals; 
  num_buckets = 0; 
}


void SoDaSpect::initPlot()
{

  freq_span_disp = 10e3;
  val_ref = 10.0;

  num_buckets = 0; 
  freqs = NULL; 
  vals = NULL; 

  setCanvasBackground(Qt::black);
  //QwtLinearScaleEngine se;
  //setAxisScaleDiv(QwtPlot::xBottom, se.divideScale(144.1e6,144.2e6,10,5));
  enableAxis(QwtPlot::xBottom, true);
  enableAxis(QwtPlot::yLeft, true);
  enableAxis(QwtPlot::yRight, true);

  freq_draw_p = new SoDaFreqScaleDraw(); 
  setAxisScaleDraw(QwtPlot::xBottom, freq_draw_p);

  setFreqCenter(144.15e6);
  setRefLevel(20);

  curve_p = new QwtPlotCurve();
  curve_p->attach(this);

  grid_p = new QwtPlotGrid();
  grid_p->setPen(Qt::gray);
  grid_p->attach(this);

  curve_p->setPen(Qt::red);
  // curve_p->setSamples(xdat, ydat, 1000);

  // setup the plotpicker -- this creates click events for us.
  picker_p = new SoDaPlotPicker(QwtPlot::xBottom, QwtPlot::yLeft, canvas());

  connect(picker_p, SIGNAL(selected(const QPointF&)), SLOT(pickPoint(const QPointF&)));

  QColor fcol = Qt::yellow;
  fcol.setAlpha(128);
  freq_marker.setBrush(fcol);
  freq_marker.setPen(fcol);
  // make a dummy marker
  freq_marker.setRect(QRectF(1.0, -100.0, 2.0, 100.0));
  freq_marker.attach(this);
  replot();
  show();
}

void SoDaSpect::updateData(double cfreq, float * y)
{
  if(cfreq != center_freq_in) resetFreqAxis(cfreq); 
  for(int i = 0; i < num_buckets; i++) vals[i] = y[i]; 
  curve_p->setSamples(freqs, vals, num_buckets);
  replot();
}

void SoDaSpect::setRefLevel(int rlvl)
{
  val_ref = ((double) rlvl);
  replotYAxis();
}

void SoDaSpect::setDynamicRange(double drange)
{
  val_range = drange; 
  replotYAxis();
}

void SoDaSpect::replotYAxis()
{
  double y_min = val_ref - val_range;
  double y_step = val_range * 0.1; 
  setAxisScale(QwtPlot::yLeft, y_min, val_ref, y_step);
  setAxisScale(QwtPlot::yRight, y_min, val_ref, y_step);
  replot();
}


void SoDaSpect::replotXAxis()
{
  double min = center_freq_disp - freq_span_disp * 0.5;
  double max = center_freq_disp + freq_span_disp * 0.5;

  setAxisScale(QwtPlot::xBottom, min, max, freq_span_disp / 5.0); // step);

  QwtLinearScaleEngine se;
  setAxisScaleDiv(QwtPlot::xBottom, se.divideScale(min, max, 5, 5));
  freq_draw_p->setFreqStep(center_freq_disp, freq_span_disp / 5.0);
  replot();
}

void SoDaSpect::setFreqCenter(double cf, bool check_boundary) 
{
  (void) check_boundary;
  center_freq_disp = cf; 
  replotXAxis();
}


double SoDaSpect::correctCenterFreq(double cfreq)
{
  if((cfreq + 0.5 * freq_span_disp) > (center_freq_in + 0.5 * freq_span_in)) {
    cfreq = (center_freq_in + 0.5 * (freq_span_in - freq_span_disp));
  }
  if((cfreq - 0.5 * freq_span_disp) < (center_freq_in - 0.5 * freq_span_in)) {
    cfreq = (center_freq_in - 0.5 * (freq_span_in - freq_span_disp));
  }
  return cfreq; 
}

void SoDaSpect::setFreqSpan(double fs, bool check_boundary) {
  freq_span_disp = fs;   
  if(check_boundary) {
    center_freq_disp = correctCenterFreq(center_freq_disp);
    if((marker_freq < (center_freq_disp - 0.5 * freq_span_disp)) ||
       (marker_freq > (center_freq_disp + 0.5 * freq_span_disp))) {
      center_freq_disp = correctCenterFreq(marker_freq);
    }
  }
  replotXAxis();
}


void SoDaSpect::pickPoint(const QPointF & pos)
{
  setFreqMarker(pos.x());
  emit xClick(pos.x());
}

void SoDaSpect::setFreqMarker(double freq)
{
  double f = freq + marker_lo_offset; 
  double width = marker_hi_offset - marker_lo_offset;
  freq_marker.setRect(QRectF(f, -200.0, width, 300.0));

  marker_freq = f;   
  replot();
}

void SoDaSpect::resetFreqAxis(double cfreq) {
  // load up the X axis values. (frequency)
  double fincr = freq_span_in / ((double) (num_buckets-1));
  double fr = cfreq - 0.5 * freq_span_in; 
  for(int i = 0; i < num_buckets; i++) {
    freqs[i] = fr; 
    fr += fincr; 
  }
}

void SoDaSpect::configureSpectrum(double cfreq, double span, long buckets) {
  marker_freq = cfreq; 
  center_freq_in = cfreq; 
  freq_span_in = span; 
  if(num_buckets < buckets) {
    if(freqs != NULL) delete[] freqs; 
    if(vals != NULL) delete[] vals; 
    freqs = new double[buckets];
    vals = new double[buckets];
    num_buckets = buckets; 
  }

  resetFreqAxis(cfreq);  
  // load up the Y axis values. 
  for(int i = 0; i < num_buckets; i++) {
    vals[i] = -200.0; 
  }
    
  // set the center display frequency and span. 
  setFreqCenter(cfreq); 
  setFreqSpan(200.0e3); // default span. 
}
