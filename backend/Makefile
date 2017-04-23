# * RETICAP - Real-time Action Potential Recording System 0.9 *
# *                (>) GPL 2013 Barkin Ilhan                  *
# *        Unified Neuroscience Research Laboratories         *
# *                    barkin@unrlabs.org                     *
# *               http://www.unrlabs.org/barkin               *

obj-m    := reticap-rtai.o

CURRENT = $(shell uname -r)
KDIR    := /lib/modules/$(shell uname -r)/build
PWD    := $(shell pwd)
EXTRA_CFLAGS += -O2 -I/usr/realtime/include -I/usr/src/comedi/include/linux -I/usr/include -Wall -ffast-math -mhard-float

default:
	@echo --------------------------------------------
	@echo PWD = $(PWD)
	@echo CURRENT = $(CURRENT)
	@echo KDIR = $(KDIR)
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules 

clean:
	-rm -f *.o *.ko *.o.d *.symvers .*.cmd .*.flags *.mod.c
