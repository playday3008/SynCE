#!/usr/bin/env python
# -*- coding: iso-8859-1 -*-
#	Created:	Fri Apr  2 20:12:10 CEST 2004	by M. Biermaier	on macosbw
#	Version:	Mon Apr  5 12:23:13 CEST 2004	on macosbw
#	$Id$

"""
Part of the SynCE / pyrapi add-ons.

Fake for a pyrapi module.
"""

__author__		= "M. Biermaier"
__version__		= "$Revision$"
__date__		= "$Date$"


# Imports [
#--------------------------------------------------
import sys
import os

theHOME			= os.getenv ("HOME")
sys.path.append (theHOME + "/Python")
import DebugPrint	# DebugPrinting

import pyrapi
import pickle
import base64
#--------------------------------------------------
# Imports ]


# Constants [
#--------------------------------------------------
DEBUG			= False
DEBUG_PRINT		= True
#--------------------------------------------------
# Constants ]


# Global VARs [
#--------------------------------------------------
#--------------------------------------------------
# Global VARs ]


# Trace2 [
#--------------------------------------------------
def TraceBeg ():
	if DEBUG > 1: print ">>>>>", sys._getframe(1).f_code.co_name

def TraceEnd ():
	if DEBUG > 1: print "<<<<<", sys._getframe(1).f_code.co_name
#--------------------------------------------------
# Trace2 ]

# pyrapi-fake [
#--------------------------------------------------

# Tuple2ObjectSlot [
#--------------------------------------------------
def Tuple2ObjectSlot (aTuple):
	TraceBeg ()

	anObject, aSlotIndex	= aTuple				# We don't need 'aSlotIndex' here...
	anObject.slotIndex		= aSlotIndex			# ...but we have to save it anywhere

	TraceEnd ()
	return anObject
#--------------------------------------------------
# Tuple2ObjectSlot ]

# openCeFile [
#--------------------------------------------------
def openCeFile (file_name, mode):

	TraceBeg ()

	pyrapi.synce_log_set_level (pyrapi.SYNCE_LOG_LEVEL_LOWEST)		# Init client

	result	= pyrapi.ProcessStream (cmd = "openCeFile", par1 = file_name, par2 = mode)

	#print result

	theData	= pickle.loads (result)
	file	= Tuple2ObjectSlot (theData)

	TraceEnd ()

	return file
#--------------------------------------------------
# openCeFile ]

# CeFile [
#--------------------------------------------------
class CeFile:
	def __init__ (self, file_handle, mode):
		 self._file_handle	= file_handle
		 self._mode			= mode
		 self.slotIndex		= -1					# This is the file-reference on the server

	# read [
	#--------------------------------------------------
	def read (self):
		TraceBeg ()

		result	= pyrapi.ProcessStream (cmd = "CeFile.read", par1 = str (self.slotIndex))

		buf 	= pickle.loads (result)
		#print "buf: [%s]" % buf

		TraceEnd ()
		return buf
	#--------------------------------------------------
	# read ]

	# write [
	#--------------------------------------------------
	def write (self, buffer):
		TraceBeg ()

		buf		= base64.encodestring (buffer).replace ("\n", "\r") # "readline" on the receiver-side
																	# doesn't work if "\n" is contained in
																	# the string!
		result	= pyrapi.ProcessStream (cmd = "CeFile.write", par1 = str (self.slotIndex), par2 = buf)

		TraceEnd ()
		return result
	#--------------------------------------------------
	# write ]

	# close [
	#--------------------------------------------------
	def close (self):
		TraceBeg ()

		pyrapi.ProcessStream (cmd = "CeFile.close", par1 = str (self.slotIndex))

		TraceEnd ()
	#--------------------------------------------------
	# close ]
#--------------------------------------------------
# CeFile ]

#--------------------------------------------------
# pyrapi-fake ]

# main [
#--------------------------------------------------
def main (theArgs):

	global DEBUG
	global DEBUG_PRINT

	import getopt

	TraceBeg ()

	# Initialize DebugPrint [
	#--------------------------------------------------
	debugPrint		= DebugPrint.DebugPrint ()
	debugPrint.doPrint= DEBUG_PRINT
	dPrint			= debugPrint.LabelValue	# Name-change!
	#--------------------------------------------------
	# Initialize DebugPrint ]

	# Test
	dPrint ("sys.argv", sys.argv)
	debugPrint.String ("---")

	Usage		= \
    """usage: %s [-d n] [-D] [-V]
    -d n ...... debug-level 
                0 = off
                1 = general
                2 = trace
                3 = Debug-File
    -D ........ DebugPrint
    -V ........ print version and exit"""

	# Process Options [
	#--------------------------------------------------
	try:
		opts, args	= getopt.getopt (theArgs [1:], 'd:DV')
	except getopt.GetoptError:
		print Usage % sys.argv [0]
		sys.exit (1)

	if DEBUG:
		print opts, args

	DEBUG_PRINT	= False

	for o, a in opts:
		if o in ("-d", "--debugLevel"):
			try:
				DEBUG	= int (a)
			except ValueError:
				print "debugLevel needs and integer"
				print Usage % sys.argv [0]
				sys.exit (1)
		if o in ("-D", "--debugPrint"):
			DEBUG_PRINT	= True
		if o in ("-V", "--version"):
			print sys.argv [0] \
				+ " " + __version__.replace ("$", "") \
				+ __date__.replace ("$", "")
			sys.exit (0)
	#--------------------------------------------------
	# Process Options ]

	TraceEnd ()
#--------------------------------------------------
# main ]

if __name__ == "__main__": main (sys.argv)

# vim: ts=4 sw=4
