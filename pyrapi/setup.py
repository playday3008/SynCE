#!/usr/bin/env python

from distutils.core import setup, Extension

setup(name="pyrapi",
      version="0.1",
      description="librapi2 wrapper",
      author="Richard Taylor",
      author_email="r.taylor@bcs.org.uk",
      url="http://synce.sourceforge.net/",
      packages=['pyrapi'],
      package_dir={'pyrapi':'src'},
      ext_modules=[Extension("pyrapi.pyrapi", ["src/pyrapi.c"],
                             libraries=["rapi"])]
     )
