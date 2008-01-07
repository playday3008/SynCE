#!/bin/sh
set -e
./cvsclean 
export CFLAGS="-Wall -Werror"
export PKG_CONFIG_PATH="/var/tmp/synce/lib/pkgconfig:$PKG_CONFIG_PATH"
./autogen.sh \
  --enable-recurrence \
  --prefix=/var/tmp/synce
make
