dnl Process this file with autoconf to produce a configure script.
AC_INIT(src/synce-trayicon.c)

AM_INIT_AUTOMAKE(synce-trayicon, 0.3)
AM_CONFIG_HEADER(config.h)

dnl Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET

dnl Check for librapi2
AM_PATH_LIBRAPI2

PKG_CHECK_MODULES(GNOME2, gtk+-2.0 gdk-2.0 atk libgnomeui-2.0)
AC_SUBST(GNOME2_CFLAGS)
AC_SUBST(GNOME2_LIBS)

dnl Checks for header files.

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

AC_OUTPUT([Makefile
           src/Makefile
					 pixmaps/Makefile])

