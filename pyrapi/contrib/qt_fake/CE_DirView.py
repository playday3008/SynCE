#!/usr/bin/env python
# -*- coding: iso-8859-1 -*-
#	Created:	Fre Mär 26 18:28:40 CET 2004	by M. Biermaier	on linuxorange
"""Version:	Sam Mai  1 10:49:24 CEST 2004	on linuxorange
$Id$

GUI for files on PocketPC.
Show files from PocketPC in a TreeView.

Features:
	- Runs on
	  o Linux
	  o MacOS X
	  o Zaurus
	- 'Drag & Drop'-support on Desktops.
	  o Drag/drop files to/from konqueror on Linux.
	  o Drag/drop files to/from Finder on MacOS X.
	- Process local FileSystem with 'local'-option.
	  o Multiple instances possible on same computer.
	  o 'Drag & Drop' files to/from local FileSystem to/from PocketPC
	- Transparent Client/Server solution possible.
	  o Only one computer on the network needs SynCE installed
	    and a PocketPC connected.
	  o Other client-Computers on the network can access the PocketPC
	    FileSystem.
	  o Clients need only a Python-Fake-Modul installed (no SynCE,
	    no direct PocketPC-connection).

Based on:
pyrapi - A python wrapper class around the librapi2 library
	by Richard Taylor <r.taylor@bcs.org.uk>

SynCE - The purpose of the SynCE project is to provide a means of
	communication with a Windows CE device from a computer running
	Linux, *BSD or other unices.
	by David Eriksson <twogood@users.sourceforge.net>

PyQt - Python Bindings for the Qt Toolkit
	by Phil Thompson <phil@riverbankcomputing.co.uk>

QT - A multiplatform C++ GUI application framework
	by Trolltech AS http://www.trolltech.com

dirview.py
	QT-example reworked by Phil Thompson (?)
"""

__author__		= "M. Biermaier"
__version__		= "$Revision$"
__date__		= "$Date$"
__FileVersion__ = __doc__.split ("\n") [0]			# The first line of the DocString.


# Imports [
#--------------------------------------------------
import sys
import os

try:
	# M.B. 2003-01-15
	# We set Qtopia before the "import", because "import"
	# may raise other errors (e.g. missing libraries).
	Qtopia			= 1
	from qtpe import QPEApplication
	from qtpe import QPEMenuBar
except ImportError:
	Qtopia			= 0

# We have a problem on MacOS X...
if sys.platform == "darwin":
	import qtui
from qt import *

theHOME			= os.getenv ("HOME")
sys.path.append (theHOME + "/Python")

import DebugPrint		# DebugPrinting

from pyrapi import pyrapi
import time
import getopt
#--------------------------------------------------
# Imports ]


# Constants [
#--------------------------------------------------
SETTINGS_NAME	= ".CE_DirView.cfg"

# Debug switches
DEBUG			= False
DEBUG_PRINT		= False
DEBUG_OBJECTS	= False
DEBUG_POSITIONS	= False
DEBUG_COMPARE	= False
DEBUG_RENAME	= False
DEBUG_COPY		= False
DEBUG_MOUSE		= False

SHOW_SYM_LINKS	= False		# True:  show symbolic links
							# False: follow symbolic links

SIM_CONTEXT_MENU= Qtopia

LocalDir		= False

# Settings
prefStatusWant	= True

AppName			= "DirectoryBrowser"
theCaption		= " - Directory Browser - " + __version__.replace ("$", "")

# Column Numbers
COL_NAME		= 0
COL_TYPE		= 1
COL_SIZE		= 2
COL_LASTWRITETIME= 3

# Drag 'n' Drop actions
ActionNone		= 0
ActionCopy		= 1
ActionMove		= 2
ActionLink		= 3

# For Qtopia
STATUS_BAR_H	= 30

folder_closed_xpm = [
	"16 16 9 1",
	"g c #808080",
	"b c #c0c000",
	"e c #c0c0c0",
	"# c #000000",
	"c c #ffff00",
	". c None",
	"a c #585858",
	"f c #a0a0a4",
	"d c #ffffff",
	"..###...........",
	".#abc##.........",
	".#daabc#####....",
	".#ddeaabbccc#...",
	".#dedeeabbbba...",
	".#edeeeeaaaab#..",
	".#deeeeeeefe#ba.",
	".#eeeeeeefef#ba.",
	".#eeeeeefeff#ba.",
	".#eeeeefefff#ba.",
	".##geefeffff#ba.",
	"...##gefffff#ba.",
	".....##fffff#ba.",
	".......##fff#b##",
	".........##f#b##",
	"...........####."
]

folder_open_xpm = [
	"16 16 11 1",
	"# c #000000",
	"g c #c0c0c0",
	"e c #303030",
	"a c #ffa858",
	"b c #808080",
	"d c #a0a0a4",
	"f c #585858",
	"c c #ffdca8",
	"h c #dcdcdc",
	"i c #ffffff",
	". c None",
	"....###.........",
	"....#ab##.......",
	"....#acab####...",
	"###.#acccccca#..",
	"#ddefaaaccccca#.",
	"#bdddbaaaacccab#",
	".eddddbbaaaacab#",
	".#bddggdbbaaaab#",
	"..edgdggggbbaab#",
	"..#bgggghghdaab#",
	"...ebhggghicfab#",
	"....#edhhiiidab#",
	"......#egiiicfb#",
	"........#egiibb#",
	"..........#egib#",
	"............#ee#"
]

folder_locked = [
	"16 16 10 1",
	"h c #808080",
	"b c #ffa858",
	"f c #c0c0c0",
	"e c #c05800",
	"# c #000000",
	"c c #ffdca8",
	". c None",
	"a c #585858",
	"g c #a0a0a4",
	"d c #ffffff",
	"..#a#...........",
	".#abc####.......",
	".#daa#eee#......",
	".#ddf#e##b#.....",
	".#dfd#e#bcb##...",
	".#fdccc#daaab#..",
	".#dfbbbccgfg#ba.",
	".#ffb#ebbfgg#ba.",
	".#ffbbe#bggg#ba.",
	".#fffbbebggg#ba.",
	".##hf#ebbggg#ba.",
	"...###e#gggg#ba.",
	".....#e#gggg#ba.",
	"......###ggg#b##",
	".........##g#b##",
	"...........####."
]

pix_file = [
	"16 16 7 1",
	"# c #000000",
	"b c #ffffff",
	"e c #000000",
	"d c #404000",
	"c c #c0c000",
	"a c #ffffc0",
	". c None",
	"................",
	".........#......",
	"......#.#a##....",
	".....#b#bbba##..",
	"....#b#bbbabbb#.",
	"...#b#bba##bb#..",
	"..#b#abb#bb##...",
	".#a#aab#bbbab##.",
	"#a#aaa#bcbbbbbb#",
	"#ccdc#bcbbcbbb#.",
	".##c#bcbbcabb#..",
	"...#acbacbbbe...",
	"..#aaaacaba#....",
	"...##aaaaa#.....",
	".....##aa#......",
	".......##......."
]
#--------------------------------------------------
# Constants ]


# Global VARs [
#--------------------------------------------------
folderLocked	= None
folderClosed	= None
folderOpened	= None
fileNormal		= None
#--------------------------------------------------
# Global VARs ]


# Trace2 [
#--------------------------------------------------
def TraceBeg ():
	"""Universal trace entry."""
	if DEBUG > 1: print ">>>>>", sys._getframe (1).f_code.co_name

def TraceEnd ():
	"""Universal trace exit."""
	if DEBUG > 1: print "<<<<<", sys._getframe (1).f_code.co_name
#--------------------------------------------------
# Trace2 ]

# dPrint [
#--------------------------------------------------
def dPrint (aLabel, aValue):
	"""Allow DebugPrinting of unicode-strings."""

	if isinstance (aValue, QString):
		dPrint1 (aLabel, aValue.latin1 ())
	else:
		dPrint1 (aLabel, aValue)
#--------------------------------------------------
# dPrint ]


# TrimCvsString [
#--------------------------------------------------
def TrimCvsString (theString):
	"""Trim "Revision", "Date", ..."""
	return str(theString).replace ("$" "Revision: ", "").replace ("$" "Date: ", ""). replace (" $", "")
#--------------------------------------------------
# TrimCvsString ]


# DoQuestionDialog [
#--------------------------------------------------
def DoQuestionDialog (Parent, Caption, Text):
	"""Needed on Zaurus."""

	import QuestionDialog

	TraceBeg ()

	theQuestionDialog	= QuestionDialog.Question (modal=True)

	# Fill Values [
	#--------------------------------------------------
	theQuestionDialog.setCaption (Caption)
	theQuestionDialog.textLabelQuestion.setText (Text)
	#--------------------------------------------------
	# Fill Values ]

	theQuestionDialog.textLabelCvsInfo.setText (TrimCvsString (theQuestionDialog.textLabelCvsInfo.text ()))

	# Show the Dialog
	#--------------------------------------------------
	ReturnValue	= theQuestionDialog.exec_loop ()
	#--------------------------------------------------
	# Show the Dialog

	TraceEnd ()

	return ReturnValue
#--------------------------------------------------
# DoQuestionDialog ]


# CE_FileInfo [
#--------------------------------------------------
class CE_FileInfo (QFileInfo):
	"""Implements PocketPC FileInfo."""

	def __init__ (self):
		QFileInfo.__init__ (self)
		ce_file			= ""
		ce_filePath		= ""

	# filePath [
	#--------------------------------------------------
	def filePath (self):
		"""Path."""
		TraceBeg ()

		TraceEnd ()
		return self.ce_filePath
	#--------------------------------------------------
	# filePath ]

	# fileName [
	#--------------------------------------------------
	def fileName (self):
		""""ce_file.cFileName"."""
		TraceBeg ()

		TraceEnd ()
		return self.ce_file.cFileName
	#--------------------------------------------------
	# fileName ]

	# isFile [
	#--------------------------------------------------
	def isFile (self):
		"""Check "ce_file.dwFileAttributes"."""
		TraceBeg ()

		# FIXME
		# A file is an object, that's not a directory? Sure?
		
		TraceEnd ()
		return self.ce_file.dwFileAttributes != 16
	#--------------------------------------------------
	# isFile ]

	# isDir [
	#--------------------------------------------------
	def isDir (self):
		"""Directory."""
		TraceBeg ()

		TraceEnd ()
		return self.ce_file.dwFileAttributes == 16
	#--------------------------------------------------
	# isDir ]

	# fileSize [
	#--------------------------------------------------
	def fileSize (self):
		""""ce_file.nFileSizeLow" (!)."""
		TraceBeg ()

		TraceEnd ()
		return self.ce_file.nFileSizeLow
	#--------------------------------------------------
	# fileSize ]

	# ftLastWriteTime [
	#--------------------------------------------------
	def ftLastWriteTime (self):
		""""time.ctime (...)"."""
		TraceBeg ()

		TraceEnd ()
		return time.ctime (self.ce_file.ftLastWriteTime)
	#--------------------------------------------------
	# ftLastWriteTime ]

	# ftLastWriteTimeRaw [
	#--------------------------------------------------
	def ftLastWriteTimeRaw (self):
		""""ce_file.ftLastWriteTime"."""
		TraceBeg ()

		TraceEnd ()
		return self.ce_file.ftLastWriteTime
	#--------------------------------------------------
	# ftLastWriteTimeRaw ]
