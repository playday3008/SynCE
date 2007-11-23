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
import formats30
from constants import *

SupportedFormats = ["NONE", "OS20", "OS30"]
DefaultFormat = "OS20"

#
# This handler array dispatches the appropriate format converter according to the 
# format specified

HandlerArray = { "NONE" : None,
                 "OS20" : formats.parser.parser.convert,
		 "OS30" : formats30.parser.parser.convert
	       }

#
# ConvertFormat
#
# EXPORTED
#
# Provides a single call point for format conversions, now that we do them from
# more than one place.

def ConvertFormat(direction, type_id, data, fmt_type):
	if fmt_type in HandlerArray.keys():
		converter = HandlerArray[fmt_type]
		if converter != None:
			return converter(data,type_id,direction)
		else:
			return data
	else:
		raise Exception("Unknown format conversion %s" % fmt_type)

