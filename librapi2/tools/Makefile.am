## useful flags
CFLAGS   = -ggdb -Wno-long-long -Wall @APPLE_CFLAGS@ -I../src -I../src/support
CXXFLAGS = -ggdb -Werror -Wno-long-long -Wall -ansi -pedantic -I../src -I../src/support
LDADD = ../src/librapi.la

noinst_PROGRAMS = oidinfo

oidinfo_SOURCES = oidinfo.c

bin_PROGRAMS = pcp pls pmkdir pmv prm prmdir prun pstatus

pcp_SOURCES    = pcommon.c pcp.c
pls_SOURCES    = pcommon.c pls.c
pmkdir_SOURCES = pcommon.c pmkdir.c
pmv_SOURCES    = pcommon.c pmv.c
prm_SOURCES    = pcommon.c prm.c
prmdir_SOURCES = pcommon.c prmdir.c
prun_SOURCES   = pcommon.c prun.c
pstatus_SOURCES= pstatus.c

SUBDIRS = man

