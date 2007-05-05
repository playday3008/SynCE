# -*- coding: utf-8 -*-
############################################################################
#    Dr J A Gow 24/03/2007                                                 #
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

import formats
from constants import *

FormatHandlers = { DIR_TO_AIRSYNC   : { SYNC_ITEM_TASKS    : formats.task.to_airsync,
                                        SYNC_ITEM_CONTACTS : formats.contact.to_airsync,
                                        SYNC_ITEM_CALENDAR : formats.event.to_airsync
                                      },
                   DIR_FROM_AIRSYNC : { SYNC_ITEM_TASKS    : formats.task.from_airsync,
                                        SYNC_ITEM_CONTACTS : formats.contact.from_airsync,
                                        SYNC_ITEM_CALENDAR : formats.event.from_airsync
                                      }
                 }

#
# ConvertFormat
#
# EXPORTED
#
# Provides a single call point for format conversions, now that we do them from
# more than one place.

def ConvertFormat(direction, type_id, data):
    if FormatHandlers[direction].has_key(type_id):
        return FormatHandlers[direction][type_id](data)
    else:
	raise Exception("Unable to convert data of item type %d" % type_id)
