#!/bin/sh
set -x
export CFLAGS="-Wall -Werror -ggdb3"
./autogen.sh --prefix=/var/tmp/synce --enable-dbus &&
make install
