#!/usr/bin/env python
# -*- coding: utf-8 -*-

import gobject
import dbus
import dbus.glib

bus = dbus.SessionBus()
proxy_obj = bus.get_object("org.synce.vdccm.EventManager", "/org/synce/vdccm/EventManager")
mgr = dbus.Interface(proxy_obj, "org.synce.vdccm.EventManager")

def device_connected_cb(name):
    print "device_connected: '%s'" % name

def device_disconnected_cb(name):
    print "device_disconnected: '%s'" % name

mgr.connect_to_signal("DeviceConnected", device_connected_cb)
mgr.connect_to_signal("DeviceDisconnected", device_disconnected_cb)

mainloop = gobject.MainLoop()
mainloop.run()