ifneq ($(KERNELRELEASE),)
obj-m	:= ipaq.o

else
KERNEL_BUILD_DIR := /lib/modules/$(shell uname -r)/build
USB_SERIAL_SOURCE_DIR := /usr/src/linux-$(shell uname -r | sed -e 's/-default//' -e 's/-smp//')/drivers/usb/serial
PWD := $(shell pwd)

FILES := usb-serial.h ipaq.h ipaq.c

default: $(FILES)
	$(MAKE) -C $(KERNEL_BUILD_DIR) SUBDIRS=$(PWD) modules

all: default

usb-serial.h: $(USB_SERIAL_SOURCE_DIR)/usb-serial.h
	ln -sf $< $@

ipaq.h: $(USB_SERIAL_SOURCE_DIR)/ipaq.h
	ln -sf $< $@

ipaq.c: $(USB_SERIAL_SOURCE_DIR)/ipaq.c
	cp $< $@
	patch -N -p0 < free_len_zero.patch
	patch -N -p0 < tty_hangup.patch

install: ipaq.ko
	install -m 744 $< /lib/modules/$(shell uname -r)/kernel/drivers/usb/serial

clean:
	rm -rf $(FILES) .tmp_versions *.mod.c *.orig *.o *.ko .*cmd

endif

