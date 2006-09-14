#!/bin/sh
set -e
./cvsclean 
./bootstrap 
export CFLAGS="-Wall -Werror"
export PKG_CONFIG_PATH="/var/tmp/synce/lib/pkgconfig:$PKG_CONFIG_PATH"
./configure \
  --enable-recurrence \
  --prefix=/var/tmp/synce
make
