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
   
    data_files=[('synceKPM/data', [  'synceKPM/data/blue_22x22.png', 
                            'synceKPM/data/blue_48x48.png',
                            'synceKPM/data/folder.png',  
                            'synceKPM/data/green_22x22.png',
                            'synceKPM/data/green_48x48.png', 
                            'synceKPM/data/lock.svg'
                         ]),
               ],
)

