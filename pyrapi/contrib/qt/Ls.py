#!/usr/bin/env python
# -*- coding: iso-8859-1 -*-
#	Created:	Fre Mär 26 09:38:48 CET 2004	by M. Biermaier	on linuxorange
#	Version:	Tue Apr 20 15:59:49 CEST 2004	on macosbw
#	$Id$

"""
SynCE ls.

Show files on PocketPC.
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

import time
import getopt
from pyrapi import pyrapi
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

# rListDir [
#--------------------------------------------------
def rListDir (path, recurse = False, long = False):

	# FileDates [
	#--------------------------------------------------
	def FileDates ():
		TraceBeg ()

		if fileobj.dwFileAttributes == 16:
			theType	= "DIR "
		else:
			theType	= "    "
		file_list.append ("%s%-40s %7d  %s  %s  %s %10d" % (
							theType,
							fileobj.cFileName.decode ('utf-8'),
							fileobj.nFileSizeLow,
							time.ctime (fileobj.ftCreationTime),
							time.ctime (fileobj.ftLastAccessTime),
							time.ctime (fileobj.ftLastWriteTime),
							fileobj.dwFileAttributes
							))

		TraceEnd ()
	#--------------------------------------------------
	# FileDates ]

	# FilePath [
	#--------------------------------------------------
	def FilePath ():
		TraceBeg ()

		file_list.append ("%s\\%s" % (
							path.decode ('utf-8'),
							fileobj.cFileName.decode ('utf-8')
							))

		TraceEnd ()
	#--------------------------------------------------
	# FilePath ]

	TraceBeg ()

	file_list	= []
	dir_list	= []		# Process directories delayed in long format

	for fileobj in pyrapi.CeFindAllFiles (
							path + r"\*.*",
							pyrapi.FAF_NAME | \
							pyrapi.FAF_ATTRIBUTES | \
							pyrapi.FAF_CREATION_TIME | \
							pyrapi.FAF_LASTACCESS_TIME | \
							pyrapi.FAF_LASTWRITE_TIME | \
							pyrapi.FAF_SIZE_LOW \
							):

		if long:
			FileDates ()
		else:
			FilePath ()

		if fileobj.dwFileAttributes == 16 and recurse:
			dirName	= path + '\\' + fileobj.cFileName
			if long:
				# Collect directories
				dir_list.append (dirName)
			else:
				# Process directory
				new_list = rListDir (dirName, recurse, long)
				if (new_list != None):
					file_list	= file_list + new_list

	if long:
		for dir in dir_list:
			file_list.append ("\n%s:" % dir.decode ('utf-8'))
			newdir_list = rListDir (dir, recurse, long)
			if (newdir_list != None):
				file_list	= file_list + newdir_list

	return file_list

	TraceEnd ()
#--------------------------------------------------
# rListDir ]

# main [
#--------------------------------------------------
def main (theArgs):

	TraceBeg ()

	# Initialize DebugPrint [
	#--------------------------------------------------
	debugPrint		= DebugPrint.DebugPrint ()
	debugPrint.doPrint= DEBUG_PRINT
	dPrint			= debugPrint.LabelValue	# Name-change!
	#--------------------------------------------------
	# Initialize DebugPrint ]

	# Test
	#dPrint ("sys.argv", sys.argv)
	#debugPrint.String ("---")

	# Ls [
	#--------------------------------------------------
	opts, args	= getopt.getopt (theArgs [1:], 'rls')

	if DEBUG:
		print opts, args

	recurse	= False
	long	= False
	string	= False

	pyrapi.synce_log_set_level (pyrapi.SYNCE_LOG_LEVEL_LOWEST)

	for o, a in opts:
		if o in ("-r", "--recurse"):
			recurse	= 1
		if o in ("-l", "--long"):
			long	= 1
		if o in ("-s", "--string"):
			string	= 1

	if len (args) > 0:
		path	= args[0]
	else:
		path	= ""

	ResultString	= ""
	for f in rListDir (path, recurse, long):
		if not string:
			print f.encode ('latin1')
		else:
			ResultString	+= f + "\n"
	#--------------------------------------------------
	# Ls ]

	TraceEnd ()
	return ResultString
#--------------------------------------------------
# main ]

if __name__ == "__main__": main (sys.argv)

# vim: ts=4 sw=4
