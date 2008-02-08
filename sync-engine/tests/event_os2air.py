#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import logging
sys.path.insert(0, "..")
from SyncEngine import constants
from SyncEngine import formatapi
import libxml2

logging.basicConfig(level=logging.DEBUG,
                    stream=sys.stdout,
                    format='%(asctime)s %(levelname)s %(name)s : %(message)s')

src = libxml2.parseDoc(sys.stdin.read())

as_doc=formatapi.ConvertFormat(constants.DIR_TO_AIRSYNC,
			       constants.SYNC_ITEM_CALENDAR,
			       src,
			       "OS20")
print as_doc.serialize("utf-8",1)
