AM_CFLAGS = @LIBSYNCE_CFLAGS@
AM_LDFLAGS = @LIBSYNCE_LIBS@

INCLUDES = -I$(top_srcdir)/lib
LDADD = -L$(top_builddir)/lib $(top_builddir)/lib/librra.la

MINOR_TOOLS_LIST = rra-get-types rra-get-ids \
	rra-get-data rra-put-data rra-delete \
	rra-decode rra-subscribe \
	rra-get-recurring-appointments \
	rra-contact-to-vcard rra-contact-from-vcard \
	rra-appointment-from-vevent rra-appointment-to-vevent \
	rra-task-from-vtodo rra-task-to-vtodo \
	rra-file-unpack rra-file-pack \
	rra-timezone

bin_PROGRAMS = synce-matchmaker 

noinst_PROGRAMS =

if ENABLE_MINOR_TOOLS
bin_PROGRAMS += $(MINOR_TOOLS_LIST)
else
noinst_PROGRAMS += $(MINOR_TOOLS_LIST)
endif

synce_matchmaker_SOURCES = rra-partners.c

rra_get_types_SOURCES   = rra-get-types.c
rra_get_ids_SOURCES     = rra-get-ids.c
rra_get_data_SOURCES    = rra-get-data.c
rra_put_data_SOURCES    = rra-put-data.c
rra_delete_SOURCES      = rra-delete.c
rra_decode_SOURCES      = rra-decode.c
rra_subscribe_SOURCES    = rra-subscribe.c
#rra_db_vcal_SOURCES     = rra-db-vcal.c appointment.h appointment.c

rra_get_recurring_appointments_SOURCES = rra-get-recurring-appointments.c

rra_contact_to_vcard_SOURCES    = rra-contact-to-vcard.c
rra_contact_from_vcard_SOURCES  = rra-contact-from-vcard.c

rra_appointment_to_vevent_SOURCES    = rra-appointment-to-vevent.c
rra_appointment_from_vevent_SOURCES  = rra-appointment-from-vevent.c

rra_task_to_vtodo_SOURCES    = rra-task-to-vtodo.c
rra_task_from_vtodo_SOURCES  = rra-task-from-vtodo.c

rra_file_unpack_SOURCES  = rra-file-unpack.c
rra_file_pack_SOURCES  = rra-file-pack.c

rra_timezone_SOURCES = rra-timezone.c

##rra_lock_SOURCES = rra-lock.c
