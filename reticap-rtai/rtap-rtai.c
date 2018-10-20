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

/* This is the acquisition backend RTAI+comedi module which acquires
   single-channel high frequency AP data specifically using a COMEDI
   supported or Advantech PCL-818 (programmed directly) DAQ Card

   Acquisition is started with the message from userspace and the data
   acquired message is sent to userspace using a SYN-ACK handshake protocol
   working over two FIFOs (for data streaming to either side). The acquired
   data is sent to userspace via the same SYN-ACK handshake protocol however
   the acquired data itself is sent over an SHM buffer allocated at the time
   of module initialization. */

//#define DIAGNOSTIC
#define DAQ_COMEDI

#include <linux/module.h>
#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>
#include <rtai_shm.h>
#include <rtai_sem.h>
//#include <rtai_math.h>

#ifndef DIAGNOSTIC

#ifdef DAQ_COMEDI
#include <kcomedilib.h>
#else
#define PCL818BASE 0x300
#endif

#endif

#include "../acq_common.h"

static RT_TASK acq_task;
static RTIME tickperiod;
static SEM acq_sem;

static int acq_active=0,sample_idx=0;
static cmd_msg fb_msg,bf_msg;

static short *acqaddr;

#ifndef DIAGNOSTIC
#ifdef DAQ_COMEDI
static comedi_t *daqcard;
static lsampl_t dummy_sample;
static struct comedi_range* daqcard_range; 
static int ch0_range=0,ch1_range=0;
#endif
#else
static short diagdata=0;
static int theta=0;
#endif

static int i=0;

/* ========================================================================= */

#ifndef DIAGNOSTIC
#ifndef DAQ_COMEDI
short pcl818_data_read(int chn) {
 short sample;
 outb(0,PCL818BASE+0); /* Trigger A/D conversion */
 while (inb(PCL818BASE+8)&0x80); /* Wait until conversion finishes.. */
 sample=((inb(PCL818BASE+1)<<8)|inb(PCL818BASE+0))/16;
 return sample;
}
#endif
#endif

/* ========================================================================= */

static void acq_thread(int t) {
 while (1) {
  if (acq_active) {
   if (sample_idx<SAMPLE_COUNT) {

    rt_sem_wait(&acq_sem);
     /* AD convert to buffer index */
#ifdef DIAGNOSTIC
     if (sample_idx<PRE_STIM) {
      acqaddr[sample_idx]=2048.0;
     } else {
//      diagdata=(short)(2048.0+2048.0*90.0/100.0*
//                              sin((2.0*M_PI*5/(float)AD_SAMPLE_RATE)*
//                                                           (float)theta));
      diagdata=0.;
      theta++; theta%=AD_SAMPLE_RATE;

      acqaddr[sample_idx]=diagdata; /* Acquire to shared mem region */
     }
#else
#ifdef DAQ_COMEDI
     comedi_data_read(daqcard,0,0,ch0_range,AREF_GROUND,&dummy_sample);
     acqaddr[sample_idx]=(short)(dummy_sample)-32768;
#else
     acqaddr[sample_idx]=pcl818_data_read(0);
#endif
#endif
    rt_sem_signal(&acq_sem);

    sample_idx++;
   } else {
    acq_active=0;
    bf_msg.id=ACQ_BE_ACK; bf_msg.param=SAMPLE_COUNT; bf_msg.param2=0;
    rtf_put(BFFIFO,&bf_msg,sizeof(cmd_msg));
   }
  }
  rt_task_wait_period();
 }
}

/* ========================================================================= */

static void reset(void) {
 rt_sem_wait(&acq_sem);
  rtf_reset(BFFIFO); rtf_reset(FBFIFO);
 rt_sem_signal(&acq_sem);
}

static void bfWrite(unsigned short cmd,int p1,int p2) {
 bf_msg.id=cmd; bf_msg.param=p1; bf_msg.param2=p2;
 rtf_put(BFFIFO,&bf_msg,sizeof(cmd_msg));
}

/* ========================================================================= */

