#!/usr/bin/env python
# -*- coding: utf-8 -*-

import gobject
import dbus
import dbus.glib

class TestApp:
    def __init__(self):
        bus = dbus.SystemBus()
        proxy_obj = bus.get_object("org.synce.odccm", "/org/synce/odccm/DeviceManager")
        mgr = dbus.Interface(proxy_obj, "org.synce.odccm.DeviceManager")
        
        devices = mgr.GetConnectedDevices()
        print devices
        if devices:
            dev_obj = bus.get_object("org.synce.odccm", devices[0])
            dev = dbus.Interface(dev_obj, "org.synce.odccm.Device")
            print "calling RequestConnection on %s" % devices[0]
            print dev.RequestConnection()
        
        mgr.connect_to_signal("DeviceConnected", self.device_connected_cb)
        mgr.connect_to_signal("DeviceDisconnected", self.device_disconnected_cb)

        session_bus = dbus.SessionBus()
        notif_obj = session_bus.get_object("org.freedesktop.Notifications", "/org/freedesktop/Notifications")
        self.notify_iface = dbus.Interface(notif_obj, "org.freedesktop.Notifications")
        
    def device_connected_cb(self, name):
        print "device_connected: '%s'" % name
        self.notify_iface.Notify("SynCE", 0, "", "PDA connected", "The PDA '%s' just connected." % name, [], {}, 3000)

    def device_disconnected_cb(self, name):
        print "device_disconnected: '%s'" % name
        self.notify_iface.Notify("SynCE", 0, "", "PDA disconnected", "The PDA '%s' just disconnected." % name, [], {}, 3000)

TestApp()
mainloop = gobject.MainLoop()
mainloop.run()