## useful flags
AM_CFLAGS = -g -Wall -Wno-long-long -Werror -ansi @CFLAGS@ -I../lib
LDADD = ../lib/librra.la

bin_PROGRAMS = rra-partners

noinst_PROGRAMS = rra-get-types rra-get-ids \
	rra-get-data rra-put-data rra-delete \
	rra-decode \
	rra-contact-to-vcard rra-contact-from-vcard \
	rra-appointment-from-vevent rra-appointment-to-vevent \
	rra-task-from-vtodo rra-task-to-vtodo

rra_get_types_SOURCES   = rra-get-types.c
rra_get_ids_SOURCES     = rra-get-ids.c
rra_get_data_SOURCES    = rra-get-data.c
rra_put_data_SOURCES    = rra-put-data.c
rra_delete_SOURCES      = rra-delete.c
rra_decode_SOURCES      = rra-decode.c
#rra_db_vcal_SOURCES     = rra-db-vcal.c appointment.h appointment.c
rra_partners_SOURCES    = rra-partners.c

rra_contact_to_vcard_SOURCES    = rra-contact-to-vcard.c
rra_contact_from_vcard_SOURCES  = rra-contact-from-vcard.c

rra_appointment_to_vevent_SOURCES    = rra-appointment-to-vevent.c
rra_appointment_from_vevent_SOURCES  = rra-appointment-from-vevent.c

rra_task_to_vtodo_SOURCES    = rra-task-to-vtodo.c
rra_task_from_vtodo_SOURCES  = rra-task-from-vtodo.c

##rra_lock_SOURCES = rra-lock.c

