dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.53)
AC_INIT(usb-rndis-ng, 0.1)
AC_CONFIG_SRCDIR(src/main.c)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)

AM_MAINTAINER_MODE
AC_PROG_MAKE_SET

dnl Check for programs.
AC_PROG_CC
AC_PROG_INSTALL

dnl Check for libraries.
PKG_CHECK_MODULES(GLIB, glib-2.0,
  [], AC_MSG_ERROR(Cannot find glib-2.0))

PKG_CHECK_MODULES(LIBUSB, libusb,
  [], AC_MSG_ERROR(Cannot find libusb))

dnl Check for header files.
AC_STDC_HEADERS

AC_OUTPUT([
Makefile
src/Makefile
])

