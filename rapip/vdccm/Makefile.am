bin_PROGRAMS = vdccm
vdccm_SOURCES = vdccm.c list_lib.c list_lib.h network_lib.c network_lib.h select_lib.c select_lib.h timer_lib.c timer_lib.h

# set the include path found by configure
INCLUDES= $(all_includes)

# the library search path.
vdccm_LDFLAGS = $(all_libraries) 
vdccm_LDADD = -lrapi
