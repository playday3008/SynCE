#!/usr/bin/env python
# -*- coding: utf-8 -*-

import dbus
import dbus.glib
import gtk

class TestApp:
    def __init__(self):
        self.devices = {}

        bus = dbus.SystemBus()
        self.bus = bus
        try:
            proxy_obj = bus.get_object("org.synce.odccm", "/org/synce/odccm/DeviceManager")
        except dbus.DBusException:
            print "Error: Could not connect to odccm. Is it started?"
            raise

        mgr = dbus.Interface(proxy_obj, "org.synce.odccm.DeviceManager")

        mgr.connect_to_signal("DeviceConnected", self.device_connected_cb)
        mgr.connect_to_signal("DeviceDisconnected", self.device_disconnected_cb)

        session_bus = dbus.SessionBus()
        notif_obj = session_bus.get_object("org.freedesktop.Notifications", "/org/freedesktop/Notifications")
        self.notify_iface = dbus.Interface(notif_obj, "org.freedesktop.Notifications")

        for obj_path in mgr.GetConnectedDevices():
            self._add_device(obj_path, False)

        print "Waiting for device to hotplug"
        
    def device_connected_cb(self, obj_path):
        self._add_device(obj_path, True)

    def device_disconnected_cb(self, obj_path):
        if obj_path in self.devices:
            device = self.devices[obj_path]
            self.notify_iface.Notify("SynCE", 0, "", "PDA disconnected", "'%s' just disconnected." % device.name, [], {}, 3000)
            del self.devices[obj_path]

    def _add_device(self, obj_path, just_connected):
        device = CeDevice(self.bus, obj_path)
        self.devices[obj_path] = device

        if just_connected:
            self.notify_iface.Notify("SynCE", 0, "", "PDA connected", "A %s %s '%s' just connected." % \
                (device.model_name, device.platform_name, device.name), [], {}, 3000)


ODCCM_DEVICE_PASSWORD_FLAG_SET     = 1
ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE = 2

class CeDevice:
    def __init__(self, bus, obj_path):
        self.obj_path = obj_path
        dev_obj = bus.get_object("org.synce.odccm", obj_path)
        dev = dbus.Interface(dev_obj, "org.synce.odccm.Device")
        self.name = dev.GetName()
        self.platform_name = dev.GetPlatformName()
        self.model_name = dev.GetModelName()
        self.dev_iface = dev
        
        self._print_debug()
        
        self._password_flags_changed()

    def _print_debug(self):
        dev = self.dev_iface
        print "Created CeDevice with obj_path=\"%s\"" % self.obj_path
        print "  GetIpAddress:", dev.GetIpAddress()
        print "  GetGuid:", dev.GetGuid()
        print "  GetOsVersion:", dev.GetOsVersion()
        print "  GetName:", dev.GetName()
        print "  GetVersion:", dev.GetVersion()
        print "  GetCpuType:", dev.GetCpuType()
        print "  GetCurrentPartnerId:", dev.GetCurrentPartnerId()
        print "  GetId:", dev.GetId()
        print "  GetPlatformName:", dev.GetPlatformName()
        print "  GetModelName:", dev.GetModelName()
        print "  GetPasswordFlags:", dev.GetPasswordFlags()

    def password_flags_changed_cb(self, added, removed):
        print "password_flags_changed_cb: added=0x%08x removed=0x%08x" % (added, removed)
        self._password_flags_changed()

    def _password_flags_changed(self):
        flags = self.dev_iface.GetPasswordFlags()

        if flags & ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE:
            print "Device requires password, asking user"
            authenticated = False
            while not authenticated:
                dlg = EntryDialog(None, "Password required",
                                  "The PDA '%s' is password-protected.  Enter password:" % self.name,
                                  True)
                if dlg.run() != gtk.RESPONSE_ACCEPT:
                    print "Dialog canceled by user"
                    dlg.destroy()
                    return
                authenticated = self.dev_iface.ProvidePassword(dlg.get_text())
                dlg.destroy()
                if not authenticated:
                    print "Password mismatch"
            print "Password accepted. Have a nice day."
        else:
            print "Device is not requiring a password"


class EntryDialog(gtk.Dialog):
    def __init__(self, parent, title, text, password=False):
        gtk.Dialog.__init__(self, title, parent,
                            gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT | gtk.DIALOG_NO_SEPARATOR,
                            (gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT,
                             gtk.STOCK_OK, gtk.RESPONSE_ACCEPT | gtk.CAN_DEFAULT)
                            )

        self.set_default_response(gtk.RESPONSE_ACCEPT)
        label = gtk.Label(text)
        label.set_alignment(0.0, 0.5)
        self.vbox.pack_start(label, False)
        self._label = label

        entry = gtk.Entry()
        entry.set_visibility(not password)
        entry.set_activates_default(True)
        self.vbox.pack_start(entry, False, True, 5)
        self._entry = entry

        self.show_all()

    def get_text(self):
        return self._entry.get_text()


TestApp()
gtk.main()
