dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.53)
AC_INIT(src/dccm.c)

dnl AC_CANONICAL_HOST
dnl AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE(dccm, 0.1)
AM_CONFIG_HEADER(dccm_config.h)

dnl Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET

dnl Check for libsynce
AM_PATH_LIBSYNCE

dnl Checks for header files.

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

AC_REPLACE_FUNCS(daemon)

AC_OUTPUT([Makefile
           src/Makefile
					 man/Makefile])

