#!/bin/sh
set -x
export CFLAGS=-Werror
export CXXFLAGS=-Werror
./bootstrap &&
./configure --with-libsynce=/var/tmp/synce --prefix=/var/tmp/synce &&
make install
