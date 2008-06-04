ifneq ($(KERNELRELEASE),)
include Kbuild
else
# Normal Makefile

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

install:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules_install
	depmod -ae

clean:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean

endif
