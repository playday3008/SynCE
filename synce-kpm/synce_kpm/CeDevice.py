import dbus
import dbus.glib

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
        
        #self._print_debug()

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

