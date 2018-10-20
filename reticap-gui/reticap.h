/*
 RetiCAP - Real-time Compound Action Potential Recorder
   Copyright (C) 2013 Barkin Ilhan

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If no:t, see <https://www.gnu.org/licenses/>.

 Contact info:
 E-Mail:  barkin@unrlabs.org
 Website: http://icon.unrlabs.org/staff/barkin/
 Repo:    https://github.com/4e0n/
*/

#ifndef RETICAP_H
#define RETICAP_H

#include <QtGui>
#include <QFile>
#include <QTextStream>
#include <QVector>
#include <QIntValidator>
#include <cmath>
#include "../acq_common.h"
#include "apframe.h"
#include "apthread.h"
#include <rtai_fifos.h>
#include <rtai_shm.h>

const int AP_FRAME_WIDTH=640;
const int AP_FRAME_HEIGHT=400;

const int APP_WIDTH=AP_FRAME_WIDTH+4;
const int APP_HEIGHT=AP_FRAME_HEIGHT+160;

class RETICAP : public QMainWindow {
 Q_OBJECT
 public:
  RETICAP(QWidget *parent,int fbf,int bff,short *shmb,int sps) :
                                                      QMainWindow(parent) {
   setGeometry(2,2,APP_WIDTH,APP_HEIGHT);
   fbFifo=fbf; bfFifo=bff; shmBuffer=shmb; sampleRate=sps;

   // Read from setup file
   stimCount=5; timeWSize=100; stimOngoing=false; curAPCount=0;

   // Status Bar
   statusBar=new QStatusBar(this);
   statusBar->setGeometry(0,APP_HEIGHT-20,APP_WIDTH,20);
   statusBar->show();

   stimGB=new QGroupBox("Stimulation",this);
   stimGB->setGeometry(2,AP_FRAME_HEIGHT+8,392,54);
   stimCLabel=new QLabel("Count:",stimGB);
   stimCLabel->setGeometry(8,22,46,20);
   stimCLineEdit=new QLineEdit(QString::number(stimCount),stimGB);
   stimCLineEdit->setGeometry(54,22,40,20);
   stimCLineEdit->setMaxLength(4);
   stimCValidator=new QIntValidator(1,1000,this);
   stimCLineEdit->setValidator(stimCValidator);
   connect(stimCLineEdit,SIGNAL(textEdited(QString)),
           this,SLOT(stimCountChanged(QString)));

   stimRateLabel=new QLabel("Rate:",stimGB);
   stimRateLabel->setGeometry(104,22,40,20);
   stimRateComboBox=new QComboBox(stimGB);
   stimRateComboBox->setGeometry(142,22,60,20);
   stimRateComboBox->addItem("0.2");
   stimRateComboBox->addItem("0.5");
   stimRateComboBox->addItem("1.0");
   stimRateComboBox->addItem("2.0");
   stimRateComboBox->addItem("4.0");
   stimRateComboBox->setCurrentIndex(2);
   connect(stimRateComboBox,SIGNAL(currentIndexChanged(QString)),
           this,SLOT(stimRateChanged(QString)));
   stimPerSLabel=new QLabel("/s",stimGB);
   stimPerSLabel->setGeometry(206,22,20,20);
   singleSButton=new QPushButton("Single",stimGB);
   singleSButton->setGeometry(238,18,50,28);
   connect(singleSButton,SIGNAL(clicked()),
           this,SLOT(stimulate1Time()));
   contSButton=new QPushButton("Continuous",stimGB);
   contSButton->setGeometry(296,18,86,28);
   connect(contSButton,SIGNAL(clicked()),
           this,SLOT(stimulateNTimes()));

   twGB=new QGroupBox("Time Window",this);
   twGB->setGeometry(402,AP_FRAME_HEIGHT+8,240,54);
   timeWSlider=new QSlider(Qt::Horizontal,twGB);
   timeWSlider->setGeometry(8,26,170,20);
   timeWSlider->setMinimum(20); timeWSlider->setMaximum(200);
   timeWSlider->setSingleStep(5); timeWSlider->setPageStep(20);
   timeWSlider->setTracking(true); timeWSlider->setValue(timeWSize);
   timeWLabel=new QLabel(QString::number(timeWSize)+" ms",twGB);
   timeWLabel->setGeometry(182,26,50,20);
   connect(timeWSlider,SIGNAL(valueChanged(int)),
           this,SLOT(timeWChanged(int)));
   connect(timeWSlider,SIGNAL(sliderReleased()),
           this,SLOT(timeWFinalValue()));
   
   dataGB=new QGroupBox("AP Data",this);
   dataGB->setGeometry(2,AP_FRAME_HEIGHT+62+8,316,54);
   dummyLabel=new QLabel("Filename Base:",dataGB);
   dummyLabel->setGeometry(8,22,100,20);
   fnLineEdit=new QLineEdit("AP-",dataGB);
   fnLineEdit->setGeometry(112,22,140,20);
   saveButton=new QPushButton("Save",dataGB);
   saveButton->setGeometry(256,18,50,28);
   connect(saveButton,SIGNAL(clicked()),
           this,SLOT(saveBuffer()));

   aboutButton=new QPushButton("About",this);
   aboutButton->setGeometry(APP_WIDTH-66,APP_HEIGHT-62,50,24);
   aboutButton->setGeometry(450,AP_FRAME_HEIGHT+62+18,60,36);
   connect(aboutButton,SIGNAL(clicked()),
           this,SLOT(about()));

   quitButton=new QPushButton("Quit",this);
   quitButton->setGeometry(530,AP_FRAME_HEIGHT+62+18,60,36);
   connect(quitButton,SIGNAL(clicked()),
           this,SLOT(quit()));

   updateBuffer();

   apThread=new APThread(this,&apBuffer,shmBuffer,fbFifo,bfFifo);
   connect(apThread,SIGNAL(acqInstance(int)),this,SLOT(acqInstanceHK(int)));
   connect(apThread,SIGNAL(acqAllComplete()),this,SLOT(acqAllCompleteHK()));

   stimCountChanged(stimCLineEdit->text());
   stimRateChanged(stimRateComboBox->currentText());

   apFrame=new APFrame(this,2,4,AP_FRAME_WIDTH,AP_FRAME_HEIGHT,
                        &apBuffer,&bufCount,&curAPCount);

   setWindowTitle("RetiCAP - Realtime Action Potential Recorder");
   show();
  }

