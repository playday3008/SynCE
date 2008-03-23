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

import sys
import os
import time

import getopt

def usage():
    print "Usage: synce-kpm [opts]"
    print
    print "Options"
    print "-------"
    print " -h, --help              Display usage info"
    print
    print " -d, --start-dataserver  Start the dataserver only (for dbus-activation)"
    print " -i, --start-iconified   Start the GUI in tray"



def main():

    try:
        opts, args = getopt.getopt(sys.argv[1:], "di", ["help", "start-dataserver", "start-iconified"])
    except getopt.GetoptError, err:
# print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)

    startIconified = False
    startDataServerOnly = False

    for o, a in opts:
        if o in ("-d", "--start-dataserver"):
            startDataServerOnly = True
        elif o in ("-h", "--help"):
            usage()
            sys.exit()
        elif o in ("-i", "--start-iconified"):
            startIconified = True
        else:
            assert False, "unhandled option"


    if startDataServerOnly:
        import synceKPM.main_dataserver
        synceKPM.main_dataserver.main(True)

    else:
        try:
            forkPid = os.fork()
            if forkPid > 0:
                import synceKPM.main_dataserver
                synceKPM.main_dataserver.main(False)
            else:
                import synceKPM.main_gui
                synceKPM.main_gui.main(startIconified)

        except OSError,e:
            print "error: failed to fork - running in foreground (%s)" % e.strerror
            pass