#--------------------------------------------------
# CE_FileInfo ]


# CE_Dir [
#--------------------------------------------------
class CE_Dir (QDir):
	"""Implements PocketPC directory-tree."""

	def __init__ (self, filename=None):
		QDir.__init__ (self, filename)
		self.pathName	= filename

	# drives [
	#--------------------------------------------------
	def drives (self):
		"""CE-root."""
		TraceBeg ()

		files	= []

		theFileInfo				= CE_FileInfo ()

		theFileInfo.ce_file		= file
		theFileInfo.ce_filePath	= ""
			
		files.append (theFileInfo)

		TraceEnd ()
		return files
	#--------------------------------------------------
	# drives ]

	# entryInfoList [
	#--------------------------------------------------
	def entryInfoList (self):
		"""Get FileInformation via "pyrapi"."""
		TraceBeg ()

		entries	= []

		path	= str (self.pathName.latin1 ())
		if path == "\\":
			path = ""
		if DEBUG > 3:
			dPrint ("path", path)
			dPrint ("type (path)", type (path))

		for file in pyrapi.CeFindAllFiles (
								path + r"\*.*",
								pyrapi.FAF_NAME | \
								pyrapi.FAF_ATTRIBUTES | \
								pyrapi.FAF_CREATION_TIME | \
								pyrapi.FAF_LASTACCESS_TIME | \
								pyrapi.FAF_LASTWRITE_TIME | \
								pyrapi.FAF_SIZE_LOW \
								):
			theFileInfo				= CE_FileInfo ()
			theFileInfo.ce_file		= file
			theFileInfo.ce_filePath	= path
			entries.append (theFileInfo)

		TraceEnd ()

		if DEBUG > 3:
			dPrint ("entries", entries)
		return entries
	#--------------------------------------------------
	# entryInfoList ]

	# isReadable [
	#--------------------------------------------------
	def isReadable (self):
		"""Always return true..."""
		TraceBeg ()

		# FIXME
		# Should have some better indicator...

		TraceEnd ()
		return True
	#--------------------------------------------------
	# isReadable ]
#--------------------------------------------------
# CE_Dir ]


# CE_File [
#--------------------------------------------------
class CE_File (QFile):
	"""Implements PocketPC File."""

	def __init__ (self, filename=None):
		QFile.__init__ (self, filename)
		self.ce_fileName	= filename

	# name [
	#--------------------------------------------------
	def name (self):
		""""ce_fileName"."""
		TraceBeg ()

		TraceEnd ()
		return QString (self.ce_fileName)
	#--------------------------------------------------
	# name ]
#--------------------------------------------------
# CE_File ]


# CE_compare [
#--------------------------------------------------
def CE_compare (aListViewItem, i, col, ascending):
	"""Used for sorting."""
	TraceBeg ()

	theResult	= 0

	if DEBUG_COMPARE:
		dPrint ("CE_compare", "----------")
		dPrint ("CE_compare: aListViewItem", aListViewItem)
		dPrint ("CE_compare: i", i)
		dPrint ("CE_compare: col", col)
		dPrint ("CE_compare: text", aListViewItem.text (col))

	try:
		if col == COL_SIZE:
			if DEBUG_COMPARE:
				dPrint ("CE_compare: aListViewItem.theFileSize", str (aListViewItem.theFileSize))
				dPrint ("CE_compare: i.theFileSize", str (i.theFileSize))
			if aListViewItem.theFileSize > i.theFileSize:
				theResult	= 1
			elif aListViewItem.theFileSize < i.theFileSize:
				theResult	= -1
			else:
				pass

		elif col == COL_LASTWRITETIME:
			if aListViewItem.theFtLastWriteTimeRaw > i.theFtLastWriteTimeRaw:
				theResult	= 1
			elif aListViewItem.theFtLastWriteTimeRaw < i.theFtLastWriteTimeRaw:
				theResult	= -1
			else:
				pass

		else:
			theResult	= QListViewItem.compare (aListViewItem, i, col, ascending)

	except AttributeError:
		# M.B. Thu Apr  1 16:09:17 CEST 2004
		# I think this problem is MacOS X (or my MacOS X-installation-) specific.
		print "Err"
		print "aListViewItem:", aListViewItem
		print "text:         ", aListViewItem.text (0)
		print "col:          ", col
		print "i:            ", i

	TraceEnd ()

	if sys.platform == "darwin":
											# M.B. Don Apr  1 10:38:24 CEST 2004
		return theResult * (-1)				# Don't know why, but at least on MacOS X the sort indicator always
											# shows the wrong direction...
	else:
		return theResult
#--------------------------------------------------
# CE_compare ]

# PathToFolder [
#--------------------------------------------------
def PathToFolder (thePath):
	"""Build the first part of a pathName."""
	TraceBeg ()

	NameList		= QStringList ()
	names			= NameList.split (PATH_SEPEARATOR, thePath, True)
	folderPath		= QString ()
	for name in names [:-1]:
		folderPath.append (name)
		folderPath.append (PATH_SEPEARATOR)

	TraceEnd ()

	return folderPath
#--------------------------------------------------
# PathToFolder ]


# FileItem [
#--------------------------------------------------
class FileItem (QListViewItem):
	"""File."""

	def __init__ (self, parent=None, s1=None, s2=None):
		QListViewItem.__init__ (self, parent, s1, s2)
		self.pix					= None
		self.MB_ObjName				= s1
		self.p						= parent
		self.NameLink				= parent
		self.theFileSize			= 0
# M.B. 2004-04-28
		self.theFtLastWriteTime		= QString ()
		self.theFtLastWriteTimeRaw	= 0
		self.thePathName			= ""	# Used for rename

	# dump [
	#--------------------------------------------------
	def dump (self):
		"""Dump info for debugging."""
		TraceBeg ()

		dPrint ("[ Begin of dump for", self)
		dPrint ("MB_ObjName", self.MB_ObjName)
		dPrint ("p", self.p)
		dPrint ("NameLink", self.NameLink)
		dPrint ("] End   of dump for", self)

		TraceEnd ()
	#--------------------------------------------------
	# dump ]

	def setPixmap (self, p):
		#TraceBeg ()
		self.pix	= p
		self.setup ()
		self.widthChanged (0)
		self.invalidateHeight ()
		self.repaint ()
		#TraceEnd ()
	
	def pixmap (self, i):
		#TraceBeg ()
		if i: return None
		else: return self.pix;
		#TraceEnd ()

	# text [
	#--------------------------------------------------
	def text (self, column):
		"""Take care of Unicode-names and FileTimes..."""

		if column == COL_NAME:
			tempString	= QListViewItem.text (self, column).latin1 ()
			if len (tempString) > 0:
				try:
					return tempString.decode ('utf-8')
				except UnicodeDecodeError:
					return tempString.decode ('latin1').encode ('utf-8').decode ('utf-8')
			else:
				return ""
		if column == COL_SIZE:
			return str (self.theFileSize)
		elif column == COL_LASTWRITETIME:
			# UNICODE
			if LocalDir:
				return str (self.theFtLastWriteTime.latin1 ())
			else:
				return str (self.theFtLastWriteTime)
		else:
			return QListViewItem.text (self, column)
	#--------------------------------------------------
	# text ]

	# compare [
	#--------------------------------------------------
	def compare (self, i, col, ascending):
		"""Used for sorting."""

		return CE_compare (self, i, col, ascending)
	#--------------------------------------------------
	# compare ]

	# okRename [
	#--------------------------------------------------
	def okRename (self, col):
		"""Rename file."""
		TraceBeg ()

		if DEBUG_RENAME:
			print "okRename: self:             ", self
			print "type (self.thePathName):    ", type (self.thePathName)
			print "self.thePathName:           ", self.thePathName.latin1 ()
			print "OLD: type (self.text (col)):", type (self.text (col))
			print "OLD: self.text (col):       ", self.text (col)

		OldLeafName	= self.text (col)
		OldName		= QString (self.thePathName)
		OldName.append (PATH_SEPEARATOR)
		OldName.append (self.text (col))

		QListViewItem.okRename (self, col)

		NewName		= QString (self.thePathName)
		NewName.append (PATH_SEPEARATOR)
		NewName.append (self.text (col))

		if DEBUG_RENAME:
			print "NEW: type (self.text (col)):", type (self.text (col))
			print "NEW: self.text (col):       ", self.text (col)
			print "type (OldName):             ", type (OldName)
			print "Rename: [%s] -> [%s]" % (OldName.latin1 (), NewName.latin1 ())

		#if 1:
		try:
			if LocalDir:
				# M.B. Mit Apr 28 19:55:05 CEST 2004
				# FIXME
				# Duplicate items in view possible on Linux.
				# Remove duplicate item by hand.
				os.rename (str (OldName.utf8 ()), str (NewName.utf8 ()))
			else:
				pyrapi.CeMoveFile (str (OldName.utf8 ()), str (NewName.utf8 ()))

		#if 0:
		except:
			QMessageBox.warning	(
								None, AppName + " Rename File",
								QString	("Could not rename\nfile \"%1\"\nto \"%2\".\n"
										"Perhaps a file with this name allready exists?"
										). arg (OldName, NewName)
								)
			self.setText (col, OldLeafName)

		TraceEnd ()
	#--------------------------------------------------
	# okRename ]
#--------------------------------------------------
# FileItem ]


