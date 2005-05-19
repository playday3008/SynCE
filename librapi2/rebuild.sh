#!/bin/sh
set -x
./bootstrap &&
./configure --with-libsynce=/var/tmp/synce --prefix=/var/tmp/synce &&
make install
