#!/usr/bin/env python

from distutils.core import setup, Extension

setup(name="pyrapi",
      version="0.2",
      description="syce wrapper",
      author="Richard Taylor",
      author_email="r.taylor@bcs.org.uk",
      url="http://synce.sourceforge.net/",
      packages=['pyrapi'],
      package_dir={'pyrapi':'src'},
      ext_modules=[Extension("pyrapi._pyrapi", ["src/pyrapi_wrap.c"],
                             libraries=["rapi","synce"])]
     )
