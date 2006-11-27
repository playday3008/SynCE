#!/usr/bin/env python
# -*- coding: utf-8 -*-

import gobject
import dbus
import dbus.glib

class TestApp:
    def __init__(self):
        bus = dbus.SessionBus()
        proxy_obj = bus.get_object("org.synce.vdccm.EventManager", "/org/synce/vdccm/EventManager")
        mgr = dbus.Interface(proxy_obj, "org.synce.vdccm.EventManager")
        mgr.connect_to_signal("DeviceConnected", self.device_connected_cb)
        mgr.connect_to_signal("DeviceDisconnected", self.device_disconnected_cb)

        notif_obj = bus.get_object("org.freedesktop.Notifications", "/org/freedesktop/Notifications")
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