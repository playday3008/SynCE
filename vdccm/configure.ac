#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libdescriptor, 0.1, voc@users.sourceforge.net)
AC_CONFIG_SRCDIR([config.h.in])
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(vdccm, 0.10.0)

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_DISABLE_STATIC
AM_PROG_LIBTOOL

dnl Check for libsynce
PKG_CHECK_MODULES(LIBSYNCE, libsynce >= 0.10.0)

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_LSTAT
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_CHECK_FUNCS([gethostbyaddr gethostbyname gettimeofday select socket strerror])

AC_CONFIG_FILES([Makefile
                 lib/Makefile
                 src/Makefile
                 tools/Makefile])
AC_OUTPUT