# Directory [
#--------------------------------------------------
class Directory (QListViewItem):
	"""Directory."""

	def __init__ (self, parent=None, filename=None, col2=None):
		QListViewItem.__init__ (self, parent, filename, col2)
		self.pix					= None
		self.theFileSize			= 0
		self.theFtLastWriteTimeRaw	= 0
		global folderLocked, folderClosed, folderOpened, fileNormal
		self.showDirsOnly	= False
		dPrint ("Directory.__init__", "------------------------------")
		dPrint ("Directory.__init__ filename", filename)
		dPrint ("Directory.__init__ parent", parent)
		self.NameLink		= parent
		if filename == None:
			self.MB_ObjName		= "Directory ---None---"
		elif filename == "":
			self.MB_ObjName		= "Directory ---Empty String---"
		else:
			self.MB_ObjName		= filename

		self.thePathName	= QString ()

		if isinstance (parent, QListView):
			self.p	= None
			self.showDirsOnly	= DirectoryView ().showDirsOnly ()
			self.f	= QFile (QString (PATH_SEPEARATOR))
			#self.f	= CE_File (QString (PATH_SEPEARATOR))

		else:
			dPrint ("Directory.__init__ parent.f.name ()", parent.f.name ())
			self.p	= parent
			dPrint ("Directory.__init__ self", self)
			dPrint ("Directory.__init__ self.p", self.p)
			self.showDirsOnly	= parent.showDirsOnly
			self.f	= QFile (QString (filename))
			
			### print filename.decode ('utf-8')

		if LocalDir:
			self.readable	= QDir (self.fullName ()).isReadable ()
		else:
			self.readable	= CE_Dir (self.fullName ()).isReadable ()

		if not self.readable: self.setPixmap (folderLocked)
		else: self.setPixmap (folderClosed)

	# dump [
	#--------------------------------------------------
	def dump (self):
		"""Dump info for debugging."""
		TraceBeg ()

		dPrint ("[ Begin of dump for", self)
		dPrint ("parent", self.parent)
		dPrint ("theFileSize", self.theFileSize)
		dPrint ("showDirsOnly", self.showDirsOnly)
		dPrint ("NameLink", self.NameLink)
		dPrint ("p", self.p)
		dPrint ("f", self.f)
		dPrint ("readable", self.readable)
		dPrint ("] End   of dump for", self)

		TraceEnd ()
	#--------------------------------------------------
	# dump ]

	def setPixmap (self, px):
		#TraceBeg ()
		self.pix	= px
		self.setup ()
		self.widthChanged (0)
		self.invalidateHeight ()
		self.repaint ()
		#TraceEnd ()
	
	def pixmap (self, i):
		#TraceBeg ()
		#TraceEnd ()
		if i: return None
		else: return self.pix
	
	def setOpen (self, o):

		TraceBeg ()
		if o: self.setPixmap (folderOpened)
		else: self.setPixmap (folderClosed)

		if o and not self.childCount ():
			s	= QString (self.fullName ())

			if LocalDir:
				thisDir	= QDir (s)
			else:
				thisDir	= CE_Dir (s)

			if not thisDir.isReadable ():
				self.readable	= False
				self.setExpandable (False)
				return
			
			# Get FileInfo [
			#--------------------------------------------------
			self.listView ().setUpdatesEnabled (False)

			# Reset ProgressBar [
			#--------------------------------------------------
			if Qtopia:
				theStatusBar.clear ()
				theProgressBar.show ()
			theProgressBar.reset ()
			qApp.processEvents ()
			#--------------------------------------------------
			# Reset ProgressBar ]

			files	= thisDir.entryInfoList ()

			if files:
				# Init ProgressBar [
				#--------------------------------------------------
				theProgressBar.setTotalSteps (len (files))
				i	= 0
				theProgressBar.setProgress (0)
				#--------------------------------------------------
				# Init ProgressBar ]

				if LocalDir:
					fi	= QFileInfo ()
				else:
					fi	= CE_FileInfo ()

				for it in files:
					fi	= it

					if DEBUG > 3:
						dPrint ("it", it)
						dPrint ("fi.fileName ()", fi.fileName ())

					# UNICODE
					#if str (fi.fileName ()) == "." or str (fi.fileName ()) == "..":
					if LocalDir:
						theFileName		= str (fi.fileName ().latin1 ())
					else:
						theFileName		= str (fi.fileName ())

					if theFileName == "." or theFileName == "..":
						continue # nothing
					elif fi.isSymLink () and not self.showDirsOnly and SHOW_SYM_LINKS:
						item	= FileItem (self, fi.fileName (), "Symbolic Link")
						item.setPixmap (fileNormal)
					elif fi.isDir ():
						Directory (self, fi.fileName ())
					elif not self.showDirsOnly:
						if fi.isFile ():
							item	= FileItem (self, fi.fileName (), "File")

							if LocalDir:
								item.theFileSize			= fi.size ()
								item.theFtLastWriteTime		= fi.lastModified ().toString ()
								if Qtopia:
									#item.theFtLastWriteTimeRaw	= fi.lastModified ().toString ("yyyyMMddhhmmss")
									#item.theFtLastWriteTimeRaw	= fi.lastModified ().toString ()
									HelpDate					= QDateTime ()
									item.theFtLastWriteTimeRaw	= int (fi.lastModified ().secsTo (HelpDate.currentDateTime ()))
								else:
									item.theFtLastWriteTimeRaw	= fi.lastModified ().toTime_t ()
							else:
								item.theFileSize			= fi.fileSize ()
								item.theFtLastWriteTime		= fi.ftLastWriteTime ()
								item.theFtLastWriteTimeRaw	= fi.ftLastWriteTimeRaw ()

						else:
							item	= FileItem (self, fi.fileName (), "Special")
						item.setPixmap (fileNormal)

					# Update ProgressBar [
					#--------------------------------------------------
					i	+= 1
					theProgressBar.setProgress (i)
					if sys.platform == 'darwin':
						# Or no progress update is shown...
						qApp.processEvents (1)
					#--------------------------------------------------
					# Update ProgressBar ]

			else:
				# If we have an empty directory, we want to show 100% complete...
				if Qtopia:
					theProgressBar.setTotalSteps (100)
					theProgressBar.setProgress (100)
				else:
					theProgressBar.setProgress (100, 100)

			# M.B. Mit Apr 28 12:05:39 CEST 2004
			# Don't know, why we need this...
			if LocalDir:
				if Qtopia:
					theProgressBar.setTotalSteps (100)
					theProgressBar.setProgress (100)
				else:
					theProgressBar.setProgress (100, 100)

			# Uncomment the following line if you don't want to see always the "100%" indicator.
			#theProgressBar.reset ()

			if Qtopia:
				theProgressBar.hide ()
				theStatusBar.message ((__version__ + " " + __date__).replace ("$", ""))

			self.listView ().setUpdatesEnabled (True)
			#--------------------------------------------------
			# Get FileInfo ]

		QListViewItem.setOpen (self, o)
		TraceEnd ()

	def setup (self):
		TraceBeg ()
		self.setExpandable (True)
		QListViewItem.setup (self)
		TraceEnd ()

	def fullName (self):
		TraceBeg ()
		s	= QString ()
		if self.p:
			s	= self.p.fullName ()
			s.append (self.f.name ())
			s.append (PATH_SEPEARATOR)
		else:
			s	= self.f.name ()
		return s
		TraceEnd ()

	def text (self, column):
		TraceBeg ()
		if column == COL_NAME:
# M.B. 2004-04-13
			#return self.f.name ()	# Hope, this is not needed!
			#tempString	= QListViewItem.text (self, column)
			tempString	= QListViewItem.text (self, column).latin1 ()
			if len (tempString) > 0:
				try:
					return tempString.decode ('utf-8')
				except UnicodeDecodeError:
					return tempString.decode ('latin1').encode ('utf-8').decode ('utf-8')
			else:
				return ""
		elif column == COL_TYPE:
			if self.readable:
				return "Directory"
			else:
				return "Unreadable Directory"
		elif column == COL_SIZE:
			return str (self.theFileSize)
		elif column == COL_LASTWRITETIME:
			return "---"
		else:
			return "???"
		TraceEnd ()

	# compare [
	#--------------------------------------------------
	def compare (self, i, col, ascending):
		"""Used for sorting."""

		return CE_compare (self, i, col, ascending)
	#--------------------------------------------------
	# compare ]

	# okRename [
	#--------------------------------------------------
	def okRename (self, col):
		"""Rename directory."""

		TraceBeg ()

		if DEBUG_RENAME:
			print "okRename: self:         ", self
			print "type (self.thePathName):", type (self.thePathName)
			print "self.thePathName:       ", self.thePathName.latin1 ()
			print "OLD: self.text (col):   ", self.text (col)

		OldLeafName		= self.text (col)
		folderPath		= PathToFolder (self.thePathName)

		QListViewItem.okRename (self, col)

		NewName		= folderPath.append (self.text (col))

		if DEBUG_RENAME:
			print "folderPath:             ", folderPath.latin1 ()
			print "NEW: self.text (col):   ", self.text (col)
			print "type (NewName):         ", type (NewName)
			print "NewName:                ", NewName.latin1 ()
			print "Rename: [%s] -> [%s]" % (self.thePathName.latin1 (), NewName.latin1 ())

		try:
			if LocalDir:
				# M.B. Mit Apr 28 19:55:05 CEST 2004
				# FIXME
				# Duplicate items in view possible on Linux.
				# Remove duplicate item by hand.
				os.rename (str (self.thePathName.utf8 ()), str (NewName.utf8 ()))
			else:
				pyrapi.CeMoveFile (str (self.thePathName.utf8 ()), str (NewName.utf8 ()))
		except:
			QMessageBox.warning	(
								None, AppName + " Rename Folder",
								QString	("Could not rename\nfolder \"%1\"\nto \"%2\".\n"
										"Perhaps a folder with this name allready exists?"
										). arg (self.thePathName, NewName)
								)
			self.setText (col, OldLeafName)

		TraceEnd ()
	#--------------------------------------------------
	# okRename ]
#--------------------------------------------------
# Directory ]
	

if not Qtopia:
	# FileDrag [
	#--------------------------------------------------
	class FileDrag (QIconDrag):
		"""WorkAround for our MacOS X FileDrag-problem."""

		def __init__ (self, dragSource = None, name = None):
			QIconDrag.__init__ (self, dragSource, name)
			self.urls	= QStringList ()

		# format [
		#--------------------------------------------------
		def format (self, i):
			"""We only support "text/uri-list"..."""
			TraceBeg ()
			TraceEnd ()
			if i == 0:
				return "text/uri-list"
			else:
				return None
		#--------------------------------------------------
		# format ]

		# encodedData [
		#--------------------------------------------------
		def encodedData (self, mime):
			"""Construct "file:/x/y/z"."""
			TraceBeg ()

			a	= ""
			if mime == "text/uri-list":
				a	= self.urls.join ("\r\n")

			#print "mime:    ", mime
			#print "a:       ", a
			#print "type (a):", type (a)
			#mime: text/uri-list
			#a:    /tmp/ImageBrowser-Cache/Cache_00000008
			#type (a): <class '__main__.qt.QString'>

			a.insert (0, "file:")
			dPrint ("FileDrag.encodedData a", a)
			TraceEnd ()
			return QByteArray (str (a.utf8 ()))
		#--------------------------------------------------
		# encodedData ]

		# canDecode [
		#--------------------------------------------------
		def canDecode (self, e):
			"""We only support "text/uri-list"..."""
			TraceBeg ()
			TraceEnd ()
			return e.provides ("text/uri-list")
		#--------------------------------------------------
		# canDecode ]

		# append [
		#--------------------------------------------------
		def append (self, item, pr, tr, url):
			"""Vehicle to carry the FileName ("url")."""
			TraceBeg ()
			TraceEnd ()
			QIconDrag.append (self, item, pr, tr)
			self.urls.append (url)
		#--------------------------------------------------
		# append ]
	#--------------------------------------------------
	# FileDrag ]


