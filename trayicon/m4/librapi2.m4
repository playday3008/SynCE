dnl $Id$ vim: syntax=config
dnl Check for librapi2

AC_DEFUN(AM_PATH_LIBRAPI2, [

  AC_ARG_WITH(librapi2,
    AC_HELP_STRING(
      [--with-librapi2[=DIR]],
      [Search for librapi2 in DIR/include and DIR/lib]),
      [
				LDFLAGS="$LDFLAGS -L${withval}/lib"
				CPPFLAGS="$CPPFLAGS -I${withval}/include"
			]
    )

	AC_CHECK_LIB(rapi,CeRapiInit,,[
		AC_MSG_ERROR([Can't find RAPI library])
		])
	AC_CHECK_HEADERS(rapi.h,,[
		AC_MSG_ERROR([Can't find rapi.h])
		])

])
