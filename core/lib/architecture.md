@page architecture Architecture of the core connection

@section ipconnection Physical and IP connection

The files and tools mentioned in this section can be found in the @b scripts directory of the source.

The synce system relies on @b udev to send notifications of devices being connected and disconnected.
The behaviour of udev is configured in the file @b 85-synce.rules , to look for use of the ipaq and
rndis_host drivers for serial and rndis devices respectively.

Older serial devices, particularly those using generic serial/USB converters, may not be automatically
detectable through udev using these criteria, for which the @b synce-serial script provides a manual
way of triggering the connection.

Once udev detects a device activity that matches these criteria, the scripts @b synce-udev-rndis or
@b synce-udev-serial are invoked, depending on device type. Udev scripts are blocking, so these are
simply wrappers that can background the real connection scripts, @b udev-synce-rndis and
@b udev-synce-serial .

For a serial device, @b udev-synce-serial starts or stops a ppp connection to the device, and signals
the @dccm process via dbus of the connection or disconnection event.

For an rndis device, @b udev-synce-rndis first attempts to configure a network connection
using the dhcp client, for which we use a simple customised configuration, since many distributions
have a default configuration that is unsuitable for this. If this fails, it falls back to a static
network. The @dccm process is then notified via dbus.

@section appconnection Application connection and dccm

@b dccm is a daemon process that manages device connections after a valid IP connection has been
established, and provides a means for client applications to establish a connection to a device. The
process is started and controlled by dbus communication.

After the connection scripts have established a network connection, dccm is signalled over dbus, which
starts the process if necessary.

dccm then listens for the initial handshake and device info from the device, in the case of rndis devices
first sending a trigger message. Once the device information has been received, the device is advertised
on dbus as being available for client applications.

Client applications can request a list of attached devices via dbus, as well as basic device information.

When a client makes an application level connection to a device, ie. a RAPI connection, this is also
established through dccm, and the socket file descriptor is passed to the requesting application over a
Unix domain socket, the location of which is communicated over dbus. This process is transparent to the
application using the RAPI API.

