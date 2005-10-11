ifeq ($(KERNELRELEASE),)

#
# Modify the line below if needed
#
LINUX_SOURCE_PATH := /usr/src/linux-$(shell uname -r)
#
# You should not need to modify anything else
#

KERNEL_BUILD_DIR := /lib/modules/$(shell uname -r)/build
USB_SERIAL_SOURCE_DIR := $(LINUX_SOURCE_PATH)/drivers/usb/serial
VERSION = 2.6.13.3
PWD := $(shell pwd)

PATCHED := .patched
FILES := usb-serial.h ipaq.h ipaq.c

default: $(USB_SERIAL_SOURCE_DIR) $(FILES) $(PATCHED)
	$(MAKE) -C $(KERNEL_BUILD_DIR) SUBDIRS=$(PWD) modules

all: default

$(USB_SERIAL_SOURCE_DIR):
	@echo "Can't find directory $(USB_SERIAL_SOURCE_DIR)"
	@echo "Edit $(PWD)/Makefile and provide the correct path to the Linux source tree."
	@false

usb-serial.h: $(USB_SERIAL_SOURCE_DIR)/usb-serial.h
	ln -sf $< $@

ipaq.h: ipaq.h-$(VERSION)
	ln -sf $< $@

ipaq.c: $(PATCHED)
 
$(PATCHED): ipaq.c-$(VERSION)
	cp -p $^ ipaq.c
	patch -N -p0 < ipaq-version.diff
	patch -N -p4 < ipaq-psion-teklogix.diff
	patch -N -p4 < ipaq-smartphones.diff
	touch $(PATCHED)


install: ipaq.ko
	install -m 644 $< /lib/modules/$(shell uname -r)/kernel/drivers/usb/serial

clean:
	rm -rf $(FILES) $(PATCHED) .tmp_versions *.mod.c *.orig *.o *.ko .*cmd

else
obj-m	:= ipaq.o
endif

