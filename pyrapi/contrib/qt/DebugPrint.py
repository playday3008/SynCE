#!/usr/bin/env python
# -*- coding: iso-8859-1 -*-
#	Created:	Fre Mär  5 10:18:38 CET 2004	by M. Biermaier	on linuxorange
#	Version:	Mon Mär  8 11:32:35 CET 2004	on linuxorange
#	$Id$

"""
DebugPrint.

Perform methodes for debug printing.
"""

__author__		= "M. Biermaier"
__version__		= "$Revision$"
__date__		= "$Date$"

#--------------------------------------------------
# DebugPrint [
#--------------------------------------------------
class DebugPrint:

	doPrint			= False
	LabelWidth		= 25
	NO_NL			= True

	# LabelValue [
	#--------------------------------------------------
	def LabelValue (self, aLabel, aValue):
		if self.doPrint:
			print "%s: %*s%s" % (aLabel, self.LabelWidth - len (aLabel), " ", aValue)
	#--------------------------------------------------
	# LabelValue ]

	# String [
	#--------------------------------------------------
	def String (self, aString, noNL = False):
		if self.doPrint:
			if noNL:
				print aString,
			else:
				print aString
	#--------------------------------------------------
	# String ]
#--------------------------------------------------
# DebugPrint ]

# vim: ts=4 sw=4
