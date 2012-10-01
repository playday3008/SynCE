MINOR_TOOLS_MANS = rra-appointment-from-vevent.1

MINOR_TOOLS_LINKS = rra-appointment-to-vevent.1 \
	rra-contact-from-vcard.1 \
	rra-contact-to-vcard.1 \
	rra-decode.1 \
	rra-delete.1 \
	rra-get-data.1 \
	rra-get-ids.1 \
	rra-get-recurring-appointments.1 \
	rra-get-types.1 \
	rra-put-data.1 \
	rra-subscribe.1 \
	rra-task-from-vtodo.1 \
	rra-task-to-vtodo.1 \
	rra-timezone.1 \
	rra-file-unpack.1 \
	rra-file-pack.1

CLEANFILES = $(MINOR_TOOLS_LINKS)

MATCHMAKER_MANS = synce-matchmaker.1

man_MANS = $(MATCHMAKER_MANS)

EXTRA_DIST = $(MATCHMAKER_MANS) $(MINOR_TOOLS_MANS)

if ENABLE_MINOR_TOOLS
man_MANS += $(MINOR_TOOLS_MANS) $(MINOR_TOOLS_LINKS)
endif

$(MINOR_TOOLS_LINKS): rra-appointment-from-vevent.1
	echo ".so man1/rra-appointment-from-vevent.1" > $@ 
