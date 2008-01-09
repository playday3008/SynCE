#!/usr/bin/env python

import commands
import sys
import os
import os.path
from optparse import OptionParser
import SyncEngine.kernel


def GetEnginePluginSource():
    sepath = str(SyncEngine.kernel).split()[3][1:-2]
    sepath = os.path.dirname(sepath)
    sepath = os.path.dirname(sepath)
    sepath = os.path.join(sepath,"plugins")
    return sepath


def get_plugindir():
    (exitstatus, plugindir) = commands.getstatusoutput('pkg-config opensync-1.0 --variable=plugindir')
    if plugindir == '':
        if exitstatus == 0:
            print 'Could not get OpenSync plugin directory, even though OpenSync is installed..'
        else:
            print 'Could not get OpenSync plugin directory; is the OpenSync dev package installed?'
        sys.exit(1)
    elif exitstatus == 32512:
        print 'Could not get OpenSync plugin directory; pkg-config is not installed.'
        sys.exit(1)

    plugindir = os.path.join(os.path.dirname(plugindir),"plugins", "python-plugins")

    if not os.path.isdir(plugindir):
        os.system('mkdir -p %s' % plugindir)

    return plugindir

def get_opensync_version():
    return commands.getoutput('pkg-config opensync-1.0 --modversion')

if __name__ == '__main__':
    parser = OptionParser()
    parser.add_option('-i', '--ignore-root', action='store_true', dest='ignore_root', default=False, help='Check for root')
    (options, args) = parser.parse_args()

    user = commands.getoutput('whoami')
    if user != 'root' and not options.ignore_root:
        print 'Error: must be run as root. To ignore this, use --ignore-root'
        sys.exit(1)

    opensync_version = get_opensync_version()
    plugindir = get_plugindir()
    print 'Resetting python-plugins directory...'

    os.system('rm -f %s/opensync-plugin-0.30later.py %s/opensync-plugin.py %s/synce-opensync-plugin-3x.py %s/synce-opensync-plugin-2x.py 2> /dev/null' % (plugindir, plugindir, plugindir, plugindir))

    print 'OpenSync version %s installed.' % opensync_version

    if opensync_version >= 0.30:
        driver = 'synce-opensync-plugin-3x.py'
    else:
        driver = 'synce-opensync-plugin-2x.py'
    print 'Using driver %s' % driver

    pluginsource=GetEnginePluginSource()
    os.system('cp %s/%s %s' % (pluginsource,driver, plugindir))
    print 'Done.'
