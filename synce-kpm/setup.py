#!/usr/bin/env python

from setuptools import setup, find_packages

setup(
    author = 'Guido Diepen',
    author_email='guido@guidodiepen.nl',
    description='SynCE KDE PDA Manager',
    url='http://www.guidodiepen.nl',
    name = 'synce-kpm',
    version = '0.11',
    packages = find_packages(),
    scripts = ['synce-kpm'],

    include_package_data=True, 
    package_data = { '' : ['synceKPM/data/*.png'] },
)

