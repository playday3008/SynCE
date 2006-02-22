ifeq ($(KERNELRELEASE),)

#
# Customization here:
#

# Change this line if your linux source code is somewhere else
LINUX_SOURCE_PATH := /usr/src/linux-source-$(shell uname -r)

# Use this VERSION line if your kernel is < 2.6.15
VERSION = 2.6.13.3

# Use this VERSION line if your kernel is >= 2.6.15
#VERSION = 2.6.15

#
# You should not need to modify anything else
#

KERNEL_BUILD_DIR := /lib/modules/$(shell uname -r)/build
USB_SERIAL_SOURCE_DIR := $(LINUX_SOURCE_PATH)/drivers/usb/serial
PWD := $(shell pwd)

PATCHED := .patched
FILES := usb-serial.h ipaq.h ipaq.c

default: $(LINUX_SOURCE_PATH) $(USB_SERIAL_SOURCE_DIR) $(FILES) $(PATCHED)
	$(MAKE) -C $(KERNEL_BUILD_DIR) SUBDIRS=$(PWD) modules

all: default

$(LINUX_SOURCE_PATH) $(USB_SERIAL_SOURCE_DIR):
	@echo "Can't find directory $@"
	@echo "Edit $(PWD)/Makefile and provide the correct path to the Linux source tree."
	@false

usb-serial.h: $(USB_SERIAL_SOURCE_DIR)/usb-serial.h
	ln -sf $< $@

ipaq.h: ipaq.h-$(VERSION)
	ln -sf $< $@

ipaq.c: $(PATCHED)
 
$(PATCHED): ipaq.c-$(VERSION)
	cp -p $^ ipaq.c
	patch -N -p0 < ipaq-select-endpoints.diff
	patch -N -p0 < ipaq-version.diff
	-patch -N -p4 < ipaq-psion-teklogix.diff
	patch -N -p4 < ipaq-smartphones.diff
	patch -N -p0 < ipaq-compal.diff
	touch $(PATCHED)


install: ipaq.ko
	install -m 644 $< /lib/modules/$(shell uname -r)/kernel/drivers/usb/serial

clean:
	rm -rf $(FILES) $(PATCHED) .tmp_versions *.mod.c *.orig *.o *.ko .*cmd

else
obj-m	:= ipaq.o
endif

