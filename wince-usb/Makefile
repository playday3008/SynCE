# replace with the path to libusb sources where usbi.h can be found.
# it's not enough to install libusb-devel - you must get the sources.
LIBUSB_SRCDIR=.

# replace with path where libusb can be found
LIBDIR=/usr/lib

ipaqd: ipaqd.c $(LIBUSB_SRCDIR)/usbi.h
	cc -o ipaqd -I$(LIBUSB_SRCDIR) ipaqd.c -L$(LIBDIR) -lutil -lusb

clean:
	/bin/rm -f ipaqd
