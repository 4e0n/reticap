/* RETICAP - Real-time Action Potential Recording System 0.9 */
/*                (>) GPL 2013 Barkin Ilhan                  */
/*        Unified Neuroscience Research Laboratories         */
/*                    barkin@unrlabs.org                     */
/*               http://www.unrlabs.org/barkin               */

#ifndef APTHREAD_H
#define APTHREAD_H

#include <QTextStream>
#include <QStatusBar>
#include <QThread>
#include <QMutex>
#include "../acq_common.h"

class APThread : public QThread {
 Q_OBJECT
 public:
  APThread(QObject* parent,QVector<QVector<float> > *apb,
            short *shmb,float *g,int fbf,int bff) : QThread(parent) {
   apBuffer=apb; shmBuffer=shmb; gain=g; fbFifo=fbf; bfFifo=bff;
   threadActive=true; acqActive=false; singleStim=false;
  }

  void fbWrite(unsigned short code,int p1,int p2) {
   fbMsg.id=code; fbMsg.param=p1; fbMsg.param2=p2;
   write(fbFifo,&fbMsg,sizeof(cmd_msg));
  }

  void setStimCount(int c) { stimCount=c; }
  void setSOA(int t) { soa=t; }

  void cancelStimulation() {
   mutex.lock();
    terminate(); wait(1000);
   mutex.unlock();
  }

  void stimulate1() {
   singleStim=true;
   start(QThread::TimeCriticalPriority); // Highest, Normal
  }

  void stimulateN() {
   singleStim=false;
   start(QThread::TimeCriticalPriority); // Highest, Normal
  }

  bool threadActive;

  virtual void run() {
   int dCount=(*apBuffer)[0].size();
   if (singleStim) {
    mutex.lock();
     fbWrite(ACQ_TRIG,0,0);
     // wait for ACK
     read(bfFifo,&bfMsg,sizeof(cmd_msg));
     if (bfMsg.id==ACQ_BE_ACK) {
      for (int i=0;i<dCount;i++)
       (*apBuffer)[0][i]=
          (VRANGE*(float)shmBuffer[i]/4096.0-VRANGE/2.0)/(*gain);
//      qDebug("Single stimulus delivered.");
      emit acqInstance(1);
      emit acqAllComplete();
     }
    mutex.unlock();
   } else {
    for (int i=0;i<stimCount;i++) {
     mutex.lock();
      fbWrite(ACQ_TRIG,0,0);
      // wait for ACK
      read(bfFifo,&bfMsg,sizeof(cmd_msg));
      if (bfMsg.id==ACQ_BE_ACK) {
       for (int j=0;j<dCount;j++)
        (*apBuffer)[i][j]=
           (VRANGE*(float)shmBuffer[j]/4096.0-VRANGE/2.0)/(*gain);
//       qDebug("Multistim instance");
       emit acqInstance(i+1);
      }
     mutex.unlock();
     msleep(soa);
    }
    emit acqAllComplete();
   }
  }

 signals:
  void acqInstance(int);
  void acqAllComplete();

 private:
  QVector<QVector<float> > *apBuffer;
  short *shmBuffer; int fbFifo,bfFifo; cmd_msg fbMsg,bfMsg;
  bool acqActive,singleStim;
  int bufOffset; int stimCount,soa;
  float *gain;

  QStatusBar *statusBar;
  QMutex mutex;
};

#endif
