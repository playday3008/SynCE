#!/bin/sh
set -x
export CFLAGS="-Wall -Werror -ggdb3"
for f in synce unshield dynamite; do
  export PKG_CONFIG_PATH="/var/tmp/$f/lib/pkgconfig:$PKG_CONFIG_PATH"
done
./bootstrap &&
./configure \
  --prefix=/var/tmp/orange \
  --with-libgsf \
  --enable-msi &&
make install
