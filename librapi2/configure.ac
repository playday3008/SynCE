dnl Process this file with autoconf to produce a configure script.
AC_INIT(src/rapi.c)

dnl AC_CANONICAL_HOST
dnl AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE(librapi, 0.1)
AM_CONFIG_HEADER(rapi_config.h)

dnl Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET
AM_PATH_CHECK()

dnl Check for dmalloc first
AC_ARG_WITH(dmalloc,
		AC_HELP_STRING([--with-dmalloc=PREFIX],[use dmalloc (default is NO)]),
[case "${withval}" in
  yes) dmalloc=true ;;
  no)  dmalloc=false ;;
  *) dmalloc=true; dmalloc_prefix="-L${withval}" ;;
esac],[dmalloc=false])

if ${dmalloc}; then
	AC_CHECK_LIB(dmalloc,main,,[
			AC_MSG_ERROR([Can't find dmalloc library])
		],${dmalloc_prefix})
	AC_CHECK_HEADERS(dmalloc.h,,[
			AC_MSG_ERROR([Can't find dmalloc.h])
		])
fi

dnl Checks for libraries.
AC_LIB_RPATH

dnl Check for libsynce
AM_PATH_LIBSYNCE

dnl Checks for header files.

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

AC_OUTPUT([Makefile
           src/Makefile
					 src/config/Makefile
					 src/support/Makefile
					 tests/Makefile
					 tests/rapi/Makefile
					 tools/Makefile])

