AC_INIT(Fur_config,version-0.2)
AC_PROG_CC

PKG_CONFIG_PATH=/usr/local/lib:$PKG_CONFIG_PATH

AC_LANG(C)

AC_CHECK_LIB([rapi],
             [CeCreateFile],
             [LDFLAGS="$LDFLAGS -lrapi"],
             [AC_MSG_ERROR(librapi must be present!)])

dnl AC_CHECK_LIB([rapi],
dnl 	     [CeSetEndOfFile],
dnl 	     [],
dnl 	     [AC_MSG_ERROR(librapi2 must be patched and recompiled to run this version of FUR!)])

AC_CHECK_LIB([rapi],
	     [CeSetFilePointer],
	     [],
	     [AC_MSG_ERROR(librapi2 must be patched and recompiled to run this version of FUR!)])

AC_CHECK_LIB([synce],
             [wstr_from_ascii],
             [LDFLAGS="$LDFLAGS -lsynce"],
             [AC_MSG_ERROR(libsynce must be present!)])

AC_CHECK_HEADERS([rapi.h synce.h],
                 [],
                 [AC_MSG_ERROR(Synce and rapi must be present !)])

if test `pkg-config fuse`
then
   AC_MSG_NOTICE([pkg-config didn't found fuse]) 

   AC_CHECK_LIB([fuse],
                [fuse_main],
                [LDFLAGS="$LDFLAGS -lfuse"],
                [AC_MSG_ERROR(Fuse must be installed in order for Fur to work!)])

   AC_CHECK_HEADERS([fuse],
                [],
                [AC_MSG_ERROR(Fuse must be installed in order for Fur to work!)])
else			
   CFLAGS="$CFLAGS `pkg-config --cflags fuse`"
   LDFLAGS="$LDFLAGS `pkg-config --libs fuse`"
fi

dnl custom test to verify the fuse presence 
dnl for some reason this doesn't work
dnl AC_CHECK_HEADERS([fuse.h],
dnl                  [],
dnl 	         [AC_MSG_ERROR(Fuse must be installed in order for Fur to work!)])

if pkg-config --exists fuse
   then
   CFLAGS="$CFLAGS `pkg-config --cflags fuse`"
else
  AC_MSG_ERROR(Fuse must be installed in order for Fur to work!)	
fi

AC_ARG_ENABLE([void-chmod],
	[AS_HELP_STRING([--enable-void-chmod],
	   [enable a fake fuse hook for "chmod" that just returns 0])],[CFLAGS="$CFLAGS -DVOID_CHMOD"],[])

AC_MSG_NOTICE([$CC flags will be $CFLAGS])
AC_MSG_NOTICE([linker flags will be $LDFLAGS])

AC_OUTPUT(Makefile)

