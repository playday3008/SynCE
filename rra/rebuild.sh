#!/bin/sh
./cvsclean && ./bootstrap && ./configure --prefix=/var/tmp/synce --with-libsynce=/var/tmp/synce --with-librapi2=/var/tmp/synce && make
