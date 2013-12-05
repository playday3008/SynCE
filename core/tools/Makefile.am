AM_CFLAGS = -I$(top_srcdir)/lib/utils -I$(top_builddir)/lib/utils

INCLUDES = -I$(top_srcdir)/lib/rapi -I$(top_srcdir)/lib/rapi/support
LDADD = $(top_builddir)/lib/libsynce.la

noinst_PROGRAMS = oidinfo

oidinfo_SOURCES = oidinfo.c

bin_PROGRAMS = synce-pcp \
	synce-pls \
	synce-pmkdir \
	synce-pmv \
	synce-prm \
	synce-prmdir \
	synce-prun \
	synce-pstatus \
	rapiconfig \
	synce-pkillall \
	synce-pshortcut \
	synce-psettime \
	synce-list-programs \
	synce-registry \
	synce-database

synce_pcp_SOURCES    = pcommon.c pcp.c
synce_pls_SOURCES    = pcommon.c pls.c
synce_pmkdir_SOURCES = pcommon.c pmkdir.c
synce_pmv_SOURCES    = pcommon.c pmv.c
synce_prm_SOURCES    = pcommon.c prm.c
synce_prmdir_SOURCES = pcommon.c prmdir.c
synce_prun_SOURCES   = pcommon.c prun.c
synce_pstatus_SOURCES= pstatus.c
synce_pkillall_SOURCES=pkillall.c
synce_pshortcut_SOURCES=pcommon.c pshortcut.c
synce_psettime_SOURCES=psettime.c

rapiconfig_SOURCES = rapiconfig.c

synce_list_programs_SOURCES = synce-list-programs.c
synce_registry_SOURCES = synce-registry.c pcommon.c
synce_database_SOURCES = synce-database.c pcommon.c

noinst_HEADERS = pcommon.h

bin_SCRIPTS = synce-install-cab \
	synce-remove-program
