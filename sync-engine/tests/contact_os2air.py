#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import logging
sys.path.insert(0, "..")
from SyncEngine import formats
from xml.dom import minidom

logging.basicConfig(level=logging.DEBUG,
                    stream=sys.stdout,
                    format='%(asctime)s %(levelname)s %(name)s : %(message)s')

src = minidom.parseString(sys.stdin.read())
dst = formats.contact.to_airsync(src)
print dst.toprettyxml(encoding="utf-8")