int fbfifohandler(unsigned int fifo,int rw) {
 if (rw=='w') {
  rtf_get(FBFIFO,&fb_msg,sizeof(cmd_msg));
  switch (fb_msg.id) {
   case ACQ_TRIG: reset();
                  sample_idx=0; acq_active=1; // theta=0;
                  //rt_printk("rtap-acq-rtai.o: Acquisition started.\n");
                  break;
   case ACQ_RESET: reset();
                   bfWrite(ACQ_BE_ACK,0,0);
                   //rt_printk(
                   // "rtap-acq-rtai.o: Acquisition backend reset.\n");
   default:        break;
  }
 }
 return 0;
}

static int __init rtap_init(void) {
 /* Initialize Shared Memory */
 if ((acqaddr=rtai_kmalloc('ACQA',SAMPLE_COUNT*sizeof(short)))==NULL) {
  rt_printk("rtap-acq-rtai.o: Error while allocating SHM buffer!\n");
  return -1;
 }
 /* Just to be sure - ..fill with zeros */
 for (i=0;i<SAMPLE_COUNT/2;i++) writew(0,acqaddr+i);
 rt_printk("rtap-acq-rtai.o: SHM Allocation successful -> acqaddr=%p.\n",
           acqaddr);

#ifdef DIAGNOSTIC
 rt_printk("rtap-acq-rtai.o: Diagnostic mode.\n");
#else

#ifdef DAQ_COMEDI
 /* Initialize comedi devices */
 daqcard=comedi_open("/dev/comedi0");
 comedi_lock(daqcard,0);

 rt_printk("rtap-acq-rtai.o: Comedi Device Allocation successful ->\n");
 rt_printk("rtap-acq-rtai.o:  DAQCard=%p.\n",daqcard);
#else
 rt_printk("rtap-acq-rtai.o: Real mode .. no comedi.. ->\n");
#endif

#endif

 /* Initialize transfer fifos for commands from the 2-spaces */
 rtf_create_using_bh(FBFIFO,1000*sizeof(cmd_msg),0);
 rtf_create_using_bh(BFFIFO,1000*sizeof(cmd_msg),0);
 rtf_create_handler(FBFIFO,(void*)&fbfifohandler);
 rt_printk("rtap-acq-rtai.o: Up&downstream FIFOs initialized.\n");

#ifndef DIAGNOSTIC
#ifndef DAQ_COMEDI
 outb(0,PCL818BASE+9); /* Control register - Turn off all interrupts, etc. */
 for (i=0;i<15;i++) {  /* Just for maintenance.. Set all channels' ranges */
  outb(i,PCL818BASE+2); // For channel#i
  outb(8,PCL818BASE+1); // Set range to +-10V (0 is +-5V)
 }
 outb(0,PCL818BASE+9); // Set final scan range to convert from only channel#0
#endif
#endif

 /* Initialize acquisition task to acquire AP */
 rt_task_init(&acq_task,(void *)acq_thread,0,2000,0,1,0);
 tickperiod=start_rt_timer(nano2count(1e9/AD_SAMPLE_RATE));
 rt_task_make_periodic(&acq_task,rt_get_time()+tickperiod,tickperiod);

 return 0;
}

static void __exit rtap_exit(void) {
 /* Dispose realtime task */
 stop_rt_timer();
 rt_busy_sleep(1e7);
 rt_task_delete(&acq_task);

 /* Dispose transfer fifos */
 rtf_create_handler(FBFIFO,(void*)&fbfifohandler);
 rtf_destroy(BFFIFO);
 rtf_destroy(FBFIFO);
 rt_printk("rtap-acq-rtai.o: Comm. FIFOs disposed.\n");

#ifndef DIAGNOSTIC
#ifdef DAQ_COMEDI
 comedi_unlock(daqcard,0);
 comedi_close(daqcard);
 rt_printk("rtap-acq-rtai.o: Comedi devices freed.\n");
#endif
#endif

 /* Dispose Acquisition Buffer */
 rtai_kfree('ACQA');
 rt_printk("rtap-acq-rtai.o: Shared Memory freed.\n");
}

module_init(rtap_init);
module_exit(rtap_exit);

//EXPORT_NO_SYMBOLS;
MODULE_AUTHOR("Barkin Ilhan (barkin@turk.net)");
MODULE_DESCRIPTION("RTAP v0.9 - Acq.Server Realtime kernel-space backend");
MODULE_LICENSE("GPL"); 