 private slots:
  void acqInstanceHK(int i) {
   curAPCount=i;
   statusBar->showMessage("Stimulus #"+QString::number(i)+" delivered.",7000);
   apFrame->repaint();
  }

  void acqAllCompleteHK() {
   stimOngoing=false;
   enableControls(true);
   contSButton->setText("Continuous");
  }

  void stimulate1Time() {
   curAPCount=0;
   apThread->stimulate1();
  }

  void stimulateNTimes() {
   if (stimOngoing) {
    apThread->cancelStimulation();
    contSButton->setText("Continuous");
    enableControls(true);
    statusBar->showMessage("Intermittent stimulation canceled.",5000);
    stimOngoing=false;
   } else {
    curAPCount=0;
    enableControls(false);
    contSButton->setText("Cancel");
    apThread->stimulateN();
    stimOngoing=true;
   }
  }

  void stimRateChanged(QString s) {
   float sr=s.toFloat();
   apThread->setSOA((int)(1000.0/sr));
  }

  void stimCountChanged(QString s) {
   updateBuffer();
   apThread->setStimCount(s.toInt());
  }

  void timeWChanged(int x) {
   timeWLabel->setText(QString::number(x)+" ms");
  }

  void timeWFinalValue() {
   updateBuffer();
   statusBar->showMessage("Time window is set to "+
                          QString::number(sampleRate/1000*timeWSlider->value())+
                          " samples.",3000);
  }

