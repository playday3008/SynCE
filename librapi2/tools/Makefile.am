## useful flags
CFLAGS   = -ggdb -Wno-long-long -Wall -I../src -I../src/support
CXXFLAGS = -ggdb -Werror -Wno-long-long -Wall -ansi -pedantic -I../src -I../src/support
LDADD = ../src/librapi.la

noinst_PROGRAMS = pput pget

pput_SOURCES = pput.cpp
pget_SOURCES = pget.cpp

bin_PROGRAMS = pcp pls pmv prm

pcp_SOURCES = pcp.c pcommon.c
pls_SOURCES = pls.c pcommon.c
pmv_SOURCES = pmv.c pcommon.c
prm_SOURCES = prm.c pcommon.c

SUBDIRS = man

