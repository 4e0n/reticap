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

#ifndef APFRAME_H
#define APFRAME_H

#include <QFrame>
#include <QPainter>
#include <QPixmap>
#include "../acq_common.h"

const int AP_PIXMAP_HEIGHT=1000;

class APFrame : public QFrame {
 Q_OBJECT
 public:
  APFrame(QWidget *p,int x,int y,int fw,int fh,
          QVector<QVector<float> > *apb,int *bc,int *cac) : QFrame(p) {
   parent=p; frameWidth=fw; frameHeight=fh;
   apBuffer=apb; bufCount=bc; curAPCount=cac;

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


 public slots:

 protected:
  virtual void paintEvent(QPaintEvent*) {

   QPixmap bufPixmap(frameWidth,frameHeight); bufPixmap.fill(Qt::white);

   if (*curAPCount>0) {
    QPixmap apPixmap(*bufCount,AP_PIXMAP_HEIGHT); apPixmap.fill(Qt::white);
    apPainter.begin(&apPixmap);
     apPainter.setPen(Qt::black);
     for (int i=0;i<(*curAPCount);i++) {
      for (int j=0;j<(*bufCount)-1;j++) {
       apPainter.drawLine(j,
        AP_PIXMAP_HEIGHT/2-(int)((*apBuffer)[i][j]*AP_PIXMAP_HEIGHT/10.0),
                          j+1,
        AP_PIXMAP_HEIGHT/2-(int)((*apBuffer)[i][j+1]*AP_PIXMAP_HEIGHT/10.0));
       apPainter.drawLine(j-1,
        AP_PIXMAP_HEIGHT/2-(int)((*apBuffer)[i][j]*AP_PIXMAP_HEIGHT/10.0),
                          j,
        AP_PIXMAP_HEIGHT/2-(int)((*apBuffer)[i][j+1]*AP_PIXMAP_HEIGHT/10.0));
       apPainter.drawLine(j+1,
        AP_PIXMAP_HEIGHT/2-(int)((*apBuffer)[i][j]*AP_PIXMAP_HEIGHT/10.0),
                          j+2,
        AP_PIXMAP_HEIGHT/2-(int)((*apBuffer)[i][j+1]*AP_PIXMAP_HEIGHT/10.0));
       apPainter.drawLine(j,
        AP_PIXMAP_HEIGHT/2-(int)((*apBuffer)[i][j]*AP_PIXMAP_HEIGHT/10.0)-1,
                          j+1,
        AP_PIXMAP_HEIGHT/2-(int)((*apBuffer)[i][j+1]*AP_PIXMAP_HEIGHT/10.0)-1);
       apPainter.drawLine(j,
        AP_PIXMAP_HEIGHT/2-(int)((*apBuffer)[i][j]*AP_PIXMAP_HEIGHT/10.0)+1,
                          j+1,
        AP_PIXMAP_HEIGHT/2-(int)((*apBuffer)[i][j+1]*AP_PIXMAP_HEIGHT/10.0)+1);
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
//     bufPainter.drawText(trigX+(frameWidth-trigX)/4)+6,frameHeight*15/16,
//                          QString::number((*bufCount)/AD_SAMPLE_RATE));

     bufPainter.drawLine(trigX+(frameWidth-trigX)/2,0,
                         trigX+(frameWidth-trigX)/2,frameHeight-1);

     bufPainter.drawLine(trigX+(frameWidth-trigX)*3/4,0,
                         trigX+(frameWidth-trigX)*3/4,frameHeight-1);


     bufPainter.setPen(Qt::blue);
     bufPainter.drawLine(trigX,0,trigX,frameHeight-1);
     bufPainter.drawLine(trigX-1,0,trigX-1,frameHeight-1);
     bufPainter.drawLine(trigX+1,0,trigX+1,frameHeight-1);

     bufPainter.drawText(trigX+6,frameHeight*7/8,"Trigger");
    }
   bufPainter.end();

   mainPainter.begin(this);
    mainPainter.drawPixmap(0,0,bufPixmap);
   mainPainter.end();
  }

// mainPainter.setClipRect(x1,y1,dx+1,dy+1,Qt::ReplaceClip);

// }
// mainPainter.fillRect(x1+1,y2-40,x1+66,y2-1,QBrush(Qt::white));
// channelString.setNum((int)(apData->channel[0]));
// channelString.setNum((int)(*curRecSec));
// mainPainter.drawText(x1+6,y2-6,channelString);
// for (int i=0;i<nChns;i++) {
//  channelString="("+dummyString.setNum(i+1)+")"+" "+channelNames[i];
//  mainPainter.drawText(x1+6,yOffset[i%nChns],channelString);
// }

 private:
  QWidget *parent;
  QPainter mainPainter,apPainter,bufPainter;
  QFont measureFont,channelFont;
  QString dummyString,dummyString2,dummyString3,channelString;
  QFont headerFont;

  QVector<QVector<float> > *apBuffer; int *bufCount,*curAPCount;

  int frameWidth,frameHeight;

//  QPixmap *apPixmap,*bufPixmap;
};

#endif
