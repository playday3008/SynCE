bin_PROGRAMS = asyncd
asyncd_SOURCES = socket.c debug.c asyncd.c
asyncd_LDADD   = 

EXTRA_DIST = asyncd.c asyncd.h debug.c debug.h socket.c socket.h README

# set the include path found by configure
INCLUDES = $(all_includes)

# the library search path.
asyncd_LDFLAGS = $(all_libraries) 
                                                                                
AM_CFLAGS = -DDEBUG -DUSE_SYSLOG #-Wall
SUBDIRS = 