# DirectoryView [
#--------------------------------------------------
class DirectoryView (QListView):
	"""DirectoryView."""

	def __init__ (self, parent=None, name=None, sdo=False):
		QListView.__init__ (self, parent, name)
		self.dirsOnly		= sdo
		self.oldCurrent		= 0
		self.dropItem		= 0
		self.presspos		= QPoint (0,0)
		self.mousePressed	= False
		global folderLocked, folderClosed, folderOpened, fileNormal #= QPixmap ()
		self.autoopenTime	= 750

		dPrint ("----- DirectoryView.__init__ parent", parent)
		dPrint ("----- DirectoryView.__init__ name", name)
		if name == None:
			self.MB_ObjName		= "DirectoryView ---None---"
		elif name == "":
			self.MB_ObjName		= "DirectoryView ---Empty String---"
		else:
			self.MB_ObjName		= name
		self.parent			= parent
		self.NameLink		= parent
		self.theSource		= ""
		self.theSourceItem	= None
		self.theLeafName	= QString ()
	  
		self.autoopen_timer	= QTimer (self)

		if not folderLocked:
			folderLocked	= QPixmap (folder_locked)
			folderClosed	= QPixmap (folder_closed_xpm)
			folderOpened	= QPixmap (folder_open_xpm)
			fileNormal		= QPixmap (pix_file)
		
		#self.connect (self.button, PYSIGNAL ("sigClicked"),
		#				self.doAppSpecificFunction)
		self.connect (self, SIGNAL ("doubleClicked (QListViewItem *)"),
					  self.slotFolderSelected)
		self.connect (self, SIGNAL ("returnPressed (QListViewItem *)"),
					  self.slotFolderSelected)

		if not Qtopia:
			if SIM_CONTEXT_MENU:
				self.connect (self, SIGNAL ("rightButtonClicked (QListViewItem *, const QPoint &, int)"),
							  self.slotRightPressed)
			else:
				self.connect (self, SIGNAL ("contextMenuRequested (QListViewItem *, const QPoint &, int)"),
							  self.slotRightPressed)
		else:
			App.setStylusOperation (self.viewport (), QPEApplication.RightOnHold)
			self.connect (self, SIGNAL ("rightButtonClicked (QListViewItem *, const QPoint &, int)"),
						  self.slotRightPressed)

		self.setAcceptDrops (True)
		self.viewport ().setAcceptDrops (True)

		self.connect (self.autoopen_timer, SIGNAL ("timeout ()"), self.openFolder)

		self.setShowSortIndicator (True)

		# M.B. 2004-04-27
		#--------------------------------------------------
		# Cut, Copy, Paste via Menu (Zaurus)
		#--------------------------------------------------
		self.SomeToPaste	= False
		self.SourceToPaste	= None
		self.LeafToPaste	= None
		self.ItemToPaste	= None
		#--------------------------------------------------

	# dump [
	#--------------------------------------------------
	def dump (self):
		"""Dump info for debugging."""
		TraceBeg ()

		dPrint ("[ Begin of dump for", self)
		dPrint ("parent", self.parent)
		dPrint ("name", self.name)
		dPrint ("NameLink", self.NameLink)
		dPrint ("] End   of dump for", self)

		TraceEnd ()
	#--------------------------------------------------
	# dump ]

	def showDirsOnly (self):
		TraceBeg ()
		TraceEnd ()
		return self.dirsOnly
	
	def slotFolderSelected (self, i):
		TraceBeg ()
		if not i or not self.showDirsOnly ():
			TraceEnd ()
			return
		dir	= i
		self.emit (PYSIGNAL ("folderSelected"), (1, "dir.fullName ()"))
		TraceEnd ()
			
	def openFolder (self):
		TraceBeg ()
		self.autoopen_timer.stop ()
		if self.dropItem and not self.dropItem.isOpen ():
			self.dropItem.setOpen (True)
			self.dropItem.repaint ()
		TraceEnd ()
		
	def contentsDragEnterEvent (self, e):
		TraceBeg ()
		if not QUriDrag.canDecode (e):
			e.ignore ()
			return
		self.oldCurrent	= self.currentItem ()
		i	= self.itemAt (self.contentsToViewport (e.pos ()))
		if i:
			self.dropItem	= i
			self.autoopen_timer.start (self.autoopenTime)
		TraceEnd ()

	def contentsDragMoveEvent (self, e):
		TraceBeg ()
		if not QUriDrag.canDecode (e):
			e.ignore ()
			return
		vp	= self.contentsToViewport (e.pos ())
		i	= self.itemAt (vp)
# M.B. 2004-04-15
		# We allow drops only on directories
		#if i:
		if i and isinstance (i, Directory):
			self.setSelected (i, True)
			e.accept ()
			if i != self.dropItem:
				self.autoopen_timer.stop ()
				self.dropItem	= i
				self.autoopen_timer.start (self.autoopenTime)

			if e.action () == QDropEvent.Copy:
				pass
			elif e.action () == QDropEvent.Move:
				e.acceptAction ()
			elif e.action () == QDropEvent.Link:
				e.acceptAction ()
			else:
				pass
		else:
			e.ignore ()
			self.autoopen_timer.stop ()
			self.dropItem	= 0
		TraceEnd ()

	def contentsDragLeaveEvent (self, QDragLeaveEvent):
		TraceBeg ()
		self.autoopen_timer.stop ()
		self.dropItem	= 0

# M.B. 2004-03-31 Help by phil
#		setCurrentItem (self.oldCurrent)
#		setSelected (self.oldCurrent, True)
		self.setCurrentItem (self.oldCurrent) 
		self.setSelected (self.oldCurrent, True)
		TraceEnd ()

	# CopyFile [
	#--------------------------------------------------
	def CopyFile (self, source, target, pathSeperator):
		"""Copy a file from PDA to PC - or local on the PC."""

		from pyrapi import file

		USE_BUFFER_OBJ	= False

		TraceBeg ()

		dPrint ("CopyFile: type (source)", type (source))
		dPrint ("CopyFile: type (target)", type (target))
		dPrint ("CopyFile: source", source)
		dPrint ("CopyFile: target", target)
		# Local Move:
		#CopyFile: type (source):   <type 'instance'>
		#CopyFile: type (target):   <type 'str'>

		# Split [
		#--------------------------------------------------
		NameList		= QStringList ()
		names			= NameList.split (pathSeperator, source, True)
		# M.B. Die Apr 20 09:17:07 CEST 2004
		# Don't understand the following:
		# If we use the following line we get a "Segmentation fault" after a short while...
		#leafName		= names.last ()
		leafName		= QString (names.last ())
		#--------------------------------------------------
		# Split ]

		dPrint ("CopyFile: type (leafName)", type (leafName))
		dPrint ("CopyFile: leafName", leafName)

		if pathSeperator == "/":
			if LocalDir:
				altPathSeperator	= "/"
			else:
				altPathSeperator	= "\\"
		else:
			altPathSeperator	= "/"

		targetFileName	= QString (target)
		targetFileName.append (altPathSeperator)
		targetFileName.append (leafName)

		dPrint ("CopyFile [%s] [%s] (%s)" % (source.latin1 (), target.latin1 (), leafName.latin1 ()), "")
		dPrint ("type (targetFileName)", type (targetFileName))
		dPrint ("targetFileName", targetFileName)

		if pathSeperator == "/":
			# PC -> PDA [
			#--------------------------------------------------
			theInfile		= open (source.latin1 (), "r")

			if LocalDir:
				theOutFile		= open (str (targetFileName.utf8 ()), "w")
			else:
				theOutFile		= file.openCeFile (str (targetFileName.utf8 ()), "w")
			#--------------------------------------------------
			# PC -> PDA ]

		else:

			# PDA -> PC [
			#--------------------------------------------------
			dPrint ("type (source)", type (source))
			dPrint ("source", source)
			#type (source):             <type 'instance'>

			theInfile		= file.openCeFile (str (source.utf8 ()), "r")
			theOutFile		= open (str (targetFileName.utf8 ()), "w")
			#--------------------------------------------------
			# PDA -> PC ]

		# M.B. Mit Apr  7 19:59:25 CEST 2004
		# Since we have a problem with NULL-Bytes in binary-data if the
		# file is written a string, we use a Python Buffer Object...
		#theOutFile.write (theInfile.read ())

		buf			= theInfile.read ()
		if USE_BUFFER_OBJ:
			theBuf		= buffer (buf)
			result		= theOutFile.write (theBuf)
		else:
			result		= theOutFile.write (buf)

		dPrint ("result", result)
		
		theInfile.close ()
		theOutFile.close ()

		dPrint ("CopyFile (END): type (targetFileName)", type (targetFileName))
		dPrint ("CopyFile (END): targetFileName", targetFileName)
		dPrint ("CopyFile (END): type (leafName)", type (leafName))
		dPrint ("CopyFile (END): leafName", leafName)

		TraceEnd ()

		return (targetFileName, leafName)
	#--------------------------------------------------
	# CopyFile ]
	
	# insertFileInView [
	#--------------------------------------------------
	def insertFileInView (self, anItem, target, targetFileName, leafName, ignoreChildCount):
		"""Reflect the canges of a FileCopy / FileMove in the view."""
		TraceBeg ()

		dPrint ("insertFileInView", ">>>>>")
		dPrint ("anItem", anItem)
		dPrint ("anItem.childCount ()", anItem.childCount ())

		dPrint ("target", target)
		dPrint ("type (targetFileName)", type (targetFileName))
		dPrint ("targetFileName", targetFileName)
		dPrint ("type (leafName)", type (leafName))
		dPrint ("leafName", leafName.latin1 ())
		#target:                    \Temp
		#targetFileName:            \Temp\Ducky.jpg
		#leafName:                 Ducky.jpg
		dPrint ("insertFileInView", "<<<<<")

		if (anItem.childCount () > 0) or ignoreChildCount:
			if LocalDir:
				fi				= QFileInfo (targetFileName)
			else:
				fi				= CE_FileInfo ()

				Handle, Data	= pyrapi.CeFindFirstFile (str (targetFileName.utf8 ()))

				fi.ce_file		= Data
				fi.ce_filePath	= target

			item			= FileItem (anItem, leafName, "File")

			if LocalDir:
				item.theFileSize			= fi.size ()
				item.theFtLastWriteTime		= fi.lastModified ().toString ()
			else:
				item.theFileSize			= fi.fileSize ()
				item.theFtLastWriteTime		= fi.ftLastWriteTime ()
				item.theFtLastWriteTimeRaw	= fi.ftLastWriteTimeRaw ()

			item.setPixmap (fileNormal)

		TraceEnd ()
	#--------------------------------------------------
	# insertFileInView ]

	def contentsDropEvent (self, e):

		VARIANT			= 2
		
		# MoveFile [
		#--------------------------------------------------
		def MoveFile (source, target, anItem):

			targetFileName, \
			leafName		= self.CopyFile (source, target, "/")

			# Insert new file in view [
			#--------------------------------------------------
			self.insertFileInView (anItem, target, targetFileName, leafName, False)
			#--------------------------------------------------
			# Insert new file in view ]

			#self.triggerUpdate ()

			TraceEnd ()
		#--------------------------------------------------
		# MoveFile ]

		TraceBeg ()
		self.autoopen_timer.stop ()
		if not QUriDrag.canDecode (e):
			e.ignore ()
			return
		item	= self.itemAt (self.contentsToViewport (e.pos ()))
		if item:
