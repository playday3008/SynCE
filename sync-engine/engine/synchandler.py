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

from xml.dom import minidom

from threading import Thread
import logging
import time

class SyncHandler(Thread):

    def __init__(self, engine, auto_sync):
        Thread.__init__(self)
        self.logger = logging.getLogger("engine.synchandler.SyncHandler")

        self.engine = engine
        self.auto_sync = auto_sync

        self.sync_done = False
        self.stopped = False

    def stop(self):
        self.stopped = True

    def run(self):
        # Temporarily uninstall the previous handler for the beginning of the
        # synchronization.  The previous handler was for the auto-syncs, which
        # we disable temporarily while we are syncing here
        self.engine.airsync.handler_block(self.engine.sync_begin_handler_id)

        # Set up our own handler so we can catch the end of the Airsync phase
        self.sync_end_handler_id = self.engine.airsync.connect("sync-end", self._sync_end_cb)

        if not self.auto_sync:
            # If the sync wasn't automatically started, we must manually request it
            doc = minidom.Document()
            doc_node = doc.createElement("sync")
            doc_node.setAttribute("xmlns", "http://schemas.microsoft.com/as/2004/core")
            doc_node.setAttribute("type", "Interactive")
            doc.appendChild(doc_node)

            node = doc.createElement("partner")
            node.setAttribute("id", self.engine.partnerships.get_current().guid)
            doc_node.appendChild(node)

            self.logger.debug("run: sending request to device \n%s", doc_node.toprettyxml())
            self.engine.rapi_session.sync_start(doc_node.toxml())

        self.logger.debug("run: performing synchronization")

        while not self.sync_done and not self.stopped:
            time.sleep(1)

        if self.stopped:
            self.logger.warning("run: Synchronization stopped prematurely!")

        self.logger.debug("run: calling RAPI sync_pause and sync_resume")
        self.engine.rapi_session.sync_pause()
        self.engine.rapi_session.sync_resume()

        if not self.stopped:
            self.logger.debug("run: saving partnership state")
            self.engine.partnerships.get_current().save_state()

        self.logger.info("run: finished synchronization")
        self.engine.syncing.unlock()

        self.engine.airsync.handler_unblock(self.engine.sync_begin_handler_id)

        if not self.stopped:
            self.engine.Synchronized()

    def _sync_end_cb(self, res):
        self.logger.info("_sync_end_cb: Called")
        self.sync_done = True
        self.engine.airsync.disconnect(self.sync_end_handler_id)
