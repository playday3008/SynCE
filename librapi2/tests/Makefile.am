## useful flags
CFLAGS = -ggdb -Wall -ansi -pedantic -I../src @CHECK_CFLAGS@
LIBS = @CHECK_LIBS@
LDADD = ../src/librapi.la

check_PROGRAMS = test-internals test-socket

TESTS = $(check_PROGRAMS)

test_internals_SOURCES = test-internals.c

