dnl Process this file with autoconf to produce a configure script.
AC_INIT(src/rapi_buffer.c)
AM_INIT_AUTOMAKE(librapi, 0.1)
AM_CONFIG_HEADER(rapi_config.h)
	
dnl Checks for programs.
AC_PROG_AWK
AC_PROG_CC
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
dnl Checks for header files.

AC_CHECK_HEADERS(byteswap.h sys/byteswap.h endian.h machine/endian.h iconv.h)
	
AC_CHECK_HEADERS(sys/types.h,,[
		AC_MSG_ERROR([Don't know how to get u_int16_t and u_int32_t on your system])
		])

AC_CHECK_HEADERS(stdbool.h,,[
		AC_MSG_ERROR([Don't know how to get bool values on your system])
		])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN()
AC_C_CONST

dnl Checks for library functions.
AM_ICONV()
AC_CHECK_LIB(socket,main)
AC_CHECK_LIB(nsl,main)

AC_OUTPUT([Makefile
           src/Makefile
					 src/config/Makefile
					 src/support/Makefile
					 tests/Makefile])
