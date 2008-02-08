#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import logging
sys.path.insert(0, "..")
from SyncEngine import formatapi
from SyncEngine import constants
import libxml2

logging.basicConfig(level=logging.DEBUG,
                    stream=sys.stdout,
                    format='%(asctime)s %(levelname)s %(name)s : %(message)s')

src = libxml2.parseDoc(sys.stdin.read())
dst = formatapi.ConvertFormat(  constants.DIR_FROM_AIRSYNC,
		                constants.SYNC_ITEM_CALENDAR,
				src,
				"OS20")
print dst.serialize("utf-8",1)
