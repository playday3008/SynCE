dnl $Id$ vim: syntax=config
dnl Check for rra

AC_DEFUN([AM_PATH_RRA], [

  AC_ARG_WITH(rra,
    AC_HELP_STRING(
      [--with-rra[=DIR]],
      [Search for RRA in DIR/include and DIR/lib]),
      [
				CPPFLAGS="$CPPFLAGS -I${withval}/include/rra"
				LDFLAGS="$LDFLAGS -L${withval}/lib"
			]
    )

  AC_ARG_WITH(rra-include,
    AC_HELP_STRING(
      [--with-rra-include[=DIR]],
      [Search for RRA header files in DIR/rra]),
      [
				CPPFLAGS="$CPPFLAGS -I${withval}/rra"
			]
    )

  AC_ARG_WITH(rra-lib,
    AC_HELP_STRING(
      [--with-rra-lib[=DIR]],
      [Search for RRA library files in DIR]),
      [
				LDFLAGS="$LDFLAGS -L${withval}"
			]
    )

	AC_CHECK_LIB(rra,rra_syncmgr_new,,[
		AC_MSG_ERROR([Can't find RRA library])
		])
	AC_CHECK_HEADERS(rra/syncmgr.h,,[
		AC_MSG_ERROR([Can't find rra/syncmgr.h])
		])

])
