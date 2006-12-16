#!/bin/bash

make

sudo rmmod rndis_host
sudo rmmod cdc_ether
sudo rmmod usbnet

if [ "$1" == "--unload-only" ]; then
  exit 0
fi

sleep 1

sudo insmod ./usbnet.ko
sudo insmod ./cdc_ether.ko
sudo insmod ./rndis_host.ko

