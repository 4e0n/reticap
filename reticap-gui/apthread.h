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

#ifndef APTHREAD_H
#define APTHREAD_H

#include <QTextStream>
#include <QStatusBar>
#include <QThread>
#include <QMutex>
#include <rtai_fifos.h>
#include <rtai_shm.h>
#include "../acq_common.h"

class APThread : public QThread {
 Q_OBJECT
 public:
  APThread(QObject* parent,QVector<QVector<float> > *apb,
            short *shmb,int fbf,int bff) : QThread(parent) {
   apBuffer=apb; shmBuffer=shmb; fbFifo=fbf; bfFifo=bff;
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
       (*apBuffer)[0][i]=20.0*(float)shmBuffer[i]/65536.0-0.0;
      qDebug("Single stimulus delivered.");
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
        (*apBuffer)[i][j]=20.0*(float)shmBuffer[j]/65536.0-0.0;
       qDebug("Multistim instance");
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

  QStatusBar *statusBar;
  QMutex mutex;
};

#endif