# M.B. 2004-03-30
			#lst	= QStrList ()		# Is not implemented in PyQt
										# We have to use a list of python-strings instead...
			dPrint ("e", e)
			theFormat	= e.format ()
			dPrint ("e.format ()", e.format ())
			dPrint ("e.encodedData (theFormat)", e.encodedData (theFormat))

			if VARIANT == 1:
				lst	= []
				Result	= QUriDrag.decode (e, lst)
				dPrint ("Result", Result)
				dPrint ("lst", lst)
				for xString in lst:
					dPrint ("xString", xString)
					#xString: /\Programme\GPRSMonitor
					xString	= xString [1:]	# Drop first char.
			elif VARIANT == 2:
				lst	= QStringList ()
				Result	= QUriDrag.decodeLocalFiles (e, lst)
				dPrint ("Result", Result)
				dPrint ("lst", lst)
				for xString in lst:
					dPrint ("xString", xString)

# M.B. 2004-03-30
			#str	= QString ()		# Python-Bug: "str" isn't really a good python identifier ;-)
			theAction	= ActionNone
			if e.action () == QDropEvent.Copy:
				theStr	= "Copy"
				theAction	= ActionCopy
			elif e.action () ==  QDropEvent.Move:
				theStr	= "Move"
				e.acceptAction ()
				theAction	= ActionMove
			elif e.action () == QDropEvent.Link:
				theStr	= "Link"
				e.acceptAction ()
				theAction	= ActionLink
			else:
				theStr	= "Unknown";
			theStr += "\n\n"

			theActionStr	= QString (theStr)

			e.accept ()
			dPrint ("lst", lst)
# M.B. 2004-03-30
			#for i in range (0, lst.count (), 1):
			filename		= ""
			for i in range (0, len (lst), 1):
# M.B. 2004-03-30
				#filename	= QString (lst.at (i))
				filename	= QString (lst [i])
				#if VARIANT == 2:
				#	filename	= filename [1:]	# Drop first char.
				dPrint ("filename", filename)
				theActionStr.append (filename)
				theActionStr.append ("\n")

			#theStr		+= str (QString ("\nTo\n\n%1").arg (self.fullPath (item)))
			targetDir	= self.fullPath (item)
			#theStr		+= "\nTo\n\n" + targetDir.latin1 ()
			theActionStr.append ("\nTo\n\n")
			theActionStr.append (targetDir)

			if theAction == ActionMove and isinstance (item, Directory): 
				dPrint ("ActionMove: self.theSource", self.theSource)
				dPrint ("ActionMove: item", item)
				dPrint ("ActionMove: type (targetDir)", type (targetDir))
				dPrint ("ActionMove: targetDir", targetDir)
				if filename == "":
					# Local move [
					#--------------------------------------------------
					dPrint ("ActionMove: type (self.theLeafName)", type (self.theLeafName))
					dPrint ("ActionMove: self.theLeafName", self.theLeafName)

					destFile	= targetDir.append (PATH_SEPEARATOR)
					destFile.append (self.theLeafName)

					dPrint ("ActionMove: destFile", destFile)

					#if 1:
					try:
						dPrint ("ActionMove: type (self.theSource)", type (self.theSource))

						if LocalDir:
							# M.B. Mit Apr 28 19:55:05 CEST 2004
							# FIXME
							# Duplicate items in view possible on Linux.
							# Remove duplicate item by hand.
							os.rename (str (self.theSource.utf8 ()), str (destFile.utf8 ()))
						else:
							pyrapi.CeMoveFile (str (self.theSource.utf8 ()), str (destFile.utf8 ()))

						# Update the view
						dPrint ("ActionMove", ">>>>>")
						dPrint ("self.theSourceItem", self.theSourceItem)
						dPrint ("item", item)
						dPrint ("ActionMove", "<<<<<")
						self.theSourceItem.parent().takeItem (self.theSourceItem)

						# M.B. Mit Apr 14 08:36:21 CEST 2004
						# FIXME We have a problem:
						# - if we use "insertItem" the folder-information is not correctly updated.
						# - if we use "insertFileInView" the items re-appear if something is
						#   changed in the view.
						# - if we use both functions the items are shown twice.
						# I think, every "takeItem" (used above) should have a corresponding "insertItem".
						# So the bottom line is: I've to come back and fix "insertFileInView".

						#self.insertFileInView (item, targetDir, destFile, self.theLeafName, True)
						# M.B. Mit Apr 14 09:39:38 CEST 2004
						# I think "...NameLink = item" is the solution for the above problem.
						item.insertItem (self.theSourceItem)
						self.theSourceItem.NameLink	= item

					#if 0:
					except:
						QMessageBox.warning	(
											self, AppName + " Move File",
											QString	("Could not move\nfile \"%1\"\nto \"%2\".\n"
													"Perhaps a file with this name allready exists?"
													). arg (self.theSource, destFile)
											)
					#--------------------------------------------------
					# Local move ]
				else:
					# Drop from extern [
					#--------------------------------------------------
					MoveFile (filename, targetDir, item)
					#--------------------------------------------------
					# Drop from extern ]
			else:
				if theAction == ActionCopy and isinstance (item, Directory): 
					# Copy a File [
					#--------------------------------------------------
					dPrint ("ActionCopy: item", item)
					dPrint ("ActionCopy: type (self.theSource)", type (self.theSource))
					dPrint ("ActionCopy: self.theSource", self.theSource)
					dPrint ("ActionCopy: type (self.theLeafName)", type (self.theLeafName))
					dPrint ("ActionCopy: self.theLeafName", self.theLeafName)
					dPrint ("ActionCopy: type (targetDir)", type (targetDir))
					dPrint ("ActionCopy: targetDir", targetDir)

					ExistingFile	= self.theSource
					dPrint ("ActionCopy: type (ExistingFile)", type (ExistingFile))
					dPrint ("ActionCopy: ExistingFile", ExistingFile)

					NewFile			= targetDir.append (PATH_SEPEARATOR)
					NewFile.append (self.theLeafName)

					if DEBUG_COPY:
						print "ActionCopy: [%s] -> [%s]" % (ExistingFile.latin1 (), NewFile.latin1 ())

					#if 1:
					try:
						pyrapi.CeCopyFile (str (ExistingFile.utf8 ()), str (NewFile.utf8 ()), True)

						# Update the view
						self.insertFileInView (item, targetDir, NewFile, self.theLeafName, True)

					#if 0:
					except:
						QMessageBox.warning	(
											self, AppName + " Copy File",
											QString	("Could not copy\nfile \"%1\"\nto \"%2\".\n"
													"Perhaps a file with this name allready exists?"
													).arg (ExistingFile, NewFile)
											)
					#--------------------------------------------------
					# Copy a File ]
				else:
					#QMessageBox.information (self, AppName + " Drop target", theStr, "Not implemented")
					QMessageBox.information	(
											self, AppName + " Drop target",
											QString ("%1 not implemented").arg (theActionStr)
											)
		else:
			e.ignore ()
		TraceEnd ()

	# fullPath [
	#--------------------------------------------------
	def fullPath (self, item):
		"""Construct the full PathName dynamic!"""

		TraceBeg ()

		fullpath	= QString (item.text (0))
# M.B. 2004-03-30
		# while item == item.parent ():				# BUG: C++ source reads: while ( (item=item->parent()) ) ...
													# Typical '==' / '=' !!!
		#item	= item.parent
		item	= item.NameLink
		while not item == None:
			if DEBUG_OBJECTS:
				dPrint ("----------", "----------")
				dPrint ("item", item)
				dPrint ("item.MB_ObjName", item.MB_ObjName)
				dPrint ("item.parent", item.parent)
			if item.parent:
				if DEBUG_OBJECTS:
					dPrint ("item.parent ()", item.parent ())
				# FIXME Is this good to do via an exception?
				#if 1:
				try:
					if item.parent ():
						fullpath.insert (0, PATH_SEPEARATOR)
						fullpath.insert (0, item.text (0))
					else:
						fullpath.insert (0, item.text (0))
				#if 0:
				except:
					break
			#item	= item.parent
			item	= item.NameLink

		dPrint ("type (fullpath)", type (fullpath))
		dPrint ("... fullpath", fullpath)

		TraceEnd ()

		return fullpath
	#--------------------------------------------------
	# fullPath ]
	
	def contentsMousePressEvent (self, e):
		TraceBeg ()

		QListView.contentsMousePressEvent (self, e)
		p	= QPoint (self.contentsToViewport (e.pos ()))
		i	= self.itemAt (p)

		# Simulate ContextMenu - on Qtopia [
		#--------------------------------------------------
		if SIM_CONTEXT_MENU:
			if DEBUG_MOUSE:
				print "e.button:", e.button ()

			if e.button () == Qt.RightButton:
				if DEBUG_MOUSE:
					print "Right Button"
				self.emit	(SIGNAL ("rightButtonClicked (QListViewItem *, const QPoint &, int)"),
							(i, p, 1)
							)
				return
		#--------------------------------------------------
		# Simulate ContextMenu - on Qtopia ]

		if i:
			# if the user clicked into the root decoration of the item, don't try to start a drag!
			if self.rootIsDecorated (): isdecorated	= 1
			else: isdecorated	= 0
			if p.x () > self.header ().sectionPos (self.header ().mapToIndex (0)) + self.treeStepSize () *  (i.depth () + isdecorated + self.itemMargin () or
				p.x () < self.header ().sectionPos (self.header ().mapToIndex (0))):
