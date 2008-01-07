MINOR_TOOLS_MANS = rra-appointment-from-vevent.1 \
	rra-appointment-to-vevent.1 \
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
	rra-timezone.1

man_MANS = synce-matchmaker.1

if ENABLE_MINOR_TOOLS
man_MANS += $(MINOR_TOOLS_MANS)
EXTRA_DIST = $(man_MANS)
else
EXTRA_DIST = $(man_MANS) $(MINOR_TOOLS_MANS)
endif

