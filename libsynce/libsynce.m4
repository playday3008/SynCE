dnl $Id$ vim: syntax=config
dnl Check for libsynce

AC_DEFUN(AM_PATH_LIBSYNCE, [

  AC_ARG_WITH(synce,
    AC_HELP_STRING(
      [--with-synce[=DIR]],
      [Search for libsynce in DIR/include and DIR/lib]),
      [synce_prefix="-L${withval}"]
    )

  if ${synce}; then
    AC_CHECK_LIB(synce,main,,[
        AC_MSG_ERROR([Can't find synce library])
        ],${synce_prefix})
    AC_CHECK_HEADERS(synce.h,,[
        AC_MSG_ERROR([Can't find synce.h])
        ])
  fi

])
