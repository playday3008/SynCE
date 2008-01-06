#!/usr/bin/env python

from setuptools import setup, find_packages
import commands
import sys

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

setup(
    name = "SyncEngine",
    version = "0.1",
    packages = find_packages(),
    scripts = ['sync-engine',
               'tools/clean_partnerships.py',
               'tools/create_partnership.py',
               'tools/delete_partnership.py',
               'tools/list_partnerships.py',
               'tools/configure_bindings.py'],
    package_data = {
        '': ['*.xsl'],
    },
    data_files = [
        (plugindir+'/python-plugins', ['opensync-plugin-0.30later.py', 'opensync-plugin.py'])
    ]
)

