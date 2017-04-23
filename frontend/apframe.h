/* RETICAP - Real-time Action Potential Recording System 0.9 */
/*                (>) GPL 2013 Barkin Ilhan                  */
/*        Unified Neuroscience Research Laboratories         */
/*                    barkin@unrlabs.org                     */
/*               http://www.unrlabs.org/barkin               */

#ifndef APFRAME_H
#define APFRAME_H

#include <QFrame>
#include <QPainter>
#include <QPixmap>
#include <QColor>
#include <QVector>
#include <math.h>
#include "../acq_common.h"

const int AP_PIXMAP_HEIGHT=500;

class APFrame : public QFrame {
 Q_OBJECT
 public:
  APFrame(QWidget *p,int x,int y,int fw,int fh,
          QVector<QVector<float> > *apb,int *bc,int *cac,
          float *g) : QFrame(p) {
   parent=p; frameWidth=fw; frameHeight=fh;
   apBuffer=apb; bufCount=bc; curAPCount=cac; dispMode=0; gain=g;

   setGeometry(x,y,frameWidth,frameHeight);

   measureFont.setPixelSize(12);
   channelFont.setPixelSize(12);
   channelFont.setBold(true);

   repaint();
  }

   // create an offscreen pixmap of screen size
   // create an offscreen pixmap of height x bufferwidth.
   //  resize it accordingly if the tw is changed.
   // draw frame on scrpixmap etc.
   // draw average/stderr on this offline buffer
   // copy this resizing on top of scrpixmap.
   // make other postprocessing (titles, etc.)
   // copy to real screen.

   // curAPCount==1 ise sadece veriyi ciz, degilse yanina std de ciz.

  void setDispMode(int vm) { dispMode=vm; repaint(); }

 public slots:

