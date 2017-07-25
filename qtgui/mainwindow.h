#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include "soda_listener.h"
#include "../common/GuiParams.hxx"

namespace Ui {
  class MainWindow;
}

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent, SoDa::GuiParams & params);
  ~MainWindow();

public slots:
  void setRXFreq(double freq);
  void setTXFreq(double freq);  

signals:
  void closeRadio();

protected:
  void setupTopControls();
  void setupMidControls();
  void setupLogGPS();

  void setupSettings();
  void setupBandConfig();
  void setupLogEditor();
  
  void setupWaterFall();
  void setupSpectrum();
  
private:
  void closeEvent(QCloseEvent * event) {
    std::cerr << "In window close event\n";
    listener->closeRadio();
    event->accept();
  }

  void setRXFreq_nocross(double freq);
  void setTXFreq_nocross(double freq);   
  
  Ui::MainWindow *ui;

  SoDaListener * listener; 
};

#endif // MAINWINDOW_H
