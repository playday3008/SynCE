#!/usr/bin/env python
# -*- coding: iso-8859-1 -*-
#	Created:	Sam Mär 27 18:58:28 CET 2004	by M. Biermaier	on linuxorange
#	Version:	Fre Apr 30 10:48:35 CEST 2004	on linuxorange
#	$Id$

"""
GUI-Ls Socket-Server and Test-Client.

Allow to forward file-system-info from a PocketPC to a remote client.
Only the server needs "SynCE" and "pyrapi" installed - and of course
the PDA connected.
The client needs only Python - and of course a connetion to the server ;-)

Instructions:
-------------
Start the server. E.g.:
	mbier@linuxorange:~/Python/SynCE > CE_DirViewServer.py -h linuxorange

Configure the client:
	On the client side use "pyrapi.py" (!!!). This is not the
	"pyrapi.py" of the server side. It's a client-side replacement and
	talks via sockets to the server.
	If you call it as a main-program (in the sense of python) you can
	define settings that should be remembered in a file '.pyrapi.cfg'
	and used the next time the client starts.

	To define the host. E.g.:
	markus@macosbw:~/Python/SynCE > pyrapi/pyrapi.py -h linuxorange

Start the client. E.g.:
	markus@macosbw:~/Python/SynCE > pythonw CE_DirView.py

Test-Client:
	If this script is run in client-mode (option [-c]) you have to type
	command strings by hand. E.g.:
	markus@macosbw:~/Backup/Python/SynCE > CE_DirViewServer.py -c
	Connecting to server on linuxorange.mboffice:4712
	LsCmd: > CeFindAllFiles,temp,0
	LsCmd: > kill
	LsCmd: > ^d

	Tip: For a list of avaiable commands type "help".

Have fun!

Based on:
---------
pyrapi - A python wrapper class around the librapi2 library
	by Richard Taylor <r.taylor@bcs.org.uk>

SyncCE - The purpose of the SynCE project is to provide a means of
	communication with a Windows CE device from a computer running
	Linux, *BSD or other unices.
	by David Eriksson <twogood@users.sourceforge.net>
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
import DebugPrint		# DebugPrinting

from SocketServer import *
import socket
import threading
import select
import time
import getopt
#--------------------------------------------------
# Imports ]


# Constants [
#--------------------------------------------------
DEBUG			= False	# 0 ... off
						# 1 ... general
						# 2 ... trace
						# 3 ... Debug-File
DEBUG_PRINT		= False
verbose			= True
BUFFER_SIZE		= 1024	# MaxLen: 1448 ?
DEFAULT_PORT	= 4712	# FIXME What port should we use?
SETTINGS_NAME	= ".CE_DirViewServer.cfg"

# Files
MAX_FILES		= 10
FileList		= []	# Private list of open FileObjects
USE_BUFFER_OBJ	= False	# Use Python Buffer Objects for file-write
MAX_LOG_LINE_LEN= 80
SCREEN_WIDTH	= 80	# For "help"
ABORT_ON_ERR	= False	# How should "handle_error" react?
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
	if DEBUG > 1: print ">>>>>", sys._getframe (1).f_code.co_name

def TraceEnd ():
	if DEBUG > 1: print "<<<<<", sys._getframe (1).f_code.co_name
#--------------------------------------------------
# Trace2 ]

# pickport [
#--------------------------------------------------
def pickport ():

	global port

	return port
#--------------------------------------------------
# pickport ]

# Tokenize [
#--------------------------------------------------
def Tokenize (theString):
	TraceBeg ()

	theTokens	= []
	newString	= ""
	inQuote		= False

	for c in theString:
		if not inQuote and c == " ":
			if newString != "":
				theTokens.append (newString)
			newString	= ""
		else:
			if c == '"':
				inQuote	= not inQuote
			else:
				newString	+= c

	theTokens.append (newString)

	TraceEnd ()
	return theTokens
#--------------------------------------------------
# Tokenize ]

# WriteLogFile [
#--------------------------------------------------
def WriteLogFile (aString):
	TraceBeg ()

	theLogFile	= open (logfile, "a")
	theLogFile.write (time.asctime () + " " + aString + "\n")
	theLogFile.close ()
	
	TraceEnd ()
#--------------------------------------------------
# WriteLogFile ]

# WriteLogFileLines [
#--------------------------------------------------
def WriteLogFileLines (aString):
	TraceBeg ()

	OutString	= ""
	for c in aString:
		if c != "\n":
			OutString	= OutString + c
		else:
			WriteLogFile (OutString)
			OutString	= ""

	WriteLogFile (OutString)

	TraceEnd ()
#--------------------------------------------------
# WriteLogFileLines ]

# ComputeStat [
#--------------------------------------------------
def ComputeStat ():
	TraceBeg ()

	DeltaTime	= time.time () - StartTime
	year, month, day, \
	hour, minute, second, \
	weekday, julian, tz	= time.gmtime (DeltaTime)
	if julian > 1: dayStr	= str (julian - 1) + "+"
	else: dayStr	= ""

	TheResult	= \
				"Server running: " \
				+ dayStr + "%.2d:%.2d:%.2d\n" % (hour, minute, second) \
				+ "CPU time: %s\n" % str (time.clock ()) \
				+ "%8d bytes read\n" % BytesRead \
				+ "%8d bytes send" % BytesSend

	TraceEnd ()

	return TheResult
#--------------------------------------------------
# ComputeStat ]

# FormatCmdList [
#--------------------------------------------------
def FormatCmdList (theCmds):
	TraceBeg ()

	MaxLen		= 0
	for word in theCmds:
		MaxLen		= max (len (word), MaxLen)

	theCmds.sort ()

	NrOfWords 	= len (theCmds)
	NrOfCols	= SCREEN_WIDTH / MaxLen
	RowOffset	= NrOfWords / NrOfCols

	OutLines	= []

	haveWord	= True
	RowNr		= 0
	while haveWord:
		OutLine		= ""
		for i in range (NrOfCols):
			index		= RowNr * NrOfCols + i	# horizontal
			try:
				OutLine		+= "%-*s " % (MaxLen, theCmds [index])
			except:
				pass

			if index >= (NrOfWords - 1):
				haveWord	= False

		RowNr		+= 1
		OutLines.append (OutLine)
		OutLine		= ""

	TraceEnd ()

	return OutLines
#--------------------------------------------------
# FormatCmdList ]

# DirectoryMixinHandler [
#--------------------------------------------------
class DirectoryMixinHandler:

	def handle (self):

		global ServerRunning
		global BytesRead
		global BytesSend
		global StartTime

		from pyrapi import pyrapi	# The "heart" of the server.
		from pyrapi import file

		import pickle
		import base64


		# ObjectSlot2Tuple [
		#--------------------------------------------------
		def ObjectSlot2Tuple (anObject, aSlotIndex):
			TraceBeg ()

			theTuple	= (anObject, aSlotIndex)

			TraceEnd ()
			return theTuple
		#--------------------------------------------------
		# ObjectSlot2Tuple ]

		RAW_DATA	= False
		CmdWords	=	[
						"synce_log_set_level",
						"CeFindAllFiles",
						"CeFindFirstFile",
						"CeDeleteFile",
						"CeMoveFile",
						"CeCopyFile",
						"CeCopyFileA",
						"CeGetFileAttributes",
						"CeSetFileAttributes",
						"CeCreateDirectory",
						"CeRemoveDirectory",
						"openCeFile",
						"CeFile.read",
						"CeFile.write",
						"CeFile.close",
						"kill",
						"stat",
						"help",
						"vers",
						]

		TraceBeg ()
		line		= self.rfile.readline ()
		line		= line.replace ("\r", "\n")	# FIXME
		BytesRead	+= len (line)
		
		line		= line [:-1]				# Kill trailing "\n"
		dPrint ("line", line)
		#WriteLogFile (line)
		if len (line) > MAX_LOG_LINE_LEN:
			CutMarker	= "..."
		else:
			CutMarker	= ""
		WriteLogFile (line [:MAX_LOG_LINE_LEN - 1] + CutMarker)	# Limit output to first MAX_LOG_LINE_LEN chars.
		print line[:MAX_LOG_LINE_LEN - 1] + CutMarker			# We want to see someting from the client...

		TheResult	= ""
		Cmds		= line.split (",")

		#--------------------------------------------------
		if Cmds [0] == "synce_log_set_level":
			if DEBUG:
				print "Processing synce_log_set_level"
			if len (Cmds) < 2:
				Cmds.append (0)
				print "Warning: added missing parameter as 0"
			pyrapi.synce_log_set_level (int (Cmds [1]))
		#--------------------------------------------------
		elif Cmds [0] == "CeFindAllFiles":
			if DEBUG:
				print "Processing CeFindAllFiles"
			entries	= []
			if len (Cmds) < 2:
				Cmds.append ("")
				print "Warning: added missing parameter as ''"
			if len (Cmds) < 3:
				Cmds.append (0)
				print "Warning: added missing parameter as 0"
			for file in pyrapi.CeFindAllFiles (Cmds [1], int (Cmds [2])):
				if RAW_DATA:
					entries.append (file)
				else:
					anEntry	=	(
								Cmds [1],			# Path
								file.cFileName,
								str (file.dwFileAttributes),
								str (file.nFileSizeLow),
								str (file.ftCreationTime),
								str (file.ftLastAccessTime),
								str (file.ftLastWriteTime),
								)
					entries.append (anEntry)
			if DEBUG:
				print entries
			#TheResult	= str (entries)
			TheResult	= pickle.dumps (entries)
		#--------------------------------------------------
		elif Cmds [0] == "CeFindFirstFile":
			if DEBUG:
				print "Processing CeFindFirstFile"
			Handle, file	= pyrapi.CeFindFirstFile (Cmds [1])
			Data			=	(
								file.cFileName,
								str (file.dwFileAttributes),
								str (file.nFileSizeLow),
								str (file.ftCreationTime),
								str (file.ftLastAccessTime),
								str (file.ftLastWriteTime),
								)
			result			= (Handle, Data)
			if DEBUG:
				print "result: [%s]" % result
			TheResult		= pickle.dumps (result)
		#--------------------------------------------------
		elif Cmds [0] == "CeDeleteFile":
			if DEBUG:
				print "Processing CeDeleteFile"
			TheResult		= str (pyrapi.CeDeleteFile (Cmds [1]))
		#--------------------------------------------------
		elif Cmds [0] == "CeMoveFile":
			if DEBUG:
				print "Processing CeMoveFile"
			try:
				TheResult		= str (pyrapi.CeMoveFile (Cmds [1], Cmds [2]))
			except:
				TheResult		= "ERROR"
		#--------------------------------------------------
		elif Cmds [0] == "CeCopyFile":
			if DEBUG:
				print "Processing CeCopyFile"
			if len (Cmds) < 3:
				Cmds.append ("True")
				print "Warning: added missing parameter as True"
			try:
				TheResult		= str (pyrapi.CeCopyFile (Cmds [1], Cmds [2], Cmds [3] == "True"))
			except:
				TheResult		= "ERROR"
		#--------------------------------------------------
		elif Cmds [0] == "CeCopyFileA":
			if DEBUG:
				print "Processing CeCopyFileA"
			if len (Cmds) < 3:
				Cmds.append ("True")
				print "Warning: added missing parameter as True"
			try:
				TheResult		= str (pyrapi.CeCopyFileA (Cmds [1], Cmds [2], Cmds [3] == "True"))
			except:
				TheResult		= "ERROR"
		#--------------------------------------------------
		elif Cmds [0] == "CeGetFileAttributes":
			if DEBUG:
				print "Processing CeGetFileAttributes"
			TheResult		= str (pyrapi.CeGetFileAttributes (Cmds [1]))
		#--------------------------------------------------
		elif Cmds [0] == "CeSetFileAttributes":
			if DEBUG:
				print "Processing CeSetFileAttributes"
			TheResult		= str (pyrapi.CeSetFileAttributes (Cmds [1], Cmds [2]))
		#--------------------------------------------------
		elif Cmds [0] == "CeCreateDirectory":
			if DEBUG:
				print "Processing CeCreateDirectory"
			# FIXME CHECK lpSecurityAttributes are (not) needed?
			#TheResult		= str (pyrapi.CeCreateDirectory (Cmds [1], Cmds [2]))
			try:
				TheResult		= str (pyrapi.CeCreateDirectory (Cmds [1]))
			except:
				TheResult		= "ERROR"
		#--------------------------------------------------
		elif Cmds [0] == "CeRemoveDirectory":
			if DEBUG:
				print "Processing CeRemoveDirectory"
			try:
				TheResult		= str (pyrapi.CeRemoveDirectory (Cmds [1]))
			except:
				TheResult		= "ERROR"
		#--------------------------------------------------
		elif Cmds [0] == "openCeFile":
			if DEBUG:
				print "Processing openCeFile"
			if len (Cmds) < 2:
				Cmds.append ("")
				print "Warning: added missing parameter as ''"
			if len (Cmds) < 3:
				Cmds.append ("r")
				print "Warning: added missing parameter as 'r'"

			result				= file.openCeFile (Cmds [1], Cmds [2])
			if DEBUG:
				print "result:                   ", result
				print "result._file_handle:      ", result._file_handle
				print "result._read_buffer_size: ", result._read_buffer_size

			NextSlot			= FileList.index (-1)	# Raises ValueError if no more slots
			FileList [NextSlot]	= result
			theData				= ObjectSlot2Tuple (result, NextSlot)
			TheResult			= pickle.dumps (theData)

			#if DEBUG:
			#	print "TheResult: [%s]"	% TheResult
			#	print "len (TheResult):", len (TheResult)
		#--------------------------------------------------
		elif Cmds [0] == "CeFile.read":
			if DEBUG:
				print "Processing CeFile.read"
				print "Cmds [1]: [%s]"	% Cmds [1]

			theFile		= FileList [int (Cmds [1])]

			if DEBUG:
				print "theFile:                  ", theFile
				print "theFile._file_handle:     ", theFile._file_handle
				print "theFile._mode:            ", theFile._mode
				print "theFile._read_buffer_size:", theFile._read_buffer_size

			result		= theFile.read ()

			if DEBUG:
				print "result: [%s]" % result
			TheResult	= pickle.dumps (result)
		#--------------------------------------------------
		elif Cmds [0] == "CeFile.write":
			if DEBUG:
				print "Processing CeFile.write"
				print "Cmds [1]: [%s]"	% Cmds [1]

			theFile		= FileList [int (Cmds [1])]
			buf			= base64.decodestring (Cmds [2])

			if DEBUG:
				print "theFile:                  ", theFile
				print "theFile._file_handle:     ", theFile._file_handle
				print "theFile._mode:            ", theFile._mode
				print "theFile._read_buffer_size:", theFile._read_buffer_size

			if USE_BUFFER_OBJ:
				theBuf		= buffer (buf)
				result		= theFile.write (theBuf)
			else:
				result		= theFile.write (buf)

			if DEBUG:
				print "result: [%s]" % result
			TheResult	= pickle.dumps (result)
		#--------------------------------------------------
		elif Cmds [0] == "CeFile.close":
			if DEBUG:
				print "Processing CeFile.close"
				print "Cmds [1]: [%s]"	% Cmds [1]

			NextSlot	= int (Cmds [1])
			theFile		= FileList [NextSlot]

			if DEBUG:
				print "theFile:                  ", theFile
				print "theFile._file_handle:     ", theFile._file_handle
				print "theFile._mode:            ", theFile._mode
				print "theFile._read_buffer_size:", theFile._read_buffer_size

			result		= theFile.close ()

			FileList [NextSlot]	= -1			# Free slot

			if DEBUG:
				print "result: [%s]" % result
			TheResult	= str (result)
		#--------------------------------------------------
		elif Cmds [0] == "kill":
				print "Got kill. Exiting..."
				TheResult	= ComputeStat ()
				WriteLogFileLines (TheResult)
				ServerRunning	= False
		#--------------------------------------------------
		elif Cmds [0] == "stat":
				TheResult	= ComputeStat ()
				WriteLogFileLines (TheResult)
		#--------------------------------------------------
		elif Cmds [0] == "help":
				TheResult		= "Commands: "
				#for word in CmdWords:
				#	TheResult	+= word + ", "
				for theLine in FormatCmdList (CmdWords):
					TheResult	+= "\n" + theLine
		#--------------------------------------------------
		elif Cmds [0] == "vers":
				TheResult		= "Version: " \
				+ __version__.replace ("$", "") \
				+ __date__.replace ("$", "")
		#--------------------------------------------------
		else:
			ErrorString		= "Cmd [%s] not recognized." % Cmds [0]
			print ErrorString
			WriteLogFile (ErrorString)
			TheResult		= ErrorString

		dPrint ("TheResult", TheResult)
		BytesSend	+= len (TheResult)
		self.wfile.write (TheResult)
		TraceEnd ()
#--------------------------------------------------
# DirectoryMixinHandler ]


# DirectoryStreamHandler [
#--------------------------------------------------
class DirectoryStreamHandler (DirectoryMixinHandler, StreamRequestHandler):
	pass
#--------------------------------------------------
# DirectoryStreamHandler ]


# DirectoryMixinServer [
#--------------------------------------------------
class DirectoryMixinServer:

	def serve_requests (self):

		global ServerRunning

		TraceBeg ()
		while ServerRunning:
			self.handle_request ()
		self.server_close ()
		TraceEnd ()

	def handle_error (self, request, client_address):
		TraceBeg ()
		if ABORT_ON_ERR:
			self.close_request (request)
			self.server_close ()
			WriteLogFile ("Server shutdown due to ERROR.")
			WriteLogFileLines (ComputeStat ())
			raise
		else:
			WriteLogFile ("ERROR caught. Client: " + str (client_address))
			WriteLogFileLines (ComputeStat ())
		TraceEnd ()
#--------------------------------------------------
# DirectoryMixinServer ]


# DirectoryTCPServer [
#--------------------------------------------------
class DirectoryTCPServer (TCPServer):
	allow_reuse_address	= True		# Should be: setsockopt (..., SO_REUSEADDR, ...)
#--------------------------------------------------
# DirectoryTCPServer ]


# ServerThread [
#--------------------------------------------------
class ServerThread (threading.Thread):
	def __init__ (self, addr, svrcls, hdlrcls):
		threading.Thread.__init__ (self)
		self.__addr = addr
		self.__svrcls = svrcls
		self.__hdlrcls = hdlrcls

	def run (self):

		class svrcls (DirectoryMixinServer, self.__svrcls):
			pass

		TraceBeg ()
		if verbose: print "thread: creating server"
		svr = svrcls (self.__addr, self.__hdlrcls)
		svr.serve_requests ()
		if verbose: print "thread: done"
		TraceEnd ()
#--------------------------------------------------
# ServerThread ]


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
def ProcessStream (proto, addr, cmd):
	TraceBeg ()
	s	= socket.socket (proto, socket.SOCK_STREAM)
	s.connect (addr)
	debugPrint.String ("cmd: [%s]" % cmd)
	cmd	= cmd + "\n"
	s.sendall (cmd)

	buf	= data = receive (s, BUFFER_SIZE)
	while data:
		data	= receive (s, BUFFER_SIZE)
		buf		+= data
	debugPrint.String ("buf: [%s]" % buf)
	print buf
	dPrint ("len (buf)", len (buf))
	s.close ()
	TraceEnd ()
#--------------------------------------------------
# ProcessStream ]

# main [
#--------------------------------------------------
def main (theArgs):

	global DEBUG
	global DEBUG_PRINT
	global ABORT_ON_ERR
	global dPrint
	global debugPrint
	global verbose
	global ServerRunning
	global logfile
	global host
	global port
	global BytesRead
	global BytesSend
	global StartTime

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

	# Define Defaults [
	#--------------------------------------------------
	logfile	= "CE_DirViewServer.Log"
	host	= "localhost"
	port	= DEFAULT_PORT
	#--------------------------------------------------
	# Define Defaults ]

	# Read Settings [
	#--------------------------------------------------
	try:
		SettingsFile    = open (SETTINGS_NAME, "r")
		Settings		= SettingsFile.readlines ()
		SettingsFile.close ()
		# ['host=linuxorange\n', 'port=4711\n']
		dPrint ("Settings", Settings)
		for Line in Settings:
			Line	= Line [:-1]	# Kill trailing "\n"
			Params	= Line.split ("=")
			if Params [0] == "logfile":
				logfile	= Params [1]
				dPrint ("logfile", logfile)
			elif Params [0] == "host":
				host	= Params [1]
				dPrint ("host", host)
			elif Params [0] == "port":
				port	= int (Params [1])
				dPrint ("port", port)
	except:
		print "%s Warning: Could not read settings from [%s]." % (sys.argv [0], SETTINGS_NAME)
	#--------------------------------------------------
	# Read Settings ]

	# Process Options [
	#--------------------------------------------------
	Usage		= \
    """usage: %s [-c] [-d n] [-D] [-e] [-l file] [-h host] [-p port] [-v] [-V]
    -c,      --client ......... run as client
    -d n,    --debugLevel=n ... debug-level 
                                0 = off
                                1 = general
                                2 = trace
                                3 = Debug-File
    -D,      --DebugPrint ..... DebugPrint
    -e,      --error .......... abort on Error
             --help ........... display this help and exit
    -h host, --host=host ...... host
    -l file, --logfile=file ... logfile
    -p port, --port=port ...... port
    -v,      --verbose ........ explain what is being done
    -V,      --version ........ print version and exit"""

	LongOptions		=	[
						"client",
						"debugLevel=",
						"DebugPrint",
						"error",
						"help",
						"host=",
						"logfile=",
						"port=",
						"verbose",
						"version",
						]

	try:
		opts, args	= getopt.getopt (theArgs [1:], 'cd:Deh:l:p:vV', LongOptions)
	except getopt.GetoptError, msg:
		print msg
		print Usage % sys.argv [0]
		sys.exit (1)

	if DEBUG:
		print opts, args

	Server			= True
	DEBUG_PRINT		= False
	ABORT_ON_ERR	= False
	verbose			= False
	BytesRead		= 0
	BytesSend		= 0
	StartTime		= time.time ()

	for o, a in opts:
		if o in ("-c", "--client"):
			Server	= False
			debugPrint.String ("Running as Client")
		if o in ("-d", "--debugLevel"):
			try:
				DEBUG	= int (a)
			except ValueError:
				print "debugLevel needs and integer"
				print Usage % sys.argv [0]
				sys.exit (1)
		if o in ("-D", "--debugPrint"):
			DEBUG_PRINT	= True
		if o in ("-e", "--error"):
			ABORT_ON_ERR= True
		if o in ("--help"):
			print Usage % sys.argv [0]
			sys.exit (0)
		if o in ("-h", "--host"):
			host	= a
		if o in ("-l", "--logfile"):
			logfile	= a
		if o in ("-p", "--port"):
			try:
				port	= int (a)
			except ValueError:
				print "port needs and integer"
				print Usage % sys.argv [0]
				sys.exit (1)
		if o in ("-v", "--verbose"):
			verbose	= True
		if o in ("-V", "--version"):
			print sys.argv [0] \
				+ " " + __version__.replace ("$", "") \
				+ __date__.replace ("$", "")
			sys.exit (0)
	#--------------------------------------------------
	# Process Options ]

	# Write Settings [
	#--------------------------------------------------
	try:
		SettingsFile    = open (SETTINGS_NAME, "w")
		Settings		= [
						"logfile=", logfile, "\n", 
						"host=", host, "\n", 
						"port=", str (port), "\n"
						]
		SettingsFile.writelines (Settings)
		SettingsFile.close ()
	except:
		print "%s Warning: Could not save settings in [%s]." % (sys.argv [0], SETTINGS_NAME)
	#--------------------------------------------------
	# Write Settings ]

	debugPrint.doPrint= DEBUG_PRINT

	proto		= socket.AF_INET
	addr		= pickaddr (proto)

	if Server:
		# Server [
		#--------------------------------------------------
		ServerRunning	= True
		if verbose:
			dPrint ("ADDR", addr)
			dPrint ("CLASS", DirectoryTCPServer)
		t	= ServerThread (addr, DirectoryTCPServer, DirectoryStreamHandler)
		if verbose: print "server created"

		t.start ()
		StartMessage	= "Server [ %s %s] started on %s:%s" % \
						( \
						__version__.replace ("$", ""), \
						__date__.replace ("$", ""), \
						host, \
						str (port) \
						)
		print StartMessage
		WriteLogFile (StartMessage)

		# Initialize FileSlots [
		#--------------------------------------------------
		for i in range (MAX_FILES):
			FileList.append (-1)
		#--------------------------------------------------
		# Initialize FileSlots ]

		if verbose: print "server running"
		#--------------------------------------------------
		# Server ]
	else:
		# Client [
		#--------------------------------------------------
		print "Connecting to server on %s:%s" % (host, str (port))
		Running	= True
		while Running:
			try:
				TheCmd	= raw_input ("LsCmd: > ")
				ProcessStream (proto, addr, TheCmd)
			except EOFError:
				print "End"
				Running	= False
		#--------------------------------------------------
		# Client ]

	if Server:
		if verbose: print "waiting for server"
		t.join ()
	if verbose: print "done"

	TraceEnd ()
#--------------------------------------------------
# main ]

if __name__ == "__main__": main (sys.argv)

# vim: ts=4 sw=4