 protected:
  virtual void paintEvent(QPaintEvent*) {

   QPixmap bufPixmap(frameWidth,frameHeight); bufPixmap.fill(Qt::white);

   if (*curAPCount>0) {
    float vRange=VRANGE/(*gain);

    QPixmap apPixmap(*bufCount,AP_PIXMAP_HEIGHT); apPixmap.fill(Qt::white);
    apPainter.begin(&apPixmap);

     if (dispMode==0) {
      apPainter.setPen(Qt::black);
      for (int j=0;j<(*bufCount)-1;j++) {
       apPainter.drawLine(j,
        AP_PIXMAP_HEIGHT/2
         -(int)((*apBuffer)[(*curAPCount)-1][j]*AP_PIXMAP_HEIGHT/vRange),
                          j+1,
        AP_PIXMAP_HEIGHT/2
         -(int)((*apBuffer)[(*curAPCount)-1][j+1]*AP_PIXMAP_HEIGHT/vRange));
       apPainter.drawLine(j-1,
        AP_PIXMAP_HEIGHT/2
         -(int)((*apBuffer)[(*curAPCount)-1][j]*AP_PIXMAP_HEIGHT/vRange),
                          j,
        AP_PIXMAP_HEIGHT/2
         -(int)((*apBuffer)[(*curAPCount)-1][j+1]*AP_PIXMAP_HEIGHT/vRange));
       apPainter.drawLine(j+1,
        AP_PIXMAP_HEIGHT/2
         -(int)((*apBuffer)[(*curAPCount)-1][j]*AP_PIXMAP_HEIGHT/vRange),
                          j+2,
        AP_PIXMAP_HEIGHT/2
         -(int)((*apBuffer)[(*curAPCount)-1][j+1]*AP_PIXMAP_HEIGHT/vRange));
      }
     } else if (dispMode==1) {
      for (int i=0;i<(*curAPCount);i++) {
       switch (i%8) {
        case 0: apPainter.setPen(Qt::blue); break;
        case 1: apPainter.setPen(Qt::red); break;
        case 2: apPainter.setPen(Qt::darkGreen); break;
        case 3: apPainter.setPen(Qt::magenta); break;
        case 4: apPainter.setPen(Qt::darkYellow); break;
        case 5: apPainter.setPen(Qt::darkBlue); break;
        case 6: apPainter.setPen(Qt::darkRed); break;
        case 7: apPainter.setPen(Qt::darkGray); break;
       }
       for (int j=0;j<(*bufCount)-1;j++) {
        apPainter.drawLine(j,
         AP_PIXMAP_HEIGHT/2
          -(int)((*apBuffer)[i][j]*AP_PIXMAP_HEIGHT/vRange),
                           j+1,
         AP_PIXMAP_HEIGHT/2
          -(int)((*apBuffer)[i][j+1]*AP_PIXMAP_HEIGHT/vRange));
        apPainter.drawLine(j-1,
         AP_PIXMAP_HEIGHT/2
          -(int)((*apBuffer)[i][j]*AP_PIXMAP_HEIGHT/vRange),
                           j,
         AP_PIXMAP_HEIGHT/2
          -(int)((*apBuffer)[i][j+1]*AP_PIXMAP_HEIGHT/vRange));
        apPainter.drawLine(j+1,
         AP_PIXMAP_HEIGHT/2
          -(int)((*apBuffer)[i][j]*AP_PIXMAP_HEIGHT/vRange),
                           j+2,
         AP_PIXMAP_HEIGHT/2
          -(int)((*apBuffer)[i][j+1]*AP_PIXMAP_HEIGHT/vRange));
       }
      }
     } else if (dispMode==2) {
      QVector<float> mean(*bufCount); QVector<float> sem(*bufCount);
      QVector<float> sem1(*bufCount); QVector<float> sem2(*bufCount);
      
      for (int j=0;j<(*bufCount);j++) {
       for (int i=0;i<(*curAPCount);i++) mean[j]+=(*apBuffer)[i][j];
       mean[j]/=(*curAPCount);
       if ((*curAPCount)<=1) sem[j]=0;
       else {
        for (int i=0;i<(*curAPCount);i++)
         sem[j]+=((*apBuffer)[i][j]-mean[j])*((*apBuffer)[i][j]-mean[j]);
        sem[j]=sqrt(sem[j]/(float)(*curAPCount-1.0));
       }
       sem1[j]=mean[j]+sem[j]/sqrt((float)(*curAPCount));
       sem2[j]=mean[j]-sem[j]/sqrt((float)(*curAPCount));
      }

      // Draw SEM
      //apPainter.setPen(Qt::green);
      QColor cx=QColor(128,128,255,128);
      //apPainter.setBrush(cx);
      apPainter.setPen(cx);
      for (int j=0;j<(*bufCount);j++) {
       apPainter.drawLine(j,
        AP_PIXMAP_HEIGHT/2
         -(int)(sem1[j]*AP_PIXMAP_HEIGHT/vRange),
                          j,
        AP_PIXMAP_HEIGHT/2
         -(int)(sem2[j]*AP_PIXMAP_HEIGHT/vRange));
      }

      // Draw mean
      apPainter.setPen(Qt::black);
      for (int j=0;j<(*bufCount)-1;j++) {
       apPainter.drawLine(j,
        AP_PIXMAP_HEIGHT/2-(int)(mean[j]*AP_PIXMAP_HEIGHT/vRange),
                          j+1,
        AP_PIXMAP_HEIGHT/2-(int)(mean[j+1]*AP_PIXMAP_HEIGHT/vRange));
       apPainter.drawLine(j-1,
        AP_PIXMAP_HEIGHT/2-(int)(mean[j]*AP_PIXMAP_HEIGHT/vRange),
                          j,
        AP_PIXMAP_HEIGHT/2-(int)(mean[j+1]*AP_PIXMAP_HEIGHT/vRange));
       apPainter.drawLine(j+1,
        AP_PIXMAP_HEIGHT/2-(int)(mean[j]*AP_PIXMAP_HEIGHT/vRange),
                          j+2,
        AP_PIXMAP_HEIGHT/2-(int)(mean[j+1]*AP_PIXMAP_HEIGHT/vRange));
      }
     }

    apPainter.end();

    bufPixmap=apPixmap.scaled(frameWidth,frameHeight,
                              Qt::IgnoreAspectRatio,Qt::SmoothTransformation);

    bufPainter.drawPixmap(0,0,frameWidth,frameHeight,bufPixmap);

   }

   bufPainter.begin(&bufPixmap);
    bufPainter.setPen(Qt::darkGray);
    bufPainter.drawRect(0,0,frameWidth-1,frameHeight-1);

    if (*curAPCount>0) {
     bufPainter.drawLine(0,frameHeight/2-1,frameWidth-1,frameHeight/2-1);
     bufPainter.drawLine(0,frameHeight/2,frameWidth-1,frameHeight/2);
     bufPainter.drawLine(0,frameHeight/2+1,frameWidth-1,frameHeight/2+1);
     bufPainter.drawLine(0,frameHeight/4,frameWidth-1,frameHeight/4);
     bufPainter.drawLine(0,frameHeight*3/4,frameWidth-1,frameHeight*3/4);

     // Trigger Instant and postTrig cues
     bufPainter.setFont(channelFont);
     int trigX=(int)((float)PRE_STIM*(float)frameWidth/(float)(*bufCount));

     bufPainter.drawLine(trigX+(frameWidth-trigX)/4,0,
                         trigX+(frameWidth-trigX)/4,frameHeight-1);

     bufPainter.drawLine(trigX+(frameWidth-trigX)/2,0,
                         trigX+(frameWidth-trigX)/2,frameHeight-1);

     bufPainter.drawLine(trigX+(frameWidth-trigX)*3/4,0,
                         trigX+(frameWidth-trigX)*3/4,frameHeight-1);


     bufPainter.setPen(Qt::blue);
     bufPainter.drawLine(trigX,0,trigX,frameHeight-1);
     bufPainter.drawLine(trigX-1,0,trigX-1,frameHeight-1);
     bufPainter.drawLine(trigX+1,0,trigX+1,frameHeight-1);

     bufPainter.drawText(trigX+6,frameHeight*7/8,"Trigger");

     bufPainter.setPen(Qt::black);
     bufPainter.drawText(6,20,
                         QString::number(1000.0*VRANGE/(*gain)/2.0)+"mV");
     bufPainter.drawText(6,frameHeight-20,
                         QString::number(-1000.0*VRANGE/(*gain)/2.0)+"mV");
    }
   bufPainter.end();

   mainPainter.begin(this);
    mainPainter.drawPixmap(0,0,bufPixmap);
   mainPainter.end();
  }

 private:
  QWidget *parent;
  QPainter mainPainter,apPainter,bufPainter;
  QFont measureFont,channelFont;
  QString dummyString,dummyString2,dummyString3,channelString;
  QFont headerFont;

  QVector<QVector<float> > *apBuffer; int *bufCount,*curAPCount;

  int frameWidth,frameHeight;

  int dispMode; float *gain;
};

#endif
