#!/bin/sh
./cvsclean && ./bootstrap && ./configure --prefix=/var/tmp/synce --with-librapi2=/var/tmp/synce && make
