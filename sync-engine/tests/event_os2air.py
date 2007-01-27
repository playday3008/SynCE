#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
sys.path.insert(0, "..")
from engine import formats
from xml.dom import minidom

src = minidom.parseString(sys.stdin.read())
dst = formats.event.to_airsync(src)
print dst.toprettyxml(encoding="utf-8")
