#include <cmath>
#include "freqlabel.h"
#include <iostream>

FreqLabel::FreqLabel(QWidget * parent,
		     Qt::WindowFlags f)
  : QLabel(parent), incdec_position(2)
{
    (void) f;
  setFreq(144.295e6);

  setTextFormat(Qt::RichText);
}

FreqLabel::~FreqLabel() {}

QString FreqLabel::freq2String() {
  QString ret; 
  int num_digs; 
  unsigned long ifrac = frac_freq;
  // start at the left hand side
  bool first_nonzero = false; 
  for(num_digs = 0; num_digs < 6; num_digs++) {
    int dig = ifrac % 10;
    ifrac = ifrac / 10;
    if(num_digs == 3) ret = " " + ret; 
    if(incdec_position == num_digs) {
      ret = "<u>" + QString::number(dig) + "</u>" + ret;      
    }
    else {
      ret = QString::number(dig) + ret;
    }
  }

  // add the decimal point
  ret = "." + ret; 

  unsigned long ifreq = int_freq;
  for(; num_digs < 11; num_digs++) {
    int dig = ifreq % 10;
    ifreq = ifreq / 10;

    if(((num_digs % 3) == 0) && (num_digs > 6)) {
      ret = "," + ret; 
    }
    if(incdec_position == num_digs) {
      ret = "<u>" + QString::number(dig) + "</u>" + ret;      
    }
    else {
      ret = QString::number(dig) + ret;
    }   
  }

  while(ret.size() < 14) {
      ret = " " + ret;
      std::cerr << "[" << ret.toStdString() << "]" << std::endl;
  }
  return ret; 
}

void FreqLabel::setFreq(double hzfreq)
{
  double freq = hzfreq * 1e-6; 
  frequency = hzfreq; 

  int_freq = lround(floor(freq));
  double fr = freq - floor(freq);
  frac_freq = lround(fr * 1e6);
  QString flab = freq2String();

  setText(flab);

  // now set the text width/height
  QRect r = fontMetrics().boundingRect(flab);
  disp_w = r.width();
  disp_h = r.height();
}

void FreqLabel::mousePressEvent(QMouseEvent * event) 
{
    int px = event->x();
    int py = event->y();
    int wh = height();
    int ww = width();

    unsigned long incr = 0;
    if(event->button() == Qt::LeftButton) {
        if(py > (wh >> 1)) {
            incr = incr - 1;
        }
        else if(py < (wh >> 1)) {
            incr = incr + 1;
        }
    }

    if(event->button() == Qt::RightButton) {
        if(px < (ww >> 1)) {
            incdec_position += 1;
            if(incdec_position > 10) incdec_position = 10;
        }
        else if(px > (ww >> 1)) {
            incdec_position -= 1;
            if(incdec_position < 0) incdec_position = 0;
        }
    }

    if(incr != 0) {
        if(incdec_position < 6) {
            for(int i = 0; i < incdec_position; i++) {
                incr = incr * 10;
            }
            frac_freq += incr;
        }
        else {
            for(int i = 6; i < incdec_position; i++) {
                incr = incr * 10;
            }
            int_freq += incr;
            if(int_freq < 0) {
                int_freq -= incr;
                // move adj to right
                incdec_position -= 1;
            }
        }
    }
    if(frac_freq >= 1000000) {
        frac_freq -= 1000000;
        int_freq += 1;
    }
    if(frac_freq < 0) {
        frac_freq += 1000000;
        int_freq -= 1;
    }

    if(int_freq > 99999) int_freq = 99999;
    if(int_freq < 0) int_freq = 0;

    setFreq(updateFrequency());

    emit newFreq(frequency);
}