  void saveBuffer() {
   enableControls(false); contSButton->setEnabled(false);
   apDataFile.setFileName(fnLineEdit->text()+".apd");
   if (!apDataFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
    enableControls(true); contSButton->setEnabled(true);
    statusBar->showMessage("ERROR: File cannot be opened for writing.",5000);
    return;
   } else {
    apDataStream.setDevice(&apDataFile);
    apDataStream.setRealNumberPrecision(4);
    for (int i=0;i<bufCount;i++) {
     for (int j=0;j<curAPCount;j++)
      apDataStream << apBuffer[j][i] << " ";
     apDataStream << "\n";
    }
    apDataStream.setDevice(0);
    apDataFile.close();
    enableControls(true); contSButton->setEnabled(true);
    statusBar->showMessage("AP Buffer is saved to disk.",5000);
   }
  }

  void about() {
   QMessageBox::about(this,"About RetiCAP",
                           "Realtime Compound Action Potential Recorder\n"
                           "(c) 2013 Barkin Ilhan (barkin@unrlabs.org)\n"
                           "This is free software coming with\n"
                           "ABSOLUTELY NO WARRANTY; You are welcome\n"
                           "to redistribute it under conditions of GPL v3.\n");
  }

  void quit() {
   close();
  }

 private:
  void enableControls(bool e) {
   if (e) {
    stimCLabel->setEnabled(true);
    stimCLineEdit->setEnabled(true);
    stimRateLabel->setEnabled(true);
    stimPerSLabel->setEnabled(true);
    stimRateComboBox->setEnabled(true);
    singleSButton->setEnabled(true);
    twGB->setEnabled(true);
    dataGB->setEnabled(true);
    aboutButton->setEnabled(true);
    quitButton->setEnabled(true);
   } else {
    stimCLabel->setEnabled(false);
    stimCLineEdit->setEnabled(false);
    stimRateLabel->setEnabled(false);
    stimPerSLabel->setEnabled(false);
    stimRateComboBox->setEnabled(false);
    singleSButton->setEnabled(false);
    twGB->setEnabled(true);
    twGB->setEnabled(false);
    dataGB->setEnabled(false);
    aboutButton->setEnabled(false);
    quitButton->setEnabled(false);
   }
  }

  void updateBuffer() {
   stimCount=stimCLineEdit->text().toInt();
   bufCount=sampleRate/1000*(timeWSlider->value())+PRE_STIM;
   apBuffer.resize(stimCount);
   for (int i=0;i<stimCount;i++) apBuffer[i].resize(bufCount);
  }

  void fbWrite(unsigned short code,int p1,int p2) {
   fbMsg.id=code; fbMsg.param=p1; fbMsg.param2=p2;
   write(fbFifo,&fbMsg,sizeof(cmd_msg));
  }

  int sampleRate,stimCount,timeWSize,bufCount,curAPCount;

  QStatusBar *statusBar;

  QGroupBox *dummyGB; QLabel *dummyLabel;

  QGroupBox *stimGB;
  QLabel *stimCLabel,*stimRateLabel,*stimPerSLabel;
  QLineEdit *stimCLineEdit; QIntValidator *stimCValidator;
  QComboBox *stimRateComboBox;
  QPushButton *singleSButton,*contSButton;
  QGroupBox *twGB;
  QSlider *timeWSlider; QLabel *timeWLabel;
  QGroupBox *dataGB;
  QLineEdit *fnLineEdit; QPushButton *saveButton;
  QPushButton *aboutButton,*quitButton;

  APThread *apThread;
  APFrame *apFrame;

  QFile apDataFile; QTextStream apDataStream;

  cmd_msg fbMsg,bfMsg; int fbFifo,bfFifo; short *shmBuffer;

  QVector<QVector<float> > apBuffer; bool stimOngoing;
};

#endif
