#!/bin/sh
set -x
export CFLAGS=-Werror
export CXXFLAGS=-Werror
export PKG_CONFIG_PATH="/var/tmp/synce/lib/pkgconfig:$PKG_CONFIG_PATH
./bootstrap &&
./configure --prefix=/var/tmp/synce &&
make install
