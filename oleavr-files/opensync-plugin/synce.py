from opensync import *

class SyncClass:
    def __init__(self, member):
        self.__member = member

    def connect(self, ctx):
        print "Connect called!!"
        ctx.report_success()

    def get_changeinfo(self, ctx):
        print "get_changeinfo called!!"
        if self.__member.get_slow_sync("data"):
            print "Slow-sync requested"
        #change = OSyncChange()
        #change.uid = "testuid"
        #change.set_data("testdata", 9, TRUE)
        #change.format = "plain"
        #change.objtype = "data"
        #change.changetype = CHANGE_ADDED
        #change.report(ctx)
        ctx.report_success()
        print "done with get_changeinfo"

    def commit_change(self, ctx, chg):
        print "commit called!!"
        print "Opensync wants me to write data with size " + str(chg.datasize)
        print chg.format
        print "Data: \"%s\"" % chg.data
        ctx.report_success()

    def disconnect(self, ctx):
        print "disconnect called!"
        ctx.report_success()

    def sync_done(self, ctx):
        print "sync_done called!"
        ctx.report_success()

    def finalize(self):
        print "finalize called!"

def initialize(member):
    return SyncClass(member)

def get_info(info):
    info.name = "synce-plugin"
    info.longname = "Plugin to synchronize with Windows CE device"
    info.description = "by Ole Andre Vadla Ravnaas"

    info.version = 2

    info.accept_objtype("contact")
    info.accept_objformat("contact", "vcard30")
