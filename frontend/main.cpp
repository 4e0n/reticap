/* RETICAP - Real-time Action Potential Recording System 0.9 */
/*                (>) GPL 2013 Barkin Ilhan                  */
/*        Unified Neuroscience Research Laboratories         */
/*                    barkin@unrlabs.org                     */
/*               http://www.unrlabs.org/barkin               */

/* This is the acquisition 'frontend' which takes single-channel data
   acquired in realtime kernel-space backend using SHM and displaying on
   screen plus optionally writing to disk.. */

#include <QApplication>
#include <stdlib.h>
#include <rtai_fifos.h>
#include <rtai_shm.h>
#include "../acq_common.h"
#include "reticap.h"

int main(int argc,char *argv[]) {
 int fbFifo,bfFifo,adSampleRate; short *shmBuffer; cmd_msg reset_msg;
 QString dString;

 adSampleRate=AD_SAMPLE_RATE;

 if ((fbFifo=open("/dev/rtf2",O_WRONLY))<0) {
  qDebug("reticap: Cannot open f2b FIFO!");
  return -1;
 }

 if ((bfFifo=open("/dev/rtf3",O_RDONLY|O_NONBLOCK))<0) {
  qDebug("reticap: Cannot open b2f FIFO!");
  return -1;
 }

 // Send RESET Backend Command
 reset_msg.id=ACQ_RESET;
 write(fbFifo,&reset_msg,sizeof(cmd_msg)); sleep(1); reset_msg.id=0;
 read(bfFifo,&reset_msg,sizeof(cmd_msg)); // Supposed to return ACK

 if (reset_msg.id!=ACQ_BE_ACK) {
  qDebug("reticap: Kernel-space backend does not respond!");
  return -1;
 }

 // Acknowledge Signal came from backend.. Everything's OK.
 //  We may close and reopen FIFO in blocking mode...
 close(bfFifo); bfFifo=open("/dev/rtf3",O_RDONLY);

 // Buffer is for 500ms
 if ((shmBuffer=(short *)rtai_malloc('ACQA',
                                     (int(PRE_STIM)+int(POST_STIM))/2)) == 0) {
  qDebug("reticap: Realtime SHM block could not be opened!");
  return -1;
 }

 QApplication app(argc,argv);
 RETICAP reticap(0,fbFifo,bfFifo,shmBuffer,adSampleRate);
// app.setStyleSheet("QGroupBox { border: 1px solid; border-width: 1px; border-style: solid; border-radius: 5px; }"
//                   "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top center; padding: 0 3px; }");
 return app.exec();
}
