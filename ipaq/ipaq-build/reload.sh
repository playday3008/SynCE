#!/bin/bash

make

sudo rmmod ipad

if [ "$1" == "--unload-only" ]; then
  exit 0
fi

sleep 1

sudo insmod ./ipaq

