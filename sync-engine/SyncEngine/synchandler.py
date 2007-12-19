# -*- coding: utf-8 -*-
############################################################################
#    Copyright (C) 2006  Ole André Vadla Ravnås <oleavr@gmail.com>       #
#                                                                          #
#    This program is free software; you can redistribute it and#or modify  #
#    it under the terms of the GNU General Public License as published by  #
#    the Free Software Foundation; either version 2 of the License, or     #
#    (at your option) any later version.                                   #
#                                                                          #
#    This program is distributed in the hope that it will be useful,       #
#    but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#    GNU General Public License for more details.                          #
#                                                                          #
#    You should have received a copy of the GNU General Public License     #
#    along with this program; if not, write to the                         #
#    Free Software Foundation, Inc.,                                       #
#    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
############################################################################

import libxml2
import threading
import logging
import time

class SyncHandler(threading.Thread):

    def __init__(self, engine, auto_sync):
        threading.Thread.__init__(self)
        self.logger = logging.getLogger("engine.synchandler.SyncHandler")

        self.engine = engine
        self.auto_sync = auto_sync

        self.stopped = False
	self.evtSyncRunFinished = threading.Event()

    def stop(self):
        self.stopped = True
	self.evtSyncRunFinished.set()

    def run(self):
        
	# Clear the end-of-sync event
	
	self.evtSyncRunFinished.clear()
	
	# Temporarily uninstall the previous handler for the beginning of the
        # synchronization.  The previous handler was for the auto-syncs, which
        # we disable temporarily while we are syncing here
	
        self.engine.airsync.handler_block(self.engine.sync_begin_handler_id)

        # Set up our own handler so we can catch the end of the Airsync phase
        
	self.sync_end_handler_id = self.engine.airsync.connect("sync-end", self._sync_end_cb)

        if not self.auto_sync:
		
            # If the sync wasn't automatically started, we must manually request it
	    
	    doc = libxml2.newDoc("1.0")
	    doc_node=doc.newChild(None,"sync",None)
	    doc_node.setProp("xmlns", "http://schemas.microsoft.com/as/2004/core")
	    doc_node.setProp("type", "Interactive")
 
 	    partnernode = doc_node.newChild(None,"partner",None)
	    partnernode.setProp("id",self.engine.PshipManager.GetCurrentPartnership().info.guid)

            self.logger.debug("run: sending request to device \n%s", doc_node.serialize("utf-8",1))
            self.engine.rapi_session.sync_start(doc_node.serialize("utf-8",0))

        self.logger.debug("run: performing synchronization")

	self.evtSyncRunFinished.wait()

        if self.stopped:
            self.logger.warning("run: Synchronization stopped prematurely!")

        self.logger.debug("run: calling RAPI sync_pause and sync_resume")
        self.engine.rapi_session.sync_pause()
        self.engine.rapi_session.sync_resume()

        if not self.stopped:
            self.logger.debug("run: saving itemDB")
	    self.engine.PshipManager.GetCurrentPartnership().SaveItemDB()
	    
        self.logger.info("run: finished synchronization")
        self.engine.syncing.unlock()

        self.engine.airsync.handler_unblock(self.engine.sync_begin_handler_id)

        if not self.stopped:
            self.engine.Synchronized()

    def _sync_end_cb(self, res):
        self.logger.info("_sync_end_cb: Called")
	self.evtSyncRunFinished.set()
        self.engine.airsync.disconnect(self.sync_end_handler_id)
