/* RETICAP - Real-time Action Potential Recording System 0.5 */
/*                (>) GPL 2013 Barkin Ilhan                  */
/*        Unified Neuroscience Research Laboratories         */
/*                    barkin@unrlabs.org                     */
/*               http://www.unrlabs.org/barkin               */

#ifndef ACQ_COMMON
#define ACQ_COMMON

/* Debug */
//#define RTAP_DEBUG

/* General Definitions & Structures */
#define AD_SAMPLE_RATE	(int)(40000)
#define PRE_STIM_MS	(int)(5)
#define POST_STIM_MS	(int)(250)
#define PRE_STIM	(PRE_STIM_MS*AD_SAMPLE_RATE/1000)
#define POST_STIM	(POST_STIM_MS*AD_SAMPLE_RATE/1000)
#define SAMPLE_COUNT	(PRE_STIM+POST_STIM)

#define VRANGE		(float)(10.0)

#define FBFIFO		2
#define BFFIFO		3
#define	ACQ_RESET	0x0001
#define	ACQ_TRIG	0x1001
#define	ACQ_BE_ACK	0x9001
#define SAMPLE_SIZE	(sizeof(short))

typedef struct _cmd_msg {
 unsigned short id;
 int param,param2;
} cmd_msg;

#endif
