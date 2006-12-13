#!/bin/sh

find /lib/modules/$(uname -r) -name "usbnet.ko" -exec rm -v {} \;
find /lib/modules/$(uname -r) -name "cdc_ether.ko" -exec rm -v {} \;
find /lib/modules/$(uname -r) -name "rndis_host.ko" -exec rm -v {} \;
depmod -ae
