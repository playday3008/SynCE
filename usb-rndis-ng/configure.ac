dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.53)
AC_INIT(usb-rndis-ng, 0.10.0)
AC_CONFIG_SRCDIR(src/main.c)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)

AC_CONFIG_HEADERS([config.h])

AM_MAINTAINER_MODE
AC_PROG_MAKE_SET

dnl Check for programs.
AC_PROG_CC
AC_PROG_INSTALL

dnl Options.
AC_ARG_ENABLE(hal, [  --enable-hal            enable HAL support],
              enable_hal=$enableval, enable_hal=auto)

dnl Check for libraries.
PKG_CHECK_MODULES(GLIB, glib-2.0,
  [], AC_MSG_ERROR(Cannot find glib-2.0))

PKG_CHECK_MODULES(GTHREAD, gthread-2.0,
  [], AC_MSG_ERROR(Cannot find gthread-2.0))

PKG_CHECK_MODULES(LIBUSB, libusb,
  [], AC_MSG_ERROR(Cannot find libusb))

if ! test "$enable_hal" = "no"; then
  PKG_CHECK_MODULES(HAL, hal,
          [
            hal=yes
            AC_DEFINE(HAVE_HAL, , [Define if building with HAL support])
          ],
          hal=no
  )

  if test "$hal" = "no"; then
    if test "$enable_hal" = "auto"; then
      AC_MSG_NOTICE([HAL not detected, building without HAL support])
    else
      AC_MSG_ERROR([HAL not found])
    fi
  fi

  enable_hal=$hal
else
  AC_MSG_NOTICE([building without HAL support])
fi

if test "$enable_hal" = "yes"; then
  prefixes="/usr /usr/local /"
  callout_dirs="/lib64/hal/scripts /lib64/hal /lib/hal/scripts /lib/hal /share/hal/scripts"

  AC_MSG_CHECKING(for location of HAL fdi files)
  found=0
  for pfx in $prefixes; do
    dir=$pfx/share/hal/fdi
    if test -d $dir; then
      found=1
      AC_MSG_RESULT($dir)
      HAL_FDI_DIR=$dir
      break
    fi
  done

  if test $found -eq 0; then
    AC_MSG_FAILURE(not found)
  fi

  AC_MSG_CHECKING(for location of HAL callouts)
  found=0
  for pfx in $prefixes; do
    for dir in $callout_dirs; do
      path=$pfx$dir
      if test -d $path; then
        found=1
        AC_MSG_RESULT($path)
        HAL_CALLOUT_DIR=$path
        break
      fi
    done

    if test $found -eq 1; then break; fi
  done

  if test $found -eq 0; then
    AC_MSG_FAILURE(not found)
  fi

  AC_SUBST(HAL_FDI_DIR)
  AC_SUBST(HAL_CALLOUT_DIR)
fi

AM_CONDITIONAL(HAVE_HAL, test "$enable_hal" = "yes")

dnl Check for header files.
AC_STDC_HEADERS

AC_OUTPUT([
Makefile
src/Makefile
data/Makefile
data/hal/Makefile
])

