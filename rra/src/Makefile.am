## useful flags
CFLAGS = -ggdb -Wall -Wno-long-long -Werror -ansi @CFLAGS@ -I../lib
LDADD = ../lib/librra.la

noinst_PROGRAMS = rra-get-types rra-get-ids \
	rra-get-data rra-put-data rra-delete \
	rra-decode rra-db-vcard rra-db-vcal

rra_get_types_SOURCES   = rra-get-types.c
rra_get_ids_SOURCES     = rra-get-ids.c
rra_get_data_SOURCES    = rra-get-data.c
rra_put_data_SOURCES    = rra-put-data.c
rra_delete_SOURCES      = rra-delete.c
rra_decode_SOURCES      = rra-decode.c
rra_db_vcard_SOURCES    = rra-db-vcard.c contact.h contact.c
rra_db_vcal_SOURCES     = rra-db-vcal.c appointment.h appointment.c

##rra_lock_SOURCES = rra-lock.c

