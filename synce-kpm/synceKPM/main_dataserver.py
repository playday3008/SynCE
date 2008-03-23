############################################################################## 
#    Copyright (C) 2007 Guido Diepen
#    Email: Guido Diepen <guido@guidodiepen.nl>
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
##############################################################################
import gobject
import time

import dbus
import dbus.service
import dbus.mainloop.glib

import synceKPM.dataserver.dataserver
import synceKPM.constants

import sys

def main(startDataServerOnly):
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
  
    abortStart=False
    if not startDataServerOnly:
#check whether there is already one running...
        try:
            _ds = dbus.SessionBus().get_object("org.synce.kpm.dataserver",synceKPM.constants.DBUS_SYNCEKPM_DATASERVER_OBJPATH)
            print "The dataserver is already running...."
            #sys.exit(1)
            abortStart = True
        except Exception,e :
            pass
    if abortStart:
        sys.exit(0)

    session_bus = dbus.SessionBus()
    name = dbus.service.BusName(synceKPM.constants.DBUS_SYNCEKPM_DATASERVER_BUSNAME, session_bus)
    
    dataServerEventLoop = gobject.MainLoop()
    
    dataServer = synceKPM.dataserver.dataserver.DataServer(session_bus, dataServerEventLoop)

    print"Running dataserver"
    dataServerEventLoop.run()