# M.B. 2004-03-30
				#self.presspos	= e.pos ()				# BUG! This makes "presspos" a synomym for "e.pos ()"
				self.presspos.setX (e.pos ().x ())
				self.presspos.setY (e.pos ().y ())
				self.mousePressed	= True
			
		TraceEnd ()

	def contentsMouseMoveEvent (self, e):
		#TraceBeg ()
		offset	= QPoint (self.presspos.x () - e.pos ().x (),
						 self.presspos.y () - e.pos ().y ())

		if DEBUG_POSITIONS:
			dPrint ("self.presspos", "(" + str (self.presspos.x ()) +  ", " + str (self.presspos.y ()) + ")")
			dPrint ("e.pos ()", "(" + str (e.pos ().x ()) +  ", " + str (e.pos ().y ()) + ")")
			dPrint ("offset", "(" + str (offset.x ()) +  ", " + str (offset.y ()) + ")")
			dPrint ("QApplication.startDragDistance ()", QApplication.startDragDistance ())
			dPrint ("(offset).manhattanLength ()", (offset).manhattanLength ())
			dPrint ("self.mousePressed", self.mousePressed)

		if self.mousePressed and (offset).manhattanLength () > QApplication.startDragDistance ():
			self.mousePressed	= False
			item	= self.itemAt (self.contentsToViewport (self.presspos))
			if DEBUG_POSITIONS:
				dPrint ("item", item)
			if item:
				dPrint ("+++ self", self)
				dPrint ("+++ self.MB_ObjName", "[%s]" % self.MB_ObjName)
				self.theSource	= self.fullPath (item)
				dPrint ("+++ self.theSource", self.theSource)
				self.theSourceItem	= item
				if True:
				#if QFile.exists (self.theSource):
				#if CE_File.exists (self.theSource):

					# Copy PDA-File to SynCE-temporary-Directory [
					#--------------------------------------------------
					target			= QString (TempDirectory)
					dPrint ("+++ target", target)
					targetFileName, \
					self.theLeafName	= self.CopyFile (self.theSource, target, PATH_SEPEARATOR)

					dPrint ("PDA-2-tmp: type (self.theLeafName)", type (self.theLeafName))
					dPrint ("PDA-2-tmp: self.theLeafName", self.theLeafName)
					#--------------------------------------------------
					# Copy PDA-File to SynCE-temporary-Directory ]

					if sys.platform != "darwin":
						url	= QString ("file:")
						url.append (targetFileName)

						x	= QStringList ()
						x.append (url)
						
						#theUri	=  ud.localFileToUri (targetFileName)
						#dPrint ("theUri", theUri)
						#x.append (str (theUri))

						ud	= QUriDrag (self.viewport ())
						ud.setUnicodeUris (x)
					else:
						# WorkAround for our MacOS X FileDrag-problem. [
						#--------------------------------------------------
						ud	= FileDrag (self.viewport())
						id	= QIconDragItem ()
						dPrint ("+++ type (targetFileName)", type (targetFileName))
						dPrint ("+++ targetFileName", targetFileName)
						#+++ type (targetFileName):  <class '__main__.qt.QString'>
						#print dir (targetFileName)
						#id.setData (targetFileName)
						id.setData (targetFileName.utf8 ())
						ud.append	(
									id,
									QRect (0, 0, 10, 20),
									QRect (0, 0, 10, 20),
									targetFileName
									)
						#--------------------------------------------------
						# WorkAround for our MacOS X FileDrag-problem. ]

					if ud.drag ():
						QMessageBox.information (
												self, AppName + " Drag source",
												QString	("Delete \"%1\" not implemented").arg (self.theSource)
												)

					#if sys.platform != "darwin":
					#	os.remove (targetFileName)	# Remove PDA-File from SynCE-working-Directory

		#TraceEnd ()

	def contentsMouseReleaseEvent (self, QMouseEvent):
		TraceBeg ()
		self.mousePressed	= False
		TraceEnd ()

	def setDir (self, s):
		TraceBeg ()
		it	= QListViewItem (self)
		it.setOpen (False)

		if LocalDir:
			thisDir	= QDir (s)
		else:
			thisDir	= CE_Dir (s)

		it	= QListViewItem (self)
		it.setOpen (False)
		lst	= QStringList (QStringList.split (PATH_SEPEARATOR, s))
		listview2	= []
		listview2.extend (lst)
		
		item	= self.firstChild ()
		for it2 in listview2:
			while item:
				if item.text (0) == it2:
					item.setOpen (True)
					break
				item	= item.itemBelow ()
		if item:
			self.setCurrentItem (item)
		TraceEnd ()

	# ShowTextFile [
	#--------------------------------------------------
	def ShowTextFile (self, aFileName):
		"""Show a TextFile on the fly."""

		from pyrapi import file

		TraceBeg ()

		dPrint ("aFileName", aFileName)

		if LocalDir:
			theFile			= open (str (aFileName.utf8 ()), "r")
		else:
			theFile			= file.openCeFile (str (aFileName.utf8 ()), "r")

		buf				= theFile.read ()
		theFile.close ()

		#a				= unicode (buf, "unicodelittleunmarked")
		#ascii			= a.encode ("ascii", "ignore")
		ascii			= ""

		if len (ascii) > 0:
			theText			= ascii
		else:
			theText			= buf

		TraceEnd ()

		return theText
	#--------------------------------------------------
	# ShowTextFile ]

	# ShowTextWindow [
	#--------------------------------------------------
	def ShowTextWindow (self, aTitle, aText):
		"""Open a window on the fly to show a TextFile."""
		TraceBeg ()

		aDialog				= QDialog (self)
		aGrid				= QGridLayout (aDialog, 1, 1)
		if Qtopia:
			aTextView			= QTextView (aDialog)
		else:
			aTextView			= QTextEdit (aDialog)
			aTextView.setReadOnly (True)
		aDialog.setCaption (aTitle)
		aTextView.setTextFormat (Qt.PlainText)

		# Make font fixed [
		#--------------------------------------------------
		theFont				= aTextView.font ()
		if sys.platform == 'darwin':
			theFont.setFamily("Monaco")
			theFont.setPointSize (12)
		else:
			theFont.setFamily("fixed")
			theFont.setPointSize (13)
		aTextView.setFont (theFont)
		#--------------------------------------------------
		# Make font fixed ]

		aTextView.setText (aText)

		aGrid.addWidget (aTextView, 1, 1)

		# Resize window [
		#--------------------------------------------------
		if Qtopia:
			aDialog.resize (qApp.desktop().width (), qApp.desktop().height () - 50)
		else:
			aDialog.resize (600, 200)
		#--------------------------------------------------
		# Resize window ]

		aDialog.show ()
		#aDialog.exec_loop ()

		TraceEnd ()
	#--------------------------------------------------
	# ShowTextWindow ]

	# ShowPictFile [
	#--------------------------------------------------
	def ShowPictFile (self, aFileName):
		"""Show a picture on the fly."""

		from pyrapi import file

		TraceBeg ()

		if LocalDir:
			theFile			= open (str (aFileName.utf8 ()), "r")
		else:
			theFile			= file.openCeFile (str (aFileName.utf8 ()), "r")

		thePict			= theFile.read ()
		theFile.close ()

		return thePict
	#--------------------------------------------------
	# ShowPictFile ]

	# ShowPictWindow [
	#--------------------------------------------------
	def ShowPictWindow (self, aTitle, aPict):
		"""Open a window on the fly to show a picture."""
		TraceBeg ()

		aPixmap				= QPixmap ()
		if aPixmap.loadFromData (aPict):
			aDialog				= QDialog (self)
			aGrid				= QGridLayout (aDialog, 1, 1)
			aPictView			= QLabel (aDialog)
			aDialog.setCaption (aTitle)

			aPictView.setPixmap (aPixmap)

			aGrid.addWidget (aPictView, 1, 1)
			aDialog.show ()
			#aDialog.exec_loop ()

		TraceEnd ()
	#--------------------------------------------------
	# ShowPictWindow ]

	# DeleteFile [
	#--------------------------------------------------
	def DeleteFile (self):
		"""Experimental."""
		TraceBeg ()

		#print "in DeleteFile"

		TraceEnd ()
	#--------------------------------------------------
	# DeleteFile ]

	# LargeView [
	#--------------------------------------------------
	def LargeView (self):
		"""Not implemented."""
		TraceBeg ()

		print "in LargeView"

		TraceEnd ()
	#--------------------------------------------------
	# LargeView ]

	# SmallView [
	#--------------------------------------------------
	def SmallView (self):
		"""Not implemented."""
		TraceBeg ()

		print "in SmallView"

		TraceEnd ()
	#--------------------------------------------------
	# SmallView ]

	# slotRightPressed [
	#--------------------------------------------------
	def slotRightPressed (self, anItem, aPos, aCol):
		"""Show and execute context-menu."""
		TraceBeg ()

		dPrint ("slotRightPressed", "")
		dPrint ("anItem", anItem)
		dPrint ("aPos", aPos)
		dPrint ("aCol", aCol)

		if anItem:			# On item
			aMenu				= QPopupMenu (self)

			# "-1" is returned, if no item is selected.
			# So we use "-2" for non usage.
			ShowTextFile_ITEM	= -2
			ShowPictFile_ITEM	= -2
			DeleteFile_ITEM		= -2
			CreateNewDir_ITEM	= -2
			DeleteDir_ITEM		= -2

			CutFile_ITEM		= -2
			CopyFile_ITEM		= -2
			PasteFile_ITEM		= -2

			ShowInfo_ITEM		= aMenu.insertItem (qApp.translate ("qApp", "Show Object Info"))
			if isinstance (anItem, FileItem): 
				Rename_ITEM			= aMenu.insertItem (qApp.translate ("qApp", "Rename File"))
				ShowTextFile_ITEM	= aMenu.insertItem (qApp.translate ("qApp", "Show File as Text"))
				ShowPictFile_ITEM	= aMenu.insertItem (qApp.translate ("qApp", "Show File as Picture"))
				DeleteFile_ITEM		= aMenu.insertItem (qApp.translate ("qApp", "Delete File"), self.DeleteFile)
				Dummy_ITEM			= aMenu.insertSeparator (-1)
				CutFile_ITEM		= aMenu.insertItem (qApp.translate ("qApp", "Cut"))
				CopyFile_ITEM		= aMenu.insertItem (qApp.translate ("qApp", "Copy"))
				PasteFile_ITEM		= aMenu.insertItem (qApp.translate ("qApp", "Paste"))
				if not self.SomeToPaste:
					aMenu.setItemEnabled (PasteFile_ITEM, False)
			elif isinstance (anItem, Directory): 
				Rename_ITEM			= aMenu.insertItem (qApp.translate ("qApp", "Rename Folder"))
				CreateNewDir_ITEM	= aMenu.insertItem (qApp.translate ("qApp", "Create new Folder"))
				DeleteDir_ITEM		= aMenu.insertItem (qApp.translate ("qApp", "Delete Folder"))
				Dummy_ITEM			= aMenu.insertSeparator (-1)
				PasteFile_ITEM		= aMenu.insertItem (qApp.translate ("qApp", "Paste"))
				if not self.SomeToPaste:
					aMenu.setItemEnabled (PasteFile_ITEM, False)

			dPrint ("ShowInfo_ITEM", ShowInfo_ITEM)

			aMenu.setMouseTracking (True)
			id					= aMenu.exec_loop (aPos)
			self.mousePressed	= False	# Don't fall to dragging!

			theFullPath			= self.fullPath (anItem)	# Because it's used so often here...

			#--------------------------------------------------
			if id == ShowInfo_ITEM:
				# itemInfo [
				#--------------------------------------------------
				theInfo				= QString ()
				theInfo.append (str (anItem) + "\n")
				theInfo.append ("Name:                 ")
				theInfo.append (anItem.text (0))
				theInfo.append ("\n")
				theInfo.append ("fullPath:             ")
				theInfo.append (theFullPath)
				theInfo.append ("\n")
				theInfo.append ("anItem.thePathName:   ")
				theInfo.append (anItem.thePathName)
				theInfo.append ("\n")

				if isinstance (anItem, FileItem): 
					theInfo.append ("File" + "\n")
					theInfo.append ("anItem.p.thePathName: ")
					theInfo.append (anItem.p.thePathName)

				if isinstance (anItem, Directory): 
					theInfo.append ("Directory" + "\n")
					theInfo.append ("childCount (): " + str (anItem.childCount ()) + "\n")

				self.ShowTextWindow (anItem.text (0), theInfo)
				#--------------------------------------------------
				# itemInfo ]

			#--------------------------------------------------
			elif id == Rename_ITEM:
				anItem.setRenameEnabled (COL_NAME, True)
				if isinstance (anItem, FileItem):
					folderPath			= PathToFolder (theFullPath)
					dPrint ("folderPath", folderPath)
					# Move last char to front [
					#--------------------------------------------------
					#folderPath:                Temp\
					#anItem.thePathName:        \Temp
					folderPath.insert (0, folderPath.right (1))
					folderPath.remove (len (folderPath) - 1, 1)
					#--------------------------------------------------
					# Move last char to front ]
				else:
					folderPath			= QString (theFullPath)
				anItem.thePathName	= folderPath
				anItem.startRename (COL_NAME)

			#--------------------------------------------------
			elif id == ShowTextFile_ITEM:
				theText		= self.ShowTextFile (theFullPath)
				self.ShowTextWindow (anItem.text (0), theText)

			#--------------------------------------------------
			elif id == ShowPictFile_ITEM:
				thePict		= self.ShowPictFile (theFullPath)
				self.ShowPictWindow (anItem.text (0), thePict)

			#--------------------------------------------------
			elif id == DeleteFile_ITEM:
				if Qtopia:
					answer		= DoQuestionDialog	(
													self, AppName + " Delete File",
													QString ("Do you really want to delete the file \"%1\"?\n").arg (theFullPath),
													)
					if answer:
						answer		= QMessageBox.Yes
				else:
					answer		= QMessageBox.question	(
														self, AppName + " Delete File",
														#"Do you really want to delete the file \"" + theFullPath + "\"?\n",
														QString ("Do you really want to delete the file \"%1\"?\n").arg (theFullPath),
														QMessageBox.No,
														QMessageBox.Yes,
														)
				if answer == QMessageBox.Yes:
					try:
						if LocalDir:
							os.remove (str (theFullPath.utf8 ()))
						else:
							pyrapi.CeDeleteFile (str (theFullPath.utf8 ()))

						anItem.parent().takeItem (anItem)

					except:
						QMessageBox.warning	(
											self, AppName + " Delete File",
											QString	("Could not delete the file \"%1\".\n"
													"Perhaps bad characters in fileName?"
													).arg (theFullPath)
											)

			#--------------------------------------------------
			elif id == CopyFile_ITEM:
				dPrint ("CopyFile", "")
				dPrint ("CopyFile anItem.text (0)", anItem.text (0))
				dPrint ("CopyFile theFullPath", theFullPath)
				self.SomeToPaste	= True
				self.SourceToPaste	= theFullPath
				self.LeafToPaste	= anItem.text (0)
				self.ItemToPaste	= anItem

			#--------------------------------------------------
			elif id == PasteFile_ITEM:
				dPrint ("PasteFile", "")
				dPrint ("PasteFile theFullPath", theFullPath)
				dPrint ("PasteFile self.SourceToPaste", self.SourceToPaste)
				if isinstance (anItem, FileItem): 
					targetDir			= PathToFolder (theFullPath)
					theParent			= anItem.parent ()
				else:
					targetDir			= theFullPath
					theParent			= anItem
				dPrint ("PasteFile targetDir", targetDir)
				dPrint ("PasteFile self.LeafToPaste", self.LeafToPaste)

				ExistingFile		= self.SourceToPaste
				NewFile				= targetDir
				if isinstance (anItem, Directory): 
					NewFile.append (PATH_SEPEARATOR)
				NewFile.append (self.LeafToPaste)

				if DEBUG_COPY:
					print "Paste: [%s] -> [%s]" % (ExistingFile.latin1 (), NewFile.latin1 ())

				#if 1:
				try:
					if LocalDir:
						# Copy local file [
						#--------------------------------------------------
						# M.B. Mit Apr 28 20:56:26 CEST 2004
						# FIXME
						# Set file permissions.
						# Or is something like "system 'cp -p ExistingFile NewFile'" better?

						if 0:
							theInfile		= open (str (ExistingFile.utf8 ()), "r")
							theOutFile		= open (str (NewFile.utf8 ()), "w")
							buf				= theInfile.read ()
							result			= theOutFile.write (buf)
							theInfile.close ()
							theOutFile.close ()

						os.system ("cp -p " + str (ExistingFile.utf8 ()) + " " + str (NewFile.utf8 ()))

						#--------------------------------------------------
						# Copy local file ]

						# Update the view
						#
						# Works like a "move", not a "copy" :-(
						# 
						#if isinstance (anItem, FileItem): 
						#	anItem.parent ().insertItem (self.ItemToPaste)
						#else:
						#	anItem.insertItem (self.ItemToPaste)

					else:
						pyrapi.CeCopyFile (str (ExistingFile.utf8 ()), str (NewFile.utf8 ()), True)

					# Update the view
					self.insertFileInView (theParent, targetDir, NewFile, QString (self.LeafToPaste), False)

				#if 0:
				except:
					QMessageBox.warning	(
										self, AppName + " Copy File",
										QString	("Could not copy\nfile \"%1\"\nto \"%2\".\n"
												"Perhaps a file with this name allready exists?"
												).arg (ExistingFile, NewFile)
										)

			#--------------------------------------------------
			elif id == CreateNewDir_ITEM:
				NewFolderName	= "New Folder"
				try:
					aFolderName		= QString (theFullPath)
					aFolderName.append (PATH_SEPEARATOR + NewFolderName)
					result			= pyrapi.CeCreateDirectory (str (aFolderName.utf8 ()))
					Directory (anItem, NewFolderName)
				except:
					QMessageBox.warning	(
										self, AppName + " Create new Folder",
										QString	("Could not create new folder \"%1\".\n"
												"Perhaps a folder with this name allready exists?"
												).arg (NewFolderName)
										)

			#--------------------------------------------------
			elif id == DeleteDir_ITEM:
				if Qtopia:
					answer		= DoQuestionDialog	(
													self, AppName + " Delete Folder",
													QString ("Do you really want to delete the folder \"%1\"?\n").arg (theFullPath),
													)
					if answer:
						answer		= QMessageBox.Yes
				else:
					answer		= QMessageBox.question	(
														self, AppName + " Delete Folder",
														QString ("Do you really want to delete the folder \"%1\"?\n").arg (theFullPath),
														QMessageBox.No,
														QMessageBox.Yes,
														)
				if answer == QMessageBox.Yes:
					#if 1:
					try:
						if LocalDir:
							os.rmdir (str (theFullPath.utf8 ()))
						else:
							result	= pyrapi.CeRemoveDirectory (str (theFullPath.utf8 ()))

						anItem.parent().takeItem (anItem)

					#if 0:
					except:
						QMessageBox.warning	(
											self, AppName + " Delete Folder",
											QString	("Could not delete folder \"%1\".\n"
													"Perhaps the folder is not empty?"
													).arg (theFullPath)
											)

			#--------------------------------------------------

		else:				# On view
			aMenu				= QPopupMenu (self)
			aMenu.insertItem (qApp.translate ("qApp", "Large View"), self.LargeView)
			aMenu.insertItem (qApp.translate ("qApp", "Small View"), self.SmallView)
			aMenu.setMouseTracking (True)
			id					= aMenu.exec_loop (aPos)

		dPrint ("id", id)

		TraceEnd ()
	#--------------------------------------------------
	# slotRightPressed ]
