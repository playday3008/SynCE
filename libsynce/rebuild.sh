#!/bin/sh
set -x
export CFLAGS="-Wall -Werror -ggdb3"
./bootstrap &&
./configure --prefix=/var/tmp/synce &&
make install
