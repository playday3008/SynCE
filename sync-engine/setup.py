#!/usr/bin/env python

from setuptools import setup, find_packages
from synce_install_plugins import get_plugindir, get_opensync_version

if get_opensync_version() >= 0.30:
    pluginfile = ['opensync-plugin-0.30later.py']
    otherplugin = ['opensync-plugin.py']
else:
    pluginfile = ['opensync-plugin.py']
    otherplugin = ['opensync-plugin-0.30later.py']

setup(
    name = "SyncEngine",
    version = "0.1",
    packages = find_packages(),
    scripts = ['sync-engine',
               'tools/clean_partnerships.py',
               'tools/create_partnership.py',
               'tools/delete_partnership.py',
               'tools/list_partnerships.py',
               'tools/configure_bindings.py',
               'synce_install_plugins.py'],
    package_data = {
        '': ['*.xsl'],
    },
    data_files = [
        (get_plugindir()+'/python-plugins', pluginfile),
        ('/usr/share/sync-engine', otherplugin)
    ]
)

