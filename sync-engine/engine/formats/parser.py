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
import libxslt

import os

from engine.constants import *
from engine.xmlutil import *
import conversions
import logging

FMT_TO_AIRSYNC   = 1
FMT_FROM_AIRSYNC = 2

class Parser:

    FORMATS_PATH = os.path.dirname(os.path.abspath(__file__))

    XSL_CONTACT_TO_AIRSYNC   = FORMATS_PATH + "/contact-to-airsync.xsl"
    XSL_CONTACT_FROM_AIRSYNC = FORMATS_PATH + "/contact-from-airsync.xsl"
    XSL_EVENT_TO_AIRSYNC     = FORMATS_PATH + "/event-to-airsync.xsl"
    XSL_EVENT_FROM_AIRSYNC   = FORMATS_PATH + "/event-from-airsync.xsl"
    XSL_TASK_TO_AIRSYNC      = FORMATS_PATH + "/task-to-airsync.xsl"
    XSL_TASK_FROM_AIRSYNC    = FORMATS_PATH + "/task-from-airsync.xsl"

    STYLESHEETS  = { SYNC_ITEM_CONTACTS : { FMT_TO_AIRSYNC   : libxslt.parseStylesheetDoc(libxml2.parseFile(XSL_CONTACT_TO_AIRSYNC)),
                                            FMT_FROM_AIRSYNC : libxslt.parseStylesheetDoc(libxml2.parseFile(XSL_CONTACT_FROM_AIRSYNC)) },
                     SYNC_ITEM_CALENDAR : { FMT_TO_AIRSYNC   : libxslt.parseStylesheetDoc(libxml2.parseFile(XSL_EVENT_TO_AIRSYNC)),
                                            FMT_FROM_AIRSYNC : libxslt.parseStylesheetDoc(libxml2.parseFile(XSL_EVENT_FROM_AIRSYNC)) },
                     SYNC_ITEM_TASKS    : { FMT_TO_AIRSYNC   : libxslt.parseStylesheetDoc(libxml2.parseFile(XSL_TASK_TO_AIRSYNC)),
                                            FMT_FROM_AIRSYNC : libxslt.parseStylesheetDoc(libxml2.parseFile(XSL_TASK_FROM_AIRSYNC)) },
                   }

    def __init__(self):
        self.logger = logging.getLogger("engine.formats.parser.Parser")
        conversions.register_xslt_extension_functions()

    def convert(self, src_doc, item_type, format):
        if not self.STYLESHEETS.has_key(item_type):
            raise ValueError("Unsupported item type %d" % item_type)

        if not self.STYLESHEETS[item_type].has_key(format):
            raise ValueError("Unsupported format %d" % format)

        stylesheet_doc = self.STYLESHEETS[item_type][format]
        return stylesheet_doc.applyStylesheet(src_doc, None)

parser = Parser()
