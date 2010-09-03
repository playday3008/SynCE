#!/usr/bin/env python

from setuptools import setup, find_packages

setup(
    name = "sync-engine",
    version = "0.15.1",
    packages = find_packages(),
    scripts = ['sync-engine',
               'tools/clean_partnerships.py',
               'tools/create_partnership.py',
               'tools/delete_partnership.py',
               'tools/list_partnerships.py',
               'tools/configure_bindings.py',
               'synce-install-plugins.py'],
    package_data = {
        '': ['*.xsl'],
    },
    data_files=[('share/doc/sync-engine', ['config/syncengine.conf.xml', 'config/org.synce.SyncEngine.service'])]
)

