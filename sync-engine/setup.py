#!/usr/bin/env python

from setuptools import setup, find_packages

setup(
    name = "SyncEngine",
    version = "0.1",
    packages = find_packages(),
    scripts = ['sync-engine',
               'tools/clean_partnerships.py',
               'tools/create_partnership.py',
               'tools/delete_partnership.py',
               'tools/list_partnerships.py',
               'tools/select_partnership.py'],
    package_data = {
        '': ['*.xsl'],
    },
)

