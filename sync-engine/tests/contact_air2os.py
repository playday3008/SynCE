#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
sys.path.insert(0, "..")
from engine import formats
from xml.dom import minidom

src = minidom.parseString(sys.stdin.read())
dst = formats.contact.from_airsync("uid", src.documentElement)
print dst.toprettyxml(encoding="utf-8")
