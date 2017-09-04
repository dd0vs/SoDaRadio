/*
Copyright (c) 2017 Matthew H. Reilly (kb1vc)
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

#ifndef SODAPLOTPICKER_H
#define SODAPLOTPICKER_H
#include <QBrush>
#include <QColor>
#include <QPen>
#include <qwt/qwt_picker_machine.h>
#include <boost/format.hpp>

namespace GUISoDa {
  
  class PlotPicker : public QwtPlotPicker {
  public:
    PlotPicker(int xAxis, int yAxis, QWidget * canvas) : QwtPlotPicker(xAxis, yAxis, canvas)
    {
      setStateMachine(new QwtPickerClickPointMachine);
      setTrackerMode(QwtPicker::AlwaysOn);
      setTrackerPen(QPen(Qt::white));
    }

    QwtText trackerTextF(const QPointF & pos) const {
      QColor trbgcolor(Qt::white);
      trbgcolor.setAlpha(128); // translucent highlight

      QwtText text((boost::format("%.4f MHz, %.1f dB") % (pos.x() * 1e-6) % pos.y()).str().c_str());
      text.setBackgroundBrush(QBrush(trbgcolor));

      return text;
    }
  };
}
#endif // SODAPLOTPICKER_H
