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
   
    data_files=[('synceKPM/data', [  'data/blue_22x22.png', 
                            'data/blue_48x48.png',
                            'data/folder.png',  
                            'data/green_22x22.png',
                            'data/green_48x48.png', 
                            'data/lock.svg'
                         ]),
                ('data/ui',[ 'data/ui/synce-kpm-mainwindow.ui',
                             'data/ui/synce-kpm-copycab-progresswindow.ui',
                             'data/ui/synce-kpm-create-pshipwindow.ui',
                             'data/ui/synce-kpm-installwindow.ui'
                           ]) 
               ],
)

