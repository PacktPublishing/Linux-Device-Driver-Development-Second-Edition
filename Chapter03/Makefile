obj-m := cmwq.o \
         tasklet.o \
         hr-timer.o \
         std-timer.o \
         waitqueue.o \
         shared-workqueue.o \
         dedicated-workqueue.o \
         user-invoke.o

KERNELDIR ?= /lib/modules/$(shell uname -r)/build

all default: modules
install: modules_install

modules modules_install help clean:
	$(MAKE) -C $(KERNELDIR) M=$(shell pwd) $@
