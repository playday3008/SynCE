#!/bin/sh
set -x
export CFLAGS="-Wall -Werror -ggdb3"
./bootstrap &&
./configure \
  --prefix=/var/tmp/synce \
  --with-libsynce=/var/tmp/synce \
  --with-libdynamite=/var/tmp/synce \
  --with-libunshield=/var/tmp/synce \
  --with-libgsf \
  --enable-msi &&
make install
