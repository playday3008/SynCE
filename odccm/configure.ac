dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.53)
AC_INIT(odccm, 0.10.0)
AC_CONFIG_SRCDIR(README)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)

AM_MAINTAINER_MODE
AC_PROG_MAKE_SET

dnl Check for programs.
AC_PROG_CC
AC_PROG_INSTALL

dnl Check for header files.
AC_STDC_HEADERS

AC_ARG_ENABLE(legacy-support, AC_HELP_STRING([--enable-legacy-support], [Build with pre Windows Mobile 5 support]),:,enable_legacy_support=no)
AM_CONDITIONAL(ENABLE_LEGACY_SUPPORT, test x"$enable_legacy_support" != xno)
if test x"$enable_legacy_support" != xno; then
       AC_DEFINE(ENABLE_LEGACY_SUPPORT, 1, [Building with pre Windows Mobile 5 support])
fi

dnl Check for libsynce
PKG_CHECK_MODULES(LIBSYNCE, libsynce >= 0.10.0)

dnl Check for GLib
PKG_CHECK_MODULES(GLIB, [glib-2.0 >= 2.7, gobject-2.0 >= 2.4])

dnl Check for GNet
PKG_CHECK_MODULES(GNET, gnet-2.0)

dnl Check for D-Bus
PKG_CHECK_MODULES(DBUS, [dbus-1 >= 0.60, dbus-glib-1 >= 0.60])

dnl Check for HAL
PKG_CHECK_MODULES(HAL, hal)

AC_OUTPUT([
Makefile
src/Makefile
tests/Makefile
man/Makefile
])
