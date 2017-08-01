#include "mainwindow.hpp"
#include "ui_mainwindow.h"
#include <iostream>
#include <boost/format.hpp>
#include "soda_comboboxes.hpp"
#include "soda_listener.hpp"

void MainWindow::setupMidControls()
{
  // These controls include band selection, PTT, TX display,
  // antenna selection, CW QSO buttons, and the navigation panel

  connect(listener, &SoDaListener::addRXAntName, 
	  [this](const QString & v){
	    ui->RXAnt_sel->addItem(v); });
  connect(listener, &SoDaListener::addTXAntName, 
	  [this](const QString & v){
	    ui->TXAnt_sel->addItem(v); });

  connect(ui->RXAnt_sel, SIGNAL(currentTextChanged(const QString &)),
	  listener, SLOT(setRXAnt(const QString &)));
  connect(ui->TXAnt_sel, SIGNAL(currentTextChanged(const QString &)),
	  listener, SLOT(setTXAnt(const QString &)));

  connect(ui->PTT_btn, SIGNAL(toggled(bool)), 
	  listener, SLOT(setPTT(bool)));

  connect(listener, &SoDaListener::repPTT,
	  [=](bool on) { ui->TXState_lab->setText(on ? "TX ON" : "TX OFF"); });

  connect(ui->CWCurLine_le, &QLineEdit::returnPressed, 
	  [=]() 
	  { ui->CWOutBound_te->appendPlainText(ui->CWCurLine_le->text());
	    listener->sendCW(ui->CWCurLine_le->text());
	    ui->CWCurLine_le->clear(); });
	    

  connect(ui->ClrBuff_btn, SIGNAL(released()), 
	  listener, SLOT(clearCWBuffer()));

  connect(ui->MyCall_btn, &QPushButton::pressed,
	  [=]() {
	    int i;
	    for(i = 0; i < ui->RptCount_spin->value(); i++) {
	      sendCannedCW(ui->FromCall_le->text()); 
	    }
	  });

  connect(ui->MyGrid_btn, &QPushButton::pressed,
	  [=]() {
	    int i;
	    for(i = 0; i < ui->RptCount_spin->value(); i++) {
	      sendCannedCW(ui->FromGrid_le->text()); 
	    }
	  });
  
  connect(ui->MyInfo_btn, &QPushButton::pressed,
	  [=]() {
	    int i;
	    for(i = 0; i < ui->RptCount_spin->value(); i++) {
	      sendCannedCW(" " + ui->FromCall_le->text());	    	      
	    }
	    for(i = 0; i < ui->RptCount_spin->value(); i++) {
	      sendCannedCW(" " + ui->FromGrid_le->text());
	    }
	  });	  

  connect(ui->Exchange_btn, &QPushButton::pressed,
	  [=]() {
	    int i;
	    for(i = 0; i < ui->RptCount_spin->value(); i++) {
	      sendCannedCW(" " + ui->ToCall_le->text());	    	      
	    }
	    sendCannedCW(" de ");
	    for(i = 0; i < ui->RptCount_spin->value(); i++) {
	      sendCannedCW(" " + ui->FromCall_le->text());	    	      
	    }
	    for(i = 0; i < ui->RptCount_spin->value(); i++) {
	      sendCannedCW(" " + ui->FromGrid_le->text());
	    }
	  });	  
  
  connect(ui->CWV_btn, &QPushButton::pressed,
	  [=]() {
	    int i; 
	    for(i = 0; i < ui->RptCount_spin->value(); i++) {
	      sendCannedCW("VVVVVVVVVVVV"); 
	    }
	  });	  

  connect(ui->CW73_btn, &QPushButton::pressed,
	  [=]() {
	    sendCannedCW(" TNX es 73 de " + ui->FromCall_le->text());
	  });

  connect(ui->CWQSL_btn, &QPushButton::pressed,
	  [=]() {
	    sendCannedCW(" R R R  QSL QSL QSL de " + ui->FromCall_le->text()); 
	  });	  

  connect(ui->CWBK_btn, &QPushButton::pressed,
	  [=]() {
	    sendCannedCW(" _bk ");
	  });	  
}

void MainWindow::sendCannedCW(const QString & txt) 
{
  listener->sendCW(txt); 
  ui->CWOutBound_te->appendPlainText(txt); 
}
