#
# Makefile for the ipaq usb driver.
#
# Original version by Ganesh Varadarajan.
#
# $Id$
#

# Edit KDIR to suit your setup.

ifeq ($(KERNELRELEASE),)

KERNELVERSION	:= $(shell uname -r)
KDIR		:= /lib/modules/$(KERNELVERSION)/build
PWD		:= $(shell pwd)

FILES := usb-serial.h ipaq.h ipaq.c

all: ipaq.o

ipaq.o: $(FILES)
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

install: ipaq.o
	install -m 644 $< /lib/modules/$(KERNELVERSION)/kernel/drivers/usb/serial

clean::
	/bin/rm -f $(FILES)
	/bin/rm -f ipaq.o
	/bin/rm -f .ipaq.o.flags

usb-serial.h:	$(KDIR)/drivers/usb/serial/usb-serial.h
	ln -sf $< $@

ipaq.h:	$(KDIR)/drivers/usb/serial/ipaq.h
	ln -sf $< $@

ipaq.c:	$(KDIR)/drivers/usb/serial/ipaq.c
	ln -sf $< $@

else

obj-m += ipaq.o

include $(TOPDIR)/Rules.make

endif
