dnl $Id$ vim: syntax=config
dnl Check for libsynce

AC_DEFUN(AM_PATH_LIBSYNCE, [

  AC_ARG_WITH(libsynce,
    AC_HELP_STRING(
      [--with-libsynce[=DIR]],
      [Search for libsynce in DIR/include and DIR/lib]),
      [
				LDFLAGS="$LDFLAGS -L${withval}/lib"
				CPPFLAGS="$CPPFLAGS -I${withval}/include"
			]
    )

	AC_CHECK_LIB(synce,main,,[
		AC_MSG_ERROR([Can't find synce library])
		])
	AC_CHECK_HEADERS(synce.h,,[
		AC_MSG_ERROR([Can't find synce.h])
		])

])
