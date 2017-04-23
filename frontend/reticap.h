/* RETICAP - Real-time Action Potential Recording System 0.9 */
/*                (>) GPL 2013 Barkin Ilhan                  */
/*        Unified Neuroscience Research Laboratories         */
/*                    barkin@unrlabs.org                     */
/*               http://www.unrlabs.org/barkin               */

#ifndef RETICAP_H
#define RETICAP_H

#include <QtGui>
#include <QFile>
#include <QTextStream>
#include <QVector>
#include <QIntValidator>
#include <QDateTime>
#include <cmath>
#include "../acq_common.h"
#include "apframe.h"
#include "apthread.h"

const int AP_FRAME_WIDTH=640;
const int AP_FRAME_HEIGHT=360;

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
   stimCount=5; timeWSize=40; stimOngoing=false; curAPCount=0;

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
   timeWSlider->setMinimum(20); timeWSlider->setMaximum(100);
   timeWSlider->setSingleStep(1); timeWSlider->setPageStep(10);
   timeWSlider->setTracking(true); timeWSlider->setValue(timeWSize);
   timeWLabel=new QLabel(QString::number(timeWSize)+" ms",twGB);
   timeWLabel->setGeometry(182,26,50,20);
   connect(timeWSlider,SIGNAL(valueChanged(int)),
           this,SLOT(timeWChanged(int)));
   connect(timeWSlider,SIGNAL(sliderReleased()),
           this,SLOT(timeWFinalValue()));
   
   viewGB=new QGroupBox("View Parameters",this);
   viewGB->setGeometry(320,AP_FRAME_HEIGHT+62+8,200,74);
   dispModeLabel=new QLabel("DispMode:",viewGB);
   dispModeLabel->setGeometry(10,22,70,20);
   dispModeComboBox=new QComboBox(viewGB);
   dispModeComboBox->setGeometry(80,22,100,20);
   dispModeComboBox->addItem("LastSweep");
   dispModeComboBox->addItem("AllSweeps");
   dispModeComboBox->addItem("Mean+SEM");
   dispModeComboBox->setCurrentIndex(2);
   connect(dispModeComboBox,SIGNAL(currentIndexChanged(int)),
           this,SLOT(dispModeChanged(int)));
   gainLabel=new QLabel("Amp.Gain:",viewGB);
   gainLabel->setGeometry(10,46,70,20);
   gainComboBox=new QComboBox(viewGB);
   gainComboBox->setGeometry(80,46,80,20);
   gainComboBox->addItem("50"); gainComboBox->addItem("100");
   gainComboBox->addItem("200"); gainComboBox->addItem("500");
   gainComboBox->addItem("1000"); gainComboBox->addItem("2000");
   gainComboBox->addItem("5000"); gainComboBox->addItem("10000");
   gainComboBox->addItem("20000"); gainComboBox->addItem("50000");
   gainComboBox->addItem("100000"); gainComboBox->addItem("200000");
   gainComboBox->setCurrentIndex(1);
   connect(gainComboBox,SIGNAL(currentIndexChanged(QString)),
           this,SLOT(gainChanged(QString)));

   dataGB=new QGroupBox("AP Data",this);
   dataGB->setGeometry(2,AP_FRAME_HEIGHT+62+8,316,54);
   dummyLabel=new QLabel("Filename Suffix:",dataGB);
   dummyLabel->setGeometry(8,22,100,20);
   fnLineEdit=new QLineEdit("",dataGB);
   fnLineEdit->setGeometry(112,22,140,20);
   saveButton=new QPushButton("Save",dataGB);
   saveButton->setGeometry(256,18,50,28);
   connect(saveButton,SIGNAL(clicked()),
           this,SLOT(saveBuffer()));

   aboutButton=new QPushButton("About",this);
   aboutButton->setGeometry(520,AP_FRAME_HEIGHT+62+18,50,36);
   connect(aboutButton,SIGNAL(clicked()),
           this,SLOT(about()));

   quitButton=new QPushButton("Quit",this);
   quitButton->setGeometry(580,AP_FRAME_HEIGHT+62+18,50,36);
   connect(quitButton,SIGNAL(clicked()),
           this,SLOT(quit()));

   updateBuffer();

   apThread=new APThread(this,&apBuffer,shmBuffer,&gain,fbFifo,bfFifo);
   connect(apThread,SIGNAL(acqInstance(int)),this,SLOT(acqInstanceHK(int)));
   connect(apThread,SIGNAL(acqAllComplete()),this,SLOT(acqAllCompleteHK()));

   stimCountChanged(stimCLineEdit->text());
   stimRateChanged(stimRateComboBox->currentText());
   QString dummyS=gainComboBox->currentText(); gain=dummyS.toFloat();

   apFrame=new APFrame(this,2,4,AP_FRAME_WIDTH,AP_FRAME_HEIGHT,
                        &apBuffer,&bufCount,&curAPCount,&gain);
   apFrame->setDispMode(dispModeComboBox->currentIndex());

   setWindowTitle("RETICAP - Realtime Action Potential Recording Application v0.9.0");
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

  void gainChanged(QString s) {
   float sr=s.toFloat();
   gain=sr;
   apFrame->repaint();
  }

  void dispModeChanged(int x) {
   apFrame->setDispMode(x);
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
   QDateTime dt=QDateTime::currentDateTime();
   QString timeStamp=dt.toString("ddMMyyhhmmss");
   apDataFile.setFileName("AP-"+timeStamp+fnLineEdit->text()+".apd");
   if (!apDataFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
    enableControls(true); contSButton->setEnabled(true);
    statusBar->showMessage("ERROR: File cannot be opened for writing.",5000);
    return;
   } else {
    apDataStream.setDevice(&apDataFile);
    apDataStream.setRealNumberPrecision(4);
    for (int i=0;i<bufCount;i++) {
     for (int j=0;j<curAPCount;j++)
      apDataStream << apBuffer[j][i]*1000.0 << " "; // Save as mV
     apDataStream << "\n";
    }
    apDataStream.setDevice(0);
    apDataFile.close();
    enableControls(true); contSButton->setEnabled(true);
    statusBar->showMessage("AP Buffer is saved to disk.",5000);
   }
  }

  void about() {
   QMessageBox::about(this,"About RETICAP",
                           "\nRETICAP v0.9\n\n"
                           "Copyright (c) GNU_GPL 2013\n"
                           "by Barkin Ilhan (barkin@unrlabs.org)\n"
                           "  (http://www.unrlabs.org/barkin)\n\n"
                           "@Unified Neuroscience Research Labs\n"
                           "@Biophysics Department,\n"
                           " Konya University Meram Medical Faculty\n");
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
    gainLabel->setEnabled(true);
    gainComboBox->setEnabled(true);
    aboutButton->setEnabled(true);
    quitButton->setEnabled(true);
   } else {
    stimCLabel->setEnabled(false);
    stimCLineEdit->setEnabled(false);
    stimRateLabel->setEnabled(false);
    stimPerSLabel->setEnabled(false);
    stimRateComboBox->setEnabled(false);
    singleSButton->setEnabled(false);
    twGB->setEnabled(false);
    dataGB->setEnabled(false);
    gainLabel->setEnabled(false);
    gainComboBox->setEnabled(false);
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

  QGroupBox *viewGB;
  QLabel *dispModeLabel,*gainLabel;
  QComboBox *dispModeComboBox,*gainComboBox;

  QPushButton *singleSButton,*contSButton;
  QGroupBox *twGB;
  QSlider *timeWSlider; QLabel *timeWLabel;
  QGroupBox *dataGB;
  QLineEdit *fnLineEdit; QPushButton *saveButton;
  QPushButton *aboutButton,*quitButton;

  float gain;

  APThread *apThread;
  APFrame *apFrame;

  QFile apDataFile; QTextStream apDataStream;

  cmd_msg fbMsg,bfMsg; int fbFifo,bfFifo; short *shmBuffer;

  QVector<QVector<float> > apBuffer; bool stimOngoing;
};

#endif
