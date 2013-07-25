#!/usr/bin/env python

from setuptools import setup, find_packages

setup(
    author = 'Guido Diepen',
    author_email='guido@guidodiepen.nl',
    description='SynCE KDE PDA Manager',
    url='http://www.guidodiepen.nl',
    name = 'synce-kpm',
    version = '0.16',
    packages = find_packages(),
    scripts = ['synce-kpm'],

    package_data = { 'synceKPM' : ['data/*.png',
                                   'data/*.svg',
								   'data/anim/*.svg'] },
)

