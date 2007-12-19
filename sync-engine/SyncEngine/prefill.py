# -*- coding: utf-8 -*-
############################################################################
# prefill.py
#
# Asynchronous prefill mechanism to prevent d-bus timeouts on prefill
#
# Copyright (C) 2007 Dr J A Gow 
#
############################################################################

import pyrra
import threading
from constants import *
import pshipmgr
import logging


#
# PrefillThread
#
# A one-shot thread to run a prefill.

class PrefillThread(threading.Thread):
	
	def __init__(self,theEngine,theTypes):
		threading.Thread.__init__(self)
		self.engine = theEngine
		self.logger = logging.getLogger("engine.prefill.PrefillThread")
		self.types  = theTypes

	#
	# Perform one prefill and exit
	#
		
	def run(self):
		
		self.logger.info("prefill thread entered")
		ps = self.engine.PshipManager.GetCurrentPartnership()
		for a in self.types:
			if SYNC_ITEM_CLASS_TO_ID.has_key(a):
				tid = SYNC_ITEM_CLASS_TO_ID[a]
				if ps.deviceitemdbs.has_key(tid):
					self.logger.info("prefill: prefilling for item type %d" % tid)
					ps.deviceitemdbs[tid].PrefillRemoteChangeDB(self.engine.config)

		self.logger.info("prefill: prefill complete")
		self.engine.syncing.unlock()
		self.engine.PrefillComplete()
