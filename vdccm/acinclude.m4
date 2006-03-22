AC_DEFUN([SYNCE_CHECK_SYNCE],
	[
		if test x = x$synce_synce_checked; then
			synce_kde_save_cflags="$CPPFLAGS"
			synce_kde_save_ldflags="$LDFLAGS"

			AC_ARG_WITH(libsynce,
				AC_HELP_STRING(
					[--with-libsynce=DIR],
					[Search for libsynce in DIR/include and DIR/lib]
				),
				[
					SYNCE_INCLUDES="-I${withval}/include"
					SYNCE_LDFLAGS="-L${withval}/lib"
				]
			)

			AC_ARG_WITH(libsynce-include,
				AC_HELP_STRING(
					[--with-libsynce-include=DIR],
					[Search for libsynce header files in DIR]
				),
				[
					SYNCE_INCLUDES="-I${withval}"
				]
			)

			AC_ARG_WITH(libsynce-lib,
				AC_HELP_STRING(
					[--with-libsynce-lib=DIR],
					[Search for libsynce library files in DIR]
				),
				[
					SYNCE_LDFLAGS="-L${withval}"
				]
			)

			LDFLAGS="$LDFLAGS $SYNCE_LDFLAGS"
			AC_CHECK_LIB(synce, main,
				[
					SYNCE_LIB="-lsynce"
				],
				[
					AC_MSG_ERROR([Can't find synce library])
					exit
				]
			)

			CPPFLAGS="$CPPFLAGS $SYNCE_INCLUDES"
			AC_CHECK_HEADERS(synce.h,,
				[
					AC_MSG_ERROR([Can't find synce.h])
					exit
				]
			)

			AC_SUBST(SYNCE_INCLUDES)
			AC_SUBST(SYNCE_LDFLAGS)
			AC_SUBST(SYNCE_LIB)

			CPPFLAGS="$synce_kde_save_cflags"
			LDFLAGS="$synce_kde_save_ldflags"
			unset synce_kde_save_cflags
			unset synce_kde_save_ldflags
			synce_synce_checked=yes
		fi
	]
)
