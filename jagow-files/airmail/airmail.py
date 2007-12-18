#!/usr/bin/env python
# -*- coding: utf-8 -*-

############################################################################

version = (0, 1, 0)

import gobject
import os
import signal
import sys
import threading
import logging

import AirMail

logger = None
mainloop = None

def term_handler(signum, frame):
    logger.info("Received termination signal. Exiting")
    gobject.idle_add(mainloop.quit)
    server.stop()

if __name__ == "__main__":
	
    logging.basicConfig(level=logging.DEBUG,
                        stream=sys.stdout,
                        format='%(asctime)s %(levelname)s %(name)s : %(message)s')

    logger = logging.getLogger("AirMail")

    logger.debug("starting AirMail servers")
    server = AirMail.server.AirmailThread()

    logger.debug("installing termination handlers")
    signal.signal(signal.SIGTERM, term_handler)
    signal.signal(signal.SIGINT, term_handler)

    logger.debug("running main loop")

    gobject.threads_init()
    mainloop = gobject.MainLoop()
 
    server.start()
 
    try:
        mainloop.run()
    
    except KeyboardInterrupt:
        pass

    logger.debug("waiting for server to shut down.")
