#!/bin/sh
set -x
export CFLAGS="-Wall -Werror -ggdb3 -pedantic-errors -D_FORTIFY_SOURCE=2"
./bootstrap &&
./configure --prefix=/var/tmp/synce &&
make install
