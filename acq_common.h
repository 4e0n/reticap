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

#ifndef ACQ_COMMON
#define ACQ_COMMON

/* Debug */
//#define RETICAP_DEBUG

/* General Definitions & Structures */
#define AD_SAMPLE_RATE	(int)(10000)
#define PRE_STIM_MS	(int)(5)
#define POST_STIM_MS	(int)(500)
#define PRE_STIM	(PRE_STIM_MS*AD_SAMPLE_RATE/1000)
#define POST_STIM	(POST_STIM_MS*AD_SAMPLE_RATE/1000)
#define SAMPLE_COUNT	(PRE_STIM+POST_STIM)

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
