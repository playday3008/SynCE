#!/usr/bin/env python
# -*- coding: iso-8859-1 -*-
#	Created:	Mit Jun 16 10:34:08 CEST 2004	by M. Biermaier	on linuxorange
#	Version:	Mit Jun 16 13:55:00 CEST 2004	on linuxorange
#	$Id$
 
"""
Determine PyQt.

Find QT, or Qtopia.
"""

__author__		= "M. Biermaier"
__version__		= "$Revision$"
__date__		= "$Date$"


# TODO: Check for correctness. 
# Not every Python has True / False
try:
	x				= False
except:
	False			= 0
	True			= 1

# Imports [
#--------------------------------------------------
import sys

# We have a problem on MacOS X...
if sys.platform == "darwin":
	import qtui

try:
	HaveQT			= True
	from qt import *
except ImportError:
	HaveQT			= False

try:
	# M.B. 2003-01-15
	# We set Qtopia before the "import", because "import"
	# may raise other errors (e.g. missing libraries).
	Qtopia     		= True
	from qtpe import QPEApplication
except ImportError:
	Qtopia     		= False
#--------------------------------------------------
# Imports ]


# Constants [
#--------------------------------------------------
LabelWidth		= 23
#--------------------------------------------------
# Constants ]


# main [
#--------------------------------------------------
def main (theArgs):

	# FIXME Not every Python has True / False
	SHELL_READABLE	= False

	# PrintResult [
	#--------------------------------------------------
	def PrintResult (aLabel, aValue, aBool):

		if SHELL_READABLE:
			print aLabel + "=" + str (aValue)
		else:
			print "%s: %*s" % (aLabel, LabelWidth - len (aLabel), " "),
			if aBool:
				if not aValue: print "not",
				print "found"
			else:
				print aValue
	#--------------------------------------------------
	# PrintResult ]

	if len (theArgs) > 1:
		SHELL_READABLE	= theArgs [1] == "-r"

	PrintResult ("Platform", sys.platform, False)
	PrintResult ("QT", HaveQT, True)
	PrintResult ("Qtopia", Qtopia, True)
#--------------------------------------------------
# main ]

if __name__ == "__main__": main (sys.argv)

# vim: ts=4 sw=4
