#!/usr/bin/env python
# -*- coding: iso-8859-1 -*-
#	Created:	Sun Mar 28 17:17:55 CEST 2004	by M. Biermaier	on macosbw
#	Version:	Tue Apr 20 13:35:52 CEST 2004	on macosbw
#	$Id$

"""
pyrapi-client.

Client who connects to a real pyrapi-server.
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

import socket
import select
import pickle
#--------------------------------------------------
# Imports ]


# Constants [
#--------------------------------------------------
DEBUG			= False	# 0 ... off
						# 1 ... general
						# 2 ... trace
						# 3 ... Debug-File
DEBUG_PRINT		= False

host			= "localhost"
DEFAULT_PORT	= 4712	# FIXME What port should we use?
BUFFER_SIZE		= 1024  # MaxLen: 1448 ?
SETTINGS_NAME   = ".pyrapi.cfg"

# rapi [
#--------------------------------------------------
SYNCE_LOG_LEVEL_LOWEST	= 0

SYNCE_LOG_LEVEL_ERROR	= 1
SYNCE_LOG_LEVEL_WARNING	= 2
SYNCE_LOG_LEVEL_TRACE	= 3
 
SYNCE_LOG_LEVEL_HIGHEST	= 4

FAF_ATTRIBUTES			= 0x00001
FAF_CREATION_TIME		= 0x00002
FAF_LASTACCESS_TIME		= 0x00004
FAF_LASTWRITE_TIME		= 0x00008

FAF_SIZE_HIGH			= 0x00010
FAF_SIZE_LOW			= 0x00020
FAF_OID					= 0x00040
FAF_NAME				= 0x00080
#--------------------------------------------------
# rapi ]
#--------------------------------------------------
# Constants ]


# Global VARs [
#--------------------------------------------------
testfiles		= []
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

# pickport [
#--------------------------------------------------
def pickport ():

	global port

	return port
#--------------------------------------------------
# pickport ]

# pickaddr [
#--------------------------------------------------
def pickaddr (proto):

	global host

	if proto == socket.AF_INET:
		return (host, pickport ())
	else:
		fn = TESTFN + str (pickport ())
		testfiles.append (fn)
		return fn
#--------------------------------------------------
# pickaddr ]

# receive [
#--------------------------------------------------
def receive (sock, n, timeout = 20):
	TraceBeg ()
	r, w, x = select.select ([sock], [], [], timeout)
	TraceEnd ()
	if sock in r:
		return sock.recv (n)
	else:
		raise RuntimeError, "timed out on %s" % `sock`
#--------------------------------------------------
# receive ]

# ProcessStream [
#--------------------------------------------------
def ProcessStream (cmd, par1, par2 = 0, par3 = 0):

	global proto
	global addr

	TraceBeg ()
	s	= socket.socket (proto, socket.SOCK_STREAM)
	s.connect (addr)
	cmd	= cmd + "," + par1 + "," + str (par2) + "," + str (par3) + "\n"
	debugPrint.String ("cmd: [%s]" % cmd)
	s.sendall (cmd)

	buf	= data = receive (s, BUFFER_SIZE)
	while data:
		data	= receive (s, BUFFER_SIZE)
		buf		+= data
	debugPrint.String ("buf: [%s]" % buf)
	if DEBUG:
		print buf [:-1]
	dPrint ("len (buf)", len (buf))
	s.close ()
	TraceEnd ()
	return buf
#--------------------------------------------------
# ProcessStream ]

# ReadSettings [
#--------------------------------------------------
def ReadSettings ():

	global DEBUG
	global dPrint
	global host
	global port

	TraceBeg ()

	# Read Settings [
	#--------------------------------------------------
	try:
		SettingsFile    = open (SETTINGS_NAME, "r")
		Settings		= SettingsFile.readlines ()
		SettingsFile.close ()
		# ['host=linuxorange\n', 'port=4712\n']
		dPrint ("Settings", Settings)
		for Line in Settings:
			 Line	= Line [:-1]	# Kill trailing "\n"
			 Params	= Line.split ("=")

			 if Params [0] == "host":
			 	host	= Params [1]
				dPrint ("host", host)

			 elif Params [0] == "port":
			 	port	= int (Params [1])
				dPrint ("port", port)

			 elif Params [0] == "DEBUG":
			 	try:
				 	DEBUG	= int (Params [1])
				except ValueError:
					DEBUG	= Params [1] == "True"
				dPrint ("DEBUG", DEBUG)

	except:
		print "%s Warning: Could not read settings from [%s]." % (sys.argv [0], SETTINGS_NAME)
	#--------------------------------------------------
	# Read Settings ]

	TraceEnd ()
#--------------------------------------------------
# ReadSettings ]

# InitClient [
#--------------------------------------------------
def InitClient ():

	global proto
	global addr
	global debugPrint
	global DEBUG_PRINT
	global dPrint

	global host
	global port

	TraceBeg ()

	# Initialize DebugPrint [
	#--------------------------------------------------
	debugPrint		= DebugPrint.DebugPrint ()
	debugPrint.doPrint= DEBUG_PRINT
	dPrint			= debugPrint.LabelValue	# Name-change!
	#--------------------------------------------------
	# Initialize DebugPrint ]

	ReadSettings ()

	proto	= socket.AF_INET
	addr	= pickaddr (proto)

	print "Connecting to server on %s:%s" % (host, str (port))

	TraceEnd ()
#--------------------------------------------------
# InitClient ]

# synce_log_set_level [
#--------------------------------------------------
def synce_log_set_level (level):

	TraceBeg ()

	InitClient ();

	ProcessStream ("synce_log_set_level", str (level))

	TraceEnd ()
#--------------------------------------------------
# synce_log_set_level ]

# CeFileInfo [
#--------------------------------------------------
class CeFileInfo:
	def __init__ (self, FileName, FileAttributes, FileSizeLow, LastWriteTime, CreationTime = None, AccessTime = None):
		self.cFileName			= FileName
		self.dwFileAttributes	= FileAttributes
		self.nFileSizeLow		= FileSizeLow
		self.ftLastWriteTime	= LastWriteTime
		self.ftCreationTime		= CreationTime
		self.ftLastAccessTime	= AccessTime
#--------------------------------------------------
# CeFileInfo ]

# CeFindAllFiles [
#--------------------------------------------------
def CeFindAllFiles (szPath, dwFlags):

	TraceBeg ()

	buf		= ProcessStream ("CeFindAllFiles", szPath, dwFlags)
	entries	= pickle.loads (buf)

	if DEBUG:
		print entries
	files	= []
	for cefile in entries:
		if DEBUG:
			print "cefile:", cefile
		#file	= CeFileInfo (cefile)	# FIXME How pass a tuple?
		path, \
		a, \
		b, \
		c, \
		d, \
		e, \
		f = cefile
		if DEBUG:
			print "a:", a
			print "b:", b
			print "c:", c
			print "d:", d
			print "e:", e
			print "f:", f
		file	= CeFileInfo (a, int (b), int (c), int (d), int (e), int (f))
		files.append (file)

	TraceEnd ()
	return files
#--------------------------------------------------
# CeFindAllFiles ]

# CeFindFirstFile [
#--------------------------------------------------
def CeFindFirstFile (lpFileName):
	TraceBeg ()

	buf				= ProcessStream ("CeFindFirstFile", lpFileName)
	Handle, Data	= pickle.loads (buf)
	#fileInfo		= CeFileInfo (Data)	# FIXME How pass a tuple?
	a, \
	b, \
	c, \
	d, \
	e, \
	f 				= Data
	if DEBUG:
		print "a:", a
		print "b:", b
		print "c:", c
		print "d:", d
		print "e:", e
		print "f:", f
	fileInfo			= CeFileInfo (a, int (b), int (c), int (d), int (e), int (f))

	TraceEnd ()
	return (Handle, fileInfo)
#--------------------------------------------------
# CeFindFirstFile ]

# CeDeleteFile [
#--------------------------------------------------
def CeDeleteFile (lpFileName):
	TraceBeg ()

	result		= ProcessStream ("CeDeleteFile", lpFileName)

	TraceEnd ()
	return result
#--------------------------------------------------
# CeDeleteFile ]

# CeMoveFile [
#--------------------------------------------------
def CeMoveFile (lpExistingFileName, lpNewFileName):
	TraceBeg ()

	result		= ProcessStream ("CeMoveFile", lpExistingFileName, lpNewFileName)
	if result == "ERROR":
		raise

	TraceEnd ()
#--------------------------------------------------
# CeMoveFile ]

# CeCopyFile [
#--------------------------------------------------
def CeCopyFile (lpExistingFileName, lpNewFileName, bFailIfExists):
	TraceBeg ()

	if bFailIfExists:
		Par3	= "True"
	else:
		Par3	= "False"

	result		= ProcessStream ("CeCopyFile", lpExistingFileName, lpNewFileName, Par3)
	if result == "ERROR":
		raise

	TraceEnd ()
#--------------------------------------------------
# CeCopyFile ]

# CeCopyFileA [
#--------------------------------------------------
def CeCopyFileA (lpExistingFileName, lpNewFileName, bFailIfExists):
	TraceBeg ()

	if bFailIfExists:
		Par3	= "True"
	else:
		Par3	= "False"

	result		= ProcessStream ("CeCopyFileA", lpExistingFileName, lpNewFileName, Par3)
	if result == "ERROR":
		raise

	TraceEnd ()
#--------------------------------------------------
# CeCopyFileA ]

# CeGetFileAttributes [
#--------------------------------------------------
def CeGetFileAttributes (lpFileName):
	TraceBeg ()

	result		= ProcessStream ("CeGetFileAttributes", lpFileName)

	TraceEnd ()
	return result
#--------------------------------------------------
# CeGetFileAttributes ]

# CeSetFileAttributes [
#--------------------------------------------------
def CeSetFileAttributes (lpFileName, dwFileAttributes):
	TraceBeg ()

	result		= ProcessStream ("CeSetFileAttributes", lpFileName, dwFileAttributes)

	TraceEnd ()
	return result
#--------------------------------------------------
# CeSetFileAttributes ]

# CeCreateDirectory [
#--------------------------------------------------
#def CeCreateDirectory (lpPathName, lpSecurityAttributes):
def CeCreateDirectory (lpPathName):
	TraceBeg ()

	# FIXME CHECK lpSecurityAttributes are (not) needed?
	#result		= ProcessStream ("CeCreateDirectory", lpPathName, lpSecurityAttributes)
	result		= ProcessStream ("CeCreateDirectory", lpPathName)
	if result == "ERROR":
		raise

	TraceEnd ()
	return result
#--------------------------------------------------
# CeCreateDirectory ]

# CeRemoveDirectory [
#--------------------------------------------------
def CeRemoveDirectory (lpPathName):
	TraceBeg ()

	result		= ProcessStream ("CeRemoveDirectory", lpPathName)
	if result == "ERROR":
		raise

	TraceEnd ()
	return result
#--------------------------------------------------
# CeRemoveDirectory ]

# main [
#--------------------------------------------------
def main (theArgs):

	global DEBUG
	global DEBUG_PRINT
	global dPrint
	global host
	global port

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
    """usage: %s [-d n] [-D] [-h host] [-p port] [-V]
    -d n ...... debug-level 
                0 = off
                1 = general
                2 = trace
                3 = Debug-File
    -D ........ DebugPrint
    -h host ... host
    -p port ... port
    -V ........ print version and exit"""

	# Define Defaults [
	#--------------------------------------------------
	host	= "localhost"
	port	= DEFAULT_PORT
	#--------------------------------------------------
	# Define Defaults ]

	ReadSettings ()

	# Process Options [
	#--------------------------------------------------
	try:
		opts, args	= getopt.getopt (theArgs [1:], 'd:Dh:p:V')
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
		if o in ("-h", "--host"):
			host	= a
		if o in ("-p", "--port"):
			try:
				port	= int (a)
			except ValueError:
				print "port needs and integer"
				print Usage % sys.argv [0]
				sys.exit (1)
		if o in ("-V", "--version"):
			print sys.argv [0] \
				+ " " + __version__.replace ("$", "") \
				+ " " + __date__.replace ("$", "")
			sys.exit (0)
	#--------------------------------------------------
	# Process Options ]

	# Write Settings [
	#--------------------------------------------------
	try:
		SettingsFile    = open (SETTINGS_NAME, "w")
		Settings		= 	[
							"host=", host, "\n", 
							"port=", str (port), "\n",
							"DEBUG=", str (DEBUG), "\n",
							]
		SettingsFile.writelines (Settings)
		SettingsFile.close ()
	except:
		print "%s Warning: Could not save settings in [%s]." % (sys.argv [0], SETTINGS_NAME)
	#--------------------------------------------------
	# Write Settings ]
	TraceEnd ()
#--------------------------------------------------
# main ]

if __name__ == "__main__": main (sys.argv)

# vim: ts=4 sw=4
