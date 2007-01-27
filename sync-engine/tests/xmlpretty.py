#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
from xml.dom import minidom

src = minidom.parseString(sys.stdin.read())
print src.toprettyxml(encoding="utf-8")