#--------------------------------------------------
# DirectoryView ]


# main [
#--------------------------------------------------
def main (theArgs):
	"""The main function."""

	global DEBUG
	global DEBUG_PRINT
	global DEBUG_RENAME			# Preferences
	global DEBUG_COPY			# Preferences
	global DEBUG_MOUSE			# Preferences
	global SHOW_SYM_LINKS		# Preferences

	global LocalDir

	global PATH_SEPEARATOR

	global dPrint1
	global TempDirectory
	global theStatusBar
	global theProgressBar
	global App
	global AppName

	#--------------------------------------------------
	# MenuCmds [
	#--------------------------------------------------

	# about [
	#--------------------------------------------------
	def about ():
		TraceBeg ()

		QMessageBox.about (theMainWindow, AppName,
							#self.tr ("Show thumbnails of images.\n"
							unicode (qApp.translate ("AboutDialog", "Add-on for SynCE.\n"
							"Show PocketPC FileSystem.\n"
							"\nAuthor: "))
							+ __author__  + "\n"
							+ __version__.replace ("$", "") + "\n"
							+ __date__.replace ("$", "") + "\n"
							+ "QT-Version: " + QT_VERSION_STR
							# "__FileVersion__" is only needed for developing...
							+ "\n" + __FileVersion__
							)

		TraceEnd ()
	#--------------------------------------------------
	# about ]

	# DoPreferencesDialog [
	#--------------------------------------------------
	def DoPreferencesDialog ():

		import PreferencesDialog

		global DEBUG_RENAME
		global DEBUG_COPY
		global DEBUG_MOUSE
		global SHOW_SYM_LINKS

		TraceBeg ()

		PreferencesDialog	= PreferencesDialog.Preferences (modal=True)
		if Qtopia:
			PreferencesDialog.showMaximized ()

		# Fill Values [
		#--------------------------------------------------
		PreferencesDialog.checkBoxDebugRename.setChecked (DEBUG_RENAME)
		PreferencesDialog.checkBoxDebugCopy.setChecked (DEBUG_COPY)
		PreferencesDialog.checkBoxDebugMouse.setChecked (DEBUG_MOUSE)
		PreferencesDialog.checkBoxShowSymLinks.setChecked (SHOW_SYM_LINKS)
		#--------------------------------------------------
		# Fill Values ]

		PreferencesDialog.textLabelCvsInfo.setText (TrimCvsString (PreferencesDialog.textLabelCvsInfo.text ()))

		# Show the Dialog
		#--------------------------------------------------
		ReturnValue	= PreferencesDialog.exec_loop ()
		#--------------------------------------------------
		# Show the Dialog

		if ReturnValue:
			DEBUG_RENAME		= PreferencesDialog.checkBoxDebugRename.isChecked ()
			DEBUG_COPY			= PreferencesDialog.checkBoxDebugCopy.isChecked ()
			DEBUG_MOUSE			= PreferencesDialog.checkBoxDebugMouse.isChecked ()
			SHOW_SYM_LINKS		= PreferencesDialog.checkBoxShowSymLinks.isChecked ()

		TraceEnd ()
	#--------------------------------------------------
	# DoPreferencesDialog ]

	# exit [
	#--------------------------------------------------
	def exit (self):
		theMainWindow.close ()
		qApp.quit ()
	#--------------------------------------------------
	# exit ]
	#--------------------------------------------------
	# MenuCmds ]

	TraceBeg ()

	if sys.platform == "darwin":
		# Avoid warning on MacOS X
		#--------------------------------------------------
		# Mon Apr 26 11:09:36 CEST 2004
		# Qt: QApplication: Warning argv[0] == 'CE_DirView.py' is relative.
		# In order to dispatch events correctly Mac OS X may require applications to be run with the *full* path to the executable.
		#>>> x=os.getcwd ()
		#>>> x
		#>>> '/home/mbier/Python/SynCE'
		sys.argv [0]	= os.getcwd () + "/" + sys.argv [0]
		#print "sys.argv [0]:", sys.argv [0]
		#--------------------------------------------------
		# Avoid warning on MacOS X

	# Initialize DebugPrint [
	#--------------------------------------------------
	debugPrint		= DebugPrint.DebugPrint ()
	debugPrint.doPrint= DEBUG_PRINT
	dPrint1			= debugPrint.LabelValue	# Name-change!
	#--------------------------------------------------
	# Initialize DebugPrint ]

	# Test
	dPrint ("sys.argv", sys.argv)

	# Read Settings [
	#--------------------------------------------------
	try:
		SettingsFile	= open (SETTINGS_NAME, "r")
		Settings		= SettingsFile.readlines ()
		SettingsFile.close ()
		# ['host=linuxorange\n', 'port=4711\n']
		dPrint ("Settings", Settings)

		for Line in Settings:
			Line	= Line [:-1]	# Kill trailing "\n"
			Params	= Line.split ("=")
			if Params [0] == "DEBUG_RENAME":
				DEBUG_RENAME	= Params [1] == "True"
				dPrint ("DEBUG_RENAME", DEBUG_RENAME)
			elif Params [0] == "DEBUG_COPY":
				DEBUG_COPY		= Params [1] == "True"
				dPrint ("DEBUG_COPY", DEBUG_COPY)
			elif Params [0] == "DEBUG_MOUSE":
				DEBUG_MOUSE		= Params [1] == "True"
				dPrint ("DEBUG_MOUSE", DEBUG_MOUSE)
			elif Params [0] == "SHOW_SYM_LINKS":
				SHOW_SYM_LINKS	= Params [1] == "True"
				dPrint ("SHOW_SYM_LINKS", SHOW_SYM_LINKS)
	except:
		print "%s Warning: Could not read settings from [%s]." % (sys.argv [0], SETTINGS_NAME)
	#--------------------------------------------------
	# Read Settings ]

	# Process Options [
	#--------------------------------------------------
	Usage		= \
    """usage: %s [-d n] [-D] [-h] [-l] [-V]
    -d n, --debugLevel=n ... debug-level 
                             0 = off
                             1 = general
                             2 = trace
                             3 = Debug-File
    -D,   --DebugPrint ..... DebugPrint
    -h,   --help ........... display this help and exit
    -l,   --local .......... local
    -V,   --version ........ print version and exit"""

	LongOptions		= ["debugLevel=", "DebugPrint", "help", "local", "version"]

	try:
		opts, args	= getopt.getopt (theArgs [1:], 'd:DhlV', LongOptions)
	except getopt.GetoptError, msg:
		print msg
		print Usage % sys.argv [0]
		sys.exit (1)

	if DEBUG:
		print opts, args

	DEBUG_PRINT		= False
	LocalDir		= False

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
		if o in ("-h", "--help"):
			print Usage % sys.argv [0]
			sys.exit (0)
		if o in ("-l", "--local"):
			LocalDir	= True
		if o in ("-V", "--version"):
			print sys.argv [0] \
				+ " " + __version__.replace ("$", "") \
				+ __date__.replace ("$", "")
			print __FileVersion__	# only needed for developing...
			sys.exit (0)
	#--------------------------------------------------
	# Process Options ]

	if Qtopia:
		# Trick to force a taskbar-icon [
		#--------------------------------------------------
		# Die Sep  9 21:09:22 CEST 2003
		# We want a taskbar-icon
		# "argv [0]" hat to correspodent with the "Name"-Entry in the ".desktop"-File.
		# The "Icon"-Entry shows the taskbar-icon. Easy-huh?
		if LocalDir:
			sys.argv [0]	= "DirView"
		else:
			sys.argv [0]	= "CE_DirView"
		#--------------------------------------------------
		# Trick to force a taskbar-icon ]

	debugPrint.doPrint= DEBUG_PRINT

	if LocalDir:
		PATH_SEPEARATOR	= "/"
	else:
		PATH_SEPEARATOR	= "\\"

	if Qtopia:
		print "Using Qtopia"
		App			 = QPEApplication (theArgs)
	else:
		print "Not using Qtopia"
		App			 = QApplication (theArgs)

	# Create MenuBar [
	#--------------------------------------------------
	theMainWindow		= QMainWindow ()

	fileMenu			= QPopupMenu (theMainWindow)
	fileMenu.insertItem (qApp.translate ("qApp", "&Exit"), exit, Qt.CTRL+Qt.Key_Q)

	optionsMenu			= QPopupMenu (theMainWindow)
	optionsMenu.insertItem (qApp.translate ("qApp", "Preferences..."), DoPreferencesDialog)

	helpMenu			= QPopupMenu (theMainWindow)
	helpMenu.insertItem (qApp.translate ("qApp", "&About"), about, Qt.CTRL+Qt.Key_H)

	if Qtopia:
		theMenuBar			= QPEMenuBar (theMainWindow)
	else:
		theMenuBar			= QMenuBar (theMainWindow)

	theMenuBar.insertItem (qApp.translate ("qApp", "&File"),    fileMenu)
	theMenuBar.insertItem (qApp.translate ("qApp", "&Options"), optionsMenu)
	theMenuBar.insertItem (qApp.translate ("qApp", "&About"),   helpMenu)

	qApp.connect (qApp, SIGNAL('lastWindowClosed()'), qApp, SLOT('quit()'))
	#--------------------------------------------------
	# Create MenuBar ]

	# StatusBar [
	#--------------------------------------------------
	if prefStatusWant:
		theStatusBar		= QStatusBar (theMainWindow)
		theStatusBar.message ((__version__ + " " + __date__).replace ("$", ""))
		if Qtopia:
			# IMPORTANT!!!  Mit Okt  8 17:15:34 CEST 2003
			theStatusBar.setMinimumHeight (STATUS_BAR_H)
	#--------------------------------------------------
	# StatusBar ]

	# Create DirectoryView [
	#--------------------------------------------------
	theMainView	= DirectoryView (theMainWindow)
	theMainView.addColumn ("Name")
	theMainView.addColumn ("Type")
	theMainView.addColumn ("Size")
	theMainView.addColumn ("LastWriteTime")
	theMainView.setTreeStepSize (20)
	#--------------------------------------------------
	# Create DirectoryView ]
	
	# Initialize ProgressBar [
	#--------------------------------------------------
	if Qtopia:
		if prefStatusWant:
			parent			= theStatusBar
		else:
			parent			= None
		theProgressBar  = QProgressBar (0, parent, "Reading Directory")
		theDesktopWidth	= qApp.desktop().width ()
		theProgressBar.resize (theDesktopWidth, STATUS_BAR_H)
		theProgressBar.hide ()
	else:
		theProgressBar  = QProgressBar (0, None, "Reading Directory")
		theStatusBar.addWidget (theProgressBar, 0, True)
	#--------------------------------------------------
	# Initialize ProgressBar ]
	
	if not LocalDir:
		try:
			pyrapi.synce_log_set_level (pyrapi.SYNCE_LOG_LEVEL_LOWEST)
		except:
			QMessageBox.information	(
									theMainView, AppName + " connect to SynCE",
									"Problem in contacting SynCE.\n"
									"Perhaps server not found."
									)
			print "%s Problem in contacting SynCE." % sys.argv [0]
			print "%s Perhaps server not found." % sys.argv [0]

	if LocalDir:
		theRoot	= QDir ()
	else:
		theRoot	= CE_Dir ()

	roots	= theRoot.drives ()
	if roots:
		for it in roots:
			fi = it
			dPrint ("it", it)
			dPrint ("fi.filePath ()", fi.filePath ())
			root = Directory (theMainView, fi.filePath ())
			if roots.count (it) <= 1:
				#if 1:
				try:
					root.setOpen (True)  # be interesting
				#if 0:
				except:
					QMessageBox.information	(
											theMainView, AppName + " connect to PDA",
											"Could not connect to PDA."
											)
					print "%s Could not connect to PDA." % sys.argv [0]
	
	theMainView.setAllColumnsShowFocus (True)

	theMainWindow.resize (400, 400)

	if LocalDir:
		Description		= "Local"
	else:
		Description		= "PocketPC"
	theMainWindow.setCaption (Description + theCaption)
	AppName			= Description + "_DirectoryBrowser"

	theMainWindow.setCentralWidget (theMainView)
	if Qtopia:
		theMainWindow.showMaximized ()
	else:
		theMainWindow.show ()
	
	# taskBar issue [
	#--------------------------------------------------
	if Qtopia:
		# IMPORTANT for QTPE's window behaviour!!!
		# If we don't do this the application is not selectable by the taksbar-icon
		App.showMainWidget (theMainWindow)
	
		# M.B. 2003-01-16
		# Funny: If we uncomment the following line, there is now toolbar-icon shown
		# in Qtopia.
		#App.connect (theMainDialog.ButtonCancel, SIGNAL ("pressed()"), App, SLOT("quit()"))
	#--------------------------------------------------
	# taskBar issue ]

	# Initialize Temp Directory [
	#--------------------------------------------------
	TempDirectoryName	= AppName + "-Temp"
	User				= os.getenv ("USER")
	# We have a problem on the Zaurus...
	if User == None:
		User				= "unknown"
	TempDirectory		= "/tmp/" + TempDirectoryName + "-" + User
	#print "TempDirectory", TempDirectory
	TempDir				= QDir (TempDirectory)
	if not TempDir.exists ():
		print "mkdir TempDirectory:", TempDirectory
		DirOK				= TempDir.mkdir (TempDirectory)
		if not DirOK:
			print "Couldn't create TempDirectory [%s]." % TempDirectory
	#--------------------------------------------------
	# Initialize Temp Directory ]

	App.exec_loop ()

	# Write Settings [
	#--------------------------------------------------
	try:
		SettingsFile	= open (SETTINGS_NAME, "w")
		Settings		= [
						"DEBUG_RENAME=", str (DEBUG_RENAME), "\n", 
						"DEBUG_COPY=", str (DEBUG_COPY), "\n", 
						"DEBUG_MOUSE=", str (DEBUG_MOUSE), "\n", 
						"SHOW_SYM_LINKS=", str (SHOW_SYM_LINKS), "\n", 
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

if __name__ == "__main__":
	main (sys.argv)

# vim: ts=4 sw=4
